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


function alrt_action_alert(sendthis)
    {
    window.alert(sendthis["param"]);
    }

function alrt_action_confirm(sendthis)
    {
    window.confirm(sendthis["param"]);
    }

/** Alert initializer **/
function alrt_init(alrt)
    {
    ifc_init_widget(alrt);
    var ai = alrt.ifcProbeAdd(ifAction);
    ai.Add("Alert", alrt_action_alert);
    ai.Add("Confirm", alrt_action_confirm);
    //alrt.ActionViewDOM = alrt_action_view_DOM;
    //alrt.ActionViewTreeDOM = alrt_action_view_tree_DOM;
    return alrt;
    }


// Load indication.
if (window.pg_scripts) pg_scripts['htdrv_alerter.js'] = true;
