#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "config.h"
#include "centrallix.h"
#include "cxlib/xstring.h"
#include "cxlib/xarray.h"
#include "cxlib/xhash.h"
#include "cxlib/mtlexer.h"
#include "cxss/cxss.h"

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
/* Module:	cxss (Centrallix Security Subsystem)                    */
/* Author:	Greg Beeley (GRB)                                       */
/* Date:	February 22, 2007                                       */
/*									*/
/* Description:	CXSS provides the core security support for the		*/
/*		Centrallix application platform.			*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: cxss_utility.c,v 1.1 2007/02/22 23:25:14 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/cxss/cxss_utility.c,v $

    $Log: cxss_utility.c,v $
    Revision 1.1  2007/02/22 23:25:14  gbeeley
    - (feature) adding initial framework for CXSS, the security subsystem.
    - (feature) CXSS entropy pool and key generation, basic framework.
    - (feature) adding xmlhttprequest capability
    - (change) CXSS requires OpenSSL, adding that check to the build
    - (security) Adding application key to thwart request spoofing attacks.
      Once the AML is active, application keying will be more important and
      will be handled there instead of in net_http.

    Revision 1.1.1.1  2001/08/13 18:00:46  gbeeley
    Centrallix Core initial import

    Revision 1.1.1.1  2001/08/07 02:30:51  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** cxssGenerateKey - create a pseudorandom key of the specified size
 *** using cryptographically secure methods.
 ***/
int
cxssGenerateKey(unsigned char* key, size_t n_bytes)
    {
    int rval;

	/** Get bytes from the entropy pool **/
	rval = cxss_internal_GetBytes(key, n_bytes);

    return rval;
    }



/*** cxssShred - erase the given data so that it is no longer readable
 *** even in raw memory.  At this point, basically the same as memset().
 *** But using this function signifies the intent and makes the code
 *** more readable.
 ***/
int
cxssShred(unsigned char* data, size_t n_bytes)
    {
    memset(data, '\0', n_bytes);
    return 0;
    }



/*** cxssAddEntropy - stir some entropy into the entropy pool, and
 *** increase the estimate of available entropy by the given amount.
 ***/
int
cxssAddEntropy(unsigned char* data, size_t n_bytes, int entropy_bits_estimate)
    {
    int rval;

	/** Add it to the entropy pool **/
	rval = cxss_internal_AddToPool(data, n_bytes, entropy_bits_estimate);
	
    return rval;
    }

