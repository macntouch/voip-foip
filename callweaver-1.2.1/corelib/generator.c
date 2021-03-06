/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2005, Digium, Inc.
 *
 * Mark Spencer <markster@digium.com>
 *
 * Modifications by Carlos Antunes <cmantunes@gmail.com>
 *
 * See http://www.callweaver.org for more information about
 * the CallWeaver project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*
 * Synthetic frame generation helper routines
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/corelib/generator.c $", "$Revision: 4723 $")

#include "callweaver/channel.h"	/* generator.h is included */
#include "callweaver/lock.h"

/* Needed declarations */
static void *cw_generator_thread(void *data);
static int cw_generator_start_thread(struct cw_channel *pchan);

/*
 * ****************************************************************************
 *
 * Frame generation public interface
 *
 * ****************************************************************************
 */

/* Activate channel generator */
int cw_generator_activate(struct cw_channel *chan, struct cw_generator *gen, void *params)
{
	void *gen_data;

	cw_generator_deactivate(chan);

	/* Try to allocate new generator */
	gen_data = gen->alloc(chan, params);
	if (gen_data) {
		struct cw_generator_channel_data *pgcd = &chan->gcd;

		/* We are going to play with new generator data structures */
		cw_mutex_lock(&pgcd->lock);

		if(cw_generator_start_thread(chan)) {
			/* Whoops! */
			gen->release(chan, gen_data);
			cw_mutex_unlock(&pgcd->lock);
			cw_log(LOG_ERROR, "Generator activation failed: unable to start generator thread\n");
			return -1;
		}

		/* Setup new request */
		pgcd->gen_data = gen_data;
		pgcd->gen_func = gen->generate;
                if ( chan->gen_samples )
		    pgcd->gen_samp = chan->gen_samples;
                else
		    pgcd->gen_samp = 160;
		pgcd->samples_per_second = chan->samples_per_second;
		pgcd->gen_free = gen->release;

		/* Signal generator thread to activate new generator */
		pgcd->gen_req = gen_req_null;
		cw_cond_signal(&pgcd->gen_req_cond);

		/* Our job is done */
		cw_mutex_unlock(&pgcd->lock);
		return 0;
	} else {
		/* Whoops! */
		cw_log(LOG_ERROR, "Generator activation failed\n");
		return -1;
	}
}

/* Deactivate channel generator */
void cw_generator_deactivate(struct cw_channel *chan)
{
	struct cw_generator_channel_data *pgcd = &chan->gcd;
	pthread_t thread = CW_PTHREADT_NULL;

	cw_log(LOG_DEBUG, "Trying to deactivate generator in %s\n", chan->name);

	while (pgcd->gen_is_active) {
		cw_mutex_lock(&pgcd->lock);

		/* If we can claim the thread tell the generator to shut down */
		if (pgcd->pgenerator_thread) {
			thread = *pgcd->pgenerator_thread;
			free(pgcd->pgenerator_thread);
			pgcd->pgenerator_thread = NULL;
			pgcd->gen_req = gen_req_deactivate;
    			cw_cond_signal(&pgcd->gen_req_cond);
		}

		cw_mutex_unlock(&pgcd->lock);

		/* If we claimed the thread wait for it to exit and then clean up
		 * after it. Otherwise someone else is deactivating the thread
		 * so all we need do is wait.
		 */
		if (!pthread_equal(thread, CW_PTHREADT_NULL)) {
			void *gen_data;
			void (*gen_free)(struct cw_channel *chan, void *data);

			pthread_join(thread, NULL);

			/* Now clean up. Until we clear gen_is_active and release
			 * the lock no one else is able to continue.
			 */
			cw_mutex_lock(&pgcd->lock);
			cw_cond_destroy(&pgcd->gen_req_cond);
			gen_free = pgcd->gen_free;
			gen_data = pgcd->gen_data;
			cw_clear_flag(chan, CW_FLAG_WRITE_INT);
			pgcd->gen_is_active = 0;
			cw_log(LOG_DEBUG, "Generator on %s stopped\n", chan->name);
			cw_mutex_unlock(&pgcd->lock);
			if (gen_free)
				gen_free(chan, gen_data);
		} else
			usleep(10000);
	}
}

/* Is channel generator active? */
int cw_generator_is_active(struct cw_channel *chan)
{
	struct cw_generator_channel_data *pgcd = &chan->gcd;

	return CW_ATOMIC_GET(pgcd->lock, pgcd->gen_is_active);
}

