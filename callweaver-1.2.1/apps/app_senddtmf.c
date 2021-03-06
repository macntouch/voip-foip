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
 * \brief App to send DTMF digits
 * 
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h> 
#include <string.h>
#include <stdlib.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/apps/app_senddtmf.c $", "$Revision: 4723 $")

#include "callweaver/lock.h"
#include "callweaver/file.h"
#include "callweaver/logger.h"
#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/module.h"
#include "callweaver/translate.h"
#include "callweaver/options.h"
#include "callweaver/utils.h"
#include "callweaver/app.h"

static char *tdesc = "Send DTMF digits Application";

static void *senddtmf_app;
static const char *senddtmf_name = "SendDTMF";
static const char *senddtmf_synopsis = "Sends arbitrary DTMF digits";
static const char *senddtmf_syntax = "SendDTMF(digits[, timeout_ms])";
static const char *senddtmf_descrip = 
"Sends DTMF digits on a channel. \n"
" Accepted digits: 0-9, *#abcd\n"
" Returns 0 on success or -1 on a hangup.\n";

STANDARD_LOCAL_USER;

LOCAL_USER_DECL;

static int senddtmf_exec(struct cw_channel *chan, int argc, char **argv)
{
	int res = 0;
	struct localuser *u;
	int timeout = 250;

	if (argc < 1 || argc > 2 || !argv[0][0]) {
		cw_log(LOG_ERROR, "Syntax: %s\n", senddtmf_syntax);
		return -1;
	}

	LOCAL_USER_ADD(u);

	timeout = (argc > 1 ? atoi(argv[1]) : 0);
	if (timeout <= 0)
		timeout = 250;

	res = cw_dtmf_stream(chan, NULL, argv[0], timeout);
		
	LOCAL_USER_REMOVE(u);

	return res;
}

int unload_module(void)
{
	int res = 0;
	STANDARD_HANGUP_LOCALUSERS;
	res |= cw_unregister_application(senddtmf_app);
	return res;
}

int load_module(void)
{
	senddtmf_app = cw_register_application(senddtmf_name, senddtmf_exec, senddtmf_synopsis, senddtmf_syntax, senddtmf_descrip);
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


