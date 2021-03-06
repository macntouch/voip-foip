/* $Id$
 *
 * modules/plug-in structures declarations
 *
 *
 * Copyright (C) 2001-2003 FhG Fokus
 *
 * This file is part of ser, a free SIP server.
 *
 * ser is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * For a license to use the ser software under conditions
 * other than those described here, or to purchase support for this
 * software, please contact iptel.org by e-mail at the following addresses:
 *    info@iptel.org
 *
 * ser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * History:
 * --------
 *  2003-03-10  changed module exports interface: added struct cmd_export
 *               and param_export (andrei)
 *  2003-03-16  Added flags field to cmd_export_ (janakj)
 *  2003-04-05  s/reply_route/failure_route, onreply_route introduced (jiri)
 *  2004-03-12  extra flag USE_FUNC_PARAM added to modparam type -
 *              instead of copying the param value, a func is called (bogdan)
 *  2004-09-19  switched to version.h for the module versions checks (andrei)
 *  2004-12-03  changed param_func_t to (modparam_t, void*), killed
 *               param_func_param_t   (andrei)
 *  2007-06-07  added PROC_INIT, called in the main process context
 *               (same as PROC_MAIN), buf guaranteed to be called before
 *               any other process is forked (andrei)
 *  2008-11-17  sip-router version: includes some of the openser/kamailio
 *               changes: f(void) instead of f(), free_fixup_function()
 *              dual module interface support: ser & kamailio (andrei)
 *  2008-11-18  prototypes for various fixed parameters numbers module
 *               functions (3, 4, 5 & 6) and variable parameters (andrei)
 *  2008-11-26  added fparam_free_contents() and fix_param_types (andrei)
 */

/** modules structures/exports declarations and utilities (fixups a.s.o).
 * @file sr_module.h
 */


#ifndef sr_module_h
#define sr_module_h

#include <dlfcn.h>

#include "parser/msg_parser.h" /* for sip_msg */
#include "ver_defs.h"
#include "rpc.h"
#include "route_struct.h"
#include "route.h"
#include "str.h"

/* kamailio compat */
#include "kstats_types.h"
#include "mi/mi_types.h"
#include "pvar.h"



#if defined KAMAILIO_MOD_INTERFACE || defined OPENSER_MOD_INTERFACE || \
	defined MOD_INTERFACE_V1

#define MODULE_INTERFACE_VER 1
#define cmd_export_t kam_cmd_export_t
#define module_exports kam_module_exports

#elif defined SER_MOD_INTERFACE || defined MOD_INTERFACE_V0

#define MODULE_INTERFACE_VER 0
#define cmd_export_t ser_cmd_export_t
#define module_exports ser_module_exports

#else

/* do nothing for core */

#endif

/** type used for the mod_register function export.
 *  mod_register is a function called when loading a module
 *  (if present), prior to registering the module exports.
 *  @param path - path to the module, including file name
 *  @param dlflags - pointer to the dlflags used when loading the module.
 *                   If the value is changed to a different and non-zero
 *                   value, the module will be reloaded with the new flags.
 *  @param reserved1 - reserved for future use.
 *  @param reserved2 - reserver for future use
 *  @return 0 on success, -1 on error, all the other values are reserved
 *                      for future use (<0 meaning error and >0 success)
 */
typedef  int (*mod_register_function)(char*, int*, void*, void*);

typedef  struct module_exports* (*module_register)(void);
typedef  int (*cmd_function)(struct sip_msg*, char*, char*);
typedef  int (*cmd_function3)(struct sip_msg*, char*, char*, char*);
typedef  int (*cmd_function4)(struct sip_msg*, char*, char*, char*, char*);
typedef  int (*cmd_function5)(struct sip_msg*,  char*, char*, char*,
												char*, char*);
typedef  int (*cmd_function6)(struct sip_msg*,  char*, char*, char*,
												char*, char*, char*);
/* variable number of param module function, takes as param the sip_msg,
   extra paremeters number and a pointer to an array of parameters */
typedef  int (*cmd_function_var)(struct sip_msg*, int no, action_u_t* vals );
typedef  int (*fixup_function)(void** param, int param_no);
typedef  int (*free_fixup_function)(void** param, int param_no);
typedef  int (*response_function)(struct sip_msg*);
typedef  void (*onbreak_function)(struct sip_msg*);
typedef void (*destroy_function)(void);