/* Is the caller of this function running in the generator thread? */
int cw_generator_is_self(struct cw_channel *chan)
{
	struct cw_generator_channel_data *pgcd = &chan->gcd;
	int res;

	cw_mutex_lock(&pgcd->lock);

	if (pgcd->pgenerator_thread) {
		res = pthread_equal(*pgcd->pgenerator_thread, pthread_self());
	} else {
		res = 0;
	}

	cw_mutex_unlock(&pgcd->lock);

	return res;
}

/*
 * *****************************************************************************
 *
 * Frame generation private routines
 *
 * *****************************************************************************
 */

/* The mighty generator thread */
static void *cw_generator_thread(void *data)
{
	struct cw_channel *chan = data;
	struct cw_generator_channel_data *pgcd = &chan->gcd;
	void *cur_gen_data;
	int cur_gen_samp;
	int (*cur_gen_func)(struct cw_channel *chan, void *cur_gen_data, int cur_gen_samp);
	void (*cur_gen_free)(struct cw_channel *chan, void *cur_gen_data);
	struct timeval tv;
	struct timespec ts;
	long sleep_interval_ns;
	int res;

	cw_log(LOG_DEBUG, "Generator thread started on %s\n", chan->name);

	/* Loop continuously until shutdown request is received */
	cw_mutex_lock(&pgcd->lock);

	/* Copy gen_* stuff to cur_gen_* stuff, set flag
	 * gen_is_active, calculate sleep interval and
	 * obtain current time using CLOCK_MONOTONIC. */
	cur_gen_data = pgcd->gen_data;
	cur_gen_samp = pgcd->gen_samp;
	cur_gen_func = pgcd->gen_func;
	cur_gen_free = pgcd->gen_free;
	pgcd->gen_is_active = -1;
	sleep_interval_ns = 1000000L * cur_gen_samp / ( pgcd->samples_per_second / 1000 );        // THIS IS BECAUSE It's HARDCODED TO 8000 samples per second. We should use the samples per second in the channel struct.
	gettimeofday(&tv, NULL);
	ts.tv_sec = tv.tv_sec;
	ts.tv_nsec = 1000 * tv.tv_usec;

	for (;;) {
		/* Sleep based on number of samples */
		ts.tv_nsec += sleep_interval_ns;
		if (ts.tv_nsec >= 1000000000L) {
			++ts.tv_sec;
			ts.tv_nsec -= 1000000000L;
		}
		res = cw_cond_timedwait(&pgcd->gen_req_cond, &pgcd->lock, &ts);
		if (pgcd->gen_req) {
			/* Got new request */
			break;
		} else if (res == ETIMEDOUT) {
	 		/* We've got some generating to do. */

			/* Need to unlock generator lock prior
			 * to calling generate callback because
			 * it will try to acquire channel lock
			 * at least by cw_write. This mean we
			 * can receive new request here */
			cw_mutex_unlock(&pgcd->lock);
			res = cur_gen_func(chan, cur_gen_data, cur_gen_samp);
			cw_mutex_lock(&pgcd->lock);
			if (res || pgcd->gen_req) {
				/* Got generator error or new
				 * request. Deactivate current
				 * generator */
				if (!pgcd->gen_req)
					cw_log(LOG_DEBUG, "Generator self-deactivating\n");
				break;
			}
		}
	}

	/* Next write on the channel should clean out the defunct generator */
	cw_set_flag(chan, CW_FLAG_WRITE_INT);

	/* Got request to shutdown. */
	cw_log(LOG_DEBUG, "Generator thread shut down on %s\n", chan->name);
	cw_mutex_unlock(&pgcd->lock);
	return NULL;
}

/* Starts the generator thread associated with the channel. */
static int cw_generator_start_thread(struct cw_channel *pchan)
{
	struct cw_generator_channel_data *pgcd = &pchan->gcd;

	/* Just return if generator thread is running already */
	if (pgcd->pgenerator_thread) {
		return 0;
	}

	/* Create joinable generator thread */
	pgcd->pgenerator_thread = malloc(sizeof (pthread_t));
	if (!pgcd->pgenerator_thread) {
		return -1;
	}
	cw_cond_init(&pgcd->gen_req_cond, NULL);
	if(cw_pthread_create(pgcd->pgenerator_thread, NULL, cw_generator_thread, pchan)) {
		free(pgcd->pgenerator_thread);
		pgcd->pgenerator_thread = NULL;
		cw_cond_destroy(&pgcd->gen_req_cond);
		return -1;
	}

	/* All is good */
	return 0;
}

