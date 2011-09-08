/*
 * $Id$
 *
 * SNMPStats Module 
 * Copyright (C) 2006 SOMA Networks, INC.
 * Written by: Jeffrey Magder (jmagder@somanetworks.com)
 *
 * This file is part of Kamailio, a free SIP server.
 *
 * Kamailio is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * Kamailio is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 * History:
 * --------
 * 2006-11-23 initial version (jmagder)
 *
 * Note: this file originally auto-generated by mib2c using
 * mib2c.array-user.conf
 *
 * The file implements the openserSIPStatusCodesTable.  For a full description
 * of the table, please see the OPENSER-SIP-COMMON-MIB.
 * 
 * This file is much larger and more complicated than the files for other
 * tables.  This is because the table is settable, bringing a lot of SNMP
 * overhead.  Most of the file consists of the original auto-generated
 * code (aside from white space and comment changes).
 *
 * The functions that have been modified to implement this table are the
 * following:
 *
 * 1) openserSIPStatusCodesTable_create_row()
 *
 *    - The row structure has been modified from its default to store the
 *      number of messages that have been recieved and sent with a certain
 *      status code, at the time this row was created.  This function 
 *      populates that data. 
 *
 * 2) openserSIPStatusCodesTable_extract_index() 
 *
 *    - Modified to fail if the index is invalid.  The index is invalid if it
 *      does not match up with the global userLookupCounter.
 *
 * 3) openserSIPStatusCodesTable_can_[activate|deactivate|delete]()
 *   
 *    - Simplified to always allow activation/deactivation/deletion. 
 *
 * 4) openserSIPStatusCodesTable_set_reserve1()
 *
 *    - The reserve1 phase passes if the row is new, and the rowStatus column
 *      is being set to 'createAndGo'
 *
 *    - The reserve1 phase passes if the row is not new, and the rowStatus
 *      column is being set to 'destroy'
 *
 * 5) openserSIPStatusCodesTable_get_value()
 *
 *    - Instead of returning a variable binding to either
 *      openserSIPStatusCodeIns or openserSIPStatusCodeOuts, the function 
 *      returns a variable binding equal to the current value as per the 
 *      statistics framework, minus either openserSIPStatusCodeIns or
 *      openserSIPStatusCodeOuts
 *
 * You can safely ignore the other functions.  
 *
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <net-snmp/library/snmp_assert.h>

#include "snmpSIPStatusCodesTable.h"
#include "snmpstats_globals.h"
#include "../../lib/kcore/statistics.h"

static netsnmp_handler_registration *my_handler = NULL;
static netsnmp_table_array_callbacks cb;

oid openserSIPStatusCodesTable_oid[] = { 
	openserSIPStatusCodesTable_TABLE_OID };

size_t openserSIPStatusCodesTable_oid_len = 
OID_LENGTH(openserSIPStatusCodesTable_oid);


/* 
 * Initializes the openserSIPStatusCodesTable module.  This step is easier
 * than in the other tables because there is no table population.  All table
 * population takes place during run time. 
 */
void init_openserSIPStatusCodesTable(void)
{
	initialize_table_openserSIPStatusCodesTable();
}


/* the *_row_copy routine */
static int openserSIPStatusCodesTable_row_copy(
		openserSIPStatusCodesTable_context * dst,
		openserSIPStatusCodesTable_context * src)
{
	if(!dst||!src)
		return 1;
		
	
	/* copy index, if provided */
	if(dst->index.oids)
	{
		free(dst->index.oids);
	}

	if(snmp_clone_mem( (void*)&dst->index.oids, src->index.oids, 
				src->index.len * sizeof(oid) )) 
	{
		dst->index.oids = NULL;
		return 1;
	}

	dst->index.len = src->index.len;
	

	/* copy components into the context structure */
	dst->openserSIPStatusCodeMethod    = src->openserSIPStatusCodeMethod;
	dst->openserSIPStatusCodeValue     = src->openserSIPStatusCodeValue;
	dst->openserSIPStatusCodeIns       = src->openserSIPStatusCodeIns;
	dst->openserSIPStatusCodeOuts      = src->openserSIPStatusCodeOuts;
	dst->openserSIPStatusCodeRowStatus = src->openserSIPStatusCodeRowStatus;

	return 0;
}

