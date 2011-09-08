/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 2005, Joshua Colp
 *
 * Joshua Colp <jcolp@asterlink.com>
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
 * \brief Directed Call Pickup Support
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

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/apps/app_directed_pickup.c $", "$Revision: 5369 $")

#include "callweaver/file.h"
#include "callweaver/logger.h"
#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/module.h"
#include "callweaver/lock.h"
#include "callweaver/app.h"

static const char *tdesc = "Directed Call Pickup Application";

#define PICKUPMARK "PICKUPMARK"

static void *pickup_app;
static const char *pickup_name = "Pickup";
static const char *pickup_synopsis = "Directed Call Pickup application.";
static const char *pickup_syntax = "Pickup(extension[@context])";
static const char *pickup_descrip =
"Steals any calls to a specified extension that are in a ringing state and bridges them to the current channel. Context is an optional argument.\n";

STANDARD_LOCAL_USER;

LOCAL_USER_DECL;

static struct cw_channel * find_channel_by_mark(char *mark)
{
    struct cw_channel * res= NULL;
    const char *tmp = NULL;
    struct cw_channel *cur = NULL;

      while ( (cur = cw_channel_walk_locked(cur)) != NULL) 
      {
	  if (!cur->pbx &&
             ((cur->_state == CW_STATE_RINGING) ||
             (cur->_state == CW_STATE_RING))) 
	     {
		tmp = pbx_builtin_getvar_helper(cur, PICKUPMARK);
		if (tmp &&  (!strcasecmp(tmp, mark))) 
		    {
			res=cur;
                        break;
		    };
    	    };
          cw_mutex_unlock(&cur->lock);
      };
return res;
}

static int pickup_exec(struct cw_channel *chan, int argc, char **argv)
{
	char workspace[256] = "";
	struct localuser *u = NULL;
	struct cw_channel *origin = NULL, *target = NULL;
	char *tmp = NULL, *exten = NULL, *context = NULL;
	int res = 0, locked = 0;

	if (argc != 1) {
		cw_log(LOG_ERROR, "Syntax: %s\n", pickup_syntax);
		return -1;	
	}

	LOCAL_USER_ADD(u);
	
	/* Get the extension and context if present */
	exten = argv[0];
	context = strchr(argv[0], '@');
	if (context) {
		*context = '\0';
		context++;
	}

	/* Find a channel to pickup */
if (context && (!strcmp(context,PICKUPMARK))) {
target=find_channel_by_mark(exten);
} else {
origin = cw_get_channel_by_exten_locked(exten, context);
	if (origin) {
		/* AGX: using CDR=null has caused some crash in my systems, so lets check it now */
		if (origin->cdr)
			cw_cdr_getvar(origin->cdr, "dstchannel", &tmp, workspace,
			       sizeof(workspace), 0);
		if (tmp) {
			/* We have a possible channel... now we need to find it! */
			target = cw_get_channel_by_name_locked(tmp);
			if (target)
			  locked = 1;
		} else {
			cw_log(LOG_DEBUG, "No target channel found.\n");
			res = -1;
		}
		cw_mutex_unlock(&origin->lock);
	} else {
		cw_log(LOG_DEBUG, "No originating channel found.\n");
	}
	
	if (res)
		goto out;
}
	if (target && (!target->pbx) && ((target->_state == CW_STATE_RINGING) || (target->_state == CW_STATE_RING))) {
		cw_log(LOG_DEBUG, "Call pickup on chan '%s' by '%s'\n", target->name,
			chan->name);
		res = cw_answer(chan);
		if (res) {
			cw_log(LOG_WARNING, "Unable to answer '%s'\n", chan->name);
			res = -1;
			goto out;
		}
		res = cw_queue_control(chan, CW_CONTROL_ANSWER);
		if (res) {
			cw_log(LOG_WARNING, "Unable to queue answer on '%s'\n",
				chan->name);
			res = -1;
			goto out;
		}
		res = cw_channel_masquerade(target, chan);
		if (res) {
			cw_log(LOG_WARNING, "Unable to masquerade '%s' into '%s'\n", chan->name, target->name);
			res = -1;
			goto out;
		}
	} else {
		cw_log(LOG_DEBUG, "No call pickup possible...\n");
		res = -1;
	}
	
 out:
	if (target)
		cw_mutex_unlock(&target->lock);

	LOCAL_USER_REMOVE(u);

	return res;
}

int unload_module(void)
{
	int res = 0;
	STANDARD_HANGUP_LOCALUSERS;
	res |= cw_unregister_application(pickup_app);
	return res;
}

int load_module(void)
{
	pickup_app = cw_register_application(pickup_name, pickup_exec, pickup_synopsis, pickup_syntax, pickup_descrip);
	return 0;
}

char *description(void)
{
	return (char *) tdesc;
}

int usecount(void)
{
	int res;

	STANDARD_USECOUNT(res);

	return res;
}


