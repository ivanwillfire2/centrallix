#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "cxlib/mtask.h"
#include "cxlib/mtlexer.h"
#include "cxlib/exception.h"
#include "stparse_ne.h"
#include "stparse.h"
#include "cxlib/mtsession.h"
#include "cxlib/xstring.h"
#include "cxlib/newmalloc.h"
#include "cxlib/magic.h"
#include "expression.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2001 LightSys Technology Services, Inc.		*/
/* 									*/
/* This program is free software; you can redistribute it and/or modify	*/
/* it under the terms of the GNU General Public License as published by	*/
/* the Free Software Foundation; either version 2 of the License, or	*/
/* (at your option) any later version.					*/
/* 									*/
/* This program is distributed in the hope that it will be useful,	*/
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/* GNU General Public License for more details.				*/
/* 									*/
/* You should have received a copy of the GNU General Public License	*/
/* along with this program; if not, write to the Free Software		*/
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  		*/
/* 02111-1307  USA							*/
/*									*/
/* A copy of the GNU General Public License has been included in this	*/
/* distribution in the file "COPYING".					*/
/* 									*/
/* Module: 	stparse.c,stparse.h  					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	September 29, 1998					*/
/* Description:	Parser to handle structured data.  This module parses	*/
/*		AND generates "structure files".  It does NOT remember	*/
/*		comments when generating a file.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: stparse.c,v 1.19 2009/07/14 22:08:08 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/utility/stparse.c,v $

    $Log: stparse.c,v $
    Revision 1.19  2009/07/14 22:08:08  gbeeley
    - (feature) adding cx__download_as object attribute which is used by the
      HTTP interface to set the content disposition filename.
    - (feature) adding "filename" property to the report writer to use the
      cx__download_as feature to specify a filename to the browser to "Save
      As...", so reports have a more intelligent name than just "report.rpt"
      (or whatnot) when downloaded.

    Revision 1.18  2009/06/24 17:33:19  gbeeley
    - (change) adding domain param to expGenerateText, so it can be used to
      generate an expression string with lower domains converted to constants
    - (bugfix) better handling of runserver() embedded within runclient(), etc
    - (feature) allow subtracting strings, e.g., "abcde" - "de" == "abc"
    - (bugfix) after a property has been set using reverse evaluation, tag it
      as modified so it shows up as changed in other expressions using that
      same object param list
    - (change) condition() function now uses short-circuit evaluation
      semantics, so parameters are only evaluated as they are needed... e.g.
      condition(a,b,c) if a is true, b is returned and c is never evaluated,
      and vice versa.
    - (feature) add structure for reverse-evaluation of functions.  The
      isnull() function now supports this feature.
    - (bugfix) save/restore the coverage mask before/after evaluation, so that
      a nested subexpression (eval or subquery) using the same object list
      will not cause an inconsistency.  Basically a reentrancy bug.
    - (bugfix) some functions were erroneously depending on the data type of
      a NULL value to be correct.
    - (feature) adding truncate() function which is similar to round().
    - (feature) adding constrain() function which limits a value to be
      between a given minimum and maximum value.
    - (bugfix) first() and last() functions were not properly resetting the
      value to NULL between GROUP BY groups
    - (bugfix) some expression-to-JS fixes

    Revision 1.17  2008/03/29 02:26:17  gbeeley
    - (change) Correcting various compile time warnings such as signed vs.
      unsigned char.

    Revision 1.16  2008/02/25 23:14:33  gbeeley
    - (feature) SQL Subquery support in all expressions (both inside and
      outside of actual queries).  Limitations:  subqueries in an actual
      SQL statement are not optimized; subqueries resulting in a list
      rather than a scalar are not handled (only the first field of the
      first row in the subquery result is actually used).
    - (feature) Passing parameters to objMultiQuery() via an object list
      is now supported (was needed for subquery support).  This is supported
      in the report writer to simplify dynamic SQL query construction.
    - (change) objMultiQuery() interface changed to accept third parameter.
    - (change) expPodToExpression() interface changed to accept third param
      in order to (possibly) copy to an already existing expression node.

    Revision 1.15  2007/03/21 04:48:09  gbeeley
    - (feature) component multi-instantiation.
    - (feature) component Destroy now works correctly, and "should" free the
      component up for the garbage collector in the browser to clean it up.
    - (feature) application, component, and report parameters now work and
      are normalized across those three.  Adding "widget/parameter".
    - (feature) adding "Submit" action on the form widget - causes the form
      to be submitted as parameters to a component, or when loading a new
      application or report.
    - (change) allow the label widget to receive obscure/reveal events.
    - (bugfix) prevent osrc Sync from causing an infinite loop of sync's.
    - (bugfix) use HAVING clause in an osrc if the WHERE clause is already
      spoken for.  This is not a good long-term solution as it will be
      inefficient in many cases.  The AML should address this issue.
    - (feature) add "Please Wait..." indication when there are things going
      on in the background.  Not very polished yet, but it basically works.
    - (change) recognize both null and NULL as a null value in the SQL parsing.
    - (feature) adding objSetEvalContext() functionality to permit automatic
      handling of runserver() expressions within the OSML API.  Facilitates
      app and component parameters.
    - (feature) allow sql= value in queries inside a report to be runserver()
      and thus dynamically built.

    Revision 1.14  2007/03/06 16:16:55  gbeeley
    - (security) Implementing recursion depth / stack usage checks in
      certain critical areas.
    - (feature) Adding ExecMethod capability to sysinfo driver.

    Revision 1.13  2007/02/22 23:26:44  gbeeley
    - (debug) adding stPrint_ne() to print out a pStruct tree.

    Revision 1.12  2005/09/30 04:37:10  gbeeley
    - (change) modified expExpressionToPod to take the type.
    - (feature) got eval() working
    - (addition) added expReplaceString() to search-and-replace in an
      expression tree.

    Revision 1.11  2005/09/17 01:28:19  gbeeley
    - Some fixes for handling of direct object attributes in expressions,
      such as /path/to/object:attributename.

    Revision 1.10  2005/02/26 06:42:41  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.9  2004/02/24 20:25:41  gbeeley
    - misc changes: runclient check in evaltree in stparse, eval() function
      rejected in sybase driver, update version in centrallix.conf, .cmp
      extension added for component-decl in types.cfg

    Revision 1.8  2003/11/12 22:21:39  gbeeley
    - addition of delete support to osml, mq, datafile, and ux modules
    - added objDeleteObj() API call which will replace objDelete()
    - stparse now allows strings as well as keywords for object names
    - sanity check - old rpt driver to make sure it isn't in the build

    Revision 1.7  2003/06/27 21:19:48  gbeeley
    Okay, breaking the reporting system for the time being while I am porting
    it to the new prtmgmt subsystem.  Some things will not work for a while...

    Revision 1.6  2003/05/30 17:39:53  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.5  2003/03/11 02:19:06  jorupp
     * fix stGenerateMsg so it actually _writes_ to the file instead of reads from it :)

    Revision 1.4  2002/06/19 23:29:34  gbeeley
    Misc bugfixes, corrections, and 'workarounds' to keep the compiler
    from complaining about local variable initialization, among other
    things.

    Revision 1.3  2001/10/22 17:36:05  gbeeley
    Beginning to add support for JS scripting facilities.

    Revision 1.2  2001/10/17 18:23:04  gbeeley
    Fixed incorrect licensing header (was LGPL; now GPL).

    Revision 1.1  2001/10/16 23:53:02  gbeeley
    Added expressions-in-structure-files support, aka version 2 structure
    files.  Moved the stparse module into the core because it now depends
    on the expression subsystem.  Almost all osdrivers had to be modified
    because the structure file api changed a little bit.  Also fixed some
    bugs in the structure file generator when such an object is modified.
    The stparse module now includes two separate tree-structured data
    structures: StructInf and Struct.  The former is the new expression-
    enabled one, and the latter is a much simplified version.  The latter
    is used in the url_inf in net_http and in the OpenCtl for objects.
    The former is used for all structure files and attribute "override"
    entries.  The methods for the latter have an "_ne" addition on the
    function name.  See the stparse.h and stparse_ne.h files for more
    details.  ALMOST ALL MODULES THAT DIRECTLY ACCESSED THE STRUCTINF
    STRUCTURE WILL NEED TO BE MODIFIED.

    Revision 1.1.1.1  2001/08/13 18:04:22  gbeeley
    Centrallix Library initial import

    Revision 1.2  2001/07/04 02:57:58  gbeeley
    Fixed a few error checking and array-overflow problems.

    Revision 1.1.1.1  2001/07/03 01:02:59  gbeeley
    Initial checkin of centrallix-lib


 **END-CVSDATA***********************************************************/


