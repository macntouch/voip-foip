/*
 * CallWeaver -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2005, Digium, Inc.
 *
 * Created by Olle E. Johansson, Edvina.net 
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
 * \brief URI encoding / decoding
 * 
 * \note For now this code only supports 8 bit characters, not unicode,
         which we ultimately will need to support.
 * 
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/funcs/func_uri.c $", "$Revision: 4723 $")

#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/logger.h"
#include "callweaver/utils.h"
#include "callweaver/app.h"
#include "callweaver/module.h"


static void *urldecode_function;
static const char *urldecode_func_name = "URIDECODE";
static const char *urldecode_func_synopsis = "Decodes an URI-encoded string.";
static const char *urldecode_func_syntax = "URIDECODE(data)";
static const char *urldecode_func_desc = "";

static void *urlencode_function;
static const char *urlencode_func_name = "URIENCODE";
static const char *urlencode_func_synopsis = "Encodes a string to URI-safe encoding.";
static const char *urlencode_func_syntax = "URIENCODE(data)";
static const char *urlencode_func_desc = "";


/*! \brief builtin_function_uriencode: Encode URL according to RFC 2396 */
static char *builtin_function_uriencode(struct cw_channel *chan, int argc, char **argv, char *buf, size_t len) 
{
	char uri[BUFSIZ];

	if (argc != 1 || !argv[0][0]) {
		cw_log(LOG_ERROR, "Syntax: %s\n", urlencode_func_syntax);
		return NULL;
	}

	cw_uri_encode(argv[0], uri, sizeof(uri), 1);
	cw_copy_string(buf, uri, len);

	return buf;
}

/*!\brief builtin_function_uridecode: Decode URI according to RFC 2396 */
static char *builtin_function_uridecode(struct cw_channel *chan, int argc, char **argv, char *buf, size_t len) 
{
	if (argc != 1 || !argv[0][0]) {
		cw_log(LOG_ERROR, "Syntax: %s\n", urldecode_func_syntax);
		return NULL;
	}
	
	cw_copy_string(buf, argv[0], len);
	cw_uri_decode(buf);
	return buf;
}


static char *tdesc = "URI encode/decode functions";

int unload_module(void)
{
	int res = 0;

        res |= cw_unregister_function(urldecode_function);
	res |= cw_unregister_function(urlencode_function);
	return res;
}

int load_module(void)
{
        urldecode_function = cw_register_function(urldecode_func_name, builtin_function_uridecode, NULL, urldecode_func_synopsis, urldecode_func_syntax, urldecode_func_desc);
	urlencode_function = cw_register_function(urlencode_func_name, builtin_function_uriencode, NULL, urlencode_func_synopsis, urlencode_func_syntax, urlencode_func_desc);
	return 0;
}

char *description(void)
{
	return tdesc;
}

int usecount(void)
{
	return 0;
}
