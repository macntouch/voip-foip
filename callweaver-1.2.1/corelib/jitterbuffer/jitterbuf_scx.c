/*
 * scx_jitterbuf: jitterbuffering algorithm
 *
 * Copyright (C) 2005, Attractel OOD
 *
 * Contributors:
 * Slav Klenov <slav@securax.org>
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

/*! \file 
 * 
 * \brief Jitterbuffering algorithm.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "jitterbuf_scx.h"


/* private scx_jb structure */
struct scx_jb
{
	struct scx_jb_frame *frames;
	struct scx_jb_frame *tail;
	struct scx_jb_conf conf;
	long rxcore;
	long delay;
	long next_delivery;
	int force_resynch;
};


static struct scx_jb_frame * alloc_jb_frame(struct scx_jb *jb);
static void release_jb_frame(struct scx_jb *jb, struct scx_jb_frame *frame);
static void get_jb_head(struct scx_jb *jb, struct scx_jb_frame *frame);
static int resynch_jb(struct scx_jb *jb, void *data, long ms, long ts, long now);


static struct scx_jb_frame * alloc_jb_frame(struct scx_jb *jb)
{
	return (struct scx_jb_frame *) calloc(1, sizeof(struct scx_jb_frame));
}


static void release_jb_frame(struct scx_jb *jb, struct scx_jb_frame *frame)
{
	free(frame);
}


static void get_jb_head(struct scx_jb *jb, struct scx_jb_frame *frame)
{
	struct scx_jb_frame *fr;
	
	/* unlink the frame */
	fr = jb->frames;
	jb->frames = fr->next;
	if(jb->frames != NULL)
	{
		jb->frames->prev = NULL;
	}
	else
	{
		/* the jb is empty - update tail */
		jb->tail = NULL;
	}
	
	/* update next */
	jb->next_delivery = fr->delivery + fr->ms;
	
	/* copy the destination */
	memcpy(frame, fr, sizeof(struct scx_jb_frame));
	
	/* and release the frame */
	release_jb_frame(jb, fr);
}


struct scx_jb * scx_jb_new(struct scx_jb_conf *conf)
{
	struct scx_jb *jb;
	
	jb = calloc(1, sizeof(struct scx_jb));
	if(jb == NULL)
	{
		return NULL;
	}
	
	/* First copy our config */
	memcpy(&jb->conf, conf, sizeof(struct scx_jb_conf));
	/* we dont need the passed config anymore - continue working with the saved one */
	conf = &jb->conf;
	
	/* validate the configuration */
	if(conf->jbsize < 1)
	{
		conf->jbsize = SCX_JB_SIZE_DEFAULT;
	}
	if(conf->resync_threshold < 1)
	{
		conf->resync_threshold = SCX_JB_RESYNCH_THRESHOLD_DEFAULT;
	}
	
	/* Set the constant delay to the jitterbuf */
	jb->delay = conf->jbsize;
	
	return jb;
}


void scx_jb_destroy(struct scx_jb *jb)
{
	/* jitterbuf MUST be empty before it can be destroyed */
	assert(jb->frames == NULL);
	
	free(jb);
}


static int resynch_jb(struct scx_jb *jb, void *data, long ms, long ts, long now)
{
	long diff, offset;
	struct scx_jb_frame *frame;
	
	/* If jb is empty, just reinitialize the jb */
	if(jb->frames == NULL)
	{
		/* debug check: tail should also be NULL */
		assert(jb->tail == NULL);
		
		return scx_jb_put_first(jb, data, ms, ts, now);
	}
	
	/* Adjust all jb state just as the new frame is with delivery = the delivery of the last
	   frame (e.g. this one with max delivery) + the length of the last frame. */
	
	/* Get the diff in timestamps */
	diff = ts - jb->tail->ts;
	
	/* Ideally this should be just the length of the last frame. The deviation is the desired
	   offset */
	offset = diff - jb->tail->ms;
	
	/* Do we really need to resynch, or this is just a frame for dropping? */
	if(!jb->force_resynch && (offset < jb->conf.resync_threshold && offset > -jb->conf.resync_threshold))
	{
		return SCX_JB_DROP;
	}
	
	/* Reset the force resynch flag */
	jb->force_resynch = 0;
	
	/* apply the offset to the jb state */
	jb->rxcore -= offset;
	frame = jb->frames;
	while(frame)
	{
		frame->ts += offset;
		frame = frame->next;
	}
	
	/* now jb_put() should add the frame at a last position */
	return scx_jb_put(jb, data, ms, ts, now);
}


void scx_jb_set_force_resynch(struct scx_jb *jb)
{
	jb->force_resynch = 1;
}


int scx_jb_put_first(struct scx_jb *jb, void *data, long ms, long ts, long now)
{
	/* this is our first frame - set the base of the receivers time */
	jb->rxcore = now - ts;
	
	/* init next for a first time - it should be the time the first frame should be played */
	jb->next_delivery = now + jb->delay;
	
	/* put the frame */
	return scx_jb_put(jb, data, ms, ts, now);
}