/*** Some function headers ***/
int st_internal_GenerateAttr(pStructInf info, pXString xs, int level, pParamObjects objlist);
/*int st_internal_FLStruct(pLxSession s, pStructInf info);*/

/*** stAllocInf - allocate a new StructInf structure.
 ***/
pStructInf
stAllocInf()
    {
    pStructInf this;

	/** Allocate the structure **/
	this = (pStructInf)nmMalloc(sizeof(StructInf));
	memset(this,0, sizeof(StructInf));
	SETMAGIC(this,MGK_STRUCTINF);
	this->Name = nmMalloc(ST_NAME_STRLEN);
	this->UserData = NULL;

    return this;
    }


/*** stFreeInf - release an existing StructInf, and any sub infs
 ***/
int
stFreeInf(pStructInf this)
    {
    int i,j;

	ASSERTMAGIC(this,MGK_STRUCTINF);

	/** Free any subinfs first **/
	for(i=0;i<this->nSubInf;i++) if (this->SubInf[i]) stFreeInf(this->SubInf[i]);

	/** Free the subinf array **/
	if (this->SubInf) nmFree(this->SubInf, sizeof(pStructInf)*ST_ALLOCSIZ(this));

	/** String need to be deallocated? **/
	if (this->Name) nmFree(this->Name,ST_NAME_STRLEN);
	if (this->UsrType) nmFree(this->UsrType,ST_USRTYPE_STRLEN);

	/** Release the expression, if any. **/
	if (this->Value) expFreeExpression(this->Value);

	/** Disconnect from parent if there is one. **/
	if (this->Parent)
	    {
	    ASSERTMAGIC(this->Parent,MGK_STRUCTINF);
	    for(i=0;i<this->Parent->nSubInf;i++)
	        {
		if (this == this->Parent->SubInf[i])
		    {
		    this->Parent->nSubInf--;
		    for(j=i;j<this->Parent->nSubInf;j++)
		        {
			this->Parent->SubInf[j] = this->Parent->SubInf[j+1];
			}
		    this->Parent->SubInf[this->Parent->nSubInf] = NULL;
		    }
		}
	    }

	/** Free the current one. **/
	nmFree(this,sizeof(StructInf));

    return 0;
    }


/*** stAddInf - add a subinf to the main inf structure
 ***/
int
stAddInf(pStructInf main_inf, pStructInf sub_inf)
    {
    pStructInf* oldlist;
    int oldsize;

	ASSERTMAGIC(main_inf, MGK_STRUCTINF);
	ASSERTMAGIC(sub_inf, MGK_STRUCTINF);

	/** Too many? **/
	if (main_inf->nSubInf == ST_SUBINF_LIMIT) return -1;

	/** Enough allocation? **/
	if (main_inf->nSubInf+1 >= ST_ALLOCSIZ(main_inf) || !main_inf->SubInf)
	    {
	    oldlist = main_inf->SubInf;
	    oldsize = ST_ALLOCSIZ(main_inf);
	    main_inf->nSubAlloc++;
	    main_inf->SubInf=(pStructInf*)nmMalloc(sizeof(pStructInf)*ST_ALLOCSIZ(main_inf));
	    if (oldlist) 
	        {
		memcpy(main_inf->SubInf,oldlist,oldsize*sizeof(pStructInf));
	        nmFree(oldlist,oldsize*sizeof(pStructInf));
		}
	    }

	/** Add it. **/
	main_inf->SubInf[main_inf->nSubInf++] = sub_inf;
	sub_inf->Parent = main_inf;

    return 0;
    }


/*** stAddAttr - adds an attribute to the existing inf.
 ***/
pStructInf
stAddAttr(pStructInf inf, char* name)
    {
    pStructInf newinf;

	ASSERTMAGIC(inf, MGK_STRUCTINF);
	newinf = stAllocInf();
	if (!newinf) return NULL;
	memccpy(newinf->Name, name,0,ST_NAME_STRLEN-1);
	newinf->Name[ST_NAME_STRLEN-1] = '\0';
	newinf->Flags |= ST_F_ATTRIB;
	if (stAddInf(inf, newinf) < 0)
	    {
	    stFreeInf(newinf);
	    return NULL;
	    }

    return newinf;
    }


/*** stAddGroup - adds a subgroup to an existing inf.
 ***/
pStructInf
stAddGroup(pStructInf inf, char* name, char* type)
    {
    pStructInf newinf;

	ASSERTMAGIC(inf, MGK_STRUCTINF);
	newinf = stAllocInf();
	if (!newinf) return NULL;
	memccpy(newinf->Name, name,0,ST_NAME_STRLEN-1);
	newinf->Name[ST_NAME_STRLEN-1] = '\0';
	newinf->UsrType = nmMalloc(ST_USRTYPE_STRLEN);
	memccpy(newinf->UsrType, type,0,ST_USRTYPE_STRLEN-1);
	newinf->Name[ST_USRTYPE_STRLEN-1] = '\0';
	newinf->Flags |= ST_F_GROUP;
	if (stAddInf(inf, newinf) < 0)
	    {
	    stFreeInf(newinf);
	    return NULL;
	    }

    return newinf;
    }


/*** stAddValue - adds a value to the attribute inf.
 ***/