/*
 * the *_extract_index routine (Mostly auto-generated)
 *
 * This routine is called when a set request is received for an index that 
 * was not found in the table container. Here, we parse the oid in the
 * individual index components and copy those indexes to the context. Then 
 * we make sure the indexes for the new row are valid.
 *
 * It has been modified from its original form in that the indexes aren't
 * returned if they are invalid.  An index is invalid if it is not between 
 * 100 and 699 (Inclusive).
 */
int openserSIPStatusCodesTable_extract_index( 
		openserSIPStatusCodesTable_context * ctx, netsnmp_index * hdr)
{

	/*
	 * temporary local storage for extracting oid index
	 *
	 * extract index uses varbinds (netsnmp_variable_list) to parse
	 * the index OID into the individual components for each index part.
	 */
	netsnmp_variable_list var_openserSIPStatusCodeMethod;
	netsnmp_variable_list var_openserSIPStatusCodeValue;
	int err;

	/*
	 * copy index, if provided
	 */
	if(hdr) {
		netsnmp_assert(ctx->index.oids == NULL);
		if((hdr->len > MAX_OID_LEN) || 
				snmp_clone_mem( 
					(void*)&ctx->index.oids,
					hdr->oids,
					hdr->len * sizeof(oid))) 
		{
			return -1;
		}

		ctx->index.len = hdr->len;
	}

 	/* Initialize the two variables responsible for holding our two indices.
	 */
	memset(&var_openserSIPStatusCodeMethod, 0x00, 
			 sizeof(var_openserSIPStatusCodeMethod));

	memset( &var_openserSIPStatusCodeValue, 0x00, 
			sizeof(var_openserSIPStatusCodeValue) );

	var_openserSIPStatusCodeMethod.type = ASN_UNSIGNED; 
	var_openserSIPStatusCodeValue.type  = ASN_UNSIGNED;

	var_openserSIPStatusCodeMethod.next_variable = 
		&var_openserSIPStatusCodeValue;

	var_openserSIPStatusCodeValue.next_variable = NULL;

	/* parse the oid into the individual index components */
	err = parse_oid_indexes( hdr->oids, hdr->len, 
			&var_openserSIPStatusCodeMethod );

	if (err == SNMP_ERR_NOERROR) {

		/* copy index components into the context structure */
		ctx->openserSIPStatusCodeMethod = 
			*var_openserSIPStatusCodeMethod.val.integer;
		ctx->openserSIPStatusCodeValue  = 
			*var_openserSIPStatusCodeValue.val.integer;
   
   
		if (*var_openserSIPStatusCodeMethod.val.integer < 1)
		{
			err = -1;
		}

		if (*var_openserSIPStatusCodeValue.val.integer < 100 ||
		    *var_openserSIPStatusCodeValue.val.integer > 699) {
			err = -1;
		}

	}

	
	/* parsing may have allocated memory. free it. */
	snmp_reset_var_buffers( &var_openserSIPStatusCodeMethod );

	return err;
}


/*
 * This is an auto-generated function.  In general the *_can_activate routine 
 * is called when a row is changed to determine if all the values set are
 * consistent with the row's rules for a row status of ACTIVE.  If not, then 0
 * can be returned to prevent the row status from becomming final. 
 *
 * For our purposes, we have no need for this check, so we always return 1.
 */
int openserSIPStatusCodesTable_can_activate(
		openserSIPStatusCodesTable_context *undo_ctx,
		openserSIPStatusCodesTable_context *row_ctx,
		netsnmp_request_group * rg)
{
	return 1;
}


