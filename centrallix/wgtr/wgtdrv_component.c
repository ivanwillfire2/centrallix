#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "obj.h"
#include "cxlib/mtask.h"
#include "cxlib/mtsession.h"
#include "wgtr.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1998-2006 LightSys Technology Services, Inc.		*/
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
/* Module: 	wgtr/wgtdrv_component.c					*/
/* Author:	Greg Beeley (GRB)		 			*/
/* Creation:	October 18, 2006					*/
/* Description:								*/
/************************************************************************/

/**CVSDATA***************************************************************
 

 **END-CVSDATA***********************************************************/


/*** wgtcmpVerify - allows the driver to check elsewhere in the tree
 *** to make sure that the conditions it requires for proper functioning
 *** are present - checking for other widgets that might be necessary,
 *** checking interface versions on widgets to be interacted with, etc.
 ***/
int
wgtcmpVerify(pWgtrVerifySession s)
    {
    return 0;
    }


/*** wgtcmpNew - after a node has been filled out with initial values,
 *** the driver uses this function to take care of any other initialization
 *** that needs to be done on a per-node basis. By far the most important
 *** is declaring interfaces.
 ***/
int
wgtcmpNew(pWgtrNode node)
    {

	node->Flags |= WGTR_F_NONVISUAL;
	node->Flags |= WGTR_F_CONTAINER;

    return 0;
    }


int
wgtcmpInitialize()
    {
    char*   name="Component Instance Driver";

	wgtrRegisterDriver(name, wgtcmpVerify, wgtcmpNew);
	wgtrAddType(name, "component");

	return 0;
    }
