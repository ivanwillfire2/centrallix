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
/* Module: 	expression.h, exp_compiler.c 				*/
/* Author:	Greg Beeley (GRB)					*/
/* Creation:	February 5, 1999					*/
/* Description:	Provides expression tree construction and evaluation	*/
/*		routines.  Formerly a part of obj_query.c.		*/
/*									*/
/*		--> exp_compiler.c: compiles expressions from an MLX	*/
/*		    token stream into an expression tree.		*/
/************************************************************************/

/**CVSDATA***************************************************************

    $Id: exp_compiler.c,v 1.1 2001/08/13 18:00:47 gbeeley Exp $
    $Source: /srv/bld/centrallix-repo/centrallix/expression/exp_compiler.c,v $

    $Log: exp_compiler.c,v $
    Revision 1.1  2001/08/13 18:00:47  gbeeley
    Initial revision

    Revision 1.2  2001/08/07 19:31:52  gbeeley
    Turned on warnings, did some code cleanup...

    Revision 1.1.1.1  2001/08/07 02:30:51  gbeeley
    Centrallix Core Initial Import


 **END-CVSDATA***********************************************************/


/*** exp_internal_CompileExpression_r - builds an expression tree from a SQL
 *** where-clause expression.  This function is the recursive element for
 *** parenthesized groupings.
 ***/