/* 
 * This is an auto-generated function.  In general the *_can_deactivate routine
 * is called when a row that is currently ACTIVE is set to a state other than
 * ACTIVE. If there are conditions in which a row should not be allowed to
 * transition out of the ACTIVE state (such as the row being referred to by
 * another row or table), check for them here.
 *
 * Since this table has no reason why this shouldn't be allowed, we always
 * return 1;
 */
int openserSIPStatusCodesTable_can_deactivate(
		openserSIPStatusCodesTable_context *undo_ctx,
		openserSIPStatusCodesTable_context *row_ctx,
		netsnmp_request_group * rg)
{
	return 1;
}


/*
 * This is an auto-generated function.  In general the *_can_delete routine is
 * called to determine if a row can be deleted.  This usually involved checking
 * if it can be deactivated, and if it can be, then checking for other
 * conditions.  
 *
 * Since this table ha no reason why row deletion shouldn't be allowed, we
 * always return 1, unless we can't deactivate.  
 */
int openserSIPStatusCodesTable_can_delete(
		openserSIPStatusCodesTable_context *undo_ctx,
		openserSIPStatusCodesTable_context *row_ctx,
		netsnmp_request_group * rg)
{
	if(openserSIPStatusCodesTable_can_deactivate(undo_ctx,row_ctx,rg) != 1)
		return 0;
	
	return 1;
}

/*
 * (Mostly auto-generated function) 
 *
 * The *_create_row routine is called by the table handler to create a new row
 * for a given index. This is the first stage of the row creation process.  The
 * *_set_reserve_* functions can be used to prevent the row from being inserted
 * into the table even if the row passes any preliminary checks set here. 
 *
 * Returns a newly allocated openserSIPRegUserLookupTable_context structure (a
 * row in the table) if the indexes are legal.  NULL will be returned otherwise.
 *
 * The function has been modified from its original form, in that it will store
 * the number of messages 'in' to the system, and the number of messages 'out'
 * of the system, that had a status code matching this rows status code, at the
 * time this row was created.  
 *
 * This value will be used in the future to calculate the delta between now and
 * the time this row has been read.  
 * */
openserSIPStatusCodesTable_context *
openserSIPStatusCodesTable_create_row( netsnmp_index* hdr)
{
	stat_var *in_status_code;
	stat_var *out_status_code;

	openserSIPStatusCodesTable_context * ctx =
		SNMP_MALLOC_TYPEDEF(openserSIPStatusCodesTable_context);
	if(!ctx)
		return NULL;
		
	/* The *_extract_index funtion already validates the indices, so we
	 * don't need to do any further evaluations here.  */
	if(openserSIPStatusCodesTable_extract_index( ctx, hdr )) {
		if (NULL != ctx->index.oids)
			free(ctx->index.oids);
		free(ctx);
		return NULL;
	}


	/* The indices were already set up in the extract_index function
	 * above. */
	ctx->openserSIPStatusCodeIns       = 0;
	ctx->openserSIPStatusCodeOuts      = 0;
	ctx->openserSIPStatusCodeRowStatus = 0;

	/* Retrieve the index for the status code, and then assign the starting
	 * values.  The starting values will be used to calculate deltas during
	 * the next snmpget/snmpwalk/snmptable/etc. */
	int codeIndex = ctx->openserSIPStatusCodeValue;

	ctx->startingInStatusCodeValue  = 0;
	ctx->startingOutStatusCodeValue = 0;

	in_status_code  = get_stat_var_from_num_code(codeIndex, 0);
	out_status_code = get_stat_var_from_num_code(codeIndex, 1);

	if (in_status_code != NULL) 
	{
		ctx->startingInStatusCodeValue  = get_stat_val(in_status_code);
	}

	if (out_status_code != NULL) 
	{
		ctx->startingOutStatusCodeValue = get_stat_val(out_status_code);
	}

	return ctx;
}


/* 
 * Auto-generated function.  The *_duplicate row routine
 */
