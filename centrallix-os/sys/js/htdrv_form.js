// Copyright (C) 1998-2001 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

function form_cb_register(aparam)
    {
    if(aparam.kind=="formstatus")
	{
	this.statuswidgets.push(aparam);
	aparam.setvalue(this.mode);
	}
    else
	{
	this.elements.push(aparam);
	}
    }

/** A child 'control' has changed it's data **/
function form_cb_data_notify(control)
    {
    control._form_IsChanged=true;
    if(this.mode=='New' || this.mode=='Modify')
	{
	if(!this.IsUnsaved)
	    {
	    this.IsUnsaved=true;
	    this.SendEvent('StatusChange');
	    this.SendEvent('DataChange');
	    }
	}
    }

/** A child 'control' got or lost focus **/
function form_cb_focus_notify(control)
    {
    if(this.mode=='View')
	{
	this.ChangeMode('Modify');
	}
    if(this.mode=='NoData')
	{
	this.ChangeMode('New');
	}
    }

/** the tab key was pressed in child 'control' **/
function form_cb_tab_notify(control)
    {
    var ctrlnum;
    for(var i in this.elements)
	{
	if(this.elements[i].name=control.name)
	    {
	    ctrlnum=i;
	    }
	}
    if(this.elements[ctrlnum+1])
	{
	/* bounce the tab up the call tree */
	} else {
	/* set the focus */
	}
    }

/** Objectsource says there's data ready **/
function form_cb_data_available(aparam)
    {
    this.cb["DataAvailable"].run();
    }

/** Objectsource wants us to dump our data **/
function form_cb_is_discard_ready()
    {
    if(!this.IsUnsaved)
	{
	this.osrc.QueryContinue(this);
	return 0;
	}
    var savefunc=new Function("this.cb['OperationCompleteSuccess'].add(this,new Function('this.osrc.QueryContinue(this);'));this.cb['OperationCompleteFail'].add(this,new Function('this.osrc.QueryCancel(this);'));this.ActionSave();");
    this.cb['_3bConfirmSave'].add(this,savefunc);
    this.cb['_3bConfirmDiscard'].add(this,new Function('this.ClearAll();this.osrc.QueryContinue(this);'));
    this.cb['_3bConfirmCancel'].add(this,new Function('this.osrc.QueryCancel(this);'));
    this.show3bconfirm();
    return 0;
    
    if(confirm("OK to save or discard changes, CANCEL to stay here"))
	{
	if(confirm("OK to save changes, CANCEL to discard them."))
	    { /* save */
	    this.cb["OperationCompleteSuccess"].add(this,new Function("this.osrc.QueryContinue(this);"))
	    this.cb["OperationCompleteFail"].add(this,new Function("this.osrc.QueryCancel(this);"))
	    this.ActionSave();
	    }
	else
	    { /* cancel (discard) */
	    this.osrc.QueryContinue(this);
	    }
	}
    else
	{ /* cancel (don't allow new query) */
	this.osrc.QueryCancel(this);
	}
    }

/** Objectsource says our object is available **/
function form_cb_object_available(data)
    {
    if(this.mode!='View')
	{
	this.ChangeMode('View');
	}
    this.data=data;
    this.ClearAll();
    if (this.data)
	{
	for(var i in this.elements)
	    {
	    if(this.elements[i]._form_fieldid==undefined)
		{
		for(var j in this.data)
		    {
		    if(this.elements[i].fieldname && this.elements[i].fieldname==this.data[j].oid)
			{
			this.elements[i]._form_type=data[j].type;
			this.elements[i]._form_fieldid=j;
			}
		    }
		if(this.elements[i]._form_fieldid==undefined)
		    this.elements[i]._form_fieldid=null;
		}
	    if(this.elements[i]._form_fieldid!=null && this.data[this.elements[i]._form_fieldid].value)
		{
		this.elements[i].setvalue(this.data[this.elements[i]._form_fieldid].value);
		}
	    else
		{
		this.elements[i].clearvalue();
		}
	    }
	}
    }

/** Objectsource says the operation is complete **/
function form_cb_operation_complete(r)
    {
    if(r)
	this.cb['OperationCompleteSuccess'].run();
    else
	this.cb['OperationCompleteFail'].run();
    }

/** Objectsource says the object was deleted **/
function form_cb_object_deleted()
    {
    this.cb['ObjectDeleted'].run();
    }

/** Objectsource says the object was created **/
function form_cb_object_created()
    {
    this.cb['ObjectCreated'].run();
    }

/** Objectsource says the object was modified **/
function form_cb_object_modified()
    {
    this.cb['ObjectModified'].run();
    }

