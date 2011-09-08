<?xml version='1.0'?>
<!--
 * $Id: db_berkeley.xsl 3142 2007-11-15 14:09:15Z henningw $
 *
 * XSL converter script for generating module parameter templates.
 *
 * Copyright (C) 2008 Henning Westerholt
 *
 * This file is part of Kamailio, a free SIP server.
 *
 * Kamailio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * Kamailio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
-->

<!--
 * Yes, this is probably not pretty, as XSL is not the perfect tool to generate
 * C code. But we've now this infrastructure for the databases, and it works.
-->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'
                xmlns:xi="http://www.w3.org/2001/XInclude">

<xsl:import href="common.xsl"/>
<xsl:output method="text" indent="no" omit-xml-declaration="yes"/>

    <!-- Create the file for the tables in module subdirectory -->
<xsl:template match="/">
	<xsl:variable name="createfile" select="concat($dir, concat('/', concat('db_', $prefix, '.h')))"/>
	<xsl:document href="{$createfile}" method="text" indent="no" omit-xml-declaration="yes">

<!-- doxygen header -->
<xsl:text>
/*!
 * \file
 * \ingroup db
 * \brief Database support for modules.
 *
 * Database support functions for modules.
 *
 * @cond
 * WARNING:
 * This file was autogenerated from the XML source file
</xsl:text>
<xsl:value-of select="concat(' * ', $dir, 'kamailio-', $prefix, '.xml.')"/>
<xsl:text>
 * It can be regenerated by running 'make modules' in the db/schema
 * directory of the source code. You need to have xsltproc and
 * docbook-xsl stylesheets installed.
 * ALL CHANGES DONE HERE WILL BE LOST IF THE FILE IS REGENERATED
 * @endcond
 */

</xsl:text>

<!-- include guard -->
<xsl:value-of select="concat('#ifndef db_', $prefix, '_h&#x0A;')"/>
<xsl:value-of select="concat('#define db_', $prefix, '_h&#x0A;')"/>
<xsl:text>

/* necessary includes */
#include "../../db/db.h"
#include "../../str.h"
#include "../../ut.h"

#include &lt;string.h&gt;


</xsl:text>
<xsl:text>/* database variables */&#x0A;&#x0A;</xsl:text>
<xsl:value-of select="concat('extern str ', $prefix, '_db_url;&#x0A;')"/>
<xsl:value-of select="concat('extern db_con_t * ', $prefix, '_dbh;&#x0A;')"/>
<xsl:value-of select="concat('extern db_func_t ', $prefix, '_dbf;&#x0A;&#x0A;')"/>

<!-- macro for module parameter -->
<xsl:value-of select="concat('#define ', $prefix, '_DB_URL { &quot;db_url&quot;, STR_PARAM, &amp;', $prefix, '_db_url.s },&#x0A;&#x0A;')"/>

<xsl:apply-templates select="/database[1]"/>

<xsl:text>
/*
 * Closes the DB connection.
 */
</xsl:text>
<xsl:value-of select="concat('void ', $prefix, '_db_close(void);&#x0A;')"/>

<xsl:text>
/*!
 * Initialises the DB API, check the table version and closes the connection.
 * This should be called from the mod_init function.
 *
 * \return 0 means ok, -1 means an error occured.
 */
</xsl:text>
<xsl:value-of select="concat('int ', $prefix, '_db_init(void);&#x0A;')"/>

<xsl:text>
/*!
 * Initialize the DB connection without checking the table version and DB URL.
 * This should be called from child_init. An already existing database
 * connection will be closed, and a new one created.
 *
 * \return 0 means ok, -1 means an error occured.
 */
</xsl:text>
<xsl:value-of select="concat('int ', $prefix, '_db_open(void);&#x0A;')"/>

<xsl:text>
/*!
 * Update the variable length after eventual assignments from the config script.
 * This is necessary because we're using the 'str' type.
 */
</xsl:text>
<xsl:value-of select="concat('void ', $prefix, '_db_vars(void);&#x0A;')"/>

<!-- include guard end -->
		<xsl:text>&#x0A;</xsl:text>
		<xsl:text>#endif&#x0A;</xsl:text>

</xsl:document>
</xsl:template>

	<!-- create table name parameter -->
	<xsl:template match="table">
	<xsl:variable name="table.name">
		<xsl:call-template name="get-name"/>
	</xsl:variable>

	<!-- macro for db table -->
	<xsl:value-of select="concat('#define ', $table.name, '_DB_TABLE { &quot;', $table.name, '_table&quot;, STR_PARAM, &amp;', $prefix, '_table.s },&#x0A;&#x0A;')"/>
	<xsl:value-of select="concat('extern str ', $table.name, '_table;&#x0A;')"/>
	<xsl:text>&#x0A;</xsl:text>

	<xsl:text>/* column names */&#x0A;</xsl:text>
	<xsl:apply-imports/>

	<!-- macros for db columns -->
	<xsl:value-of select="concat('#define ', $table.name, '_DB_COLS \&#x0A;')"/>
	<xsl:for-each select="column">
		<xsl:variable name="column.name">
			<xsl:call-template name="get-name"/>
		</xsl:variable>
		<xsl:value-of select="concat('{ &quot;', $table.name, '_', $column.name, '_col&quot;, STR_PARAM, &amp;', $table.name, '_', $column.name, '_col.s }, \&#x0A;')"/>
	</xsl:for-each>

	<xsl:text>&#x0A;</xsl:text>
	<xsl:text>/* table version */&#x0A;</xsl:text>
	<xsl:apply-templates select="version"/>
	</xsl:template>

	<!-- create version parameter -->
	<xsl:template match="version">
	<xsl:variable name="table.name">
		<xsl:call-template name="get-name">
		<xsl:with-param name="select" select="parent::table"/>
	</xsl:call-template>
	</xsl:variable>
	<xsl:value-of select="concat('extern const unsigned int ', $table.name, '_version;&#x0A;')"/>
	<xsl:text>&#x0A;</xsl:text>
	</xsl:template>
	<!-- Create column parameter -->
	<xsl:template match="column">
	<xsl:variable name="table.name">
		<xsl:call-template name="get-name">
		<xsl:with-param name="select" select="parent::table"/>
		</xsl:call-template>
	</xsl:variable>
	<xsl:variable name="column.name">
		<xsl:call-template name="get-name"/>
	</xsl:variable>
	<xsl:value-of select="concat('extern str ', $table.name, '_', $column.name, '_col;&#x0A;')"/>
	</xsl:template>

</xsl:stylesheet>
