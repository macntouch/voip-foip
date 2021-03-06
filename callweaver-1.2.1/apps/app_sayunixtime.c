/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (c) 2003 Tilghman Lesher.  All rights reserved.
 *
 * Tilghman Lesher <app_sayunixtime__200309@the-tilghman.com>
 *
 * This code is released by the author with no restrictions on usage.
 *
 * See http://www.callweaver.org for more information about
 * the CallWeaver project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 */

/*! \file
 *
 * \brief SayUnixTime application
 * 
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/apps/app_sayunixtime.c $", "$Revision: 4723 $")

#include "callweaver/file.h"
#include "callweaver/logger.h"
#include "callweaver/options.h"
#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/module.h"
#include "callweaver/say.h"


static char *tdesc = "Say time";

static void *sayunixtime_app;
static char *sayunixtime_name = "SayUnixTime";
static char *sayunixtime_synopsis = "Says a specified time in a custom format";
static char *sayunixtime_syntax = "SayUnixTime([unixtime][, timezone[, format]])";
static char *sayunixtime_descrip =
"  unixtime: time, in seconds since Jan 1, 1970.  May be negative.\n"
"              defaults to now.\n"
"  timezone: timezone, see /usr/share/zoneinfo for a list.\n"
"              defaults to machine default.\n"
"  format:   a format the time is to be said in.  See voicemail.conf.\n"
"              defaults to \"ABdY 'digits/at' IMp\"\n"
"  Returns 0 or -1 on hangup.\n";

static void *datetime_app;
static char *datetime_name = "DateTime";
static char *datetime_syntax = "DateTime([unixtime][, timezone[, format]])";
static char *datetime_descrip =
"  unixtime: time, in seconds since Jan 1, 1970.  May be negative.\n"
"              defaults to now.\n"
"  timezone: timezone, see /usr/share/zoneinfo for a list.\n"
"              defaults to machine default.\n"
"  format:   a format the time is to be said in.  See voicemail.conf.\n"
"              defaults to \"ABdY 'digits/at' IMp\"\n"
"  Returns 0 or -1 on hangup.\n";

STANDARD_LOCAL_USER;

LOCAL_USER_DECL;

static int sayunixtime_exec(struct cw_channel *chan, int argc, char **argv)
{
	struct timeval tv;
	time_t unixtime;
	struct localuser *u;
	char *format;
	int res=0;

	if (argc > 3) {
		cw_log(LOG_ERROR, "Syntax: %s\n", sayunixtime_syntax);
		return -1;
	}

	LOCAL_USER_ADD(u);

	if (argc < 1 || !(unixtime = (time_t)atol(argv[0]))) {
		tv = cw_tvnow();
		unixtime = (time_t)tv.tv_sec;
	}

	if (argc > 2 && argv[2][0]) {
		format = argv[2];
	} else if (!strcasecmp(chan->language, "da")) {
		format = "A dBY HMS";
	} else if (!strcasecmp(chan->language, "de")) {
		format = "A dBY HMS";
	} else {
		format = "ABdY 'digits/at' IMp";
	} 

	if (chan->_state != CW_STATE_UP)
		res = cw_answer(chan);

	if (!res)
		res = cw_say_date_with_format(chan, unixtime, CW_DIGIT_ANY, chan->language, format, (argc > 1 && argv[1][0] ? argv[1] : NULL));

	LOCAL_USER_REMOVE(u);
	return res;
}

int unload_module(void)
{
	int res = 0;
	STANDARD_HANGUP_LOCALUSERS;
	res |= cw_unregister_application(sayunixtime_app);
	res |= cw_unregister_application(datetime_app);
	return res;
}

int load_module(void)
{
	sayunixtime_app = cw_register_application(sayunixtime_name, sayunixtime_exec, sayunixtime_synopsis, sayunixtime_syntax, sayunixtime_descrip);
	datetime_app = cw_register_application(datetime_name, sayunixtime_exec, sayunixtime_synopsis, datetime_syntax, datetime_descrip);
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


