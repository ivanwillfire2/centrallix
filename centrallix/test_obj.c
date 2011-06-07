#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include "cxlib/mtask.h"
#include "cxlib/mtlexer.h"
#include "cxlib/util.h"
#include "obj.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "centrallix.h"
#ifdef HAVE_READLINE
/* Some versions of readline get upset if HAVE_CONFIG_H is defined! */
#ifdef HAVE_CONFIG_H
#undef HAVE_CONFIG_H
#include <readline/readline.h>
#define HAVE_CONFIG_H
#else
#include <readline/readline.h>
#endif
#include <readline/history.h>
#endif
#ifndef CENTRALLIX_CONFIG
#define CENTRALLIX_CONFIG /usr/local/etc/centrallix.conf
#endif

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
/* Module:	test_obj.c                                              */
/* Author:	Greg Beeley                                             */
/* Date:	November 1998                                           */
/*									*/
/* Description:	This module provides command-line access to the OSML	*/
/*		for testing purposes.  It does not provide a network	*/
/*		interface and does not include the DHTML generation	*/
/*		subsystem when compiled.				*/
/*									*/
/*		THIS MODULE IS **NOT** SECURE AND SHOULD NEVER BE USED	*/
/*		IN PRODUCTION WHERE THE DEVELOPER IS NOT CONTROLLING	*/
/*		ALL ASPECTS OF INPUTS AND DATA BEING HANDLED.  FIXME ;)	*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: test_obj.c,v 1.46 2010/09/08 21:40:41 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/test_obj.c,v $

    $Log: test_obj.c,v $
    Revision 1.46  2010/09/08 21:40:41  gbeeley
    - (bugfix) when outputting text/csv data, change any newlines to spaces

    Revision 1.45  2009/06/24 15:58:12  gbeeley
    - (feature) comments (beginning with #) now allowed in test obj scripts
    - (bugfix) really long lines were messing up the test_obj saved command
      history

    Revision 1.44  2008/09/14 05:12:31  gbeeley
    - (bugfix) csv output was failing if the attribute type for a column changed
      midway through the query (this can sometimes happen with NULL values).
    - (feature) test_obj now keeps a persistent command history.

    Revision 1.43  2008/08/13 21:20:24  jncraton
    - (bugfix) Presentation hints are now freed properly

    Revision 1.42  2008/04/06 20:34:31  gbeeley
    - (bugfix) "csv" command was outputting CSV strings encoded improperly.
    - (cleanup) use rl_insert instead of rl_insert_text for rl_bind_key when
      disabling tabbing.

    Revision 1.41  2008/03/09 07:55:43  gbeeley
    - Grrr.  Can you imagine that one of the tables we're dealing with here
      has over 500 columns in it?  Like I said, Grrrrr.

    Revision 1.40  2008/02/25 23:14:33  gbeeley
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

    Revision 1.39  2007/12/13 23:23:04  gbeeley
    - (bugfix) test_obj should behave itself when inserting a new record /
      object and the name isn't immediately available at the beginning of
      the insert operation.

    Revision 1.38  2007/11/16 21:39:30  gbeeley
    - (feature) added 'csv' command to generate CSV output from a query in
      the test_obj command-line interface
    - (feature) added capability to redirect output from test_obj to places
      other than standard output, via commands entered at the prompt

    Revision 1.37  2007/03/05 20:02:44  gbeeley
    - (bugfix) make tab completion use a looser check for the possibility of
      subobjects.

    Revision 1.36  2007/02/17 04:34:51  gbeeley
    - (bugfix) test_obj should open destination objects with O_TRUNC
    - (bugfix) prtmgmt should remember 'configured' line height, so it can
      auto-adjust height only if the line height is not explicitly set.
    - (change) report writer should assume some default margin settings on
      tables/table cells, so that tables aren't by default ugly :)
    - (bugfix) various floating point comparison fixes
    - (feature) allow top/bottom/left/right border options on the entire table
      itself in a report.
    - (feature) allow setting of text line height with "lineheight" attribute
    - (change) allow table to auto-scale columns should the total of column
      widths and separations exceed the available inner width of the table.
    - (feature) full justification of text.

    Revision 1.35  2005/02/26 06:42:36  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.34  2004/12/31 04:18:17  gbeeley
    - bug fix for printing Binary type attributes
    - bug fix for memory leaks due to open LxSession's

    Revision 1.33  2004/08/30 03:20:41  gbeeley
    - objInfo() can return NULL if it is not supported for an object.

    Revision 1.32  2004/07/02 00:23:24  mmcgill
    Changes include, but are not necessarily limitted to:
        - fixed test_obj hints printing, added printing of hints to show command
        to make them easier to read.
        - added objDuplicateHints, for making deep copies of hints structures.
        - made sure GroupID and VisualLength2 were set to their proper defualts
          inf objPresentationHints() [obj_attr.c]
        - did a bit of restructuring in the sybase OS driver:
    	* moved the type conversion stuff in sybdGetAttrValue into a seperate
    	  function (sybd_internal_GetCxValue, sybd_internal_GetCxType). In
    	* Got rid of the Types union, made it an ObjData struct instead
    	* Stored column lengths in ColLengths
    	* Fixed a couple minor bugs
        - Roughed out a preliminary hints implementation for the sybase driver,
          in such a way that it shouldn't be *too* big a deal to add support for
          user-defined types.

    Revision 1.31  2004/05/04 18:22:59  gbeeley
    - Adding DATA_T_BINARY data type for counted (non-zero-terminated)
      strings of data.

    Revision 1.30  2004/02/24 20:25:40  gbeeley
    - misc changes: runclient check in evaltree in stparse, eval() function
      rejected in sybase driver, update version in centrallix.conf, .cmp
      extension added for component-decl in types.cfg

    Revision 1.29  2003/09/02 15:37:13  gbeeley
    - Added enhanced command line interface to test_obj.
    - Enhancements to v3 report writer.
    - Fix for v3 print formatter in prtSetTextStyle().
    - Allow spec pathname to be provided in the openctl (command line) for
      CSV files.
    - Report writer checks for params in the openctl.
    - Local filesystem driver fix for read-only files/directories.
    - Race condition fix in UX printer osdriver
    - Banding problem workaround installed for image output in PCL.
    - OSML objOpen() read vs. read+write fix.

    Revision 1.28  2003/05/30 17:39:47  gbeeley
    - stubbed out inheritance code
    - bugfixes
    - maintained dynamic runclient() expressions
    - querytoggle on form
    - two additional formstatus widget image sets, 'large' and 'largeflat'
    - insert support
    - fix for startup() not always completing because of queries
    - multiquery module double objClose fix
    - limited osml api debug tracing

    Revision 1.27  2003/04/25 04:09:29  gbeeley
    Adding insert and autokeying support to OSML and to CSV datafile
    driver on a limited basis (in rowidkey mode only, which is the only
    mode currently supported by the csv driver).

    Revision 1.26  2003/04/04 05:02:44  gbeeley
    Added more flags to objInfo dealing with content and seekability.
    Added objInfo capability to objdrv_struct.

    Revision 1.25  2003/04/03 21:41:07  gbeeley
    Fixed xstring modification problem in test_obj as well as const path
    modification problem in the objOpen process.  Both were causing the
    cxsec stuff in xstring to squawk.

    Revision 1.24  2003/03/31 23:23:39  gbeeley
    Added facility to get additional data about an object, particularly
    with regard to its ability to have subobjects.  Added the feature at
    the driver level to objdrv_ux, and to the "show" command in test_obj.

    Revision 1.23  2003/03/30 22:49:24  jorupp
     * get rid of some compile warnings -- compiles with zero warnings under gcc 3.2.2

    Revision 1.22  2003/03/10 15:41:39  lkehresman
    The CSV objectsystem driver (objdrv_datafile.c) now presents the presentation
    hints to the OSML.  To do this I had to:
      * Move obj_internal_InfToHints() to a global function objInfToHints.  This
        is now located in utility/hints.c and the include is in include/hints.h.
      * Added the presentation hints function to the CSV driver and called it
        datPresentationHints() which returns a valid objPresentationHints object.
      * Modified test_obj.c to fix a crash bug and reformatted the output to be
        a little bit easier to read.
      * Added utility/hints.c to Makefile.in (somebody please check and make sure
        that I did this correctly).  Note that you will have to reconfigure
        centrallix for this change to take effect.

    Revision 1.21  2003/03/03 21:33:31  lkehresman
    Fixed a bug in test_obj that would segfault if no attributes were returned
    from an object.

    Revision 1.20  2003/02/26 01:32:59  jorupp
     * added presentation hints support to test_obj
    	one little problem -- for some reason, asking about just one attribute doesn't work

    Revision 1.19  2003/02/25 03:31:39  gbeeley
    Completed the 'help' message in test_obj.

    Revision 1.18  2002/09/28 01:05:30  jorupp
     * added tab completion
     * fixed bug where list/ls was relying on the pointers returned by getAttrValue being valid after another getAttrValue

    Revision 1.17  2002/09/27 22:26:03  gbeeley
    Finished converting over to the new obj[GS]etAttrValue() API spec.  Now
    my gfingrersd asre soi rtirewd iu'm hjavimng rto trype rthius ewithj nmy
    mnodse...

    Revision 1.16  2002/09/06 02:47:30  jorupp
     * removed luke's username and password hack

    Revision 1.15  2002/09/06 02:43:35  lkehresman
    Hmm.. probably shouldn't have committed my username and password with
    test_obj.

    See: http://dman.ddts.net/~dman/humorous/shooting.html
    Add:
      Centrallix:
        % cd /usr/src/centrallix
        % cvs commit

    Revision 1.14  2002/09/06 02:39:11  lkehresman
    Got OSML interaction to work with the MIME libraries thanks to
    jorupp magic.

    Revision 1.13  2002/08/13 14:22:50  lkehresman
    * removed unused variables in test_obj
    * added an incomplete "help" command to test_obj

    Revision 1.12  2002/08/10 02:43:19  gbeeley
    Test-obj now automatically displays 'system' attributes on a show
    command.  This includes inner_type, outer_type, name, and annotation,
    all of which are not supposed to be returned by the attribute enum
    functions.  FYI inner_type and content_type are synonyms (neither
    should be returned by GetFirst/Next Attr).

    Revision 1.11  2002/06/19 23:29:33  gbeeley
    Misc bugfixes, corrections, and 'workarounds' to keep the compiler
    from complaining about local variable initialization, among other
    things.

    Revision 1.10  2002/06/13 15:21:04  mattphillips
    Adding autoconf support to centrallix

    Revision 1.9  2002/06/09 23:44:45  nehresma
    This is the initial cut of the browser detection code.  Note that each widget
    needs to register which browser and style is supported.  The GNU regular
    expression library is also needed (comes with GLIBC).

    Revision 1.8  2002/06/01 19:08:46  mattphillips
    A littl ebit of code cleanup...  getting rid of some compiler warnings.

    Revision 1.7  2002/05/02 01:14:56  gbeeley
    Added dynamic module loading support in Centrallix, starting with the
    Sybase driver, using libdl.

    Revision 1.6  2002/02/14 01:05:07  gbeeley
    Fixed test_obj so that it works with the new config file stuff.

    Revision 1.5  2001/11/12 20:43:43  gbeeley
    Added execmethod nonvisual widget and the audio /dev/dsp device obj
    driver.  Added "execmethod" ls__mode in the HTTP network driver.

    Revision 1.4  2001/10/02 16:24:24  gbeeley
    Changed %f printf conversion to more intuitive %g.

    Revision 1.3  2001/09/28 19:06:18  gbeeley
    Fixed EOF handling on readline()==NULL; fixed "query" command to use inbuf
    instead of sbuf.

    Revision 1.2  2001/09/18 15:39:23  mattphillips
    Added GNU Readline support.  This adds full commandline editting support, and
    scrollback support.  No tab completion yet, though.

    NOTE: The readline and readline-devel packages (for RPM based distributions)
    are required for building now.

    Revision 1.1.1.1  2001/08/13 18:00:46  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:30:51  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/

void* my_ptr;
unsigned long ticks_last_tab=0;
pObjSession s;

struct
    {
    char    UserName[32];
    char    Password[32];
    char    CmdFile[256];
    pFile   Output;
    char    OutputFilename[256];
    char    Command[1024];
    unsigned int WaitSecs;
    }
    TESTOBJ;

#define BUFF_SIZE 1024

typedef struct
    {
    char *buffer;
    int buflen;
    } WriteStruct, *pWriteStruct;

int text_gen_callback(pWriteStruct dst, char *src, int len, int a, int b)
    {
    dst->buffer = (char*)realloc(dst->buffer,dst->buflen+len+1);
    if(!dst->buffer)
	return -1;
    memcpy(dst->buffer+dst->buflen,src,len);
    dst->buflen+=len;
    return 0;
    }

int 
printExpression(pExpression exp)
    {
    pWriteStruct dst;
    pParamObjects tmplist;

	if(!exp)
	    return -1;
	dst = (pWriteStruct)nmMalloc(sizeof(WriteStruct));
	dst->buffer=(char*)malloc(1);
	dst->buflen=0;

	tmplist = expCreateParamList();
	expAddParamToList(tmplist,"this",NULL,EXPR_O_CURRENT);
	expGenerateText(exp,tmplist,text_gen_callback,dst,'"',"cxsql", 0);
	dst->buffer[dst->buflen]='\0';
	expFreeParamList(tmplist);

	fdPrintf(TESTOBJ.Output,"%s ",dst->buffer);

	free(dst->buffer);
	nmFree(dst,sizeof(WriteStruct));
	return 0;
    }



int
testobj_show_hints(pObject obj, char* attrname)
    {
    pObjPresentationHints hints;
    int i;

    hints = objPresentationHints(obj, attrname);
    if(!hints)
	{
	mssError(1,"unable to get presentation hints for %s",attrname);
	return -1;
	}

    fdPrintf(TESTOBJ.Output,"Presentation Hints for \"%s\":\n",attrname);
    fdPrintf(TESTOBJ.Output,"  Constraint   : ");
    printExpression(hints->Constraint); fdPrintf(TESTOBJ.Output,"\n");
    fdPrintf(TESTOBJ.Output,"  DefaultExpr  : ");
    printExpression(hints->DefaultExpr); fdPrintf(TESTOBJ.Output,"\n");
    fdPrintf(TESTOBJ.Output,"  MinValue     : ");
    printExpression(hints->MinValue); fdPrintf(TESTOBJ.Output,"\n");
    fdPrintf(TESTOBJ.Output,"  MaxValue     : ");
    printExpression(hints->MaxValue); fdPrintf(TESTOBJ.Output,"\n");
    fdPrintf(TESTOBJ.Output,"  EnumList     : ");
    for(i=0;i<hints->EnumList.nItems;i++)
	{
	fdPrintf(TESTOBJ.Output,"    %s\n",(char*)xaGetItem(&hints->EnumList,i));
	}
    fdPrintf(TESTOBJ.Output,"\n");
    fdPrintf(TESTOBJ.Output,"  EnumQuery    : %s\n",hints->EnumQuery);
    fdPrintf(TESTOBJ.Output,"  Format       : %s\n",hints->Format);
    fdPrintf(TESTOBJ.Output,"  VisualLength : %i\n",hints->VisualLength);
    fdPrintf(TESTOBJ.Output,"  VisualLength2: %i\n",hints->VisualLength2);
    fdPrintf(TESTOBJ.Output,"  BitmaskRO    : ");
    for(i=0;i<32;i++)
	{
	fdPrintf(TESTOBJ.Output,"%i",hints->BitmaskRO>>(31-i) & 0x01);
	}
    fdPrintf(TESTOBJ.Output,"\n");
    fdPrintf(TESTOBJ.Output,"  Style        : %i\n",hints->Style);
    fdPrintf(TESTOBJ.Output,"  GroupID      : %i\n",hints->GroupID);
    fdPrintf(TESTOBJ.Output,"  GroupName    : %s\n",hints->GroupName);
    fdPrintf(TESTOBJ.Output,"  FriendlyName : %s\n",hints->FriendlyName);

    objFreeHints(hints);
    return 0;
    }



int
testobj_show_attr(pObject obj, char* attrname)
    {
    int type;
    int intval;
    char* stringval;
    pDateTime dt;
    double dblval;
    pMoneyType m;
    int i;
    pStringVec sv;
    pIntVec iv;
    Binary_t bn;
    pObjPresentationHints hints;

	type = objGetAttrType(obj,attrname);
	if (type < 0) 
	    {
	    fdPrintf(TESTOBJ.Output,"  %20.20s: (no such attribute)\n", attrname);
	    return -1;
	    }
	switch(type)
	    {
	    case DATA_T_INTEGER:
		if (objGetAttrValue(obj,attrname,DATA_T_INTEGER,POD(&intval)) == 1)
		    fdPrintf(TESTOBJ.Output,"  %20.20s: NULL",attrname);
		else
		    fdPrintf(TESTOBJ.Output,"  %20.20s: %d",attrname, intval);
		break;

	    case DATA_T_STRING:
		if (objGetAttrValue(obj,attrname,DATA_T_STRING,POD(&stringval)) == 1)
		    fdPrintf(TESTOBJ.Output,"  %20.20s: NULL",attrname);
		else
		    fdPrintf(TESTOBJ.Output,"  %20.20s: \"%s\"",attrname, stringval);
		break;

	    case DATA_T_BINARY:
		if (objGetAttrValue(obj,attrname,DATA_T_BINARY,POD(&bn)) == 1)
		    fdPrintf(TESTOBJ.Output,"  %20.20s: NULL", attrname);
		else
		    {
		    fdPrintf(TESTOBJ.Output,"  %20.20s: %d bytes: ", attrname, bn.Size);
		    for(i=0;i<bn.Size;i++)
			{
			fdPrintf(TESTOBJ.Output,"%2.2x  ", bn.Data[i]);
			}
		    }
		break;

	    case DATA_T_DATETIME:
		if (objGetAttrValue(obj,attrname,DATA_T_DATETIME,POD(&dt)) == 1 || dt==NULL)
		    fdPrintf(TESTOBJ.Output,"  %20.20s: NULL",attrname);
		else
		    fdPrintf(TESTOBJ.Output,"  %20.20s: %2.2d-%2.2d-%4.4d %2.2d:%2.2d:%2.2d", 
			attrname,dt->Part.Month+1, dt->Part.Day+1, dt->Part.Year+1900,
			dt->Part.Hour, dt->Part.Minute, dt->Part.Second);
		break;
	    
	    case DATA_T_DOUBLE:
		if (objGetAttrValue(obj,attrname,DATA_T_DOUBLE,POD(&dblval)) == 1)
		    fdPrintf(TESTOBJ.Output,"  %20.20s: NULL",attrname);
		else
		    fdPrintf(TESTOBJ.Output,"  %20.20s: %g", attrname, dblval);
		break;

	    case DATA_T_MONEY:
		if (objGetAttrValue(obj,attrname,DATA_T_MONEY,POD(&m)) == 1 || m == NULL)
		    fdPrintf(TESTOBJ.Output,"  %20.20s: NULL", attrname);
		else
		    fdPrintf(TESTOBJ.Output,"  %20.20s: %s", attrname, objDataToStringTmp(DATA_T_MONEY, m, 0));
		break;

	    case DATA_T_INTVEC:
		if (objGetAttrValue(obj,attrname,DATA_T_INTVEC,POD(&iv)) == 1 || iv == NULL)
		    {
		    fdPrintf(TESTOBJ.Output,"  %20.20s: NULL",attrname);
		    }
		else
		    {
		    fdPrintf(TESTOBJ.Output,"  %20.20s: ", attrname);
		    for(i=0;i<iv->nIntegers;i++) 
			fdPrintf(TESTOBJ.Output,"%d%s",iv->Integers[i],(i==iv->nIntegers-1)?"":",");
		    }
		break;

	    case DATA_T_STRINGVEC:
		if (objGetAttrValue(obj,attrname,DATA_T_STRINGVEC,POD(&sv)) == 1 || sv == NULL)
		    {
		    fdPrintf(TESTOBJ.Output,"  %20.20s: NULL",attrname);
		    }
		else
		    {
		    fdPrintf(TESTOBJ.Output,"  %20.20s: ",attrname);
		    for(i=0;i<sv->nStrings;i++) 
			fdPrintf(TESTOBJ.Output,"\"%s\"%s",sv->Strings[i],(i==sv->nStrings-1)?"":",");
		    }
		break;

	    default:
		fdPrintf(TESTOBJ.Output,"  %20.20s: <unknown type>",attrname);
		break;
	    }
	hints = objPresentationHints(obj, attrname);
	if (hints)
	    {
	    fdPrintf(TESTOBJ.Output," [Hints: ");
	    if (hints->EnumQuery != NULL) fdPrintf(TESTOBJ.Output,"EnumQuery=[%s] ", hints->EnumQuery);
	    if (hints->Format != NULL) fdPrintf(TESTOBJ.Output,"Format=[%s] ", hints->Format);
	    if (hints->AllowChars != NULL) fdPrintf(TESTOBJ.Output,"AllowChars=[%s] ", hints->AllowChars);
	    if (hints->BadChars != NULL) fdPrintf(TESTOBJ.Output,"BadChars=[%s] ", hints->BadChars);
	    if (hints->Length != 0) fdPrintf(TESTOBJ.Output,"Length=%d ", hints->Length);
	    if (hints->VisualLength != 0) fdPrintf(TESTOBJ.Output,"VisualLength=%d ", hints->VisualLength);
	    if (hints->VisualLength2 != 1) fdPrintf(TESTOBJ.Output,"VisualLength2=%d ", hints->VisualLength2);
	    if (hints->Style != 0) fdPrintf(TESTOBJ.Output,"Style=%d ", hints->Style);
	    if (hints->StyleMask != 0) fdPrintf(TESTOBJ.Output,"StyleMask=%d ", hints->StyleMask);
	    if (hints->GroupID != -1) fdPrintf(TESTOBJ.Output,"GroupID=%d ", hints->GroupID);
	    if (hints->GroupName != NULL) fdPrintf(TESTOBJ.Output,"GroupName=[%s] ", hints->GroupName);
	    if (hints->FriendlyName != NULL) fdPrintf(TESTOBJ.Output,"FriendlyName=[%s] ", hints->FriendlyName);
	    if (hints->Constraint != NULL) { fdPrintf(TESTOBJ.Output,"Constraint="); printExpression(hints->Constraint); }
	    if (hints->DefaultExpr != NULL) { fdPrintf(TESTOBJ.Output,"DefaultExpr="); printExpression(hints->DefaultExpr); }
	    if (hints->MinValue != NULL) { fdPrintf(TESTOBJ.Output,"MinValue="); printExpression(hints->MinValue); }
	    if (hints->MaxValue != NULL) { fdPrintf(TESTOBJ.Output,"MaxValue="); printExpression(hints->MaxValue); }
	    objFreeHints(hints);
	    fdPrintf(TESTOBJ.Output,"]\n");
	    }

    return 0;
    }

int handle_tab(int unused_1, int unused_2)
    {
    pXString xstrInput;
    pXString xstrLastInputParam;
    pXString xstrPath;
    pXString xstrPartialName;
    pXString xstrQueryString;
    pXString xstrMatched;
    int i;
    pObject obj,qobj;
    pObjQuery qry;
    int count=0;
    short secondtab=0;
    pObjectInfo info;

#define DOUBLE_TAB_DELAY 50
    
    /** init all the XStrings **/
    xstrInput=nmMalloc(sizeof(XString));
    xstrLastInputParam=nmMalloc(sizeof(XString));
    xstrPath=nmMalloc(sizeof(XString));
    xstrPartialName=nmMalloc(sizeof(XString));
    xstrQueryString=nmMalloc(sizeof(XString));
    xstrMatched=nmMalloc(sizeof(XString));
    xsInit(xstrInput);
    xsInit(xstrLastInputParam);
    xsInit(xstrPath);
    xsInit(xstrPartialName);
    xsInit(xstrQueryString);
    xsInit(xstrMatched);

    /** figure out if this is a 'double' tab (should list the possibilities) **/
    if(ticks_last_tab+DOUBLE_TAB_DELAY>mtRealTicks())
	{
	secondtab=1;
	/* don't want this branch to be taken next time, even if it's within DOUBLE_TAB_DELAY */
	ticks_last_tab=0;
	}
    else
	ticks_last_tab=mtRealTicks();

    /** beep **/
    printf("%c",0x07);

    /** get the current line into an XString **/
    xsCopy(xstrInput,rl_copy_text (0,256),-1);
    
    /** get a new line if we're going to print a list **/
    if(secondtab) printf("\n");
    
    /** find the space, grab everything after it (or the whole thing if there is no space) **/
    i=xsFindRev(xstrInput," ",-1,0);
    if(i < 0)
	xsCopy(xstrLastInputParam,xstrInput->String,xstrInput->Length);
    else
	xsCopy(xstrLastInputParam,xstrInput->String+i+1,xstrInput->Length-i+1);


    /** is this a relative or absolute path **/
    if(xstrLastInputParam->String[0]!='/')
	{
	/* objGetWD has the trailing slash aready at the root */
	if(strlen(objGetWD(s))>1)
	    xsInsertAfter(xstrLastInputParam,"/",-1,0);
	xsInsertAfter(xstrLastInputParam,objGetWD(s),-1,0);
	}

    /** split on the last slash **/
    i=xsFindRev(xstrLastInputParam,"/",-1,0);
    xsCopy(xstrPath,xstrLastInputParam->String,i+1);
    xsCopy(xstrPartialName,xstrLastInputParam->String+i+1,-1);

    /** open the directory **/
    obj=objOpen(s,xstrPath->String,O_RDONLY,0400,NULL);

    /** build the query **/
    xsPrintf(xstrQueryString,"substring(:name,0,%d)=\"%s\"",xstrPartialName->Length,xstrPartialName->String);

    /** open the query **/
    info = objInfo(obj);
    if (!info || !(info->Flags & (OBJ_INFO_F_CANT_HAVE_SUBOBJ | OBJ_INFO_F_NO_SUBOBJ)))
	qry = objOpenQuery(obj,xstrQueryString->String,NULL,NULL,NULL);
    else
	qry = NULL;

    /** fetch and compare **/
    while(qry && (qobj=objQueryFetch(qry,O_RDONLY)))
	{
	char *ptr;
	int i;
	/* this is our second one -- output the first one (if second tab...)*/
	if(count==1)
	    if(secondtab) 
		printf("%s\n",xstrMatched->String);

	/* get this entry's name */
	objGetAttrValue(qobj,"name",DATA_T_STRING,POD(&ptr));
	/* compare with xstrMatched and shorten xstrMatched so that it contains the longest string
	 *   that starts both of them */
	for(i=0;i<=strlen(ptr) && i<=strlen(xstrMatched->String);i++)
	    {
	    if(ptr[i]!=xstrMatched->String[i])
		{
		xsSubst(xstrMatched, i, -1, "", 0);
		}
	    }
	/** if this isn't the first returned object, output it unless second tab.. **/
	if(count>0)
	    {
	    if(secondtab) 
		printf("%s\n",ptr);
	    }
	else
	    /* otherwise, put the entire thing in xstrMatched */
	    xsCopy(xstrMatched,ptr,-1);

	count++;
	objClose(qobj);
	}

    if(qry)
	objQueryClose(qry);

    /** there were objects found -- add the longest common string to the input line **/
    if(count)
	{
	rl_insert_text(xstrMatched->String+xstrPartialName->Length);
	}

    if(count==1)
	{
	/** decide if this is a 'directory' that we should put a '/' on the end of **/
	pXString xstrTemp;
	pObject obj2;

	/* next tab should always be a 'first' after a successful match */
	ticks_last_tab=0;

	/** setup XString **/
	xstrTemp=nmMalloc(sizeof(XString));
	xsInit(xstrTemp);

	/** build path for open **/
	xsPrintf(xstrTemp,"%s/%s",xstrPath->String,xstrMatched->String);

	obj2=objOpen(s,xstrTemp->String,O_RDONLY,0400,NULL);
	if(obj2)
	    {
	    /** see if there are any subobjects -- only need to fetch 1 to check **/
	    info = objInfo(obj2);
	    if (!info || !(info->Flags & (OBJ_INFO_F_CANT_HAVE_SUBOBJ | OBJ_INFO_F_NO_SUBOBJ)))
		qry=objOpenQuery(obj2,NULL,NULL,NULL,NULL);
	    else
		qry=NULL;
	    if(qry && (qobj=objQueryFetch(qry,O_RDONLY)))
		{
		rl_insert_text("/");
		objClose(qobj);
		}
	    else
		{
		/** put a newline after the errors that were probably thrown **/
		printf("\n");
		rl_on_new_line();
		}
	    
	    /** close the object and query we opened **/
	    if(qry) objQueryClose(qry);
	    objClose(obj2);
	    }

	/** cleanup **/
	xsDeInit(xstrTemp);
	nmFree(xstrTemp,sizeof(XString));
	}

    if(obj)
	objClose(obj);

    
    /** get rid of all the XStrings **/
    xsDeInit(xstrInput);
    xsDeInit(xstrLastInputParam);
    xsDeInit(xstrPath);
    xsDeInit(xstrPartialName);
    xsDeInit(xstrQueryString);
    xsDeInit(xstrMatched);
    nmFree(xstrInput,sizeof(XString));
    nmFree(xstrLastInputParam,sizeof(XString));
    nmFree(xstrPath,sizeof(XString));
    nmFree(xstrPartialName,sizeof(XString));
    nmFree(xstrQueryString,sizeof(XString));
    nmFree(xstrMatched,sizeof(XString));

    /** if we output lines, go to an new one and inform readline **/
    if(secondtab)
	{
	printf("\n");
	rl_on_new_line();
	}

    return 0;
    }


