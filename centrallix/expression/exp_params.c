#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "obj.h"
#include "mtask.h"
#include "xarray.h"
#include "xhash.h"
#include "mtlexer.h"
#include "expression.h"
#include "mtsession.h"

/************************************************************************/
/* Centrallix Application Server System 				*/
/* Centrallix Core       						*/
/* 									*/
/* Copyright (C) 1999-2001 LightSys Technology Services, Inc.		*/
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
/* Module: 	expression.h, exp_params.c 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 5, 1999					*/
/* Description:	Provides expression tree construction and evaluation	*/
/*		routines.  Formerly a part of obj_query.c.		*/
/*		--> exp_params.c: handles the object parameter list	*/
/*		mechanism.						*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: exp_params.c,v 1.1 2001/08/13 18:00:48 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/expression/exp_params.c,v $

    $Log: exp_params.c,v $
    Revision 1.1  2001/08/13 18:00:48  gbeeley
    Initial revision

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:53  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** expCreateParamList - sets up and initializes the object parameter list
 *** that can be filled out and used later in the expression compilation and
 *** evaluation stages.
 ***/
pParamObjects
expCreateParamList()
    {
    pParamObjects objlist;

    	/** Allocate the structure **/
	objlist = (pParamObjects)nmMalloc(sizeof(ParamObjects));
	if (!objlist) return NULL;
	memset(objlist, 0, sizeof(ParamObjects));
	objlist->CurrentID = -1;
	objlist->ParentID = -1;
	objlist->MainFlags = 0;
	objlist->SeqID = 1;
	objlist->PSeqID = (EXP.PSeqID++);

    return objlist;
    }


/*** expFreeParamList - frees memory used by, and deinitializes, an object
 *** parameter list structure.
 ***/
int 
expFreeParamList(pParamObjects this)
    {
    int i;

    	/** Free the names, if allocated **/
	for(i=0;i<EXPR_MAX_PARAMS;i++) if (this->Flags[i] & EXPR_O_ALLOCNAME)
	    {
	    nmSysFree(this->Names[i]);
	    this->Names[i] = NULL;
	    }

	/** Free the structure. **/
	nmFree(this, sizeof(ParamObjects));

    return 0;
    }


/*** expAddParamToList - adds a new parameter slot to the parameter listing,
 *** possibly with or without a direct object reference (obj can be NULL).
 *** The name, slot number, and flags (EXPR_O_xxx) are used when the 
 *** compiler builds the expression tree.
 ***/
int 
expAddParamToList(pParamObjects this, char* name, pObject obj, int flags)
    {
    int i;

    	/** Too many? **/
	if (this->nObjects >= EXPR_MAX_PARAMS) 
	    {
	    mssError(1,"EXP","Parameter Object list overflow -- too many objects in expression");
	    return -1;
	    }

	/** Ok, add parameter. **/
	for(i=0;i<EXPR_MAX_PARAMS;i++) if (this->Names[i] == NULL)
	    {
	    /** Setup the entry for this parameter. **/
	    this->SeqID++;
	    this->SeqIDs[i] = this->SeqID;
	    this->Objects[i] = obj;
	    this->Flags[i] = flags | EXPR_O_CHANGED;
	    this->GetTypeFn[i] = objGetAttrType;
	    this->GetAttrFn[i] = objGetAttrValue;
	    this->SetAttrFn[i] = objSetAttrValue;
	    this->nObjects++;
	    if (flags & EXPR_O_ALLOCNAME)
	        {
		this->Names[i] = nmSysStrdup(name);
		}
	    else
	        {
		this->Names[i] = name;
		}

	    /** Check for parent id and current id **/
	    if (flags & EXPR_O_PARENT) this->ParentID = i;
	    if (flags & EXPR_O_CURRENT) this->CurrentID = i;
	    if (this->nObjects == 1) this->CurrentID = i;
	    break;
	    }

    return 0;
    }


/*** expRemoveParamFromList - removes an existing parameter from the param
 *** objects list, by name.
 ***/
int
expRemoveParamFromList(pParamObjects this, char* name)
    {
    int i;

    	/** Find the thing and delete it **/
	for(i=0;i<EXPR_MAX_PARAMS;i++) if (this->Names[i] != NULL && !strcmp(name, this->Names[i]))
	    {
	    if (this->Flags[i] & EXPR_O_ALLOCNAME) nmSysFree(this->Names[i]);
	    this->Flags[i] = 0;
	    this->Objects[i] = NULL;
	    this->nObjects--;
	    this->Names[i] = NULL;
	    if (this->CurrentID == i)
	        {
		if (i==0) this->CurrentID = -1;
		else this->CurrentID--;
		}
	    if (this->ParentID == i)
	        {
		if (i==0) this->ParentID = -1;
		else this->ParentID--;
		}
	    return 0;
	    }

    return -1;
    }


