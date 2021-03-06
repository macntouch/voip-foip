/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (c) 2004 - 2005 Tilghman Lesher.  All rights reserved.
 *
 * Tilghman Lesher <app_verbose_v001@the-tilghman.com>
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
 * \brief Verbose logging application
 *
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/apps/app_verbose.c $", "$Revision: 4723 $")

#include "callweaver/options.h"
#include "callweaver/logger.h"
#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/module.h"


static char *tdesc = "Send verbose output";

static void *verbose_app;
static const char *verbose_name = "Verbose";
static const char *verbose_synopsis = "Send arbitrary text to verbose output";
static const char *verbose_syntax = "Verbose([level, ]message)";
static const char *verbose_descrip =
"  level must be an integer value.  If not specified, defaults to 0."
"  Always returns 0.\n";

STANDARD_LOCAL_USER;

LOCAL_USER_DECL;

static int verbose_exec(struct cw_channel *chan, int argc, char **argv)
{
	static char *prefix[] = {
		"",
		VERBOSE_PREFIX_1,
		VERBOSE_PREFIX_2,
		VERBOSE_PREFIX_3,
		VERBOSE_PREFIX_4,
	};
	int level;
	struct localuser *u;

	level = 0;
	if (argc == 2 && isdigit(argv[0][0])) {
		level = atoi(argv[0]);
		argv++, argc--;
	}

	if (argc != 1 || level < 0 || level >= arraysize(prefix)) {
		cw_log(LOG_ERROR, "Syntax: %s\n", verbose_syntax);
		return -1;
	}

	LOCAL_USER_ADD(u);
	cw_verbose("%s%s\n", prefix[level], argv[0]);
	LOCAL_USER_REMOVE(u);

	return 0;
}

int unload_module(void)
{
	int res = 0;
	STANDARD_HANGUP_LOCALUSERS;
	res |= cw_unregister_application(verbose_app);
	return res;
}

int load_module(void)
{
	verbose_app = cw_register_application(verbose_name, verbose_exec, verbose_synopsis, verbose_syntax, verbose_descrip);
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