/** Moves form to "NoData" mode **/
/**   If unsaved data exists (New or Modify mode), prompt for Save/Discard **/
/** Clears any rows in osrc replica (doesn't delete from DB) **/
function form_action_clear(aparam)
    {
    if(this.mode=="NoData")
	return; /* Already in NoData Mode */
    if(this.IsUnsaved && (this.mode=="New" || this.mode=="Modify"))
	{
	if(confirm("OK to save or discard changes, CANCEL to stay here"))
	    {
	    if(confirm("OK to save changes, CANCEL to discard them."))
		{ /* save */
		this.cb["OperationCompleteSuccess"].add(this,new Function("this.ActionClear();"));
		this.ActionSave();
		return 0;
		}
	    else
		{ /* discard is ok */
		this.ClearAll();
		}
	    }
	else
	    { /* user wants to stay here */
	    return 0;
	    }
	}
    this.ClearAll();
    }

/** in New - clear, remain in New **/
/** in View - delete the record, move to View mode on the next record, or NoData **/
/** in Query - clear, remain in Query **/
function form_action_delete(aparam)
    {
    switch(this.mode)
	{
	case "New":
	    this.ClearAll();
	    break;
	case "View":
	    break;
	case "Query":
	    this.ClearAll();
	    break;
	default:
	    break; /* else do nothing */
	}
    }

/** in Modify, cancel changes, switch to 'View' **/
/** in New, cancel changes, switch to 'NoData' **/
/** in Query, clear, remain in query mode **/
function form_action_discard(aparam)
    {
    switch(this.mode)
	{
	case "Modify":
	    this.ClearAll();
	    this.ChangeMode("View");
	    break;
	case "New":
	    this.ClearAll();
	    this.ChangeMode("NoData");
	    break;
	case "Query":
	    this.ClearAll();
	    break;
	}
    }

/** in NoData, move to New **/
/** in View, move to Modify (note this can be implicit on click -- can be disabled) **/
function form_action_edit(aparam)
    {
    switch(this.mode)
	{
	case "NoData":
	    this.ChangeMode("New");
	    break;
	case "View":
	    this.ChangeMode("Modify");
	    break;
	}
    }

function form_show_3bconfirm()
    {
    var discard,save,cancel;
    if(cn_browser.mozilla)
	var lay=this._3bconfirmwindow.ContentLayer.getElementsByTagName('div');
    else if(cn_browser.netscape47)
	var lay=this._3bconfirmwindow.ContentLayer.layers;
    else
	return false;
    for(var i in lay)
	{
	if(lay[i] && lay[i].buttonName)
	    {
	    switch(lay[i].buttonName)
		{
		case '_3bConfirmCancel':
		    cancel=lay[i];
		    break;
		case '_3bConfirmSave':
		    save=lay[i];
		    break;
		case '_3bConfirmDiscard':
		    discard=lay[i];
		    break;
		}
	    }
	}
    if(!(save && discard && cancel))
	{
	alert("You didn't use all the buttons, or you named them wrong.");
	return 0;
	}
/** set up pointers so the EventClick method can find this form... **/
    save._form_form=this;
    discard._form_form=this;
    cancel._form_form=this;
    save.EventClick=this._3bconfirm_save;
    discard.EventClick=this._3bconfirm_discard;
    cancel.EventClick=this._3bconfirm_cancel;
    
    var funclate=new Function("this.cb['_3bConfirmCancel'].clear();this.cb['_3bConfirmDiscard'].clear();this.cb['_3bConfirmSave'].clear();");
    var func=new Function("pg_setmodal(null);var v=new Object();v.IsVisible=0;this._3bconfirmwindow.ActionSetVisibility(v);");
    
/** funclate will fire last (or very close, with a level of 1000) **/
    this.cb['_3bConfirmCancel'].add(this,funclate,null,1000);
    this.cb['_3bConfirmDiscard'].add(this,funclate,null,1000);
    this.cb['_3bConfirmSave'].add(this,funclate,null,1000);
    
/** func will fire first (or very close, with a level of -1000) **/
    this.cb['_3bConfirmCancel'].add(this,func,null,-1000);
    this.cb['_3bConfirmDiscard'].add(this,func,null,-1000);
    this.cb['_3bConfirmSave'].add(this,func,null,-1000);
    
/** set modal and make the user do something
 **   modal will be unset and we'll get control back via callbacks **/
    this._3bconfirmwindow.ActionSetVisibility(1);
    pg_setmodal(this._3bconfirmwindow.ContentLayer)
    
    }