int
stAddValue(pStructInf inf, char* strval, int intval)
    {
    pExpression new_exp;
    pExpression list_exp;
    int n;

	ASSERTMAGIC(inf, MGK_STRUCTINF);

	/** Build the new expression **/
	new_exp = expAllocExpression();
	if (strval)
	    {
	    new_exp->NodeType = EXPR_N_STRING;
	    new_exp->DataType = DATA_T_STRING;
	    n = strlen(strval);
	    if (n >= 64)
	        {
		new_exp->String = nmSysStrdup(strval);
		new_exp->Alloc = 1;
		}
	    else
	        {
		new_exp->Alloc = 0;
		new_exp->String = new_exp->Types.StringBuf;
		strcpy(new_exp->String,strval);
		}
	    }
	else
	    {
	    new_exp->NodeType = EXPR_N_INTEGER;
	    new_exp->DataType = DATA_T_INTEGER;
	    new_exp->Integer = intval;
	    }

	/** Add the new expression to the inf structure **/
	if (!inf->Value)
	    {
	    inf->Value = new_exp;
	    n = 1;
	    }
	else if (inf->Value->NodeType != EXPR_N_LIST)
	    {
	    list_exp = expAllocExpression();
	    list_exp->NodeType = EXPR_N_LIST;
	    expAddNode(list_exp,inf->Value);
	    expAddNode(list_exp,new_exp);
	    inf->Value = list_exp;
	    n = 2;
	    }
	else
	    {
	    expAddNode(inf->Value, new_exp);
	    n = inf->Value->Children.nItems;
	    }

    return n;
    }


/*** stCreateStruct - creates a new command inf.
 ***/
pStructInf
stCreateStruct(char* name, char* type)
    {
    pStructInf newinf;

	newinf = stAllocInf();
	if (!newinf) return NULL;
	newinf->Flags |= (ST_F_GROUP | ST_F_TOPLEVEL);
	if (name)
	    {
	    memccpy(newinf->Name, name,0,ST_NAME_STRLEN-1);
	    newinf->Name[ST_NAME_STRLEN-1] = '\0';
	    }
	else
	    {
	    newinf->Name[0] = '\0';
	    }
	if (type)
	    {
	    newinf->UsrType = nmMalloc(ST_USRTYPE_STRLEN);
	    memccpy(newinf->UsrType, type,0,ST_USRTYPE_STRLEN-1);
	    newinf->Name[ST_USRTYPE_STRLEN-1] = '\0';
	    }

    return newinf;
    }


/*** stLookup - find an attribute or subgroup in the protoinf and 
 *** return the attribute inf.
 ***/
pStructInf
stLookup(pStructInf this, char* name)
    {
    pStructInf inf = NULL;
    int i;

	if (!this) return NULL;
	ASSERTMAGIC(this, MGK_STRUCTINF);

	/** Search for a subinf with the right name **/
	for(i=0;i<this->nSubInf;i++)
	    {
	    if (!strcmp(this->SubInf[i]->Name, name)) 
		{
		inf = this->SubInf[i];
		break;
		}
	    }

    return inf;
    }


/*** stAttrValue - return the value of an attribute inf.
 ***/
int
stAttrValue(pStructInf this, int* intval, char** strval, int nval)
    {
    pExpression find_exp;

	/** Do some error-cascade checking. **/
	if (!this) return -1;
	
	ASSERTMAGIC(this, MGK_STRUCTINF);
	if (!(this->Flags & ST_F_ATTRIB)) return -1;
	if (nval == 0 && this->Value->NodeType != EXPR_N_LIST)
	    {
	    find_exp = this->Value;
	    }
	else if (nval != 0 && this->Value->NodeType != EXPR_N_LIST)
	    {
	    return -1;
	    }
	else if (nval >= this->Value->Children.nItems || nval < 0)
	    {
	    return -1;
	    }
	else
	    {
	    find_exp = (pExpression)(this->Value->Children.Items[nval]);
	    }

	/** String or int val? **/
	if (intval && find_exp->DataType == DATA_T_INTEGER)
	    *intval = find_exp->Integer;
	if (strval && find_exp->DataType == DATA_T_STRING)
	    *strval = find_exp->String;

    return 0;
    }


/*** stGetExpression - returns the appropriate expression element that
 *** represents the <n>th element.
 ***/
pExpression
stGetExpression(pStructInf this, int nval)
    {
    pExpression find_exp;

	if (!(this->Flags & ST_F_ATTRIB)) return NULL;
	if (nval == 0 && this->Value->NodeType != EXPR_N_LIST)
	    {
	    find_exp = this->Value;
	    }
	else if (nval != 0 && this->Value->NodeType != EXPR_N_LIST)
	    {
	    return NULL;
	    }
	else if (nval >= this->Value->Children.nItems || nval < 0)
	    {
	    return NULL;
	    }
	else
	    {
	    find_exp = (pExpression)(this->Value->Children.Items[nval]);
	    }

    return find_exp;
    }


/*** stGetAttrValue - returns the value of an expression in a
 *** structure file.
 ***/
int
stGetAttrValue(pStructInf this, int type, pObjData pod, int nval)
    {
    return stGetAttrValueOSML(this, type, pod, nval, NULL);
    }


/*** stGetAttrValueOSML - return the value of an expression, evaluated
 *** in the context of an OSML session.
 ***/
int
stGetAttrValueOSML(pStructInf this, int type, pObjData pod, int nval, pObjSession sess)
    {
    pExpression find_exp;
    pParamObjects objlist;

	/** Do some error-cascade checking. **/
	if (!this) return -1;
	
	ASSERTMAGIC(this, MGK_STRUCTINF);

	/** Get the correct expression **/
	find_exp = stGetExpression(this, nval);
	if (!find_exp) return -1;

	/** expression code? **/
	if (type == DATA_T_CODE && (find_exp->Flags & (EXPR_F_RUNCLIENT | EXPR_F_RUNSERVER)))
	    {
	    pod->Generic = find_exp;
	    return 0;
	    }

	/** If external ref, do eval **/
	if ((find_exp->ObjCoverageMask & EXPR_MASK_EXTREF) && !(find_exp->Flags & EXPR_F_RUNCLIENT))
	    {
	    objlist = expCreateParamList();
	    objlist->Session = sess;
	    expEvalTree(find_exp, objlist);
	    expFreeParamList(objlist);
	    }

	/** Correct type requested? **/
	if (find_exp->Flags & EXPR_F_NULL) return 1;
	if (type != DATA_T_ANY && type != find_exp->DataType) return -1;

    return expExpressionToPod(find_exp, find_exp->DataType, pod);
    }


/*** stGetAttrType - return the data type of an expression.
 ***/