pExpression
exp_internal_CompileExpression_r(pLxSession lxs, int level, pParamObjects objlist, int cmpflags)
    {
    pExpression etmp,etmp2,eptr,expr = NULL;
    int t,err=0,i;
    int was_unary = 0;
    int was_prefix_unary = 0;
    char* sptr;

	/** Loop through the tokens **/
	while (!err)
	    {
	    /** Get an immediate/identifier token **/
	    t = mlxNextToken(lxs);
	    if (t == MLX_TOK_ERROR) break;

	    /** If last operator was postfix unary (i.e., "x IS NULL") **/
	    if (was_unary) goto SKIP_OPERAND;

	    /** If open paren, recursively handle this. **/
	    if (t == MLX_TOK_OPENPAREN)
		{
		etmp = exp_internal_CompileExpression_r(lxs,level+1,objlist, cmpflags | EXPR_CMP_WATCHLIST);
		if (!etmp)
		    {
                    if (expr) expFreeExpression(expr);
		    expr = NULL;
		    err=1;
		    break;
		    }
		}
	    else if (t == MLX_TOK_CLOSEPAREN)
	        {
		if (expr == NULL && (cmpflags & EXPR_CMP_WATCHLIST))
		    {
		    /** Empty list. () **/
		    expr = expAllocExpression();
		    expr->NodeType = EXPR_N_LIST;
		    break;
		    }
		else
		    {
		    /** Error. **/
		    if (expr) expFreeExpression(expr);
		    expr = NULL;
		    mssError(1,"EXP","Unexpected close-paren ')' in expression");
		    err = 1;
		    break;
		    }
		}
	    else
		{
	        etmp = expAllocExpression();
	        switch(t)
                    {
		    /*case MLX_TOK_MINUS:
			break;*/

                    case MLX_TOK_INTEGER:
                        etmp->NodeType = EXPR_N_INTEGER;
                        etmp->Integer = mlxIntVal(lxs);
			etmp->DataType = DATA_T_INTEGER;
                        break;

		    case MLX_TOK_DOLLAR:
		        t = mlxNextToken(lxs);
			if (t != MLX_TOK_INTEGER && t != MLX_TOK_DOUBLE)
			    {
			    expFreeExpression(etmp);
			    etmp = NULL;
			    if (expr) expFreeExpression(expr);
			    expr = NULL;
			    mssError(1,"EXP","Expected numeric value after '$', got '%s'", mlxStringVal(lxs, NULL));
			    err = 1;
			    break;
			    }
			etmp->NodeType = EXPR_N_MONEY;
			etmp->DataType = DATA_T_MONEY;
			if (t == MLX_TOK_INTEGER)
			    {
			    i = mlxIntVal(lxs);
			    etmp->Types.Money.WholePart = i;
			    etmp->Types.Money.FractionPart = 0;
			    }
			else
			    {
			    sptr = mlxStringVal(lxs,NULL);
			    objDataToMoney(DATA_T_STRING, sptr, &(etmp->Types.Money));
			    }
			break;

		    case MLX_TOK_DOUBLE:
		        etmp->NodeType = EXPR_N_DOUBLE;
			etmp->Types.Double = mlxDoubleVal(lxs);
			etmp->DataType = DATA_T_DOUBLE;
			break;
    		    
		    case MLX_TOK_KEYWORD:
                    case MLX_TOK_STRING:
                        etmp->NodeType = EXPR_N_STRING;
                        etmp->Alloc = 1;
                        etmp->String = mlxStringVal(lxs,&(etmp->Alloc));
			etmp->DataType = DATA_T_STRING;

			/** Check for a function() **/
			if (t==MLX_TOK_KEYWORD)
			    {
			    t = mlxNextToken(lxs);
			    if (!strcasecmp(etmp->String, "not"))
			        {
				mlxHoldToken(lxs);
				was_prefix_unary = 1;
				etmp->NameAlloc = etmp->Alloc;
				etmp->Name = etmp->String;
				etmp->Alloc = 0;
				etmp->String = NULL;
				etmp->NodeType = EXPR_N_NOT;
				}
			    else if (t == MLX_TOK_OPENPAREN)
			        {
				etmp->NameAlloc = etmp->Alloc;
				etmp->Name = etmp->String;
				etmp->Alloc = 0;
				etmp->String = NULL;
				etmp->NodeType = EXPR_N_FUNCTION;
				if (!strcasecmp(etmp->Name,"count") || !strcasecmp(etmp->Name,"avg") ||
				    !strcasecmp(etmp->Name,"max") || !strcasecmp(etmp->Name,"min") ||
				    !strcasecmp(etmp->Name,"sum"))
				    {
				    etmp->Flags |= (EXPR_F_AGGREGATEFN | EXPR_F_AGGLOCKED);
				    /*etmp->AggExp = expAllocExpression();*/
				    }

				/** Ok, parse the elements in the param list until we get a close-paren **/
				while(lxs->TokType != MLX_TOK_CLOSEPAREN)
				    {
				    /** We preview the next token to see if the param list is empty. **/
				    mlxNextToken(lxs);
				    if (lxs->TokType == MLX_TOK_CLOSEPAREN) break;
				    mlxHoldToken(lxs);

				    /** Compile the param and add it as a child item **/
				    etmp2 = exp_internal_CompileExpression_r(lxs,level+1,objlist,cmpflags & ~EXPR_CMP_WATCHLIST);
				    if (!etmp2)
				        {
					if (expr) expFreeExpression(expr);
					if (etmp) expFreeExpression(etmp);
					expr = NULL; 
					err=1;
					break;
					}
				    etmp2->Parent = etmp;
				    xaAddItem(&etmp->Children, (void*)etmp2);
				    
				    /** If comma, un-hold the token **/
				    if (lxs->TokType == MLX_TOK_COMMA) mlxNextToken(lxs);
				    }
				}
			    else
			        {
				if (!strcmp(etmp->String,"NULL")) etmp->Flags |= (EXPR_F_NULL | EXPR_F_PERMNULL);
				mlxHoldToken(lxs);
				}
			    }
                        break;


		    case MLX_TOK_FILENAME:
		        /** Referencing an attribute of a particular object? **/
			etmp->NodeType = EXPR_N_OBJECT;
			etmp->NameAlloc = 1;
			etmp->Name = mlxStringVal(lxs,&(etmp->NameAlloc));
			etmp->ObjID = -1;
			etmp->ObjCoverageMask = 0;
			if (mlxNextToken(lxs) != MLX_TOK_COLON || mlxNextToken(lxs) != MLX_TOK_KEYWORD)
			    {
                            expFreeExpression(etmp);
                            if (expr) expFreeExpression(expr);
                            expr = NULL;
			    mssError(1,"EXP","Expected :keyword reference after filename, got '%s'", mlxStringVal(lxs, NULL));
                            err=1;
                            break;
			    }
			etmp2 = expAllocExpression();
			etmp2->NodeType = EXPR_N_PROPERTY;
			etmp2->NameAlloc = 1;
			etmp2->Name = mlxStringVal(lxs,&(etmp2->NameAlloc));
			etmp2->Parent = etmp;
			etmp2->ObjID = -1;
			etmp2->ObjCoverageMask = 0;
			xaAddItem(&etmp->Children, (void*)etmp2);
			break;
    
                    case MLX_TOK_COLON:
		        /** Is this a cur reference (:) or parent reference (::)? **/
			t = mlxNextToken(lxs);
			if (t == MLX_TOK_COLON)
			    {
			    /*etmp->ObjID = objlist->ParentID;*/
			    etmp->ObjID = EXPR_OBJID_PARENT;
			    t = mlxNextToken(lxs);
			    }
			else
			    {
			    /*etmp->ObjID = objlist->CurrentID;*/
			    etmp->ObjID = EXPR_OBJID_CURRENT;
			    }

			/** Ok, verify next element is the name of the attribute **/
			if (t != MLX_TOK_KEYWORD && t != MLX_TOK_FILENAME)
			    {
                            expFreeExpression(etmp);
                            if (expr) expFreeExpression(expr);
                            expr = NULL;
                            err=1;
			    mssError(1,"EXP","Expected source or explicit filename after :, got '%s'", mlxStringVal(lxs, NULL));
                            break;
			    }

			/** Setup the expression tree nodes. **/
			etmp->NodeType = EXPR_N_PROPERTY;
			etmp->NameAlloc = 1;
			etmp->Name = mlxStringVal(lxs,&(etmp->NameAlloc));

			/** Is this an explicit named reference (:objname:attrname)? **/
			t = mlxNextToken(lxs);
			if (t == MLX_TOK_COLON)
			    {
			    t = mlxNextToken(lxs);
			    for(i=0;i<objlist->nObjects;i++) 
			      if (!strcmp(etmp->Name,objlist->Names[i]))
			        {
				if (etmp->NameAlloc)
				    {
				    nmSysFree(etmp->Name);
				    etmp->NameAlloc = 0;
				    }
				etmp->Name = NULL;
				etmp->ObjID = i;
				break;
				}
			    if (etmp->Name != NULL) /* didn't find?? */
			        {
				mssError(1,"EXP","Could not locate :objectname '%s' in object list", etmp->Name);
				expFreeExpression(etmp);
				if (expr) expFreeExpression(expr);
				expr = NULL;
				err=1;
				break;
				}
			    etmp->NameAlloc = 1;
			    etmp->Name = mlxStringVal(lxs,&(etmp->NameAlloc));
			    }
			else
			    {
			    mlxHoldToken(lxs);
			    }

			i=0;
			i = expObjID(etmp,objlist);
			if (i>=0)
			    {
			    objlist->Flags[i] |= EXPR_O_REFERENCED;
			    etmp->ObjCoverageMask |= (1<<(i));
			    }
			etmp->Parent = expAllocExpression();
			xaAddItem(&(etmp->Parent->Children),(void*)etmp);
			etmp->Parent->ObjID = etmp->ObjID;
			if (i>=0) etmp->Parent->ObjCoverageMask |= (1<<(i));
			etmp = etmp->Parent;
			etmp->NodeType = EXPR_N_OBJECT;
                        break;
    
                    default:
                        expFreeExpression(etmp);
                        if (expr) expFreeExpression(expr);
                        expr = NULL;
                        err=1;
			mssError(1,"EXP","Unexpected '%s' in expression", mlxStringVal(lxs, NULL));
                        break;
                    }
	        if (err) break;
	        etmp->Flags &= ~EXPR_F_OPERATOR;
		}

	    /** Now figure out what to do with the non-op token **/
	    if (expr == NULL)
		{
		expr = etmp;
		eptr = expr;
		}
	    else
		{
		xaAddItem(&(eptr->Children),(void*)etmp);
		etmp->Parent = eptr;
		eptr = etmp;
		}

	    /** Was this thing a prefix-unary operator? **/
	    if (was_prefix_unary) 
	        {
		was_prefix_unary = 0;
		continue;
		}

	    /** Now get the operator token or end-of-stream **/
	    t = mlxNextToken(lxs);

	SKIP_OPERAND:
	    was_unary = 0;

	    /** Got a comma and we're watching for a comma-separated list within these ()? **/
	    if (t == MLX_TOK_COMMA && (cmpflags & EXPR_CMP_WATCHLIST))
	        {
		/** Ok, add a LIST node for the top level if not already **/
		if (expr->NodeType != EXPR_N_LIST)
		    {
	    	    etmp = expAllocExpression();
		    etmp->NodeType = EXPR_N_LIST;
		    expr->Parent = etmp;
		    xaAddItem(&(etmp->Children),(void*)expr);
		    expr = etmp;
		    eptr = expr;
		    }
		else
		    {
		    eptr = expr;
		    }
		continue;
		}

	    /** Otherwise, comma or EOF specifies end-of-expression. **/
	    if (t == MLX_TOK_EOF || t == MLX_TOK_COMMA || t == MLX_TOK_SEMICOLON) 
	        {
		mlxHoldToken(lxs);
		break;
		}

	    /** Close-paren -- exiting out of a subexpression? **/
	    if (level > 0 && t == MLX_TOK_CLOSEPAREN) break;

	    sptr = NULL;
	    if (t == MLX_TOK_KEYWORD && (cmpflags & EXPR_CMP_ASCDESC))
	        {
		sptr = mlxStringVal(lxs,NULL);
		if (!strcmp(sptr,"desc"))
		    {
		    expr->Flags |= EXPR_F_DESC;
		    break;
		    }
		else if (!strcmp(sptr,"asc"))
		    {
		    break;
		    }
		}
	    if (t==MLX_TOK_ERROR) 
		{
		/*if (etmp) expFreeExpression(etmp);*/
		if (expr) expFreeExpression(expr);
		expr = NULL;
		err = 1;
		mssError(0,"EXP","Error reading expression token stream");
		mlxNoteError(lxs);
		break;
		}
	    etmp = expAllocExpression();
	    switch(t)
		{
		case MLX_TOK_KEYWORD:
		    if (!sptr) sptr = mlxStringVal(lxs,NULL);
		    if (!strcasecmp(sptr,"and")) etmp->NodeType = EXPR_N_AND;
		    else if (!strcasecmp(sptr,"or")) etmp->NodeType = EXPR_N_OR;
		    else if (!strcasecmp(sptr,"in")) etmp->NodeType = EXPR_N_IN;
		    else if (!strcasecmp(sptr,"like")) etmp->NodeType = EXPR_N_LIKE;
		    else if (!strcasecmp(sptr,"contains")) etmp->NodeType = EXPR_N_CONTAINS;
		    else if (!strcasecmp(sptr,"is"))
		        {
			t = mlxNextToken(lxs);
			if (t != MLX_TOK_KEYWORD)
			    {
		            expFreeExpression(etmp);
		            if (expr) expFreeExpression(expr);
			    err=1;
			    mssError(1,"EXP","Expected NULL or NOT NULL after IS");
			    mlxNoteError(lxs);
			    expr = NULL;
			    }
			sptr = mlxStringVal(lxs,NULL);
			if (!strcasecmp(sptr,"null"))
			    {
			    etmp->NodeType = EXPR_N_ISNULL;
			    was_unary = 1;
			    }
			else
			    {
			    mssError(1,"EXP","Expected NULL or NOT NULL after IS");
			    mlxNoteError(lxs);
		            expFreeExpression(etmp);
		            if (expr) expFreeExpression(expr);
			    err=1;
			    expr = NULL;
			    }
			}
		    else 
			{
		        expFreeExpression(etmp);
		        if (expr) expFreeExpression(expr);
			mssError(1,"EXP","Valid keywords for operators: AND/OR/LIKE/CONTAINS/IS");
			mlxNoteError(lxs);
			err=1;
			expr = NULL;
			}
		    break;

		case MLX_TOK_ASTERISK: /* outer join? */
		    if (cmpflags & EXPR_CMP_OUTERJOIN)
		        {
			if (mlxNextToken(lxs) == MLX_TOK_EQUALS)
			    {
			    etmp->NodeType = EXPR_N_COMPARE;
			    etmp->Flags |= EXPR_F_LOUTERJOIN;
			    etmp->CompareType = MLX_CMP_EQUALS;
			    break;
			    }
			else
			    {
			    mlxHoldToken(lxs);
		    	    etmp->NodeType = EXPR_N_MULTIPLY;
		    	    break;
			    }
			}
		    if (mlxNextToken(lxs) != MLX_TOK_EQUALS)
		        {
			mlxHoldToken(lxs);
			etmp->NodeType = EXPR_N_MULTIPLY;
			break;
			}

		    /** asterisk but no outer join or not allowed? **/
		    expFreeExpression(etmp);
		    if (expr) expFreeExpression(expr);
		    mssError(1,"EXP","*= encountered but outerjoins not allowed in expression");
		    err=1;
		    expr = NULL;
		    break;

		case MLX_TOK_EQUALS:
		    etmp->NodeType = EXPR_N_COMPARE;
		    etmp->CompareType = MLX_CMP_EQUALS;

		    /** Check outer join? **/
		    if (cmpflags & EXPR_CMP_OUTERJOIN)
		        {
			if (mlxNextToken(lxs) == MLX_TOK_ASTERISK)
			    {
			    etmp->Flags |= EXPR_F_ROUTERJOIN;
			    }
			else
			    {
			    mlxHoldToken(lxs);
			    }
			}
		    break;

		case MLX_TOK_COMPARE:
		    etmp->NodeType = EXPR_N_COMPARE;
		    etmp->CompareType = mlxIntVal(lxs);
		    break;

		case MLX_TOK_PLUS:
		    etmp->NodeType = EXPR_N_PLUS;
		    break;

		case MLX_TOK_SLASH:
		    etmp->NodeType = EXPR_N_DIVIDE;
		    break;

		case MLX_TOK_FILENAME:
		    sptr = mlxStringVal(lxs,NULL);
		    if (sptr && !strcmp(sptr,"/"))
		        {
			etmp->NodeType = EXPR_N_DIVIDE;
			}
		    else
		        {
			mssError(1,"EXP","Unexpected filename in expression string");
			}
		    break;

		case MLX_TOK_MINUS:
		    etmp->NodeType = EXPR_N_MINUS;
		    break;

		default:
		    expFreeExpression(etmp);
		    if (expr) expFreeExpression(expr);
		    mssError(1,"EXP","Unexpected token in expression string");
		    mlxNoteError(lxs);
		    err=1;
		    expr = NULL;
		    break;
		}
	    if (err) break;
	    etmp->Flags |= EXPR_F_OPERATOR;

	    /** Now put the operator in the right place. **/
	    if (expr->Flags & EXPR_F_OPERATOR)
		{
		/** Scan until the precedence is correct **/
		while(eptr->Parent && EXP.Precedence[etmp->NodeType] >= EXP.Precedence[eptr->Parent->NodeType])
		    {
		    eptr = eptr->Parent;
		    }

		/** If at top, update expr, else substitute node **/
		if (!(eptr->Parent))
		    {
		    expr->Parent = etmp;
		    xaAddItem(&(etmp->Children),(void*)expr);
		    expr = etmp;
		    eptr = expr;
		    }
		else
		    {
		    xaRemoveItem(&(eptr->Parent->Children),xaFindItem(&(eptr->Parent->Children),(void*)eptr));
		    etmp->Parent = eptr->Parent;
		    eptr->Parent = etmp;
		    xaAddItem(&(etmp->Parent->Children),(void*)etmp);
		    xaAddItem(&(etmp->Children),(void*)eptr);
		    eptr = etmp;
		    }
		}
	    else
		{
		expr->Parent = etmp;
		xaAddItem(&(etmp->Children),(void*)expr);
		expr = etmp;
		eptr = expr;
		}
	    }

    return expr;
    }