function form_3bconfirm_cancel()
    {
    if(this._form_form)
	this._form_form.cb['_3bConfirmCancel'].run();
    }

function form_3bconfirm_discard()
    {
    if(this._form_form)
	this._form_form.cb['_3bConfirmDiscard'].run();
    }

function form_3bconfirm_save()
    {
    if(this._form_form)
	this._form_form.cb['_3bConfirmSave'].run();
    }

function form_test_3bconfirm()
    {
    this.cb['_3bConfirmCancel'].add(this,new Function("alert('cancel was clicked');"))
    this.cb['_3bConfirmDiscard'].add(this,new Function("alert('discard was clicked');"))
    this.cb['_3bConfirmSave'].add(this,new Function("alert('save was clicked');"))
    this.show3bconfirm();
    }

/** in View, Query, or NoData, clear, move to New **/
/** in Modify, give save/discard/cancel prompt **/
/** (if from NoData or View mode after query, auto-fill criteria) **/
function form_action_new(aparam)
    {
    switch(this.mode)
	{
	case "Query":
	case "NoData":
	case "View":
	case "Modify":
	    this.ChangeMode("New");
	    /* if there was a query run, fill in it's values... */
	    break;
	}
    }

function form_action_view(aparam)
    {
    this.ChangeMode("View");
    }

/** tell osrc to go to first record **/
function form_action_first(aparam)
    {
    this.osrc.ActionFirst();
    return 0;
    }

/** tell osrc to go to last record **/
function form_action_last(aparam)
    {
    this.osrc.ActionLast();
    return 0;
    }

/** Save changed data, move the osrc **/
function form_action_next(aparam)
    {
    this.osrc.ActionNext();
    return 0;
    }

/** Save changed data, move the osrc **/
function form_action_prev(aparam)
    {
    this.osrc.ActionPrev();
    return 0;
    }

/** Helper function -- called from other mode change functions **/
function form_change_mode(newmode)
    {
    if (newmode == 'Modify' && !this.allowmodify)
	return;
    else if (newmode == 'View' && !this.allowview)
	return;
    else if (newmode == 'Query' && !this.allowquery)
	return;
    else if (newmode == 'New' && !this.allownew)
	return;
    else if (newmode == 'NoData' && !this.allownodata)
	return;

    //confirm('Form is going from '+this.mode+' to '+newmode+' mode.');
    if(this.mode=='Modify' && this.IsUnsaved)
	{
	var savefunc=new Function("this.cb['OperationCompleteSuccess'].add(this,new Function('this.Action"+newmode+"();'));this.ActionSave();");
	var discardfunc=new Function("this.IsUnsaved=false;this.ChangeMode('"+newmode+"');");
	this.cb['_3bConfirmDiscard'].add(this,discardfunc);
	this.cb['_3bConfirmSave'].add(this,savefunc);
	this.show3bconfirm();
	return;
	}
    this.oldmode = this.mode;
    this.mode = newmode;
    this.IsUnsaved = false;
    
    if(this.mode=='Query')
	{
	this.ClearAll();
/** This would auto-fill the form boxes with the last query, but has
 **   other issues -- how do you tell what fields to query on?
 **   -- you can't go on what fields were changed, can't use true value, etc.
 **   ---- disabled for now -- Jonathan Rupp 6/23/2002
 **/
/*
	for(var j in this.osrc.queryobject)
	    if(j!='oid' && j!='joinstring')
		for(var i in this.elements)
		    if(this.elements[i].fieldname==this.osrc.queryobject[j].oid)
			this.elements[i].setvalue(this.osrc.queryobject[j].value);
*/
	}
    
//"    alert(newmode + ' - ' + this.oldmode);
    if(this.mode=='NoData' || this.mode=='View')
	{
	this.EnableModifyAll();
	}
    if(this.mode=='Query' || this.mode=='New')
	{
	this.EnableNewAll();
	}
    for(var i in this.statuswidgets)
	{
	this.statuswidgets[i].setvalue(this.mode);
	}
    this.SendEvent('StatusChange');
    this.SendEvent(this.mode);
    return 1;
    }

function form_send_event(event)
    {
    //confirm(eval('this.Event'+event));
    if(!eval('this.Event'+event)) return 1;
    var evobj = new Object();
    evobj.Caller = this;
    evobj.Status = this.mode;
    evobj.IsUnsaved = this.IsUnsaved;
    cn_activate(this, event, evobj);
    delete evobj;
    }