typedef int (*init_function)(void);
typedef int (*child_init_function)(int rank);


#define PARAM_STRING     (1U<<0)  /* String (char *) parameter type */
#define PARAM_INT        (1U<<1)  /* Integer parameter type */
#define PARAM_STR        (1U<<2)  /* struct str parameter type */
#define PARAM_USE_FUNC   (1U<<(8*sizeof(int)-1))
#define PARAM_TYPE_MASK(_x)   ((_x)&(~PARAM_USE_FUNC))

/* temporary, for backward compatibility only until all modules adjust it */
#define STR_PARAM PARAM_STRING
#define INT_PARAM PARAM_INT
#define USE_FUNC_PARAM PARAM_USE_FUNC

typedef unsigned int modparam_t;

typedef int (*param_func_t)( modparam_t type, void* val);

/* magic parameter number values */

#define NO_SCRIPT     -1    /* export not usable from scripts */
#define VAR_PARAM_NO  -128  /* function has variable number of parameters
							   (see cmd_function_var for the prototype) */

/* special fixup function flags.
 * They are kept in the first 2 bits inside the pointer
 */
#define FIXUP_F_FPARAM_RVE (unsigned long)1 /* fparam fixup, rve ready */

#define call_fixup(fixup, param, param_no) \
	((fixup) ? (fixup)(param, param_no) : 0)

/* Macros - used as rank in child_init function */
#define PROC_MAIN      0  /* Main ser process */
#define PROC_TIMER    -1  /* Timer attendant process */
#define PROC_RPC      -2  /* RPC type process */
#define PROC_FIFO      PROC_RPC  /* FIFO attendant process */
#define PROC_TCP_MAIN -4  /* TCP main process */
#define PROC_UNIXSOCK -5  /* Unix socket server */
#define PROC_ATTENDANT -10  /* main "attendant process */
#define PROC_INIT     -127 /* special rank, the context is the main ser
							  process, but this is guaranteed to be executed
							  before any process is forked, so it can be used
							  to setup shared variables that depend on some
							  after mod_init available information (e.g.
							  total number of processes).
							 WARNING: child_init(PROC_MAIN) is again called
							 in the same process (main), but latter 
							 (before tcp), so make sure you don't init things 
							 twice, bot in PROC_MAIN and PROC_INT */
#define PROC_NOCHLDINIT -128 /* no child init functions will be called
                                if this rank is used in fork_process() */

#define PROC_MIN PROC_NOCHLDINIT /* Minimum process rank */


#define DEFAULT_DLFLAGS	0 /* value that signals to module loader to
							use default dlopen flags in Kamailio */
#ifndef RTLD_NOW
/* for openbsd */
#define RTLD_NOW DL_LAZY
#endif

#define KAMAILIO_DLFLAGS	RTLD_NOW


#define MODULE_VERSION \
	char *module_version=SER_FULL_VERSION; \
	char *module_flags=SER_COMPILE_FLAGS; \
	unsigned int module_interface_ver=MODULE_INTERFACE_VER; 

/* ser version */
struct ser_cmd_export_ {
	char* name;             /* null terminated command name */
	cmd_function function;  /* pointer to the corresponding function */
	int param_no;           /* number of parameters used by the function */
	fixup_function fixup;   /* pointer to the function called to "fix" the
							   parameters */
	int flags;              /* Function flags */
};


/* kamailo/openser version */
struct kam_cmd_export_ {
	char* name;             /* null terminated command name */
	cmd_function function;  /* pointer to the corresponding function */
	int param_no;           /* number of parameters used by the function */
	fixup_function fixup;   /* pointer to the function called to "fix" the
							   parameters */
	free_fixup_function free_fixup; /* function called to free the "fixed"
									   parameters */
	int flags;              /* Function flags */
};


struct sr31_cmd_export_ {
	char* name;             /* null terminated command name */
	cmd_function function;  /* pointer to the corresponding function */
	int param_no;           /* number of parameters used by the function */
	fixup_function fixup;   /* pointer to the function called to "fix" the
							   parameters */
	free_fixup_function free_fixup; /* function called to free the "fixed"
									   parameters */
	int flags;              /* Function flags */
	int fixup_flags;
	void* module_exports; /* pointer to module structure */
};