/*** exp_internal_SetCoverageMask - calculates the object coverage mask
 *** for upper-level nodes.
 ***/
int
exp_internal_SetCoverageMask(pExpression exp)
    {
    int i;
    pExpression subexp;

    	/** Shortcut quit **/
	if (exp->NodeType == EXPR_N_OBJECT) return 0;

	/** Compute coverage mask for this based on child objects **/
	for(i=0;i<exp->Children.nItems;i++)
	    {
	    subexp = exp->Children.Items[i];
	    exp_internal_SetCoverageMask(subexp);
	    exp->ObjCoverageMask |= subexp->ObjCoverageMask;
	    }

    return 0;
    }


/*** exp_internal_SetAggLevel - determine the level of aggregate nesting
 *** in an expression.
 ***/
int
exp_internal_SetAggLevel(pExpression exp)
    {
    int i;
    int max_level = 0,rval;

    	/** First, search for the max level of child expressions **/
	for(i=0;i<exp->Children.nItems;i++)
	    {
	    rval = exp_internal_SetAggLevel((pExpression)(exp->Children.Items[i]));
	    if (rval > max_level) max_level = rval;
	    }

	/** Is this an aggregate function?  If so, set level one higher **/
	if (exp->NodeType == EXPR_N_FUNCTION && (exp->Flags & EXPR_F_AGGREGATEFN))
	    {
	    max_level++;
	    }

	/** Set the level and return it **/
	exp->AggLevel = max_level;

    return max_level;
    }