int
stGetAttrType(pStructInf this, int nval)
    {
    pExpression find_exp;

	/** Do some error-cascade checking. **/
	if (!this) return -1;
	
	ASSERTMAGIC(this, MGK_STRUCTINF);

	find_exp = stGetExpression(this, nval);
	if (!find_exp) return -1;

	/** Exception - if runclient, return as code, not data **/
	if (find_exp->Flags & (EXPR_F_RUNCLIENT | EXPR_F_RUNSERVER))
	    return DATA_T_CODE;

    return find_exp->DataType;
    }


/*** stStructType - return the type of structure, either attribute
 *** or group (as ST_T_xxxx).
 ***/
int
stStructType(pStructInf this)
    {
    if (this->Flags & ST_F_ATTRIB)
	return ST_T_ATTRIB;
    else
	return ST_T_SUBGROUP;
    }


/*** stSetAttrValue - sets the nth value of an attribute.
 ***/
int 
stSetAttrValue(pStructInf inf, int type, pObjData value, int nval)
    {
    pExpression new_exp, list_exp;

	/** Create the new expression node **/
	if (type == DATA_T_ANY) return -1;
	new_exp = expPodToExpression(value, type, NULL);

	/** Where should the new expression be put? **/
	if (nval == 0 && !inf->Value)
	    {
	    inf->Value = new_exp;
	    }
	else if (!inf->Value)
	    {
	    return -1;
	    }
	else if (nval == 0 && inf->Value->NodeType != EXPR_N_LIST)
	    {
	    expFreeExpression(inf->Value);
	    inf->Value = new_exp;
	    }
	else if (nval == 1 && inf->Value->NodeType != EXPR_N_LIST)
	    {
	    list_exp = expAllocExpression();
	    list_exp->NodeType = EXPR_N_LIST;
	    expAddNode(list_exp, inf->Value);
	    expAddNode(list_exp, new_exp);
	    inf->Value = list_exp;
	    }
	else if (inf->Value->NodeType == EXPR_N_LIST && nval == inf->Value->Children.nItems)
	    {
	    expAddNode(inf->Value, new_exp);
	    }
	else if (inf->Value->NodeType == EXPR_N_LIST && nval < inf->Value->Children.nItems)
	    {
	    expFreeExpression(((pExpression)(inf->Value->Children.Items[nval])));
	    inf->Value->Children.Items[nval] = (void*)new_exp;
	    }
	else
	    {
	    return -1;
	    }
	
    return 0;
    }


/*** stGetValueList - return a list of values as a nmSysMalloc'd array
 *** but only return those that match the given data type, unless the
 *** type is DATA_T_ANY.
 ***/
void*
stGetValueList(pStructInf this, int type, unsigned int* nval)
    {
    unsigned char* values;
    int i,n;
    pExpression subexp;

	/** MUST be a vector type expression. **/
	if (!this || !(this->Value) || this->Value->NodeType != EXPR_N_LIST)
	    return NULL;

	/** total up the sizes to alloc the array. **/
	n = 0;
	*nval = 0;
	for(i=0;i<this->Value->Children.nItems;i++)
	    {
	    subexp = (pExpression)(this->Value->Children.Items[i]);
	    if (subexp->DataType != type && type != DATA_T_ANY) continue;
	    (*nval)++;
	    switch(subexp->DataType)
		{
		case DATA_T_STRING: n += sizeof(void *); break;
		case DATA_T_INTEGER: n += sizeof(int); break;
		case DATA_T_DOUBLE: n += sizeof(double); break;
		case DATA_T_MONEY: n += sizeof(void *); break;
		case DATA_T_DATETIME: n += sizeof(void *); break;
		}
	    }

	/** Allocate our array **/
	if (n == 0 || (*nval) == 0) 
	    {
	    (*nval) = 0;
	    return NULL;
	    }
	values = (unsigned char*)nmSysMalloc(n);
	if (!values) return NULL;

	/** Populate the array. **/
	n = 0;
	for(i=0;i<this->Value->Children.nItems;i++)
	    {
	    stGetAttrValue(this, type, POD(values+n), i);
	    subexp = (pExpression)(this->Value->Children.Items[i]);
	    if (subexp->DataType != type && type != DATA_T_ANY) continue;
	    switch(subexp->DataType)
		{
		case DATA_T_STRING: n += sizeof(void *); break;
		case DATA_T_INTEGER: n += sizeof(int); break;
		case DATA_T_DOUBLE: n += sizeof(double); break;
		case DATA_T_MONEY: n += sizeof(void *); break;
		case DATA_T_DATETIME: n += sizeof(void *); break;
		}
	    }

    return (void*)values;
    }


/*** stAttrIsList - returns nonzero if the attribute inf is a EXPR_N_LIST value
 *** type, and 0 if it is a scalar.
 ***/
int
stAttrIsList(pStructInf this)
    {
    return (this->Value->NodeType == EXPR_N_LIST);
    }


/*** st_internal_ParseAttr - parse an attribute.  This routine
 *** should be called with the current token set at the equals
 *** sign.
 ***/
int
st_internal_ParseAttr(pLxSession s, pStructInf inf, pParamObjects objlist)
    {
    int toktype;
    pExpression exp,list_exp;
    char* str;
    int alloc;

	/** Repeat in case of comma separated listing **/
	while(1)
	    {
	    /** Version 1 or 2? **/
	    if (inf->Flags & ST_F_VERSION2)
		{
		/** Parse an expression **/
		exp = expCompileExpressionFromLxs(s, objlist, EXPR_CMP_LATEBIND);
		if (!exp)
		    {
		    mssError(0,"ST","Error in attribute expression for '%s'",inf->Name);
		    return -1;
		    }
		/** Evaluate the thing **/
		if (!(exp->Flags & EXPR_F_RUNCLIENT))
		    {
		    if (expEvalTree(exp,objlist) < 0)
			{
			mssError(0,"ST","Error in attribute expression for '%s'",inf->Name);
			return -1;
			}
		    }
	    
		/** Add the expression to the inf structure **/
		if (!inf->Value)
		    {
		    inf->Value = exp;
		    }
		else if (inf->Value->NodeType != EXPR_N_LIST)
		    {
		    list_exp = expAllocExpression();
		    list_exp->NodeType = EXPR_N_LIST;
		    expAddNode(list_exp, inf->Value);
		    expAddNode(list_exp, exp);
		    inf->Value = list_exp;
		    }
		else
		    {
		    expAddNode(inf->Value, exp);
		    }
		}
	    else
		{
		/** Version 1 - manually get token and add string or int value **/
		toktype = mlxNextToken(s);
		if (toktype == MLX_TOK_STRING || toktype == MLX_TOK_KEYWORD)
		    {
		    alloc = 0;
		    str = mlxStringVal(s,&alloc);
		    stAddValue(inf,str,0);
		    if (alloc) nmSysFree(str);
		    }
		else if (toktype == MLX_TOK_INTEGER)
		    {
		    stAddValue(inf,NULL,mlxIntVal(s));
		    }
		else
		    {
		    mssError(1,"ST","Unknown version-1 structure file attribute type for '%s'",inf->Name);
		    }
		}

	    /** Get the next token. **/
	    toktype = mlxNextToken(s);

	    /** End of attrs is different between v1 and v2 **/
	    if (inf->Flags & ST_F_VERSION2)
		{
		/** If token was a semicolon, we're done. **/
		if (toktype == MLX_TOK_SEMICOLON) break;
		}
	    else
		{
		/** If token was non-comma, we're done. **/
		if (toktype != MLX_TOK_COMMA)
		    {
		    mlxHoldToken(s);
		    break;
		    }
		}

	    /** Comma token?  Get another expression. **/
	    if (toktype == MLX_TOK_COMMA) continue;

	    /** Huh? **/
	    mssError(1,"ST","Unknown end-of-expression token for '%s'; must be comma or semicolon",inf->Name);
	    return -1;
	    }
    
    return 0;
    }
    
    
    
