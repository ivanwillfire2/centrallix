#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/magic.h"

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
/* Module: 	obj.h, obj_*.c    					*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	October 26, 1998					*/
/* Description:	Implements the ObjectSystem part of the Centrallix.    */
/*		The various obj_*.c files implement the various parts of*/
/*		the ObjectSystem interface.				*/
/*		--> obj_content.c: contains implementation of the 	*/
/*		content-access (read/write) object methods.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: obj_content.c,v 1.6 2007/03/06 16:16:55 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/objectsystem/obj_content.c,v $

    $Log: obj_content.c,v $
    Revision 1.6  2007/03/06 16:16:55  gbeeley
    - (security) Implementing recursion depth / stack usage checks in
      certain critical areas.
    - (feature) Adding ExecMethod capability to sysinfo driver.

    Revision 1.5  2005/02/26 06:42:39  gbeeley
    - Massive change: centrallix-lib include files moved.  Affected nearly
      every source file in the tree.
    - Moved all config files (except centrallix.conf) to a subdir in /etc.
    - Moved centrallix modules to a subdir in /usr/lib.

    Revision 1.4  2003/07/10 19:21:23  gbeeley
    Making sure offset and cnt/maxcnt values are not negative.  Just a
    safety check.

    Revision 1.3  2002/04/25 17:59:59  gbeeley
    Added better magic number support in the OSML API.  ObjQuery and
    ObjSession structures are now protected with magic numbers, and
    support for magic numbers in Object structures has been improved
    a bit.

    Revision 1.2  2001/09/27 19:26:23  gbeeley
    Minor change to OSML upper and lower APIs: objRead and objWrite now follow
    the same syntax as fdRead and fdWrite, that is the 'offset' argument is
    4th, and the 'flags' argument is 5th.  Before, they were reversed.

    Revision 1.1.1.1  2001/08/13 18:00:57  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:30:59  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** objRead - read content from an object at a particular optional seek
 *** offset.  Very similar to MTask's fdRead().
 ***/
int 
objRead(pObject this, char* buffer, int maxcnt, int offset, int flags)
    {
    ASSERTMAGIC(this, MGK_OBJECT);
    /** Check recursion **/
    if (thExcessiveRecursion())
	{
	mssError(1,"OSML","Could not objRead(): resource exhaustion occurred");
	return -1;
	}

    if (maxcnt < 0 || offset < 0)
	{
	mssError(1,"OSML","Parameter error calling objRead()");
	return -1;
	}
    return this->Driver->Read(this->Data, buffer, maxcnt, offset, flags, &(this->Session->Trx));
    }


/*** objWrite - write content to an object at a particular optional seek
 *** offset.  Also very similar to MTask's fdWrite().
 ***/
int 
objWrite(pObject this, char* buffer, int cnt, int offset, int flags)
    {
    int result;
    ASSERTMAGIC(this, MGK_OBJECT);
    /** Check recursion **/
    if (thExcessiveRecursion())
	{
	mssError(1,"OSML","Could not objWrite(): resource exhaustion occurred");
	return -1;
	}

    if (cnt < 0 || offset < 0)
	{
	mssError(1,"OSML","Parameter error calling objWrite()");
	return -1;
	}
    result =  this->Driver->Write(this->Data, buffer, cnt, offset, flags, &(this->Session->Trx));
    if(result == 0)
        {
        /** Notify any observers about the change. */
        obj_internal_ObserverCheckObservers(objGetPathname(this), OBJ_OBSERVER_EVENT_MODIFY);
        }
    return result;
    }