int
testobj_do_cmd(pObjSession s, char* cmd, int batch_mode)
    {
    char sbuf[BUFF_SIZE];
    char* ptr;
    char cmdname[64];
    pObject obj,child_obj,to_obj;
    pObjQuery qy;
    char* filename;
    char* filetype;
    char* fileannot;
    int cnt;
    char* attrname;
    char* methodname;
    int type;
    DateTime dtval;
    pDateTime dt;
    char* stringval;
    int intval;
    int is_where, is_orderby;
    pLxSession ls = NULL;
    char where[256];
    char orderby[256];
    double dblval;
    pMoneyType m;
    MoneyType mval;
    pObjData pod;
    ObjData od;
    int use_srctype;
    char mname[64];
    char mparam[256];
    char* mptr;
    int t,i;
    pObjectInfo info;
    Binary_t bn;
    pFile try_file;
    char* attrnames[640];
    int attrtypes[640];
    int n_attrs;
    int name_was_null;

	    /** Just a comment? **/
	    if (cmd[0] == '#')
		return 0;

	    /** Open a lexer session **/
	    ls = mlxStringSession(cmd,MLX_F_ICASE | MLX_F_EOF);
	    if (mlxNextToken(ls) != MLX_TOK_KEYWORD)
		{
		mlxCloseSession(ls);
		return -1;
		}
	    ptr = mlxStringVal(ls,NULL);
	    if (!ptr) 
		{
		mlxCloseSession(ls);
		return -1;
		}
	    strcpy(cmdname,ptr);
	    mlxSetOptions(ls,MLX_F_IFSONLY);
	    if ((t=mlxNextToken(ls)) != MLX_TOK_STRING) ptr = NULL;
	    else ptr = mlxStringVal(ls,NULL);
	    mlxUnsetOptions(ls,MLX_F_IFSONLY);

	    /** What command? **/
	    if (!strcmp(cmdname,"cd"))
		{
		if (!ptr)
		    {
		    printf("Usage: cd <directory>\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		obj = objOpen(s, ptr, O_RDONLY, 0600, "system/directory");
		if (!obj)
		    {
		    printf("cd: could not change to '%s'.\n",ptr);
		    mlxCloseSession(ls);
		    return -1;
		    }
		objSetWD(s,obj);
		objClose(obj);
		}
	    else if (!strcmp(cmdname,"csv"))
		{
		if (!ptr)
		    {
		    printf("Usage: csv <query-text>\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		qy = objMultiQuery(s, cmd + 4, NULL, 0);
		if (!qy)
		    {
		    printf("csv: could not open query!\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		n_attrs = -1;
		while((obj=objQueryFetch(qy, O_RDONLY)))
		    {
		    if (n_attrs < 0)
			{
			n_attrs = 0;
			for(attrname=objGetFirstAttr(obj);attrname;attrname=objGetNextAttr(obj))
			    {
			    if (n_attrs < sizeof(attrnames) / sizeof(char*))
				{
				attrnames[n_attrs] = nmSysStrdup(attrname);
				fdQPrintf(TESTOBJ.Output,"%[,%]\"%STR&DSYB\"",
					n_attrs != 0,
					attrname);
				n_attrs++;
				}
			    }
			fdPrintf(TESTOBJ.Output, "\n");
			}
		    for(i=0;i<n_attrs;i++)
			{
			attrtypes[i] = objGetAttrType(obj,attrnames[i]);
			if (objGetAttrValue(obj, attrnames[i], attrtypes[i], &od) == 0)
			    {
			    if (attrtypes [i] == DATA_T_CODE)
				ptr = NULL;
			    else if (attrtypes[i] == DATA_T_INTEGER || attrtypes[i] == DATA_T_DOUBLE)
				ptr = objDataToStringTmp(attrtypes[i], &od, 0);
			    else
				ptr = objDataToStringTmp(attrtypes[i], od.Generic, 0);
			    }
			else
			    {
			    ptr = NULL;
			    }
			if (ptr == NULL)
			    fdQPrintf(TESTOBJ.Output, "%[,%]", i!=0);
			else if  (attrtypes[i] == DATA_T_INTEGER || attrtypes[i] == DATA_T_DOUBLE || attrtypes[i] == DATA_T_MONEY || attrtypes[i] == DATA_T_DATETIME)
			    fdQPrintf(TESTOBJ.Output, "%[,%]%STR", i!=0, ptr);
			else
			    {
			    while (strpbrk(ptr, "\r\n")) *(strpbrk(ptr, "\r\n")) = ' ';
			    fdQPrintf(TESTOBJ.Output, "%[,%]\"%STR&DSYB\"", i!=0, ptr);
			    }
			}
		    fdPrintf(TESTOBJ.Output, "\n");
		    objClose(obj);
		    }
		objQueryClose(qy);
		}
	    else if (!strcmp(cmdname,"query"))
	        {
		if (!ptr)
		    {
		    printf("Usage: query <query-text>\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		qy = objMultiQuery(s, cmd + 6, NULL, 0);
		if (!qy)
		    {
		    printf("query: could not open query!\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		while((obj=objQueryFetch(qy, O_RDONLY)))
		    {
		    for(attrname=objGetFirstAttr(obj);attrname;attrname=objGetNextAttr(obj))
		        {
			type = objGetAttrType(obj,attrname);
			switch(type)
			    {
			    case DATA_T_INTEGER:
			        if (objGetAttrValue(obj,attrname,DATA_T_INTEGER,POD(&intval)) == 1)
			            fdPrintf(TESTOBJ.Output,"Attribute: [%s]  INTEGER  NULL\n", attrname);
				else
			            fdPrintf(TESTOBJ.Output,"Attribute: [%s]  INTEGER  %d\n", attrname,intval);
				break;
			    case DATA_T_STRING:
			        if (objGetAttrValue(obj,attrname,DATA_T_STRING,POD(&stringval)) == 1)
			            fdPrintf(TESTOBJ.Output,"Attribute: [%s]  STRING  NULL\n", attrname);
				else
			            fdPrintf(TESTOBJ.Output,"Attribute: [%s]  STRING  \"%s\"\n", attrname,stringval);
			        break;
			    case DATA_T_DOUBLE:
			        if (objGetAttrValue(obj,attrname,DATA_T_DOUBLE,POD(&dblval)) == 1)
				    fdPrintf(TESTOBJ.Output,"Attribute: [%s]  DOUBLE  NULL\n", attrname);
				else
				    fdPrintf(TESTOBJ.Output,"Attribute: [%s]  DOUBLE  %g\n", attrname, dblval);
				break;
			    case DATA_T_BINARY:
				if (objGetAttrValue(obj,attrname,DATA_T_BINARY,POD(&bn)) == 1)
				    fdPrintf(TESTOBJ.Output,"  %20.20s: NULL\n", attrname);
				else
				    {
				    fdPrintf(TESTOBJ.Output,"  %20.20s: %d bytes: ", attrname, bn.Size);
				    for(i=0;i<bn.Size;i++)
					{
					fdPrintf(TESTOBJ.Output,"%2.2x  ", bn.Data[i]);
					}
				    fdPrintf(TESTOBJ.Output,"\n");
				    }
				break;
			    case DATA_T_DATETIME:
			        if (objGetAttrValue(obj,attrname,DATA_T_DATETIME,POD(&dt)) == 1)
				    fdPrintf(TESTOBJ.Output,"Attribute: [%s]  DATETIME  NULL\n", attrname);
				else
				    fdPrintf(TESTOBJ.Output,"Attribute: [%s]  DATETIME  %s\n", attrname, objDataToStringTmp(type, dt, 0));
				break;
			    case DATA_T_MONEY:
			        if (objGetAttrValue(obj,attrname,DATA_T_MONEY,POD(&m)) == 1)
				    fdPrintf(TESTOBJ.Output,"Attribute: [%s]  MONEY  NULL\n", attrname);
				else
				    fdPrintf(TESTOBJ.Output,"Attribute: [%s]  MONEY  %s\n", attrname, objDataToStringTmp(type, m, 0));
				break;
			    }
			}
		    objClose(obj);
		    }
		objQueryClose(qy);
		}
	    else if (!strcmp(cmdname,"annot"))
	        {
		if (!ptr)
		    {
		    printf("Usage: annot <filename> <annotation>\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		obj = objOpen(s, ptr, O_RDWR, 0600, "system/object");
		if (!obj)
		    {
		    printf("annot: could not open '%s'.\n",ptr);
		    mlxCloseSession(ls);
		    return -1;
		    }
		if (mlxNextToken(ls) != MLX_TOK_STRING)
		    {
		    printf("Usage: annot <filename> <annotation>\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		ptr = mlxStringVal(ls,NULL);
		objSetAttrValue(obj, "annotation", DATA_T_STRING,POD(&ptr));
		objClose(obj);
		}
	    else if (!strcmp(cmdname,"list") || !strcmp(cmdname, "ls"))
		{
		is_where = 0;
		is_orderby = 0;
		if (ptr && !strcmp(ptr,"where"))
		    { 
		    is_where = 1;
		    ptr = "";
		    }
		if (ptr && !strcmp(ptr,"orderby"))
		    {
		    is_orderby = 1;
		    ptr = "";
		    }
		if (!ptr) ptr = "";
		obj = objOpen(s, ptr, O_RDONLY, 0600, "system/directory");
		if (!obj)
		    {
		    printf("list: could not open directory '%s'.\n",ptr);
		    mlxCloseSession(ls);
		    return -1;
		    }
		if (!is_where && !is_orderby)
		    {
		    if (t != MLX_TOK_EOF && (t=mlxNextToken(ls)) != MLX_TOK_KEYWORD) ptr = NULL;
		    else ptr = mlxStringVal(ls,NULL);
		    if (ptr && !strcmp(ptr,"where")) is_where = 1;
		    if (ptr && !strcmp(ptr,"orderby")) is_orderby = 1;
		    }
		if (is_where)
		    {
		    if (t != MLX_TOK_EOF && (t=mlxNextToken(ls)) != MLX_TOK_STRING) ptr = NULL;
		    else ptr = mlxStringVal(ls,NULL);
		    printf("where: '%s'\n",ptr);
		    strcpy(where,ptr);
		    if (((t=mlxNextToken(ls)) == MLX_TOK_KEYWORD) && !strcmp("orderby",mlxStringVal(ls,NULL)))
		        {
			if ((t=mlxNextToken(ls)) == MLX_TOK_STRING)
			    strcpy(orderby, mlxStringVal(ls,NULL));
			else
			    orderby[0] = 0;
			}
		    else
		        {
			orderby[0] = 0;
			}
		    qy = objOpenQuery(obj,where,orderby[0]?orderby:NULL,NULL,NULL);
		    }
		else if (is_orderby)
		    {
		    t = mlxNextToken(ls);
		    if (t != MLX_TOK_STRING) 
			{
			objClose(obj);
			mlxCloseSession(ls);
			return -1;
			}
		    strcpy(orderby, mlxStringVal(ls,NULL));
		    qy = objOpenQuery(obj,NULL,orderby,NULL,NULL);
		    }
		else
		    {
		    qy = objOpenQuery(obj,"",NULL,NULL,NULL);
		    }
		if (!qy)
		    {
		    objClose(obj);
		    printf("list: object '%s' doesn't support directory queries.\n",ptr);
		    mlxCloseSession(ls);
		    return -1;
		    }
		while(NULL != (child_obj = objQueryFetch(qy,O_RDONLY)))
		    {
		    if (objGetAttrValue(child_obj,"name",DATA_T_STRING,POD(&filename)) >= 0)
			{
			char *name,*type,*annot;
#define MALLOC_AND_COPY(dest,src) \
			dest=(char*)malloc(strlen(src)+1);\
			if(!dest) break;\
			strcpy(dest,src);
			MALLOC_AND_COPY(name,filename);
			objGetAttrValue(child_obj,"outer_type",DATA_T_STRING,POD(&filetype));
			MALLOC_AND_COPY(type,filetype);
			objGetAttrValue(child_obj,"annotation",DATA_T_STRING,POD(&fileannot));
			MALLOC_AND_COPY(annot,fileannot);
			fdPrintf(TESTOBJ.Output,"%-32.32s  %-32.32s    %s\n",name,annot,type);
			free(name);
			free(type);
			free(annot);
			}
		    objClose(child_obj);
		    }
		objQueryClose(qy);
		objClose(obj);
		}
	    else if (!strcmp(cmdname,"show"))
		{
		if (!ptr) ptr = "";
		obj = objOpen(s, ptr, O_RDONLY, 0600, "system/object");
		if (!obj)
		    {
		    printf("show: could not open object '%s'\n",ptr);
		    mlxCloseSession(ls);
		    return -1;
		    }
		info = objInfo(obj);
		if (info)
		    {
		    if (info->Flags)
			{
			fdPrintf(TESTOBJ.Output,"Flags: ");
			if (info->Flags & OBJ_INFO_F_NO_SUBOBJ) fdPrintf(TESTOBJ.Output,"no_subobjects ");
			if (info->Flags & OBJ_INFO_F_HAS_SUBOBJ) fdPrintf(TESTOBJ.Output,"has_subobjects ");
			if (info->Flags & OBJ_INFO_F_CAN_HAVE_SUBOBJ) fdPrintf(TESTOBJ.Output,"can_have_subobjects ");
			if (info->Flags & OBJ_INFO_F_CANT_HAVE_SUBOBJ) fdPrintf(TESTOBJ.Output,"cant_have_subobjects ");
			if (info->Flags & OBJ_INFO_F_SUBOBJ_CNT_KNOWN) fdPrintf(TESTOBJ.Output,"subobject_cnt_known ");
			if (info->Flags & OBJ_INFO_F_CAN_ADD_ATTR) fdPrintf(TESTOBJ.Output,"can_add_attrs ");
			if (info->Flags & OBJ_INFO_F_CANT_ADD_ATTR) fdPrintf(TESTOBJ.Output,"cant_add_attrs ");
			if (info->Flags & OBJ_INFO_F_CAN_SEEK_FULL) fdPrintf(TESTOBJ.Output,"can_seek_full ");
			if (info->Flags & OBJ_INFO_F_CAN_SEEK_REWIND) fdPrintf(TESTOBJ.Output,"can_seek_rewind ");
			if (info->Flags & OBJ_INFO_F_CANT_SEEK) fdPrintf(TESTOBJ.Output,"cant_seek ");
			if (info->Flags & OBJ_INFO_F_CAN_HAVE_CONTENT) fdPrintf(TESTOBJ.Output,"can_have_content ");
			if (info->Flags & OBJ_INFO_F_CANT_HAVE_CONTENT) fdPrintf(TESTOBJ.Output,"cant_have_content ");
			if (info->Flags & OBJ_INFO_F_HAS_CONTENT) fdPrintf(TESTOBJ.Output,"has_content ");
			if (info->Flags & OBJ_INFO_F_NO_CONTENT) fdPrintf(TESTOBJ.Output,"no_content ");
			if (info->Flags & OBJ_INFO_F_SUPPORTS_INHERITANCE) fdPrintf(TESTOBJ.Output,"supports_inheritance ");
			fdPrintf(TESTOBJ.Output,"\n");
			if (info->Flags & OBJ_INFO_F_SUBOBJ_CNT_KNOWN)
			    {
			    fdPrintf(TESTOBJ.Output,"Subobject count: %d\n", info->nSubobjects);
			    }
			}
		    }
		fdPrintf(TESTOBJ.Output,"Attributes:\n");
		testobj_show_attr(obj,"outer_type");
		testobj_show_attr(obj,"inner_type");
		testobj_show_attr(obj,"content_type");
		testobj_show_attr(obj,"name");
		testobj_show_attr(obj,"annotation");
		testobj_show_attr(obj,"last_modification");
		attrname = objGetFirstAttr(obj);
		while(attrname)
		    {
		    testobj_show_attr(obj,attrname);
		    attrname = objGetNextAttr(obj);
		    }
		fdPrintf(TESTOBJ.Output,"\nMethods:\n");
		methodname = objGetFirstMethod(obj);
		if (methodname)
		    {
		    while(methodname)
			{
			fdPrintf(TESTOBJ.Output,"  %20.20s()\n",methodname);
			methodname = objGetNextMethod(obj);
			}
		    }
		else
		    {
		    fdPrintf(TESTOBJ.Output,"  (no methods)\n");
		    }
		puts("");
		objClose(obj);
		}
	    else if (!strcmp(cmdname,"print"))
		{
		if (!ptr) ptr = "";
		obj = objOpen(s, ptr, O_RDONLY, 0600, "text/plain");
		if (!obj)
		    {
		    printf("print: could not open object '%s'\n",ptr);
		    mlxCloseSession(ls);
		    return -1;
		    }
		while((cnt=objRead(obj, sbuf, 255, 0, 0)) >0)
		    {
		    sbuf[cnt] = 0;
		    fdWrite(TESTOBJ.Output,sbuf,cnt,0,0);
		    }
		fdPrintf(TESTOBJ.Output,"\n");
		objClose(obj);
		}
	    else if (!strcmp(cmdname,"copy"))
	        {
		if (!ptr)
		    {
		    printf("copy1: must specify <dsttype/srctype> <source> <destination>\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		if (!strcmp(ptr,"srctype")) use_srctype = 1; else use_srctype = 0;
		if (mlxNextToken(ls) != MLX_TOK_STRING)
		    {
		    printf("copy2: must specify <dsttype/srctype> <source> <destination>\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		mlxCopyToken(ls, sbuf, 1023);
		if (mlxNextToken(ls) != MLX_TOK_STRING)
		    {
		    printf("copy3: must specify <dsttype/srctype> <source> <destination>\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		ptr = mlxStringVal(ls, NULL);
		if (use_srctype)
		    {
		    obj = objOpen(s, sbuf, O_RDONLY, 0600, "application/octet-stream");
		    if (!obj) 
			{
			mlxCloseSession(ls);
			return -1;
			}
		    objGetAttrValue(obj, "inner_type", DATA_T_STRING,POD(&stringval));
		    to_obj = objOpen(s, ptr, O_RDWR | O_CREAT | O_TRUNC, 0600, stringval);
		    if (!to_obj)
		        {
			objClose(obj);
			mlxCloseSession(ls);
			return -1;
			}
		    }
		else
		    {
		    to_obj = objOpen(s, ptr, O_RDWR | O_CREAT | O_TRUNC, 0600, "application/octet-stream");
		    if (!to_obj)
			{
			mlxCloseSession(ls);
			return -1;
			}
		    objGetAttrValue(to_obj, "inner_type", DATA_T_STRING,POD(&stringval));
		    obj = objOpen(s, sbuf, O_RDONLY, 0600, stringval);
		    if (!obj)
		        {
			objClose(to_obj);
			mlxCloseSession(ls);
			return -1;
			}
		    }
		while((cnt = objRead(obj, sbuf, 255, 0, 0)) > 0)
		    {
		    objWrite(to_obj, sbuf, cnt, 0, 0);
		    }
		objClose(obj);
		objClose(to_obj);
		}
	    else if (!strcmp(cmdname,"delete"))
	        {
		if (!ptr)
		    {
		    printf("delete: must specify object.\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		if (objDelete(s, ptr) < 0)
		    {
		    printf("delete: failed.\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		}
	    else if (!strcmp(cmdname,"create"))
	        {
		if (!ptr) 
		    {
		    printf("create: must specify object.\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		if (!strcmp(ptr,"*"))
		    obj = objOpen(s, ptr, O_RDWR | O_CREAT | OBJ_O_AUTONAME, 0600, "system/object");
		else
		    obj = objOpen(s, ptr, O_RDWR | O_CREAT, 0600, "system/object");
		if (!obj)
		    {
		    printf("create: could not create object.\n");
		    mssPrintError(TESTOBJ.Output);
		    mlxCloseSession(ls);
		    return -1;
		    }
		name_was_null = 0;
		if (!strcmp(ptr,"*"))
		    {
		    if (objGetAttrValue(obj, "name", DATA_T_STRING, POD(&stringval)) == 1)
			{
			printf("New object name cannot yet be determined - please enter attributes first.\n");
			name_was_null = 1;
			}
		    else
			{
			printf("New object name is '%s'\n", stringval);
			}
		    }
		puts("Enter attributes, blank line to end.");
		rl_bind_key ('\t', rl_insert);
		while(1)
		    {
		    char* slbuf = readline("");
		    strncpy(sbuf, slbuf, BUFF_SIZE-1);
		    sbuf[BUFF_SIZE-1] = 0;
		    if (sbuf[0] == 0) break;
		    attrname = strtok(sbuf,"=");
		    stringval = strtok(NULL,"=");
		    while(*stringval == ' ') stringval++;
		    while(attrname[strlen(attrname)-1] == ' ') attrname[strlen(attrname)-1]=0;
		    type = objGetAttrType(obj,attrname);
		    if (type < 0 || type == DATA_T_UNAVAILABLE)
		        {
		        if (*stringval >= '0' && *stringval <= '9')
		            {
			    intval = strtoi(stringval,NULL,10);
			    objSetAttrValue(obj,attrname,DATA_T_INTEGER,POD(&intval));
			    }
		        else
		            {
			    objSetAttrValue(obj,attrname,DATA_T_STRING,POD(&stringval));
			    }
			}
		    else
		        {
			switch(type)
			    {
			    case DATA_T_INTEGER: intval = objDataToInteger(DATA_T_STRING,(void*)stringval,NULL); pod = POD(&intval); break;
			    case DATA_T_STRING: pod = POD(&stringval); break;
			    case DATA_T_DOUBLE: dblval = objDataToDouble(DATA_T_STRING,(void*)stringval); pod = POD(&dblval); break;
			    case DATA_T_DATETIME: dt = &dtval; objDataToDateTime(DATA_T_STRING,(void*)stringval,dt,NULL); pod = POD(&dt); break;
			    case DATA_T_MONEY: m = &mval; objDataToMoney(DATA_T_STRING,(void*)stringval, m); pod = POD(&m); break;
			    default:
				printf("create: warning - invalid attribute type for attr '%s'\n", attrname);
				pod = NULL;
				break;
			    }
			if (pod) objSetAttrValue(obj,attrname,type,pod);
			}
		    }
		if (name_was_null)
		    {
		    if (objGetAttrValue(obj, "name", DATA_T_STRING, POD(&stringval)) == 1)
			printf("New object name cannot be determined - something went wrong.\n");
		    else
			printf("New object name is '%s'\n", stringval);
		    }
		if (objClose(obj) < 0) mssPrintError(TESTOBJ.Output);
		rl_bind_key ('\t', handle_tab);
		}
	    else if (!strcmp(cmdname,"quit"))
		{
		mlxCloseSession(ls);
		return 1;
		}
	    else if (!strcmp(cmdname,"exec"))
	        {
		if (!ptr) ptr = "";
		obj = objOpen(s, ptr, O_RDONLY, 0600, "application/octet-stream");
		if (!obj)
		    {
		    printf("exec: could not open object '%s'\n",ptr);
		    mlxCloseSession(ls);
		    return -1;
		    }
		if (mlxNextToken(ls) != MLX_TOK_STRING)
		    {
		    printf("Usage: exec <obj> <method> <parameter>\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		mlxCopyToken(ls, mname, 63);
		if (mlxNextToken(ls) != MLX_TOK_STRING)
		    {
		    printf("Usage: exec <obj> <method> <parameter>\n");
		    mlxCloseSession(ls);
		    return -1;
		    }
		mlxCopyToken(ls, mparam, 255);
		mptr = mparam;
		objExecuteMethod(obj, mname, POD(&mptr));
		objClose(obj);
		}
	    else if (!strcmp(cmdname,"hints"))
		{
		if (!ptr) ptr = "";
		obj = objOpen(s, ptr, O_RDONLY, 0600, "system/object");
		if (!obj)
		    {
		    printf("hints: could not open object '%s'\n",ptr);
		    mlxCloseSession(ls);
		    return -1;
		    }
		attrname=NULL;
		if (mlxNextToken(ls) == MLX_TOK_STRING)
		    {
		    attrname=nmMalloc(64);
		    mlxCopyToken(ls, attrname, 63);
		    attrname[63]='\0';
		    }
		if (attrname)
		    {
		    testobj_show_hints(obj, attrname);
		    nmFree(attrname,64);
		    }
		else
		    {
		    attrname = objGetFirstAttr(obj);
		    do
			{
			if (attrname != NULL)
			    {
			    testobj_show_hints(obj, attrname);
			    }
			}
		    while ((attrname = objGetNextAttr(obj)));
		    }
		objClose(obj);
		}
	    else if (!strcmp(cmdname, "output"))
		{
		if (!ptr) ptr = "/dev/tty";
		try_file = fdOpen(ptr, O_RDWR | O_CREAT | O_TRUNC, 0600);
		if (!try_file)
		    {
		    mssErrorErrno(1,"CX","Unable to open output file '%s'",ptr);
		    }
		else
		    {
		    if (TESTOBJ.Output)
			fdClose(TESTOBJ.Output, 0);
		    TESTOBJ.Output = try_file;
		    strtcpy(TESTOBJ.OutputFilename, ptr, sizeof(TESTOBJ.OutputFilename));
		    }
		}
	    else if (!strcmp(cmdname,"crash"))
		{
		raise(SIGSEGV);
		}
	    else if (!strcmp(cmdname,"sleep"))
		{
		if (!ptr)
		    {
		    mssError(1,"CX","Usage: sleep <n>");
		    }
		else
		    {
		    intval = strtoi(ptr, NULL, 10);
		    sleep(intval);
		    }
		}
	    else if (!strcmp(cmdname,"help"))
		{
		printf("Available Commands:\n");
		printf("  annot    - Add or change the annotation on an object.\n");
		printf("  cd       - Change the current working \"directory\" in the objectsystem.\n");
		printf("  copy     - Copy one object's content to another.\n");
		printf("  create   - Create a new object.\n");
		printf("  csv      - Run a SQL query and print the results in CSV format.\n");
		printf("  delete   - Delete an object.\n");
		printf("  exec     - Call a method on an object.\n");
		printf("  hints    - Show the presentation hints of an attribute (or object)\n");
		printf("  help     - Displays this help screen.\n");
		printf("  list, ls - Lists the objects in the current \"directory\" in the objectsystem.\n");
		printf("  output   - Change where output goes.\n");
		printf("  print    - Displays an object's content.\n");
		printf("  query    - Runs a SQL query.\n");
		printf("  quit     - Exits this application.\n");
		printf("  show     - Displays an object's attributes and methods.\n");
		}
	    else
		{
		printf("Unknown command '%s'\n",cmdname);
		return -1;
		}
	
	    mlxCloseSession(ls);

    return 0;
    }


void
start(void* v)
    {
    int rval,t;
    char* inbuf = NULL;
    char* user;
    char* pwd;
    char prompt[1024];
    pFile cmdfile;
    pFile histfile = NULL;
    char histname[256];
    char* home;
    pLxSession input_lx;
    char* ptr;
    int alloc;

	/** Initialize. **/
	cxInitialize();

	/** history file **/
	home = getenv("HOME");
	if (home && strlen(home) < sizeof(histname) - strlen("/.cxhistory") - 1)
	    {
	    snprintf(histname, sizeof(histname), "%s/.cxhistory", home);
	    histfile = fdOpen(histname, O_RDWR | O_CREAT, 0600);
	    if (histfile)
		{
		input_lx = mlxOpenSession(histfile, MLX_F_LINEONLY | MLX_F_EOF);
		while((t = mlxNextToken(input_lx)) > 0)
		    {
		    if (t == MLX_TOK_EOF || t == MLX_TOK_ERROR) break;
		    alloc = 1;
		    ptr = mlxStringVal(input_lx, &alloc);
		    if (ptr && *ptr) 
			{
			if (strchr(ptr, '\n'))
			    *(strchr(ptr, '\n')) = '\0';
			add_history (ptr);
			nmSysFree(ptr);
			}
		    }
		mlxCloseSession(input_lx);
		}
	    }

	/** enable tab completion **/
	rl_bind_key ('\t', handle_tab);

	/** Authenticate **/
	if (!TESTOBJ.UserName[0]) 
	    user = readline("Username: ");
	else
	    user = TESTOBJ.UserName;
	if (!TESTOBJ.Password[0])
	    pwd = getpass("Password: ");
	else
	    pwd = TESTOBJ.Password;

	if (mssAuthenticate(user,pwd) < 0)
	    puts("Warning: auth failed, running outside session context.");
	TESTOBJ.Output = fdOpen(TESTOBJ.OutputFilename, O_RDWR | O_CREAT | O_TRUNC, 0600);
	if (!TESTOBJ.Output)
	    {
	    strcpy(TESTOBJ.OutputFilename, "/dev/tty");
	    TESTOBJ.Output = fdOpen(TESTOBJ.OutputFilename, O_RDWR | O_CREAT | O_TRUNC, 0600);
	    }

	/** Open a session **/
	s = objOpenSession("/");

	/** -C cmd provided on command line? **/
	if (TESTOBJ.Command[0])
	    {
	    rval = testobj_do_cmd(s, TESTOBJ.Command, 1);
	    }

	/** Command file provided? **/
	if (TESTOBJ.CmdFile[0])
	    {
	    cmdfile = fdOpen(TESTOBJ.CmdFile, O_RDONLY, 0600);
	    if (!cmdfile)
		{
		perror(TESTOBJ.CmdFile);
		}
	    else
		{
		input_lx = mlxOpenSession(cmdfile, MLX_F_LINEONLY | MLX_F_EOF);
		while((t = mlxNextToken(input_lx)) > 0)
		    {
		    if (t == MLX_TOK_EOF || t == MLX_TOK_ERROR) break;
		    alloc = 1;
		    ptr = mlxStringVal(input_lx, &alloc);
		    if (ptr) 
			{
			rval = testobj_do_cmd(s, ptr, 1);
			nmSysFree(ptr);
			if (rval == 1) break;
			}
		    }
		mlxCloseSession(input_lx);
		fdClose(cmdfile, 0);
		}
	    }

	/** Loop, putting prompt and getting commands **/
	if (!TESTOBJ.Command[0] && !TESTOBJ.CmdFile[0]) 
	  while(1)
	    {
	    sprintf(prompt,"OSML:%.1000s> ",objGetWD(s));

	    /** If the buffer has already been allocated, return the memory to the free pool. **/
	    if (inbuf)
		{
		free (inbuf);
		inbuf = (char *)NULL;
		}   

	    /** Get a line from the user using readline library call. **/
	    inbuf = readline (prompt);   

	    /** If the line has any text in it, save it on the readline history. **/
	    if (inbuf && *inbuf)
		{
		add_history (inbuf);
		if (histfile)
		    fdPrintf(histfile, "%s\n", inbuf);
		}

	    /** If inbuf is null (end of file, etc.), exit **/
	    if (!inbuf)
	        {
		printf("quit\n");
		thExit();
		}

	    rval = testobj_do_cmd(s, inbuf, 0);
	    if (rval == 1) break;
	    }

	objCloseSession(s);

    thExit();
    }


void
show_usage()
    {
    printf("Usage:  test_obj [-c <config-file>] [-f <command-file>] [-C <command>]\n"
	   "                 [-u <user>] [-p <password>] [-q] [-o <output file>] \n"
	   "                 [-i <wait-seconds>] [-h]\n"
	   "        -h         Show this message\n"
	   "        -q         Initialize quietly\n"
	   "        -c file    Specify configuration file\n"
	   "        -C command Run a single command\n"
	   "        -f file    Run commands from a file\n"
	   "        -u user    Login as user\n"
	   "        -p pass    Specify password\n"
	   "        -o file    Send output to specified file\n"
	   "        -i secs    Terminate test_obj after secs with SIGALRM\n"
	   "\n");
    return;
    }


int 
main(int argc, char* argv[])
    {
    int ch;

	/** Default global values **/
	strcpy(CxGlobals.ConfigFileName, CENTRALLIX_CONFIG);
	CxGlobals.QuietInit = 0;
	CxGlobals.ParsedConfig = NULL;
	CxGlobals.ModuleList = NULL;
	memset(&TESTOBJ,0,sizeof(TESTOBJ));
	strcpy(TESTOBJ.OutputFilename, "/dev/tty");
	TESTOBJ.WaitSecs = 0;
    
	/** Check for config file options on the command line **/
	while ((ch=getopt(argc,argv,"ho:c:qu:p:f:C:i:")) > 0)
	    {
	    switch (ch)
	        {
		case 'i':	TESTOBJ.WaitSecs = strtoui(optarg, NULL, 10);
				break;
		case 'C':	memccpy(TESTOBJ.Command, optarg, 0, 1023);
				TESTOBJ.Command[1023] = 0;
				break;
		case 'f':	memccpy(TESTOBJ.CmdFile, optarg, 0, 255);
				TESTOBJ.CmdFile[255] = 0;
				break;
		case 'u':	memccpy(TESTOBJ.UserName, optarg, 0, 31);
				TESTOBJ.UserName[31] = 0;
				break;
		case 'p':	memccpy(TESTOBJ.Password, optarg, 0, 31);
				TESTOBJ.Password[31] = 0;
				break;
		case 'c':	memccpy(CxGlobals.ConfigFileName, optarg, 0, 255);
				CxGlobals.ConfigFileName[255] = '\0';
				break;

		case 'q':	CxGlobals.QuietInit = 1;
				break;

		case 'o':	strtcpy(TESTOBJ.OutputFilename, optarg, sizeof(TESTOBJ.OutputFilename));
				break;

		case 'h':	show_usage();
				exit(0);

		case '?':
		default:	show_usage();
				exit(1);
		}
	    }

	/** No sense in command time-out if we are operating interactively. **/
	if (TESTOBJ.WaitSecs && !(TESTOBJ.Command[0] || TESTOBJ.CmdFile[0]))
	    {
	    printf("Warning: -i specified but no command or command file given.  Forcing -i to 0.\n");
	    TESTOBJ.WaitSecs = 0;
	    }

	/** Set SIGALRM if -i is specified **/
	if (TESTOBJ.WaitSecs)
	    {
	    alarm(TESTOBJ.WaitSecs);
	    }

    mtInitialize(0, start);
    return 0;
    }