/*** st_internal_ParseGroup - parse a subgroup within a command
 *** or another subgroup.  Should be called with the current
 *** token set to the open brace.
 ***/
int
st_internal_ParseGroup(pLxSession s, pStructInf inf, pParamObjects objlist)
    {
    int toktype,nametoktype;
    pStructInf subinf;
    char* str;

	/** Check recursion **/
	if (thExcessiveRecursion())
	    {
	    mssError(1,"ST","Failed to parse structure file: resource exhaustion occurred");
	    mlxNotePosition(s);
	    return -1;
	    }

	while(1)
	    {
	    nametoktype = mlxNextToken(s);
	    if (nametoktype == MLX_TOK_CLOSEBRACE) break;
	    else if (nametoktype == MLX_TOK_KEYWORD || nametoktype == MLX_TOK_STRING)
		{
		/** Check the string **/
		str = mlxStringVal(s,NULL);
		if (!str) 
		    {
		    mssError(0,"ST","Could not obtain name of structure file subgroup in group '%s'", inf->Name);
		    mlxNotePosition(s);
		    return -1;
		    }

		/** Create a structure to use **/
		subinf = stAllocInf();
		if (stAddInf(inf, subinf) < 0)
		    {
		    stFreeInf(subinf);
		    mssError(1,"ST","Internal representation exceeded");
		    return -1;
		    }
		strncpy(subinf->Name, str, ST_NAME_STRLEN-1);
		subinf->Name[ST_NAME_STRLEN-1] = 0;

		/** If a subgroup, will have a type.  Check for it. **/
		toktype = mlxNextToken(s);
		if (toktype == MLX_TOK_STRING)
		    {
		    subinf->UsrType = (char*)nmMalloc(ST_USRTYPE_STRLEN);
		    mlxCopyToken(s,subinf->UsrType,ST_USRTYPE_STRLEN);
		    toktype = mlxNextToken(s);
		    }

		/** Check for version 2 setting **/
		if (inf->Flags & ST_F_VERSION2) subinf->Flags |= ST_F_VERSION2;

		/** Check next token.  eq means attrib, brace means subgrp **/
		if (toktype == MLX_TOK_EQUALS)
		    {
		    /** attrs must have keyword names, not strings **/
		    if (nametoktype != MLX_TOK_KEYWORD)
			{
			mssError(1,"ST","Attribute name '%s' must be a nonquoted keyword, not a string.", subinf->Name);
			mlxNotePosition(s);
			return -1;
			}
		    if (subinf->UsrType && subinf->UsrType[0] != 0) 
		        {
			mssError(1,"ST","Attribute '%s' cannot have an object type", subinf->Name);
		        mlxNotePosition(s);
			return -1;
			}
		    subinf->Flags |= ST_F_ATTRIB;
		    if (st_internal_ParseAttr(s,subinf,objlist) < 0) return -1;
		    }
		else if (toktype == MLX_TOK_OPENBRACE)
		    {
		    if (!subinf->UsrType || subinf->UsrType[0] == 0)
		        {
			mssError(1,"ST","Subgroup '%s' must have an object type",subinf->Name);
		        mlxNotePosition(s);
			return -1;
			}
		    subinf->Flags |= ST_F_GROUP;
		    if (st_internal_ParseGroup(s,subinf,objlist) < 0) return -1; 
		    }
		else if (toktype == MLX_TOK_DBLOPENBRACE)
		    {
		    if (!subinf->UsrType || subinf->UsrType[0] == 0 || strcmp(subinf->UsrType,"system/script"))
			{
			mssError(1,"ST","Script '%s' must have an object type of 'system/script'",subinf->Name);
		        mlxNotePosition(s);
			return -1;
			}
		    if (st_internal_ParseScript(s,subinf) < 0) 
		        {
			mssError(1,"ST","Error parsing script '%s'", subinf->Name);
			mlxNotePosition(s);
			return -1;
			}
		    }
		else
		    {
		    mssError(1,"ST","Expected 'type' { or = after subgroup/attr name '%s'", subinf->Name);
		    mlxNotePosition(s);
		    return -1;
		    }
		}
	    else
		{
		mssError(1,"ST","Structure file subgroup/attr name in '%s' is missing", inf->Name);
		mlxNotePosition(s);
		return -1;
		}
	    }

    return 0;
    }


/*** st_internal_IsDblOpen - check a string to see if it is a double-open-
 *** brace on a line by itself with only surrounding whitespace.
 ***/
