/* 
 * $Id$
 * 
 * Copyright (C) 2009 iptelorg GmbH
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * SER RPC lookup and register functions
 */
/*
 * History:
 * --------
 *  2009-05-11  initial version (andrei)
*/

#ifndef __rpc_lookup_h
#define __rpc_lookup_h

#include "rpc.h"
/* must be exported for listing the rpcs */
extern rpc_export_t** rpc_sarray;
extern int rpc_sarray_crt_size;

int init_rpcs();
int destroy_rpcs();

rpc_export_t* rpc_lookup(const char* name, int len);
int rpc_register(rpc_export_t* rpc);
int rpc_register_array(rpc_export_t* rpc_array);



#endif /*__rpc_lookup_h*/

/* vi: set ts=4 sw=4 tw=79:ai:cindent: */