openserSIPStatusCodesTable_context *
openserSIPStatusCodesTable_duplicate_row( 
		openserSIPStatusCodesTable_context * row_ctx)
{
	openserSIPStatusCodesTable_context * dup;

	if(!row_ctx)
		return NULL;

	dup = SNMP_MALLOC_TYPEDEF(openserSIPStatusCodesTable_context);
	if(!dup)
		return NULL;
		
	if(openserSIPStatusCodesTable_row_copy(dup,row_ctx)) {
		free(dup);
		dup = NULL;
	}

	return dup;
}


/* 
 * The *_delete_row method is auto-generated, and is called to delete a row.
 *
 * This will not be called if earlier checks said that this row can't be
 * deleted.  However, in our implementation there is never a reason why this
 * function can't be called. 
 */
netsnmp_index * openserSIPStatusCodesTable_delete_row( 
		openserSIPStatusCodesTable_context * ctx )
{
	if(ctx->index.oids)
		free(ctx->index.oids);

	free( ctx );

	return NULL;
}


/*
 * Large parts of this function have been auto-generated.  The functions purpose
 * is to check to make sure all SNMP set values for the given row, have been
 * valid.  If not, then the process is supposed to be aborted.  Otherwise, we
 * pass on to the *_reserve2 function.  
 *
 * For our purposes, our only check is to make sure that either of the following
 * conditions are true: 
 *
 * 1) If this row already exists, then the SET request is setting the rowStatus
 *    column to 'destroy'.
 *
 * 2) If this row does not already exist, then the SET request is setting the 
 *    rowStatus to 'createAndGo'. 
 *
 * Since the MIB specified there are to be no other modifications to the row,
 * any other condition is considered illegal, and will result in an SNMP error
 * being returned. 
 */
void openserSIPStatusCodesTable_set_reserve1( netsnmp_request_group *rg )
{
	openserSIPStatusCodesTable_context *row_ctx =
		(openserSIPStatusCodesTable_context *)rg->existing_row;

	netsnmp_variable_list      *var;
	netsnmp_request_group_item *current;

	int rc;

	/* Loop through the specified columns, and make sure that all values are
	 * valid. */
	for( current = rg->list; current; current = current->next ) {

		var = current->ri->requestvb;
		rc = SNMP_ERR_NOERROR;

		switch(current->tri->colnum) 
		{
			case COLUMN_OPENSERSIPSTATUSCODEROWSTATUS:

				/** RowStatus = ASN_INTEGER */
				rc = netsnmp_check_vb_type_and_size(var, 
						ASN_INTEGER,
						sizeof(
						row_ctx->openserSIPStatusCodeRowStatus));
			
				/* Want to make sure that if it already exists that it
				 * is setting it to 'destroy', or if it doesn't exist,
				 * that it is setting it to 'createAndGo' */
				if (row_ctx->openserSIPStatusCodeRowStatus == 0 && 
				    *var->val.integer != TC_ROWSTATUS_CREATEANDGO) 
				{
					rc = SNMP_ERR_BADVALUE;
				}			

				else if (row_ctx->openserSIPStatusCodeRowStatus ==
						TC_ROWSTATUS_ACTIVE && 
						*var->val.integer != 
						TC_ROWSTATUS_DESTROY) 
				{
					rc = SNMP_ERR_BADVALUE;
				}
		

				break;

			default: /** We shouldn't get here */
				rc = SNMP_ERR_GENERR;
				snmp_log(LOG_ERR, "unknown column in openserSIP"
						"StatusCodesTable_set_reserve1\n");
		}

		if (rc)
		{
		   netsnmp_set_mode_request_error(MODE_SET_BEGIN, 
				   current->ri, rc );
		}

		rg->status = SNMP_MAX(rg->status, current->ri->status);
	}

}

/*
 * Auto-generated function.  The function is supposed to check for any
 * last-minute conditions not being met.  However, we don't have any such
 * conditions, so we leave the default function as is.
 */