/* members situated at the same place in memory in both ser & kamailio
   cmd_export */
struct cmd_export_common_ {
	char* name;
	cmd_function function; 
	int param_no;
	fixup_function fixup;
};


struct param_export_ {
	char* name;             /* null terminated param. name */
	modparam_t type;        /* param. type */
	void* param_pointer;    /* pointer to the param. memory location */
};



/** allowed parameter types.
  * the types _must_ be in "fallback" order,
  * e.g. FPARAM_STR should be the last to allow fallback to it,
  *  F_PARAM_PVS should be in front of F_PARAM_AVP (so that
  *  for fix_param_types(FPARAM_AVP|FPARAM_PVS|FPARAM_STR, param) and $foo
  *  the pvars will be checked first and only if no pvar is found the
  *  param will be resolved to an avp)
  */
enum {
	FPARAM_UNSPEC = 0,
	FPARAM_INT    = (1 << 0),
	FPARAM_SELECT = (1 << 1),
	FPARAM_PVS    = (1 << 2),
	FPARAM_AVP    = (1 << 3),
	FPARAM_STRING = (1 << 4),
	FPARAM_STR    = (1 << 5),
	/* special types: no fallback between them possible */
	FPARAM_REGEX  = (1 << 6),
	FPARAM_SUBST  = (1 << 7),
	FPARAM_PVE    = (1 << 8)
};

/*
 * Function parameter
 */
typedef struct fparam {
	char* orig;                       /* The original value */
	int type;                         /* Type of parameter */
	union {
		char* asciiz;             /* Zero terminated ASCII string */
		struct _str str;          /* pointer/len string */
		int i;                    /* Integer value */
		regex_t* regex;           /* Compiled regular expression */
		avp_ident_t avp;          /* AVP identifier */
		select_t* select;         /* select structure */ 
		struct subst_expr* subst; /* Regex substitution */
		pv_spec_t* pvs;    /* kamailio pseudo-vars */
		pv_elem_t* pve;    /* kamailio pseudo-vars in a string */
	} v;
	void *fixed;
} fparam_t;


typedef struct param_export_ param_export_t;  
typedef struct ser_cmd_export_ ser_cmd_export_t;
typedef struct kam_cmd_export_ kam_cmd_export_t;
typedef struct cmd_export_common_ cmd_export_common_t;
typedef struct sr31_cmd_export_ sr31_cmd_export_t;

#if 0
union cmd_export_u{
	cmd_export_common_t c; /* common members for everybody */
	ser_cmd_export_t v0;
	kam_cmd_export_t v1;
};
#endif


/* ser module exports version */
struct ser_module_exports {
	char* name;                     /* null terminated module name */
	ser_cmd_export_t* cmds;         /* null terminated array of the exported
									   commands */
	rpc_export_t* rpc_methods;      /* null terminated array of exported rpc methods */
	param_export_t* params;         /* null terminated array of the exported
									   module parameters */
	init_function init_f;           /* Initialization function */
	response_function response_f;   /* function used for responses,
									   returns yes or no; can be null */
	destroy_function destroy_f;     /* function called when the module should
									   be "destroyed", e.g: on ser exit;
									   can be null */
	onbreak_function onbreak_f;
	child_init_function init_child_f;  /* function called by all processes
										  after the fork */
};


/* kamailio/openser proc_export (missing from ser) */
typedef void (*mod_proc)(int no);

typedef int (*mod_proc_wrapper)(void);

struct proc_export_ {
	char *name;
	mod_proc_wrapper pre_fork_function;
	mod_proc_wrapper post_fork_function;
	mod_proc function;
	unsigned int no;
};

typedef struct proc_export_ proc_export_t;