/** Clears all children, also resets IsChanged/IsUnsaved flag to false **/
function form_clear_all()
    {
    for(var i in this.elements)
	{
	this.elements[i].clearvalue();
	this.elements[i]._form_IsChanged=false;
	}
    this.IsUnsaved=false;
    //this.osrc.ActionClear();
    }

/** Disables all children **/
function form_disable_all()
    {
    for(var i in this.elements)
	{
	this.elements[i].disable();
	}
    }

/** Enables all children (for modify) **/
function form_enable_modify_all()
    {
    for(var i in this.elements)
	{
	if(this.elements[i].enablemodify)
	    this.elements[i].enablemodify();
	else
	    this.elements[i].enable();
	}
    }

/** Enables all children (for modify) **/
function form_enable_new_all()
    {
    for(var i in this.elements)
	{
	if(this.elements[i].enablenew)
	    this.elements[i].enablenew();
	else
	    this.elements[i].enable();
	}
    }


/** make all children readonly**/
function form_readonly_all()
    {
    for(var i in this.elements)
	{
	this.elements[i].readonly();
	}
    }

/** Change to query mode **/
function form_action_query()
    {
    if(!this.allowquery) {alert('Query mode not allowed');return 0;}
    return this.ChangeMode('Query');
    }

/** Execute query **/
function form_action_queryexec()
    {
    if(!this.allowquery) {alert('Query not allowed');return 0;}
    if(!(this.mode=='Query')) {alert("You can't execute a query if you're not in Query mode.");return 0;}
    var where = new String;
    var firstone = true;
/** build an query object to give the osrc **/
    var query=new Array();
    query.oid=null;
    query.joinstring='AND';
    
    for(var i in this.elements)
	{
	if(this.elements[i]._form_IsChanged)
	    {
	    var t=new Object();
	    t.oid=this.elements[i].fieldname;
	    t.value=this.elements[i].getvalue();
	    t.type=this.elements[i]._form_type;
	    query.push(t);
	    }
	}
/** Done with the query -- YEAH **/
    //if(confirm('Send to "'+this.osrc.name+'"(osrc):'+query))
	{
	this.Pending=true;
	this.IsUnsaved=false;
	//this.cb['DataAvailable'].add(this,new Function('this.osrc.ActionFirst(this)'));
	this.osrc.ActionQueryObject(query, this);
	}
    delete query;
    }

function form_action_save()
    {
    if(!this.IsUnsaved)
	{
	return 0;
	}
    this.cb['OperationCompleteSuccess'].add(this,
	   new Function("this.IsUnsaved=false;this.Pending=false;this.EnableModifyAll();this.ActionView();this.cb['OperationCompleteFail'].clear();"),null,-100);
     this.cb['OperationCompleteFail'].add(this,
	   new Function("this.Pending=false;this.EnableModifyAll();confirm('Data Save Failed');this.cb['OperationCompleteSuccess'].clear();"),null,-100);
    
/** build the object to pass to objectsource **/
    var dataobj=new Array();
    for(var i in this.elements)
	{
	if(this.elements[i]._form_IsChanged)
	    {
	    var t=new Object();
	    t.oid=this.elements[i].fieldname;
	    t.value=this.elements[i].getvalue();
	    t.type=this.elements[i]._form_type;
	    dataobj.push(t);
	    }
	}
    dataobj.oid=this.data.oid;
    this.DisableAll();
    this.Pending=true;
    this.osrc.ActionModify(dataobj,this);
    }

/** Helper function to build a query */
function form_build_query(base,where,ro)
    {
    re=/where/i;
    if(!where)
	if(ro)
	    return base;
	else
	    return base+' FOR UPDATE';
    if(re.test(base))
	if(ro)
	   return base+' AND '+where;
	else
	   return base+' AND '+where+' FOR UPDATE';
    else
	if(ro)
	    return base+' WHERE '+where;
	else
	    return base+' WHERE '+where+' FOR UPDATE';
    }


/***
 ***   The following callback helper functions might be
 ***   useful if available to all widgets.
 ***   For now, just the form will use them though
 ***/

/** Callback object init **/
function form_cbobj(n) //FIXME FIXME
    {
    this.arr=new Array();
    this.add=form_cbobj_add;
    this.run=form_cbobj_run;
    this.clear=form_cbobj_clear;
    this.name=n;
    }

/** Calls all registered functions **/
function form_cbobj_run()
    {
    if(this.arr.length<1)
	{
	//confirm('There are no callbacks registered for '+this.name+'.  This is a problem');
	return false;
	}
    this.arr.sort(form_cbobj_compare);
    for(var i in this.arr)
	{
	var j=this.arr[i];
	//if(j && confirm(j))
	if(j)
	    if(j[3])
		j[2].apply(j[1],j[3]);
	    else
		j[2].apply(j[1]);
	}
    this.clear();
    }