/*** exp_internal_SetListCounts - set the list count levels for each node so
 *** the evaluation routines know how many times to iterate the evaluation.  A
 *** non-list subtree will have list count values of 1.  When lists are
 *** encountered, its list count will equal the sum of its items' list counts.
 *** For all other nodes, the list count is equal to the product of its child
 *** items list counts.  Thus, the top-level expression's list count will be
 *** equal to the number of distinct evaluations the expression can have due
 *** to various combinations of list items.
 ***
 *** If a list is encountered with a parent IN statement, then the list count
 *** of the IN statement must be set to 1, since the sub-items of the IN list
 *** can then be automatically iterated through for proper evaluation.  This
 *** is also true for any other comparison expressions as well.  The idea is
 *** that the expression substring('abcdefghi',(1,4,7),3) has three values,
 *** but the expression :a:strval IN ('abc','def','ghi') has only one value,
 *** that is, whether 'strval' is equal to any of the three items in the list.
 *** Other boolean operators behave in the same fashion.
 ***/
int
exp_internal_SetListCounts(pExpression exp)
    {
    int i,product,sum;

    	/** Node is an EXPR_N_LIST node? **/
	if (exp->NodeType == EXPR_N_LIST)
	    {
    	    /** Evaluate all sub-nodes, summing the list counts. **/
	    sum = 0;
	    for(i=0;i<exp->Children.nItems;i++)
	        {
	        exp_internal_SetListCounts((pExpression)(exp->Children.Items[i]));
		sum += ((pExpression)(exp->Children.Items[i]))->ListCount;
	        }
	    exp->ListCount = sum;
	    }
	else if (exp->NodeType == EXPR_N_IN || exp->NodeType == EXPR_N_COMPARE)
	    {
	    /** Boolean compare node? **/
	    for(i=0;i<exp->Children.nItems;i++)
	        {
	        exp_internal_SetListCounts((pExpression)(exp->Children.Items[i]));
		}
	    exp->ListCount = 1;
	    }
	else
	    {
	    /** Node is a normal node -- list count is product (1 if no child nodes). **/
	    product = 1;
	    for(i=0;i<exp->Children.nItems;i++)
	        {
	        exp_internal_SetListCounts((pExpression)(exp->Children.Items[i]));
		product *= ((pExpression)(exp->Children.Items[i]))->ListCount;
	        }
	    exp->ListCount = product;
	    }

    return 0;
    }