int scx_jb_put(struct scx_jb *jb, void *data, long ms, long ts, long now)
{
	struct scx_jb_frame *frame, *next, *newframe;
	long delivery;
	
	/* debug check the validity of the input params */
	assert(data != NULL);
	/* do not allow frames shorter than 2 ms */
	assert(ms >= 2);
	assert(ts >= 0);
	assert(now >= 0);
	
	delivery = jb->rxcore + jb->delay + ts;
	
	/* check if the new frame is not too late */
	if(delivery < jb->next_delivery)
	{
		/* should drop the frame, but let first resynch_jb() check if this is not a jump in ts, or
		   the force resynch flag was not set. */
		return resynch_jb(jb, data, ms, ts, now);
	}
	
	/* what if the delivery time is bigger than next + delay? Seems like a frame for the future.
	   However, allow more resync_threshold ms in advance */
	if(delivery > jb->next_delivery + jb->delay + jb->conf.resync_threshold)
	{
		/* should drop the frame, but let first resynch_jb() check if this is not a jump in ts, or
		   the force resynch flag was not set. */
		return resynch_jb(jb, data, ms, ts, now);
	}

	/* find the right place in the frames list, sorted by delivery time */
	frame = jb->tail;
	while(frame != NULL && frame->delivery > delivery)
	{
		frame = frame->prev;
	}
	
	/* Check if the new delivery time is not covered already by the chosen frame */
	if(frame && (frame->delivery == delivery ||
		         delivery < frame->delivery + frame->ms ||
		         (frame->next && delivery + ms > frame->next->delivery)))
	{
		/* TODO: Should we check for resynch here? Be careful to do not allow threshold smaller than
		   the size of the jb */
		
		/* should drop the frame, but let first resynch_jb() check if this is not a jump in ts, or
		   the force resynch flag was not set. */
		return resynch_jb(jb, data, ms, ts, now);
	}
	
	/* Reset the force resynch flag */
	jb->force_resynch = 0;
	
	/* Get a new frame */
	newframe = alloc_jb_frame(jb);
	newframe->data = data;
	newframe->ts = ts;
	newframe->ms = ms;
	newframe->delivery = delivery;
	
	/* and insert it right on place */
	if(frame != NULL)
	{
		next = frame->next;
		frame->next = newframe;
		if(next != NULL)
		{
			newframe->next = next;
			next->prev = newframe;
		}
		else
		{
			/* insert after the last frame - should update tail */
			jb->tail = newframe;
			newframe->next = NULL;
		}
		newframe->prev = frame;
		
		return SCX_JB_OK;
	}
	else if(jb->frames == NULL)
	{
		/* the frame list is empty or thats just the first frame ever */
		/* tail should also be NULL is that case */
		assert(jb->tail == NULL);
		jb->frames = jb->tail = newframe;
		newframe->next = NULL;
		newframe->prev = NULL;
		
		return SCX_JB_OK;
	}
	else
	{
		/* insert on a first position - should update frames head */
		newframe->next = jb->frames;
		newframe->prev = NULL;
		jb->frames->prev = newframe;
		jb->frames = newframe;
		
		return SCX_JB_OK;
	}
}


int scx_jb_get(struct scx_jb *jb, struct scx_jb_frame *frame, long now, long interpl)
{
	assert(now >= 0);
	assert(interpl >= 2);
	
	if(now < jb->next_delivery)
	{
		/* too early for the next frame */
		return SCX_JB_NOFRAME;
	}
	
	/* Is the jb empty? */
	if(jb->frames == NULL)
	{
		/* should interpolate a frame */
		/* update next */
		jb->next_delivery += interpl;
		
		return SCX_JB_INTERP;
	}
	
	/* Isn't it too late for the first frame available in the jb? */
	if(now > jb->frames->delivery + jb->frames->ms)
	{
		/* yes - should drop this frame and update next to point the next frame (get_jb_head() does it) */
		get_jb_head(jb, frame);
		
		return SCX_JB_DROP;
	}
	
	/* isn't it too early to play the first frame available? */
	if(now < jb->frames->delivery)
	{
		/* yes - should interpolate one frame */
		/* update next */
		jb->next_delivery += interpl;
		
		return SCX_JB_INTERP;
	}
	
	/* we have a frame for playing now (get_jb_head() updates next) */
	get_jb_head(jb, frame);
	
	return SCX_JB_OK;
}


long scx_jb_next(struct scx_jb *jb)
{
	return jb->next_delivery;
}


int scx_jb_remove(struct scx_jb *jb, struct scx_jb_frame *frameout)
{
	if(jb->frames == NULL)
	{
		return SCX_JB_NOFRAME;
	}
	
	get_jb_head(jb, frameout);
	
	return SCX_JB_OK;
}