/*** expModifyParam - sets a new object for a given parameter slot (by 
 *** name).  This is used to update the object list so that the next time
 *** the evaluator runs it, it will reference attributes in a new or
 *** different object.
 ***/
int 
expModifyParam(pParamObjects this, char* name, pObject replace_obj)
    {
    int slot_id;
    int i;

    	/** Pick the slot id.  If name is NULL, use current. **/
	if (name == NULL)
	    {
	    slot_id = this->CurrentID;
	    }
	else
	    {
	    for(i=0;i<EXPR_MAX_PARAMS;i++) 
	      if (this->Names[i] && !strcmp(this->Names[i],name))
	        {
	        slot_id = i;
		break;
		}
	    }

	/** Replace the object. **/
	this->Objects[slot_id] = replace_obj;
	this->Flags[slot_id] |= EXPR_O_CHANGED;
	this->SeqID++;
	this->SeqIDs[slot_id] = this->SeqID;

    return slot_id;
    }


#if 00
/*** expSyncModified - synchronizes the status indicators in a tree
 *** to a possibly modified object list structure.  Sets the flags
 *** field to EXPR_F_NEW if the object has a status of EXPR_O_CHANGED,
 *** and propogates the stale flags up the tree to parent items.  This
 *** way, a minimum of the tree is recalculated when only one of several
 *** objects are modified.
 ***/
int
expSyncModify(pExpression tree, pParamObjects objlist)
    {
    int s=0,i;

    	/** Check sub objects, return stale if one does. **/
	for(i=0;i<tree->Children.nItems;i++)
	    {
	    if (expSyncModify((pExpression)(tree->Children.Items[i]),objlist)) s = 1;
	    }

    	/** Mark this one stale? **/
	if (s || (tree->NodeType == EXPR_N_PROPERTY && (objlist->Flags[expObjID(tree,objlist)] & EXPR_O_CHANGED)))
	    {
	    tree->Flags |= EXPR_F_NEW;
	    s = 1;
	    }

	/** Update objlist (clear CHANGED flags?) **/
	if (tree->Parent == NULL)
	    for(i=0;i<objlist->nObjects;i++) objlist->Flags[i] &= ~EXPR_O_CHANGED;

    return s;
    }
#endif


/*** expReplaceID - walks through the expression tree, changing all object
 *** id's that match oldid to newid.
 ***/
int
expReplaceID(pExpression this, int oldid, int newid)
    {
    int i;

    	/** Check this id. **/
	if (this->ObjID == oldid) this->ObjID = newid;

	/** Check child items. **/
	for(i=0;i<this->Children.nItems;i++)
	    {
	    expReplaceID((pExpression)(this->Children.Items[i]), oldid, newid);
	    }

    return 0;
    }


/*** expReplaceVariableID - walks through the expression tree, changing all object
 *** id's that are not frozen to newid.
 ***/
int
expReplaceVariableID(pExpression this, int newid)
    {
    int i;

    	/** Check this id. **/
	if (this->Flags & EXPR_F_FREEZEEVAL) return 0;
	if (this->ObjID >= 0 && 
	    (this->NodeType == EXPR_N_PROPERTY || this->NodeType == EXPR_N_OBJECT)) 
	         this->ObjID = newid;

	/** Check child items. **/
	for(i=0;i<this->Children.nItems;i++)
	    {
	    expReplaceVariableID((pExpression)(this->Children.Items[i]), newid);
	    }

    return 0;
    }


/*** expFreezeEval - evaluates all id's in a tree with the exception of 
 *** freeze_id and sets those object's values as such.  It then marks them
 *** as EXPR_F_FREEZEEVAL so that the next eval of the tree views such nodes
 *** as constants and does not evaluate them.
 ***/
int
expFreezeEval(pExpression this, pParamObjects objlist, int freeze_id)
    {
    int i;

    	/** Is this a PROPERTY object and does not match freeze_id?? **/
	if ((this->NodeType == EXPR_N_PROPERTY || this->NodeType == EXPR_N_OBJECT) && this->ObjID != -1 && this->ObjID != freeze_id)
	    {
	    this->Flags &= ~EXPR_F_FREEZEEVAL;
	    expEvalTree(this,objlist);
	    this->Flags |= EXPR_F_FREEZEEVAL;
	    return 0;
	    }

	/** Otherwise, check child items. **/
	for(i=0;i<this->Children.nItems;i++)
	    {
	    expFreezeEval((pExpression)(this->Children.Items[i]), objlist, freeze_id);
	    }

    return 0;
    }


