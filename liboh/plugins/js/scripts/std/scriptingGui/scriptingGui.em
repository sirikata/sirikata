

(function()
 {

     /**
      @param {std.ScriptingGui.Controller}
      
      @param {object: <String(visibleId):visible object>} nearbyVisMap
      -- all the visible objects that satisfy scripter's proximity
      query.

      @param {object: <String(visibleId):
      std.FileManager.FileManagerElement> scriptedVisMap -- A record
      of all visibles that we have scripted at some point.

      @param {object: <int:std.ScriptingGui.Action> actionMap -- Which
      actions are selectable by scripter.

      @param {std.ScriptingGui.Console} console -- has several
      scripting events.
      */
     std.ScriptingGui =
         function(controller,nearbyVisMap,scriptedVisMap,actionMap,console)
     {
         if (typeof(simulator) == 'undefined')
         {
             throw new Error('Cannot initialize scripting gui without ' +
                             'simulator graphics.');                 
         }


         this.controller       = controller;
         this.nearbyVisMap     = nearbyVisMap;
         this.scriptedVisMap   = scriptedVisMap;
         this.actionMap        = actionMap;
         this.console          = console;

         
         this.guiMod = simulator._simulator.addGUITextModule(
             guiName(),
             getGuiText(),
             std.core.bind(guiInitFunc,undefined,this));
         this.hasInited = false;
     };

     system.require('scriptingGuiUtil.em');
     
     std.ScriptingGui.prototype.redraw = function()
     {
         if (!this.hasInited)
             return;
         
         //trigger redraw call
         this.guiMod.call(
             'ishmaelRedraw',toHtmlNearbyMap(this),toHtmlScriptedMap(this),
             toHtmlActionMap(this));
         
     };


     /**
      @param {String} visId -- Id of visible.
      */
     //called by html to add the visible with id 
     std.ScriptingGui.prototype.hAddVisible = function(visId)
     {
         //already can script the visible.
         if (visId in this.scriptedVisMap)
         {
             this.console.guiEvent();
             return;
         }

         if (!(visId in this.nearbyVisMap))
         {
             //visible is no longer available.
             this.console.guiEvent();
             return;
         }

         this.controller.addVisible(this.nearbyVisMap[visId]);
     };

     std.ScriptingGui.prototype.hRemoveVisible = function(visId)
     {
         //FIXME: should finish this call
         throw new Error('FIXME: must finish hRemoveVisible call ' +
                         'in scriptingGui');
     };

     // //whenever user selects an action to display, then we change
     // //model of internal action that should be displaying.  
     // std.ScriptingGui.prototype.hActionSelected = function(actId)
     // {
     //     if(actId in this.actionMap)
     //         this.selectedAction = this.actionMap[actId];
     //     else
     //         this.selectedAction = undefined;
     // };
     
     
     /**
      @param {std.ScriptingGui} 
      */
     function guiInitFunc(scriptingGui)
     {
         scriptingGui.hasInited = true;
         
         //when a user clicks on any of the nearby visibles to script
         //them, want that object to move into scripted objects.  
         scriptingGui.guiMod.bind(
             'addVisible',
             std.core.bind(scriptingGui.hAddVisible,scriptingGui));

         //when a user clicks to remove any of the scripted visibles
         //from scripted map, then destroy it.
         scriptingGui.guiMod.bind(
             'removeVisible',
             std.core.bind(scriptingGui.hRemoveVisible,scriptingGui));

         
         scriptingGui.redraw();
     }
     
     /**
      @returns {object: <string (visibleId): string (visibleId)>}
      scriptingGui -- Takes the list of nearby objects and turns it
      into an object that (for now) maps visibleIds to visibleIds.

      @see ishmaelRedraw
      */
     function toHtmlNearbyMap(scriptingGui)
     {
         var returner = { };
         for (var s in scriptingGui.nearbyVisMap)
             returner[s] = s;
         return returner;
     }


     /**
      @returns {object: <string (visibleId): string (visibleId)>}
      scriptingGui -- Takes the list of nearby objects and turns it
      into an object that (for now) maps visibleIds to visibleIds.

      @see ishmaelRedraw
      */
     function toHtmlScriptedMap(scriptingGui)
     {
         var returner = { };
         for (var s in scriptingGui.scriptedVisMap)
             returner[s] = s;
         return returner;
     }


     function toHtmlActionMap(scriptingGui)
     {
         return scriptingGui.actionMap;
     }

     
     
     function guiName()
     {
         return 'ishmaelEditor';
     }


     function getGuiText()
     {

         var returner = "sirikata.ui('" + guiName() + "',";
         returner += 'function(){ ';

         returner += @

         function ishmaelWindowId()
         {
             return 'ishmael__windowID_';
         }

         //the div that surrounds all the nearby objects.
         function nearbyListId()
         {
             return 'ishmael__nearbyListID__';
         }

         //the div that surrounds all the scripted objects.
         function scriptedListId()
         {
             return 'ishmael__scriptedListID__';
         }


         function actionListId()
         {
             return 'ishmael__actionListID__';
         }

         function actionTareaId()
         {
             return 'ishmael__actionEditor__';
         }
         

         /**
          \param {String} nearbyObj (id of visible that we are
          communicating with).
          gives the div for each nearby object.
          */
         function generateNearbyDivId(nearbyObj)
         {
             var visIdDivable = divizeVisibleId(nearbyObj);
             return 'ishmael__nearbyDivID___' + visIdDivable;
         }


         /**
          \param {String} scriptedObj (id of visible that we are
          communicating with).
          gives the div for each scripted object.
          */
         function generateScriptedDivId(scriptedObj)
         {
             var visIdDivable = divizeVisibleId(scriptedObj);
             return 'ishmael__scriptedDivID___' + visIdDivable;
         }

         
         function divizeVisibleId(visId)
         {
             return visId.replace(':','');
         }
         

         $('<div>'   +

           '<b>Scripted presences</b><br/>' +
           '<select id="'+ scriptedListId() + '"  size=5>' +
           '</select><br/>'  +

           '<b>Nearby presences</b><br/>'      +
           '<div id="'+ nearbyListId() + '"  ' +
           'style="height:200px;width:250px;overflow:scroll;">'     +
           '</div>'  +

           '<select id="' + actionListId() + '" size=5>'   +
           '</select>' +

           '<textarea id="'+ actionTareaId() + '"  >' +
           '</textarea>' +
           
           '</div>' //end div at top.
          ).attr({id:ishmaelWindowId(),title:'ishmael'}).appendTo('body');


         //The id of the visible that the scripter has selected to
         //program.
         var currentlySelectedVisible = undefined;
         
         //int id of the currently selected action.
         var currentlySelectedAction = undefined;
         //map from int to std.ScriptingGui.Action objects.  It gets
         //updated whenever we received an ishmaelRedraw call.  Note
         //that the redraw call will not write over the text of
         //currentlySelectedAction.  This ensures that scripter
         //edits will not be lost while they are being written if some
         //other action triggers a call to ishmaelRedraw (for
         //instance, a new nearbyObject gets added).
         var allActions = undefined;

         $('#' + scriptedListId()).change(
             function()
             {
                 //loads the visible id 
                 var val = $('#' + scriptedListId()).val();
                 currentlySelectedVisible = val;

                 //for debugging
                 sirikata.log(
                     'error','Selected new vis to script: ' + val.toString());
             });
         
         //set a listener for action list.  whenever select an option,
         //should communicate that to emerson gui, so that can keep
         //track of selected action.
         $('#' + actionListId()).change(
             function()
             {
                 var val = 
                     $('#' + actionListId()).val();
                 changeActionText(parseInt(val));
                 sirikata.log('error', 'Selected action: '  + val.toString());
             });
         
         var inputWindow = new sirikata.ui.window(
             '#' + ishmaelWindowId(),
                 {
	             autoOpen: true,
	             height: 'auto',
	             width: 600,
                     height: 600,
                     position: 'right'
                 }
             );
         inputWindow.show();



         /**
          \param {int or undefined} idActSelected.  If an int, then
          looks through record of allActions, and sets action text
          area to that.  If undefined, then clears action text area.
          */
         function changeActionText(idActSelected)
         {
             currentlySelectedAction = idActSelected;
             var textToSetTo = '';
             
             if (typeof(idActSelected) !='undefined')
             {
                 if (! idActSelected in allActions)
                 {
                     sirikata.log('error','action ids out of ' +
                                  'sync with actions in scripting gui.');
                     return;
                 }

                 textToSetTo = allActions[idActSelected].text;
             }

             //actually update textarea with correct text
             $('#' + actionTareaId()).val(textToSetTo);
         }
         
         
         /**
          \param {object: <int:std.ScriptingGui.Action>} actionMap

          FIXEM: currentlySelectedAction is no longer passed. Instead,
          maintain it in html/js

          \param {int or undefined} currentlySelectedAction -- null if
          do not have a currently selected action.  the id of the
          action that is currently selected otherwise.  this id can be
          used to index into actionMap
          */
         ishmaelRedraw = function(
             nearbyObjs,scriptedObjs,actionMap)
         {
             redrawNearby(nearbyObjs);
             redrawScriptedObjs(scriptedObjs);
             redrawActionList(actionMap);
         };

         
         /**
          \param {object: <string (visibleId): string (visibleId)>}
          nearbyObjs -- all objects that are in vicinity.
          */
         function redrawNearby(nearbyObjs)
         {
             var newHtml = '';
             for (var s in nearbyObjs)
             {
                 newHtml += '<div id="' + generateNearbyDivId(s) + '">';
                 newHtml += s;
                 newHtml += '</div>';
             }
             $('#' + nearbyListId()).html(newHtml);

             for (var s in nearbyObjs)
             {
                 $('#' + generateNearbyDivId(s)).click(
                     function()
                     {
                         sirikata.event('addVisible',s);
                     });
             }
         }


         /**
          \param {object: <string (visibleId): string(visibleId)>}
          scriptedObjs -- all objects that we have a scripting
          relationship with.
          */
         function redrawScriptedObjs(scriptedObjs)
         {
             var newHtml = '';
             for (var s in scriptedObjs)
             {
                 if (s === currentlySelectedVisible)
                     newHtml += '<option selected ';
                 else
                     newHtml += '<option ';
                 
                 newHtml += 'value="' +s +  '">';
                 newHtml += s;
                 newHtml += '</option>';
             }
             $('#' + scriptedListId()).html(newHtml);
         }



         function redrawActionList(actionMap)
         {
             var prevCurAct = null;
             if (typeof(currentlySelectedAction) != 'undefined')
                 prevCurAct = allActions[currentlySelectedAction];

             allActions = actionMap;
             //preserves edits that scripter was potentially making
             //when other action triggered redraw (eg. a new nearby
             //object).
             if (prevCurAct !== null)
                 allActions[prevCurAct.id] = prevCurAct;

             
             var newHtml = '';
             for (var s in actionMap)
             {
                 if (s === currentlySelectedAction)
                     newHtml += '<option selected ';
                 else
                     newHtml += '<option ';

                 newHtml += 'value="' + s + '">';
                 newHtml += actionMap[s].name ;
                 newHtml += '</option>';
             }

             $('#' + actionListId()).html(newHtml);
         }

         @;
         
         returner += '});';         
         return returner;
     }
 })();


//do all further imports for file and gui control.
system.require('controller.em');
system.require('action.em');
system.require('console.em');
system.require('fileManagerElement.em');