void openserSIPStatusCodesTable_set_reserve2( netsnmp_request_group *rg )
{
	openserSIPStatusCodesTable_context *undo_ctx = 
		(openserSIPStatusCodesTable_context *)rg->undo_info;

	netsnmp_request_group_item *current;
	netsnmp_variable_list      *var;

	int rc;

	rg->rg_void = rg->list->ri;

	for( current = rg->list; current; current = current->next ) {

		var = current->ri->requestvb;
		rc = SNMP_ERR_NOERROR;

		switch(current->tri->colnum) 
		{

			case COLUMN_OPENSERSIPSTATUSCODEROWSTATUS:
				/** RowStatus = ASN_INTEGER */
				rc = netsnmp_check_vb_rowstatus(current->ri->requestvb,
					undo_ctx ? 
					undo_ctx->openserSIPStatusCodeRowStatus:0);

				rg->rg_void = current->ri;
				break;

			default: /** We shouldn't get here */
				netsnmp_assert(0); /** why wasn't this caught in reserve1? */
		}

		if (rc)
		{
			netsnmp_set_mode_request_error(MODE_SET_BEGIN, 
					current->ri, rc);
		}
	}

}

/*
 * This function is called only when all the *_reserve[1|2] functions were
 * succeful.  Its purpose is to make any changes to the row before it is
 * inserted into the table.  
 *
 * In our case, we don't require any changes.  So we leave the original
 * auto-generated code as is.   
 */
void openserSIPStatusCodesTable_set_action( netsnmp_request_group *rg )
{
	netsnmp_variable_list *var;

	openserSIPStatusCodesTable_context *row_ctx = 
		(openserSIPStatusCodesTable_context *)rg->existing_row;

	openserSIPStatusCodesTable_context *undo_ctx = 
		(openserSIPStatusCodesTable_context *)rg->undo_info;

	netsnmp_request_group_item *current;

	int row_err = 0;

	/* Depending on what the snmpset was, set the row to be created or
	 * deleted.   */
	for( current = rg->list; current; current = current->next ) 
	{
		var = current->ri->requestvb;

		switch(current->tri->colnum) 
		{
			case COLUMN_OPENSERSIPSTATUSCODEROWSTATUS:
			
				/** RowStatus = ASN_INTEGER */
				row_ctx->openserSIPStatusCodeRowStatus = 
					*var->val.integer;

				if (*var->val.integer == TC_ROWSTATUS_CREATEANDGO)
				{
					rg->row_created = 1;
				}
				else if (*var->val.integer == TC_ROWSTATUS_DESTROY)
				{
					rg->row_deleted = 1;
				}
				else {
					/* We should never be here, because the RESERVE
					 * functions should have taken care of all other
					 * values. */
				LM_ERR("Invalid RowStatus in openserSIPStatusCodesTable\n");
				}

				break;

			default: /** We shouldn't get here */
				netsnmp_assert(0); /** why wasn't this caught in reserve1? */
		}
	}

	/*
	 * done with all the columns. Could check row related
	 * requirements here.
	 */
#ifndef openserSIPStatusCodesTable_CAN_MODIFY_ACTIVE_ROW
	if( undo_ctx && RS_IS_ACTIVE(undo_ctx->openserSIPStatusCodeRowStatus) &&
		row_ctx && RS_IS_ACTIVE(row_ctx->openserSIPStatusCodeRowStatus)) 
	{
			row_err = 1;
	}
#endif

	/*
	 * check activation/deactivation
	 */
	row_err = netsnmp_table_array_check_row_status(&cb, rg, 
			row_ctx ? 
			&row_ctx->openserSIPStatusCodeRowStatus : NULL,
			undo_ctx ? 
			&undo_ctx->openserSIPStatusCodeRowStatus : NULL);
	if(row_err) {
		netsnmp_set_mode_request_error(MODE_SET_BEGIN,
				(netsnmp_request_info*)rg->rg_void, row_err);
		return;
	}

}