/*** expResetAggregates - resets the aggregate min/max/cnt/sum accumulators for
 *** either all objects or a given object id.  Set object id to -1 to reset for
 *** all objects.
 ***/
int
expResetAggregates(pExpression this, int reset_id)
    {
    /*this->Flags |= EXPR_F_DORESET;*/
    exp_internal_ResetAggregates(this,reset_id);
    return 0;
    }

int
exp_internal_ResetAggregates(pExpression this, int reset_id)
    {
    int i;
    int found_agg=0,rval;

    	/** Check reset on this. **/
	if (this->NodeType == EXPR_N_FUNCTION && (this->Flags & EXPR_F_AGGREGATEFN) &&
	    (this->ObjID == reset_id || reset_id == -1))
	    {
	    this->AggCount = 0;
	    this->AggValue = 0;
	    /*this->Integer = 0;*/
	    if (this->AggExp)
	        {
	        this->AggExp->Integer = 0;
	        this->AggExp->Types.Double = 0;
	        this->AggExp->Types.Money.WholePart = 0;
	        this->AggExp->Types.Money.FractionPart = 0;
	        this->Flags |= EXPR_F_AGGLOCKED;
		}
	    if (!strcmp(this->Name,"count")) 
	        {
		this->Flags &= ~EXPR_F_NULL;
		this->DataType = DATA_T_INTEGER;
		this->Integer = 0;
		}
	    else
	        {
		this->Flags |= EXPR_F_NULL;
		}
	    found_agg = 1;
	    }

	/** Check all sub-nodes **/
	for(i=0;i<this->Children.nItems;i++)
	    {
	    rval = exp_internal_ResetAggregates((pExpression)(this->Children.Items[i]), reset_id);
	    if (rval) found_agg = 1;
	    }

    return found_agg;
    }


/*** expUnlockAggregates - "unlock" the aggregate nodes so that they can be evaluated
 *** in the next expEvalTree operation.
 ***/
int
expUnlockAggregates(pExpression this)
    {
    int i;

    	/** Unlock on this node. **/
	this->Flags &= ~EXPR_F_AGGLOCKED;

	/** Unlock child nodes **/
	for(i=0;i<this->Children.nItems;i++) 
	    expUnlockAggregates((pExpression)(this->Children.Items[i]));

    return 0;
    }


/*** expSetParamFunctions - set the functions that will be used to get/set
 *** paramobjects attribute values and types.  Used for "custom" objects and
 *** such.
 ***/
int
expSetParamFunctions(pParamObjects this, char* name, int (*type_fn)(), int (*get_fn)(), int (*set_fn)())
    {
    int slot_id = -1;
    int i;

    	/** Pick the slot id.  If name is NULL, use current. **/
	if (name == NULL)
	    {
	    slot_id = this->CurrentID;
	    }
	else
	    {
	    for(i=0;i<EXPR_MAX_PARAMS;i++) 
	      if (this->Names[i] && !strcmp(this->Names[i],name))
	        {
	        slot_id = i;
		break;
		}
	    }
	if (slot_id == -1) return -1;

	/** Set the functions. **/
	this->GetTypeFn[slot_id] = type_fn;
	this->GetAttrFn[slot_id] = get_fn;
	this->SetAttrFn[slot_id] = set_fn;

    return 0;
    }


/*** expRemapID - remaps an ID so that an object ID referenced in the expression
 *** tree actually references a _different_ object ID in the object list.  Also
 *** used to mark an object within an expression CONSTANT so it does NOT re-
 *** evaluate.
 ***/
int
expRemapID(pExpression tree, int exp_obj_id, int objlist_obj_id)
    {
    int i;

	/** No control block? **/
    	if (!tree->Control)
	    {
	    tree->Control = (pExpControl)nmMalloc(sizeof(ExpControl));
	    for(i=0;i<EXPR_MAX_PARAMS;i++) tree->Control->ObjMap[i] = i;
	    }

	tree->Control->ObjMap[exp_obj_id] = objlist_obj_id;

    return 0;
    }


/*** expClearRemapping - Removes all remapping that has been done on an expression's
 *** parameter objects.
 ***/
int
expClearRemapping(pExpression tree)
    {
    if (tree->Control)
        {
	if (!tree->Parent) nmFree(tree->Control, sizeof(ExpControl));
        tree->Control = NULL;
	}
    return 0;
    }