/* kamailio/openser module exports version */
struct kam_module_exports {
	char* name;                     /* null terminated module name */
	unsigned int dlflags;			/*!< flags for dlopen  */
	kam_cmd_export_t* cmds;			/* null terminated array of the exported
									   commands */
	param_export_t* params;			/* null terminated array of the exported
									   module parameters */
	stat_export_t* stats;			/*!< null terminated array of the exported
									  module statistics */
	mi_export_t* mi_cmds;			/*!< null terminated array of the exported
									  MI functions */
	pv_export_t* items;				/*!< null terminated array of the exported
									   module items (pseudo-variables) */
	proc_export_t* procs;			/*!< null terminated array of the
									  additional processes required by the
									  module */
	init_function init_f;           /* Initialization function */
	response_function response_f;   /* function used for responses,
									   returns yes or no; can be null */
	destroy_function destroy_f;     /* function called when the module should
									   be "destroyed", e.g: on ser exit;
									   can be null */
	child_init_function init_child_f;  /* function called by all processes
										  after the fork */
};



/** sr/ser 3.1+ module exports version.
 * Includes ser and kamailio versions, re-arraranged + some extras.
 * Note: some of the members will be obsoleted and are kept only for
 * backward compatibility (avoid re-writing all the modules exports
 * declarations).
 */
struct sr31_module_exports {
	char* name;                     /* null terminated module name */
	sr31_cmd_export_t* cmds;      /* null terminated array of the exported
									   commands */
	param_export_t* params;         /* null terminated array of the exported
									   module parameters */
	init_function init_f;           /* Initialization function */
	response_function response_f;   /* function used for responses,
									   returns yes or no; can be null */
	destroy_function destroy_f;     /* function called when the module should
									   be "destroyed", e.g: on ser exit;
									   can be null */
	onbreak_function onbreak_f;
	child_init_function init_child_f;  /* function called by all processes
										  after the fork */
	unsigned int dlflags;           /**< flags for dlopen */
	/* ser specific exports
	   (to be obsoleted and replaced by register_...) */
	rpc_export_t* rpc_methods;      /* null terminated array of exported
									   rpc methods */
	/* kamailio specific exports
	   (to be obsoleted and replaced by register_...) */
	stat_export_t* stats;			/*!< null terminated array of the exported
									  module statistics */
	mi_export_t* mi_cmds;			/*!< null terminated array of the exported
									  MI functions */
	pv_export_t* items;				/*!< null terminated array of the exported
									   module items (pseudo-variables) */
	proc_export_t* procs;			/*!< null terminated array of the
									  additional processes required by the
									  module */
};



/* module exports in the same place in memory in both ser & kamailio */
struct module_exports_common{
	char* name;
};


union module_exports_u {
		struct module_exports_common c; /*common members for all the versions*/
		struct ser_module_exports v0;
		struct kam_module_exports v1;
};


struct sr_module{
	char* path;
	void* handle;
	unsigned int orig_mod_interface_ver;
	struct sr31_module_exports exports;
	struct sr_module* next;
};


extern struct sr_module* modules; /* global module list*/
extern response_function* mod_response_cbks;/* response callback array */
extern int mod_response_cbk_no;    /* size of reponse callbacks array */

int register_builtin_modules(void);
int load_module(char* path);
sr31_cmd_export_t* find_export_record(char* name, int param_no, int flags,
										unsigned *ver);
cmd_function find_export(char* name, int param_no, int flags);
cmd_function find_mod_export(char* mod, char* name, int param_no, int flags);
rpc_export_t* find_rpc_export(char* name, int flags);
void destroy_modules(void);
int init_child(int rank);
int init_modules(void);
struct sr_module* find_module_by_name(char* mod);

/* true if the module with name 'mod_name' is loaded */
#define module_loaded(mod_name) (find_module_by_name(mod_name)!=0)


/*! \brief
 * Find a parameter with given type and return it's
 * address in memory
 * If there is no such parameter, NULL is returned
 */
void* find_param_export(struct sr_module* mod, char* name, modparam_t type_mask, modparam_t *param_type);

/* modules function prototypes:
 * struct module_exports* mod_register(); (type module_register)
 * int   foo_cmd(struct sip_msg* msg, char* param);
 *  - returns >0 if ok , <0 on error, 0 to stop processing (==DROP)
 * int   response_f(struct sip_msg* msg)
 *  - returns >0 if ok, 0 to drop message
 */


/* API function to get other parameters from fixup */
action_u_t *fixup_get_param(void **cur_param, int cur_param_no, int required_param_no);
int fixup_get_param_count(void **cur_param, int cur_param_no);

int fix_flag( modparam_t type, void* val,
					char* mod_name, char* param_name, int* flag);