/*
 * The COMMIT phase is used to do any extra processing after the ACTION phase.
 * In our table, there is nothing to do, so the function body is empty.
 */
void openserSIPStatusCodesTable_set_commit( netsnmp_request_group *rg )
{

}


/*
 * This function is called if the *_reserve[1|2] calls failed.  Its supposed to
 * free up any resources allocated earlier.  However, we already take care of
 * all these resources in earlier functions.  So for our purposes, the function
 * body is empty. 
 */
void openserSIPStatusCodesTable_set_free( netsnmp_request_group *rg )
{

}


/* 
 * This function is called if an ACTION phase fails, to do extra clean-up work.
 * We don't have anything complicated enough to warrant putting anything in this
 * function.  Therefore, its just left with an empty function body. 
 */
void openserSIPStatusCodesTable_set_undo( netsnmp_request_group *rg )
{

}



/*
 * Initialize the openserSIPStatusCodesTable table by defining how it is
 * structured. 
 *
 * This function is mostly auto-generated.
 */
void initialize_table_openserSIPStatusCodesTable(void)
{
	netsnmp_table_registration_info *table_info;

	if(my_handler) {
		snmp_log(LOG_ERR, "initialize_table_openserSIPStatusCodes"
				"Table_handler called again\n");
		return;
	}

	memset(&cb, 0x00, sizeof(cb));

	/** create the table structure itself */
	table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);

	my_handler = netsnmp_create_handler_registration(
			"openserSIPStatusCodesTable",
			netsnmp_table_array_helper_handler,
			openserSIPStatusCodesTable_oid,
			openserSIPStatusCodesTable_oid_len,
			HANDLER_CAN_RWRITE);
			
	if (!my_handler || !table_info) {
		snmp_log(LOG_ERR, "malloc failed in initialize_table_openserSIP"
				"StatusCodesTable_handler\n");
		return; /** mallocs failed */
	}

	/** index: openserSIPStatusCodeMethod */
	netsnmp_table_helper_add_index(table_info, ASN_UNSIGNED);
	/** index: openserSIPStatusCodeValue */
	netsnmp_table_helper_add_index(table_info, ASN_UNSIGNED);

	table_info->min_column = openserSIPStatusCodesTable_COL_MIN;
	table_info->max_column = openserSIPStatusCodesTable_COL_MAX;

	/***************************************************
	 * registering the table with the master agent
	 */
	cb.get_value = openserSIPStatusCodesTable_get_value;

	cb.container = 
		netsnmp_container_find("openserSIPStatusCodesTable_primary:"
				"openserSIPStatusCodesTable:"
				"table_container");

#ifdef openserSIPStatusCodesTable_CUSTOM_SORT
	netsnmp_container_add_index(cb.container,
			netsnmp_container_find(
				"openserSIPStatusCodesTable_custom:"
				"openserSIPStatusCodesTable:"
				"table_container"));

	cb.container->next->compare = openserSIPStatusCodesTable_cmp;
#endif
	cb.can_set = 1;

	cb.create_row    =
		(UserRowMethod*)openserSIPStatusCodesTable_create_row;

	cb.duplicate_row = 
		(UserRowMethod*)openserSIPStatusCodesTable_duplicate_row;

	cb.delete_row    = 
		(UserRowMethod*)openserSIPStatusCodesTable_delete_row;

	cb.row_copy      = (Netsnmp_User_Row_Operation *)
		openserSIPStatusCodesTable_row_copy;

	cb.can_activate  = (Netsnmp_User_Row_Action *)
		openserSIPStatusCodesTable_can_activate;

	cb.can_deactivate = (Netsnmp_User_Row_Action *)
		openserSIPStatusCodesTable_can_deactivate;

	cb.can_delete     = 
		(Netsnmp_User_Row_Action *)openserSIPStatusCodesTable_can_delete;

	cb.set_reserve1   = openserSIPStatusCodesTable_set_reserve1;
	cb.set_reserve2   = openserSIPStatusCodesTable_set_reserve2;
	
	cb.set_action = openserSIPStatusCodesTable_set_action;
	cb.set_commit = openserSIPStatusCodesTable_set_commit;
	
	cb.set_free = openserSIPStatusCodesTable_set_free;
	cb.set_undo = openserSIPStatusCodesTable_set_undo;
	
	DEBUGMSGTL(("initialize_table_openserSIPStatusCodesTable",
				"Registering table openserSIPStatusCodesTable "
				"as a table array\n"));
	
	netsnmp_table_container_register(my_handler, table_info, &cb,
			cb.container, 1);
}