/** Adds a new registered function **/
function form_cbobj_add(obj,func,param,key)
    {
    if(obj==undefined) { obj = null; }
    if(func==undefined) { func = null; }
    if(param==undefined) { param = null; }
    if(key==undefined) { key=0; }
    this.arr.push(new Array(key,obj,func,param));
    }

/** Clears all registered callbacks **/
function form_cbobj_clear()
    {
    for(var i in this.arr)
	delete this.arr[i];
    }

/** Compares two arrays based on the first element **/
function form_cbobj_compare(a,b)
    { /* sort in order based on keys (element 0) */
    if(!a || !b) return 0;
    if(a[0]>b[0]) return 1;
    if(a[0]==b[0]) return 0;
    if(a[0]<b[0]) return -1;
    }

/** Form initializer **/
function form_init(aq,an,am,av,and,me,name,_3b,ro)
    {
    var form = new Object();
    form.readonly=ro;
    form.elements = new Array();
    form.statuswidgets = new Array();
    form.mode = 'NoData';
    form.cobj = null; /* current 'object' (record) */
    form.oldmode = null;
    if(osrc_current)
	{
	form.osrc = osrc_current;
	form.osrc.Register(form);
	}
    form.IsUnsaved = false;
    form.name = name;
    form.Pending = false;
/** remember what to do after callbacks.... **/
    form.cb = new Array();
    form.cb['DataAvailable'] = new form_cbobj('DataAvailable');
    form.cb['ObjectAvailable'] = new form_cbobj('ObjectAvailable');
    form.cb['OperationCompleteSuccess'] = new form_cbobj('OperationCompleteSuccess');
    form.cb['OperationCompleteFail'] = new form_cbobj('OperationCompleteFail');
    form.cb['ObjectDeleted'] = new form_cbobj('ObjectDeleted');
    form.cb['ObjectCreated'] = new form_cbobj('ObjectCreated');
    form.cb['ObjectModified'] = new form_cbobj('ObjectModified');
    form.cb['_3bConfirmCancel'] = new form_cbobj('_3bConfirmCancel');
    form.cb['_3bConfirmSave'] = new form_cbobj('_3bConfirmSave');
    form.cb['_3bConfirmDiscard'] = new form_cbobj('_3bConfirmDiscard');
/** initialize params from .app file **/
    form.allowquery = aq;
    form.allownew = an;
    form.allowmodify = am;
    form.allowview = av;
    form.allownodata = and;
    form.multienter = me;
/** initialize actions and callbacks **/
    form._3bconfirmwindow = _3b;
    form._3bconfirm_discard = form_3bconfirm_discard;
    form._3bconfirm_cancel = form_3bconfirm_cancel;
    form._3bconfirm_save = form_3bconfirm_save;
    form.show3bconfirm = form_show_3bconfirm;
    form.Actiontest3bconfirm = form_test_3bconfirm;
    form.ActionClear = form_action_clear;
    form.ActionDelete = form_action_delete;
    form.ActionDiscard = form_action_discard;
    form.ActionEdit = form_action_edit;
    form.ActionView = form_action_view;
    form.ActionFirst = form_action_first;
    form.ActionLast = form_action_last;
    form.ActionNew = form_action_new;
    form.ActionPrev = form_action_prev;
    form.ActionNext = form_action_next;
    form.ActionQuery = form_action_query;
    form.ActionQueryExec = form_action_queryexec;
    form.ActionSave = form_action_save;
    form.IsDiscardReady = form_cb_is_discard_ready;
    form.DataAvailable = form_cb_data_available;
    form.ObjectAvailable = form_cb_object_available;
    form.OperationComplete = form_cb_operation_complete;
    form.ObjectDeleted = form_cb_object_deleted;
    form.ObjectCreated = form_cb_object_created;
    form.ObjectModified = form_cb_object_modified;
    form.Register = form_cb_register;
    form.DataNotify = form_cb_data_notify;
    form.FocusNotify = form_cb_focus_notify;
    form.TabNotify = form_cb_tab_notify;
/** noone else should call these.... **/
    form.ClearAll = form_clear_all;
    form.DisableAll = form_disable_all;
    form.EnableModifyAll = form_enable_modify_all;
    form.EnableNewAll = form_enable_new_all;
    form.ReadOnlyAll = form_readonly_all;
    form.ChangeMode = form_change_mode;
    form.SendEvent = form_send_event;
    //form.InitQuery = form_init_query;
    return form;
    }
