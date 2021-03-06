/* comments and license {{{
 * vim:ts=4:sw=4:smartindent:cindent:autoindent:foldmethod=marker
 *
 * CallWeaver -- an open source telephony toolkit.
 *
 * Copyright (c) 1999 - 2005, digium, inc.
 *
 * Roy Sigurd Karlsbakk <roy@karlsbakk.net>
 *
 * See http://www.callweaver.org for more information about
 * the callweaver project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and irc
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the gnu general public license version 2. See the license file
 * at the top of the source tree.
 *
 * }}} */

/* includes and so on {{{ */
/*! \roy
 *
 * \brief functions for reading global configuration data
 * 
 */
#ifdef HAVE_CONFIG_H
#include "confdefs.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "callweaver.h"

CALLWEAVER_FILE_VERSION("$HeadURL: http://svn.callweaver.org/callweaver/tags/rel/1.2.1/funcs/func_config.c $", "$Revision: 4723 $")

#include "callweaver/module.h"
#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/utils.h"
/* }}} */

/* globals {{{ */
static void *config_function;
static const char *config_func_name = "CONFIG";
static const char *config_func_synopsis = "Read configuration values set in callweaver.conf";
static const char *config_func_syntax = "CONFIG(name)";
static const char *config_func_desc = "This function will read configuration values set in callweaver.conf.\n"
			"Possible values include cwctlpermissions, cwctlowner, cwctlgroup,\n"
			"cwctl, cwdb, cwetcdir, cwconfigdir, cwspooldir, cwvarlibdir,\n"
			"cwvardir, cwdbdir, cwlogdir, cwogidir, cwsoundsdir, and cwrundir\n";
/* }}} */

/* function_config_read() {{{ */
static char *function_config_read(struct cw_channel *chan, int argc, char **argv, char *buf, size_t len)
{
/* These doesn't seem to be available outside callweaver.c
	if (strcasecmp(argv[0], "cwrunuser") == 0) {
		cw_copy_string(buf, cw_config_CW_RUN_USER, len);
	} else if (strcasecmp(argv[0], "cwrungroup") == 0) {
		cw_copy_string(buf, cw_config_CW_RUN_GROUP, len);
	} else if (strcasecmp(argv[0], "cwmoddir") == 0) {
		cw_copy_string(buf, cw_config_CW_MOD_DIR, len);
	} else
*/
	if (strcasecmp(argv[0], "cwctlpermissions") == 0) {
		cw_copy_string(buf, cw_config_CW_CTL_PERMISSIONS, len);
	} else if (strcasecmp(argv[0], "cwctlowner") == 0) {
		cw_copy_string(buf, cw_config_CW_CTL_OWNER, len);
	} else if (strcasecmp(argv[0], "cwctlgroup") == 0) {
		cw_copy_string(buf, cw_config_CW_CTL_GROUP, len);
	} else if (strcasecmp(argv[0], "cwctl") == 0) {
		cw_copy_string(buf, cw_config_CW_CTL, len);
	} else if (strcasecmp(argv[0], "cwdb") == 0) {
		cw_copy_string(buf, cw_config_CW_DB, len);
	} else if (strcasecmp(argv[0], "cwetcdir") == 0) {
		cw_copy_string(buf, cw_config_CW_CONFIG_DIR, len);
	} else if (strcasecmp(argv[0], "cwconfigdir") == 0) {
		cw_copy_string(buf, cw_config_CW_CONFIG_DIR, len);; /* cwetcdir alias */
	} else if (strcasecmp(argv[0], "cwspooldir") == 0) {
		cw_copy_string(buf, cw_config_CW_SPOOL_DIR, len);
	} else if (strcasecmp(argv[0], "cwvarlibdir") == 0) {
		cw_copy_string(buf, cw_config_CW_VAR_DIR, len);
	} else if (strcasecmp(argv[0], "cwvardir") == 0) {
		cw_copy_string(buf, cw_config_CW_VAR_DIR, len);; /* cwvarlibdir alias */
	} else if (strcasecmp(argv[0], "cwdbdir") == 0) {
		cw_copy_string(buf, cw_config_CW_DB_DIR, len);
	} else if (strcasecmp(argv[0], "cwlogdir") == 0) {
		cw_copy_string(buf, cw_config_CW_LOG_DIR, len);
	} else if (strcasecmp(argv[0], "cwogidir") == 0) {
		cw_copy_string(buf, cw_config_CW_OGI_DIR, len);
	} else if (strcasecmp(argv[0], "cwsoundsdir") == 0) {
		cw_copy_string(buf, cw_config_CW_SOUNDS_DIR, len);
	} else if (strcasecmp(argv[0], "cwrundir") == 0) {
		cw_copy_string(buf, cw_config_CW_RUN_DIR, len);
	} else if (strcasecmp(argv[0], "systemname") == 0) {
		cw_copy_string(buf, cw_config_CW_SYSTEM_NAME, len);
	} else if (strcasecmp(argv[0], "allowspaghetticode") == 0) {
		cw_copy_string(buf, cw_config_CW_ALLOW_SPAGHETTI_CODE, len);
	} else {
		cw_log(LOG_WARNING, "Config setting '%s' not known.\n", argv[0]);
	}

	return buf;
}
/* function_config_read() }}} */

/* function_config_write() {{{ */
static void function_config_write(struct cw_channel *chan, int argc, char **argv, const char *value) 
{
	cw_log(LOG_WARNING, "This function currently cannot be used to change the CallWeaver config. Modify callweaver.conf manually and restart.\n");
}
/* function_config_write() }}} */

/* globals {{{ */
static char *tdesc = "CONFIG function";
/* globals }}} */

/* unload_module() {{{ */
int unload_module(void)
{
        return cw_unregister_function(config_function);
}
/* }}} */

/* load_module() {{{ */
int load_module(void)
{
        config_function = cw_register_function(config_func_name, function_config_read, function_config_write, config_func_synopsis, config_func_syntax, config_func_desc);
	return 0;
}
/* }}} */

/* description() {{{ */
char *description(void)
{
	return tdesc;
}
/* }}} */

/* usecount() {{{ */
int usecount(void)
{
	return 0;
}
/* }}} */

/* tail {{{

local variables:
mode: c
c-file-style: "linux"
indent-tabs-mode: nil
end:

function_config_read() }}} */