/*
 * Common function parameter fixups
 */

/*
 * Generic parameter fixup function which creates
 * fparam_t structure. type parameter contains allowed
 * parameter types
 */
int fix_param(int type, void** param);
void fparam_free_contents(fparam_t* fp);

/** fix a param to one of the given types (mask).
  */
int fix_param_types(int types, void** param);

/*
 * Fixup variable string, the parameter can be
 * AVP, SELECT, or ordinary string. AVP and select
 * identifiers will be resolved to their values during
 * runtime
 *
 * The parameter value will be converted to fparam structure
 * This function returns -1 on an error
 */
int fixup_var_str_12(void** param, int param_no);

/* Same as fixup_var_str_12 but applies to the 1st parameter only */
int fixup_var_str_1(void** param, int param_no);

/* Same as fixup_var_str_12 but applies to the 2nd parameter only */
int fixup_var_str_2(void** param, int param_no);

/** fixup variable-pve-string.
 * The parameter can be a PVAR, AVP, SELECT, PVE (pv based format string)
 * or string.
 */
int fixup_var_pve_str_12(void** param, int param_no);

/* same as fixup_var_pve_str_12 but applies to the 1st parameter only */
int fixup_var_pve_str_1(void** param, int param_no);

/* same as fixup_var_pve_str_12 but applies to the 2nd parameter only */
int fixup_var_pve_str_2(void** param, int param_no);

/*
 * Fixup variable integer, the parameter can be
 * AVP, SELECT, or ordinary integer. AVP and select
 * identifiers will be resolved to their values and 
 * converted to int if necessary during runtime
 *
 * The parameter value will be converted to fparam structure
 * This function returns -1 on an error
 */
int fixup_var_int_12(void** param, int param_no);

/* Same as fixup_var_int_12 but applies to the 1st parameter only */
int fixup_var_int_1(void** param, int param_no);

/* Same as fixup_var_int_12 but applies to the 2nd parameter only */
int fixup_var_int_2(void** param, int param_no);

/*
 * The parameter must be a regular expression which must compile, the
 * parameter will be converted to compiled regex
 */
int fixup_regex_12(void** param, int param_no);

/* Same as fixup_regex_12 but applies to the 1st parameter only */
int fixup_regex_1(void** param, int param_no);

/* Same as fixup_regex_12 but applies to the 2nd parameter only */
int fixup_regex_2(void** param, int param_no);

/*
 * The string parameter will be converted to integer
 */
int fixup_int_12(void** param, int param_no);

/* Same as fixup_int_12 but applies to the 1st parameter only */
int fixup_int_1(void** param, int param_no);

/* Same as fixup_int_12 but applies to the 2nd parameter only */
int fixup_int_2(void** param, int param_no);

/*
 * Parse the parameter as static string, do not resolve
 * AVPs or selects, convert the parameter to str structure
 */
int fixup_str_12(void** param, int param_no);

/* Same as fixup_str_12 but applies to the 1st parameter only */
int fixup_str_1(void** param, int param_no);

/* Same as fixup_str_12 but applies to the 2nd parameter only */
int fixup_str_2(void** param, int param_no);

/*
 * Get the function parameter value as string
 * Return values:  0 - Success
 *                -1 - Cannot get value
 */
int get_str_fparam(str* dst, struct sip_msg* msg, fparam_t* param);

/*
 * Get the function parameter value as integer
 * Return values:  0 - Success
 *                -1 - Cannot get value
 */
int get_int_fparam(int* dst, struct sip_msg* msg, fparam_t* param);


/**
 * Retrieve the compiled RegExp.
 * @return: 0 for success, negative on error.
 */
int get_regex_fparam(regex_t *dst, struct sip_msg* msg, fparam_t* param);


int is_fparam_rve_fixup(fixup_function f);


/** generic free fixup type function for a fixed fparam.
 * It will free whatever was allocated during the initial fparam fixup
 * and restore the original param value.
 */
void fparam_free_restore(void** param);
int fixup_free_fparam_all(void** param, int param_no);
int fixup_free_fparam_1(void** param, int param_no);
int fixup_free_fparam_2(void** param, int param_no);

free_fixup_function get_fixup_free(fixup_function f);

#endif /* sr_module_h */