/*
 * This function is called to handle SNMP GET requests.  
 *
 * The row which this function is called with, will store a message code.  The
 * function will retrieve the 'number of messages in' and 'number of messages
 * out' statistic for this particular message code from the statistics
 * framework.  
 *
 * The function will then subtract from this value the value it was initialized
 * with when the row was first created.  In this sense, the row shows how many
 * ins and how many outs have been received (With respect to the message code)
 * since this row was created. 
 */
int openserSIPStatusCodesTable_get_value(
			netsnmp_request_info *request,
			netsnmp_index *item,
			netsnmp_table_request_info *table_info )
{
	stat_var *the_stat;

	netsnmp_variable_list *var = request->requestvb;

	openserSIPStatusCodesTable_context *context = 
		(openserSIPStatusCodesTable_context *)item;

	/* Retrieve the statusCodeIdx so we can calculate deltas between current
	 * values and previous values. */
	int statusCodeIdx = context->openserSIPStatusCodeValue;

	switch(table_info->colnum) 
	{
		case COLUMN_OPENSERSIPSTATUSCODEINS:

			context->openserSIPStatusCodeIns = 0;

			the_stat = get_stat_var_from_num_code(statusCodeIdx, 0);

			if (the_stat != NULL)  
			{
				/* Calculate the Delta */
				context->openserSIPStatusCodeIns = get_stat_val(the_stat) -
					context->startingInStatusCodeValue;
			}

			snmp_set_var_typed_value(var, ASN_COUNTER,
					(unsigned char*)
					&context->openserSIPStatusCodeIns,
					sizeof(context->openserSIPStatusCodeIns));
			break;
	
		case COLUMN_OPENSERSIPSTATUSCODEOUTS:
			
			context->openserSIPStatusCodeOuts = 0;

			the_stat = get_stat_var_from_num_code(statusCodeIdx, 1);

			if (the_stat != NULL)
			{
				/* Calculate the Delta */
				context->openserSIPStatusCodeOuts =
					get_stat_val(the_stat) -
					context->startingOutStatusCodeValue;
			}
			snmp_set_var_typed_value(var, ASN_COUNTER,
					 (unsigned char*)
					 &context->openserSIPStatusCodeOuts,
					 sizeof(context->openserSIPStatusCodeOuts) );
		break;
	
		case COLUMN_OPENSERSIPSTATUSCODEROWSTATUS:
			/** RowStatus = ASN_INTEGER */
			snmp_set_var_typed_value(var, ASN_INTEGER,
					 (unsigned char*)
					 &context->openserSIPStatusCodeRowStatus,
					 sizeof(context->openserSIPStatusCodeRowStatus) );
		break;
	
	default: /** We shouldn't get here */
		snmp_log(LOG_ERR, "unknown column in "
				 "openserSIPStatusCodesTable_get_value\n");
		return SNMP_ERR_GENERR;
	}
	return SNMP_ERR_NOERROR;
}

/*
 * openserSIPRegUserLookupTable_get_by_idx
 */
const openserSIPStatusCodesTable_context *
openserSIPStatusCodesTable_get_by_idx(netsnmp_index * hdr)
{
	return (const openserSIPStatusCodesTable_context *)
		CONTAINER_FIND(cb.container, hdr );
}