/*** expCompileExpressionFromLxs - builds an expression, given an already-open 
 *** lexer session.
 ***/
pExpression
expCompileExpressionFromLxs(pLxSession s, pParamObjects objlist, int cmpflags)
    {
    pExpression e;

	/** Parse it. **/
	e = exp_internal_CompileExpression_r(s, 0, objlist, cmpflags);
	if (e) exp_internal_SetAggLevel(e);
	if (e) exp_internal_SetCoverageMask(e);

	/** Set SEQ ids. **/
	if (e) e->SeqID = 0;
	if (e) e->PSeqID = 0;

    return e;
    }
 


/*** expCompileExpression - uses the above function to parse
 *** the expression and build the expression tree.
 ***/
pExpression
expCompileExpression(char* text, pParamObjects objlist, int lxflags, int cmpflags)
    {
    pExpression e = NULL;
    pLxSession lxs;

	/** Open the lexer on the input text. **/
	lxs = mlxStringSession(text, MLX_F_EOF | lxflags);
	if (!lxs) 
	    {
	    mssError(0,"EXP","Could not begin analysis of expression");
	    return NULL;
	    }

	e = expCompileExpressionFromLxs(lxs, objlist, cmpflags);

	/** Close the lexer session. **/
	mlxCloseSession(lxs);

    return e;
    }