int
st_internal_IsDblOpen(char* str)
    {
    	
	/** Skip whitespace **/
	while(*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;

	/** Double open? **/
	if (str[0] != '{' || str[1] != '{') return 0;

	/** Skip whitespace **/
	while(*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;

	/** End of line? **/
	if (*str != '\0') return 0;

    return 1;
    }


/*** st_internal_IsDblClose - check a string to see if it is a double-close-
 *** brace on a line by itself.
 ***/
int
st_internal_IsDblClose(char* str)
    {
    	
	/** Skip whitespace **/
	while(*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;

	/** Double open? **/
	if (str[0] != '}' || str[1] != '}') return 0;

	/** Skip whitespace **/
	while(*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;

	/** End of line? **/
	if (*str != '\0') return 0;

    return 1;
    }


/*** st_internal_ParseScript - parse an embedded JavaScript segment within
 *** a structure file.  This is called once a {{ has been encountered 
 *** during group processing, and this routine will continue until a
 *** matching }} is encountered.
 ***/
int
st_internal_ParseScript(pLxSession s, pStructInf info)
    {
    int t;
    char* str;
    int alloc;
    XString xs;

    	/** Throw the lexer into linemode. **/
	mlxSetOptions(s, MLX_F_LINEONLY);
	xsInit(&xs);

	/** Start reading the lines and copy 'em to the script buffer **/
	while(1)
	    {
	    /** Token must be a string -- one script line. **/
	    t = mlxNextToken(s);
	    if (t != MLX_TOK_STRING) 
	        {
		mssError(0,"ST","Unexpected error parsing script '%s'", info->Name);
		return -1;
		}

	    /** Get the value of the thing. **/
	    alloc = 0;
	    str = mlxStringVal(s,&alloc);
	    xsConcatenate(&xs, str, -1);
	    if (alloc) nmSysFree(str);

	    /** Is this the end of the script? **/
	    if (st_internal_IsDblClose(str))
	        {
		break;
		}

	    /** Breaking out of script and into structure file again? **/
	    /*if (st_internal_IsDblOpen(str))
	        {
		}*/
	    }

	/** build the structinf entry **/
	info->Flags |= ST_F_SCRIPT;
	info->ScriptText = (unsigned char*)nmSysStrdup(xs.String);
	info->ScriptCode = NULL;

	/** Revert back to token mode **/
	xsDeInit(&xs);
	mlxUnsetOptions(s, MLX_F_LINEONLY);

    return 0;
    }


/*** st_internal_ParseStruct - parse a command structure from the 
 *** input stream.
 ***/
int
st_internal_ParseStruct(pLxSession s, pStructInf *info)
    {
    int toktype;
    Exception parse_err;
    pParamObjects objlist;
    char* msg = "";

	/** Allocate the structure **/
	*info = stAllocInf();
	if (!*info) return -1;
	(*info)->Flags |= (ST_F_TOPLEVEL | ST_F_GROUP);
	objlist = expCreateParamList();
	expSetEvalDomain(objlist, EXPR_MO_RUNSTATIC);
	/*expAddParamToList(objlist, "this", NULL, 0);*/

	/** In case we get a parse error: **/
	Catch(parse_err)
	    {
	    stFreeInf(*info);
	    expFreeParamList(objlist);
	    *info = NULL;
	    mssError(0,"ST","Error in structure file top-level group: %s", msg);
	    mlxNotePosition(s);
	    return -1;
	    }

	/** Check for version header **/
	toktype = mlxNextToken(s);
	if (toktype == MLX_TOK_DOLLAR)
	    {
	    if (mlxNextToken(s) != MLX_TOK_KEYWORD || strcmp(mlxStringVal(s,NULL),"Version"))
		{
		msg="Expected 'Version' parameter after $ at beginning of file.";
		Throw(parse_err);
		}
	    if (mlxNextToken(s) != MLX_TOK_EQUALS)
		{
		msg="Expected equals sign after structure file parameter";
		Throw(parse_err);
		}
	    if (mlxNextToken(s) != MLX_TOK_INTEGER)
	        {
		msg="Expected integer version number after Version=";
		return -1;
		Throw(parse_err);
		}
	    if (mlxIntVal(s) == 2)
		{
		(*info)->Flags |= ST_F_VERSION2;
		}
	    if (mlxNextToken(s) != MLX_TOK_DOLLAR)
	        {
		msg="Expected $ after Version parameter";
		Throw(parse_err);
		}
	    }
	else
	    {
	    mlxHoldToken(s);
	    }

	/** Get a token to see what command we have. **/
	toktype = mlxNextToken(s);
	if (toktype != MLX_TOK_KEYWORD && toktype != MLX_TOK_STRING) msg="Expected object name at beginning",Throw(parse_err);
	mlxCopyToken(s,(*info)->Name,ST_NAME_STRLEN);

	/** GRB 2/2000 - Is this a FormLayout style file? **/
	/*if (!strcmp((*info)->Name, "BEGIN"))
	    {
	    return st_internal_FLStruct(s, *info);
	    }*/

	/** If a subgroup, will have a type.  Check for it. **/
	toktype = mlxNextToken(s);
	if (toktype != MLX_TOK_STRING) msg="Expected type after object name",Throw(parse_err);
	(*info)->UsrType = (char*)nmMalloc(ST_USRTYPE_STRLEN);
	mlxCopyToken(s,(*info)->UsrType,ST_USRTYPE_STRLEN);

	/** Check for the open brace **/
	/** Double open brace {{ with "system/script" means Script group, not Struct group **/
	toktype = mlxNextToken(s);
	if (toktype == MLX_TOK_DBLOPENBRACE && !strcmp((*info)->UsrType,"system/script"))
	    {
	    if (st_internal_ParseScript(s,*info) < 0) msg="Error parsing top-level script",Throw(parse_err);
	    }
	else if (toktype == MLX_TOK_OPENBRACE)
	    {
	    /** Ok, got the command header.  Now do the attribs/subs **/
	    if (st_internal_ParseGroup(s,*info,objlist) < 0) msg="Error in top-level group",Throw(parse_err);
	    }
	else
	    {
	    msg="Expected { or {{ after top level group type";
	    Throw(parse_err);
	    }
	expFreeParamList(objlist);

    return 0;
    }


/*** stParseMsg - parse an incoming message from a file or network 
 *** connection.
 ***/
pStructInf
stParseMsg(pFile inp_fd, int flags)
    {
    pStructInf info;
    pLxSession s;

	/** Open a session with the lexical analyzer **/
	s = mlxOpenSession(inp_fd, MLX_F_CPPCOMM | MLX_F_DBLBRACE | MLX_F_FILENAMES);
	if (!s) 
	    {
	    mssError(0,"ST","Could not begin analysis of structure file");
	    return NULL;
	    }

	/** Parse a command **/
	st_internal_ParseStruct(s, &info);

	/** Close the lexer session **/
	mlxCloseSession(s);

    return info;
    }


/*** stParseMsgGeneric - parse an incoming message from a generic
 *** descriptor via a read-function.
 ***/
pStructInf
stParseMsgGeneric(void* src, int (*read_fn)(), int flags)
    {
    pStructInf info;
    pLxSession s;

	/** Open a session with the lexical analyzer **/
	s = mlxGenericSession(src,read_fn, MLX_F_CPPCOMM | MLX_F_DBLBRACE | MLX_F_FILENAMES);
	if (!s) 
	    {
	    mssError(0,"ST","Could not begin analysis of structure file");
	    return NULL;
	    }

	/** Parse a command **/
	st_internal_ParseStruct(s, &info);

	/** Close the lexer session **/
	mlxCloseSession(s);

    return info;
    }


/*** stProbeTypeGeneric() - read just enough of a structure file to figure
 *** out the type of the top level group.
 ***/
int
stProbeTypeGeneric(void* read_src, int (*read_fn)(), char* type, int type_maxlen)
    {
    pLxSession s = NULL;
    int t;
    char* str;

	/** Open a session with the lexical analyzer **/
	s = mlxGenericSession(read_src,read_fn, MLX_F_CPPCOMM | MLX_F_DBLBRACE | MLX_F_FILENAMES);
	if (!s) 
	    {
	    mssError(0,"ST","Could not begin analysis of structure file");
	    goto error;
	    }

	/** Read in the token stream until we hit the top level type. **/
	t = mlxNextToken(s);
	if (t != MLX_TOK_DOLLAR)
	    goto error;
	t = mlxNextToken(s);
	if (t != MLX_TOK_KEYWORD || (str = mlxStringVal(s, NULL)) == NULL || strcmp(str, "Version") != 0)
	    goto error;
	t = mlxNextToken(s);
	if (t != MLX_TOK_EQUALS)
	    goto error;
	t = mlxNextToken(s);
	if (t != MLX_TOK_INTEGER || mlxIntVal(s) != 2)
	    goto error;
	t = mlxNextToken(s);
	if (t != MLX_TOK_DOLLAR)
	    goto error;

	/** Next we have the top level group name **/
	t = mlxNextToken(s);
	if (t != MLX_TOK_KEYWORD && t != MLX_TOK_STRING)
	    goto error;
	t = mlxNextToken(s);
	if (t != MLX_TOK_STRING)
	    goto error;
	str = mlxStringVal(s, NULL);
	if (!str)
	    goto error;

	/** Return the type **/
	mlxCloseSession(s);
	strtcpy(type, str, type_maxlen);
	return 0;

    error:
	if (s)
	    mlxCloseSession(s);
	return -1;
    }


/*** st_internal_CkAddBuf - check to see if we need to realloc on the buffer
 *** to add n characters.
 ***/
int
st_internal_CkAddBuf(char** buf, int* buflen, int* datalen, int n)
    {

	while (*datalen + n >= *buflen)
	    {
	    *buflen += 1024;
	    *buf = (char*)nmSysRealloc(*buf, *buflen);
	    if (!*buf) 
	        {
		mssError(1,"ST","Insufficient memory to load structure file data");
		return -1;
		}
	    }

    return 0;
    }


/*** st_internal_GenerateGroup - output a group listing from an info
 *** structure.
 ***/
int
st_internal_GenerateGroup(pStructInf info, pXString xs, int level, pParamObjects objlist)
    {
    int i;

	/** Print the header line. **/
	xsConcatPrintf(xs,"%*.*s%s \"%s\"\r\n%*.*s{\r\n",level*4,level*4,"",info->Name,info->UsrType,level*4+4,level*4+4,"");

	/** Print any sub-info parts and attributes **/
	for(i=0;i<info->nSubInf;i++)
	    {
	    if (info->SubInf[i]->Flags & ST_F_ATTRIB)
		st_internal_GenerateAttr(info->SubInf[i], xs, level+1, objlist);
	    else if (info->SubInf[i]->Flags & ST_F_GROUP)
		st_internal_GenerateGroup(info->SubInf[i], xs, level+1, objlist);
	    }

	/** Print trailing line **/
	xsConcatPrintf(xs,"%*.*s}\r\n",level*4+4,level*4+4,"");

    return 0;
    }


/*** st_internal_GenerateAttr - output a single attribute from an
 *** info structure.
 ***/
int
st_internal_GenerateAttr(pStructInf info, pXString xs, int level, pParamObjects objlist)
    {
    int i;
    pExpression exp;

	/** Add the attribute name and the = sign... **/
	xsConcatPrintf(xs, "%*.*s%s = ", level*4, level*4, "", info->Name);

	/** Output the various expressions. **/
	if (info->Value->NodeType != EXPR_N_LIST)
	    {
	    expGenerateText(info->Value, objlist, xsWrite, xs, '\0', "cxsql", 0);
	    }
	else
	    {
	    for(i=0;i<info->Value->Children.nItems;i++)
	        {
		exp = (pExpression)(info->Value->Children.Items[i]);
		expGenerateText(exp, objlist, xsWrite, xs, '\0', "cxsql", 0);
		if (i != info->Value->Children.nItems-1) xsConcatenate(xs,", ",2);
		}
	    }

	/** Only add semicolon if version 2 **/
	if (info->Flags & ST_F_VERSION2) 
	    xsConcatenate(xs,";\r\n",3);
	else 
	    xsConcatenate(xs,"\r\n",2);

    return 0;
    }


/*** stGenerateMsg - generate a message to output to a file or network
 *** connection.
 ***/
int
stGenerateMsgGeneric(void* dst, int (*write_fn)(), pStructInf info, int flags)
    {
    int i;
    pXString xs;
    pParamObjects objlist;

	ASSERTMAGIC(info,MGK_STRUCTINF);

	/** Allocate our initial buffer **/
	xs = (pXString)nmMalloc(sizeof(XString));
	xsInit(xs);
	objlist = expCreateParamList();
	expAddParamToList(objlist, "this", NULL, 0);

	/** If version 2, emit the version 2 header **/
	if (info->Flags & ST_F_VERSION2) xsConcatenate(xs,"$Version=2$\r\n",-1);

	/** Start the structure header. **/
	xsConcatPrintf(xs, "%s \"%s\"\r\n    {\r\n", info->Name, info->UsrType);

	/** Print any sub-info parts and attributes **/
	for(i=0;i<info->nSubInf;i++)
	    {
	    if (info->SubInf[i]->Flags & ST_F_ATTRIB)
	        {
		st_internal_GenerateAttr(info->SubInf[i], xs, 1, objlist);
		}
	    else if (info->SubInf[i]->Flags & ST_F_GROUP)
	        {
		st_internal_GenerateGroup(info->SubInf[i], xs, 1, objlist);
		}
	    }

	/** Put the trailing brace on and write the thing. **/
	xsConcatPrintf(xs, "    }\r\n");
	write_fn(dst, xs->String, xs->Length, 0,0);
	xsDeInit(xs);
	nmFree(xs,sizeof(XString));
	expFreeParamList(objlist);

    return 0;
    }

int
stGenerateMsg(pFile fd, pStructInf info, int flags)
    {
    return stGenerateMsgGeneric((void*)fd, fdWrite, info, flags);
    }

#if 00
/*** st_internal_FLStruct - parse a FormLayout structure file.
 ***/
int
st_internal_FLStruct(pLxSession s, pStructInf info)
    {
    Exception parse_err, mem_err;
    int toktype;
    char* ptr;
    pStructInf new_info = NULL;

	/** In case we get a parse error: **/
	Catch(parse_err)
	    {
	    if (new_info) stFreeInf(new_info);
	    new_info = NULL;
	    mssError(1,"ST","Parse error while parsing a FormLayout group");
	    mlxNotePosition(s);
	    return -1;
	    }
	Catch(mem_err)
	    {
	    if (new_info) stFreeInf(new_info);
	    new_info = NULL;
	    mssError(1,"ST","Internal representation exceeded while parsing a FormLayout group");
	    mlxNotePosition(s);
	    return -1;
	    }

	/** Info already has "BEGIN"... check for type and name **/
	toktype = mlxNextToken(s);
	if (toktype == MLX_TOK_PERIOD) toktype = mlxNextToken(s);
	if (toktype != MLX_TOK_KEYWORD) Throw(parse_err);
	sprintf(info->UsrType,"widget/%.50s",mlxStringVal(s,NULL));
	toktype = mlxNextToken(s);
	if (toktype != MLX_TOK_KEYWORD && toktype != MLX_TOK_STRING) Throw(parse_err);
	sprintf(info->Name,"%.63s",mlxStringVal(s,NULL));

	/** Ok, now we have a list of attributes and subgroups **/
	while(1)
	    {
	    toktype = mlxNextToken(s);
	    if (toktype == MLX_TOK_COMMA)
	        {
		mlxNextToken(s);
		continue;
		}
	    if (toktype != MLX_TOK_KEYWORD) Throw(parse_err);
	    ptr = mlxStringVal(s,NULL);
	    if (!strcmp(ptr,"END")) break;
	    if (!strcmp(ptr,"EndProperty")) continue;
	    if (!strcmp(ptr,"ENDGRIDCOLUMN")) break;
	    if (!strcmp(ptr,"BeginProperty"))
	        {
		mlxNextToken(s);
		continue;
		}
	    if (!strcmp(ptr,"BEGIN"))
	        {
		new_info = stAllocInf();
		new_info->Type = ST_T_SUBGROUP;
		if (stAddInf(info,new_info) < 0) Throw(mem_err);
		st_internal_FLStruct(s,new_info);
		continue;
		}
	    new_info = stAllocInf();
	    new_info->Type = ST_T_ATTRIB;
	    if (stAddInf(info,new_info) < 0) Throw(mem_err);
	    sprintf(new_info->Name,"%.63s",mlxStringVal(s,NULL));
	    toktype = mlxNextToken(s);
	    if (toktype != MLX_TOK_EQUALS) Throw(parse_err);
	    toktype = mlxNextToken(s);
	    if (toktype == MLX_TOK_STRING || toktype == MLX_TOK_KEYWORD)
	        {
		new_info->StrVal[0] = nmSysStrdup(mlxStringVal(s,NULL));
		new_info->nVal = 1;
		new_info->StrAlloc[0] = 1;
		}
	    else
	        {
		new_info->IntVal[0] = mlxIntVal(s);
		new_info->nVal = 1;
		}
	    }

    return 0;
    }
#endif


/*** stCreateStruct_ne - uses the simplified one-string-value-only
 *** ne version of the structures.  Create a new top-level structure.
 ***/
pStruct
stCreateStruct_ne(char* name)
    {
    pStruct this;

	this = stAllocInf_ne();
	if (!this) return NULL;
	if (name)
	    {
	    memccpy(this->Name,name,0,63);
	    this->Name[63] = 0;
	    }
	this->Type = ST_T_STRUCT;

    return this;
    }


/*** stAddAttr_ne - Add an attribute to a struct inf
 ***/
pStruct
stAddAttr_ne(pStruct this, char* name)
    {
    pStruct subinf;

	if (this->nSubInf >= 63) return NULL;
	subinf = stAllocInf_ne();
	if (!subinf) return NULL;
	memccpy(subinf->Name,name,0,63);
	subinf->Name[63] = 0;
	subinf->Type = ST_T_ATTRIB;
	stAddInf_ne(this,subinf);

    return subinf;
    }


/*** stAddGroup_ne - add a subgroup to a group.
 ***/
pStruct
stAddGroup_ne(pStruct this, char* name)
    {
    pStruct subinf;

	if (this->nSubInf >= 63) return NULL;
	subinf = stAllocInf_ne();
	if (!subinf) return NULL;
	memccpy(subinf->Name,name,0,63);
	subinf->Name[63] = 0;
	subinf->Type = ST_T_SUBGROUP;
	stAddInf_ne(this,subinf);

    return subinf;
    }


/*** stAddValue_ne - set the string value of a group.
 ***/
int
stAddValue_ne(pStruct this, char* strval)
    {

	if (this->StrAlloc && this->StrVal)
	    nmSysFree(this->StrVal);
	this->StrAlloc = 1;
	this->StrVal = nmSysStrdup(strval);

    return 1;
    }


/*** stLookup_ne - lookup a subinf in an inf.
 ***/
pStruct
stLookup_ne(pStruct this, char* name)
    {
    int i;

	if (!this) return NULL;

	for(i=0;i<this->nSubInf;i++)
	    {
	    if (!strcmp(name, this->SubInf[i]->Name))
		return this->SubInf[i];
	    }

    return NULL;
    }


/*** stAttrValue_ne - get the value of an attribute.
 ***/
int
stAttrValue_ne(pStruct this, char** strval)
    {
    
	if (this && this->StrVal) 
	    {
	    (*strval) = this->StrVal;
	    return 0;
	    }

    return -1;
    }


/*** stAllocInf_ne - allocate an inf.
 ***/
pStruct
stAllocInf_ne()
    {
    pStruct this;

	this = (pStruct)nmMalloc(sizeof(Struct));
	if (!this) return NULL;
	memset(this,0,sizeof(Struct));

    return this;
    }


/*** stFreeInf_ne_r - recursively deallocate an inf tree
 ***/
int
stFreeInf_ne_r(pStruct this)
    {
    int i;

	/** Free any string value **/
	if (this->StrVal && this->StrAlloc)
	    nmSysFree(this->StrVal);

	/** Free any subinfs **/
	for(i=0;i<this->nSubInf;i++)
	    stFreeInf_ne_r(this->SubInf[i]);
	
	nmFree(this,sizeof(Struct));

    return 0;
    }


/*** stFreeInf_ne - deallocate an inf.
 ***/
int
stFreeInf_ne(pStruct this)
    {
    int i,j;

	/** Disconnect from parent? **/
	if (this->Parent)
	    {
	    for(i=0;i<this->Parent->nSubInf;i++)
		{
		if (this == this->Parent->SubInf[i])
		    {
		    for(j=0;j<this->Parent->nSubInf-1;j++)
			{
			this->Parent->SubInf[j] = this->Parent->SubInf[j+1];
			}
		    this->Parent->nSubInf--;
		    break;
		    }
		}
	    }

	/** Free this inf. **/
	stFreeInf_ne_r(this);

    return 0;
    }


/*** stAddInf_ne - add a subinf to a parent inf.
 ***/
int
stAddInf_ne(pStruct main_inf, pStruct sub_inf)
    {
    if (main_inf->nSubInf >= 63) return -1;
    sub_inf->Parent = main_inf;
    main_inf->SubInf[main_inf->nSubInf++] = sub_inf;
    return 0;
    }


/*** stPrint_ne - format and print a pStruct tree on stdout.
 ***/
int
stPrint_ne_r(pStruct inf, int level)
    {
    int i;
    if (inf->Type == ST_T_ATTRIB)
	printf("%*.*s%s = %s\n", level*4, level*4, "", inf->Name, inf->StrVal);
    else
	{
	printf("%*.*s%s (%s)\n%*.*s{\n", level*4, level*4, "", inf->Name, inf->StrVal?inf->StrVal:"", (level+1)*4, (level+1)*4, "");
	for(i=0;i<inf->nSubInf;i++) stPrint_ne_r(inf->SubInf[i], level+1);
	printf("%*.*s}\n", (level+1)*4, (level+1)*4, "");
	}
    return 0;
    }

int
stPrint_ne(pStruct inf)
    {
    return stPrint_ne_r(inf, 0);
    }

