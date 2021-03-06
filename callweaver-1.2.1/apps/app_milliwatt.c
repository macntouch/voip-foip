/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2005, Digium, Inc.
 *
 * Mark Spencer <markster@digium.com>
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
 * \brief Digital Milliwatt Test
 * 
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/apps/app_milliwatt.c $", "$Revision: 4723 $")

#include "callweaver/lock.h"
#include "callweaver/file.h"
#include "callweaver/logger.h"
#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/module.h"

static char *tdesc = "Digital Milliwatt (mu-law) Test Application";

static void *milliwatt_app;
static char *milliwatt_name = "Milliwatt";
static char *milliwatt_synopsis = "Generate a Constant 1000Hz tone at 0dbm (mu-law)";
static char *milliwatt_syntax = "Milliwatt()";
static char *milliwatt_descrip = 
"Generate a Constant 1000Hz tone at 0dbm (mu-law)\n";


STANDARD_LOCAL_USER;

LOCAL_USER_DECL;

static char digital_milliwatt[] = {0x1e,0x0b,0x0b,0x1e,0x9e,0x8b,0x8b,0x9e} ;

static void *milliwatt_alloc(struct cw_channel *chan, void *params)
{
	int *indexp;
	indexp = malloc(sizeof(int));
	if (indexp == NULL) return(NULL);
	*indexp = 0;
	return(indexp);
}

static void milliwatt_release(struct cw_channel *chan, void *data)
{
	free(data);
	return;
}

static int milliwatt_generate(struct cw_channel *chan, void *data, int samples)
{
	struct cw_frame wf;
	unsigned char waste[CW_FRIENDLY_OFFSET];
	unsigned char buf[640];
	int i, *indexp = (int *) data;

	if (samples > sizeof(buf))
	{
		cw_log(LOG_WARNING,"Only doing %d samples (%d requested)\n",(int)sizeof(buf),samples);
		samples = sizeof(buf);
	}
	waste[0] = 0; /* make compiler happy */
	cw_fr_init_ex(&wf, CW_FRAME_VOICE, CW_FORMAT_ULAW, "app_milliwatt");
	wf.offset = CW_FRIENDLY_OFFSET;
	wf.data = buf;
	wf.datalen = samples;
	wf.samples = samples;
	/* create a buffer containing the digital milliwatt pattern */
	for (i = 0;  i < samples;  i++)
	{
		buf[i] = digital_milliwatt[(*indexp)++];
		*indexp &= 7;
	}
	return cw_write(chan,&wf);
}

static struct cw_generator milliwattgen = 
{
	alloc: milliwatt_alloc,
	release: milliwatt_release,
	generate: milliwatt_generate,
} ;

static int milliwatt_exec(struct cw_channel *chan, int argc, char **argv)
{
	struct localuser *u;

	LOCAL_USER_ADD(u);

	cw_set_write_format(chan, CW_FORMAT_ULAW);
	cw_set_read_format(chan, CW_FORMAT_ULAW);
	if (chan->_state != CW_STATE_UP)
	{
		cw_answer(chan);
	}
	if (cw_generator_activate(chan,&milliwattgen,"milliwatt") < 0)
	{
		cw_log(LOG_WARNING,"Failed to activate generator on '%s'\n",chan->name);
		LOCAL_USER_REMOVE(u);
		return -1;
	}
	while(!cw_safe_sleep(chan, 10000));

	cw_generator_deactivate(chan);
	LOCAL_USER_REMOVE(u);
	return -1;
}

int unload_module(void)
{
	int res = 0;
	STANDARD_HANGUP_LOCALUSERS;
	res |= cw_unregister_application(milliwatt_app);
	return res;
}

int load_module(void)
{
	milliwatt_app = cw_register_application(milliwatt_name, milliwatt_exec, milliwatt_synopsis, milliwatt_syntax, milliwatt_descrip);
	return 0;
}

char *description(void)
{
	return tdesc;
}

int usecount(void)
{
	int res;
	STANDARD_USECOUNT(res);
	return res;
}


