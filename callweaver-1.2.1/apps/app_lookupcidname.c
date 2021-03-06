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
 * \brief App to set caller ID name from database, based on directory number
 * 
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/apps/app_lookupcidname.c $", "$Revision: 4723 $")

#include "callweaver/lock.h"
#include "callweaver/file.h"
#include "callweaver/logger.h"
#include "callweaver/options.h"
#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/module.h"
#include "callweaver/translate.h"
#include "callweaver/image.h"
#include "callweaver/phone_no_utils.h"
#include "callweaver/callweaver_db.h"

static char *tdesc = "Look up CallerID Name from local database";

static void *app;
static const char *name = "LookupCIDName";
static const char *synopsis = "Look up CallerID Name from local database";
static const char *syntax = "LookupCIDName()";
static const char *descrip =
  "Looks up the Caller*ID number on the active\n"
  "channel in the CallWeaver database (family 'cidname') and sets the\n"
  "Caller*ID name.  Does nothing if no Caller*ID was received on the\n"
  "channel.  This is useful if you do not subscribe to Caller*ID\n"
  "name delivery, or if you want to change the names on some incoming\n"
  "calls.  Always returns 0.\n";


STANDARD_LOCAL_USER;

LOCAL_USER_DECL;

static int
lookupcidname_exec (struct cw_channel *chan, int argc, char **argv)
{
  char dbname[64];
  struct localuser *u;

  LOCAL_USER_ADD (u);
  if (chan->cid.cid_num) {
	if (!cw_db_get ("cidname", chan->cid.cid_num, dbname, sizeof (dbname))) {
		cw_set_callerid (chan, NULL, dbname, NULL);
		  if (option_verbose > 2)
		    cw_verbose (VERBOSE_PREFIX_3 "Changed Caller*ID name to %s\n",
				 dbname);
	}
  }
  LOCAL_USER_REMOVE (u);
  return 0;
}

int
unload_module (void)
{
  int res = 0;
  STANDARD_HANGUP_LOCALUSERS;
  res |= cw_unregister_application (app);
  return res;
}

int
load_module (void)
{
  app = cw_register_application(name, lookupcidname_exec, synopsis, syntax, descrip);
  return 0;
}

char *
description (void)
{
  return tdesc;
}

int
usecount (void)
{
  int res;
  STANDARD_USECOUNT (res);
  return res;
}
