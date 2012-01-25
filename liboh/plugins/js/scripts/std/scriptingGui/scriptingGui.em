

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
             'ishmaelRedraw',toHtmlNearbyMap(this),toHtmlScriptedMap(this));
         
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

           '<div id="'+ scriptedListId() + '"  ' +
           'style="height:200px;width:250px;overflow:scroll;">'   +
           '</div>'  +
           
           '<div id="'+ nearbyListId() + '"  ' +
           'style="height:200px;width:250px;overflow:scroll;">'   +
           '</div>'  +
           
           '</div>' //end div at top.
          ).attr({id:ishmaelWindowId(),title:'ishmael'}).appendTo('body');

         var inputWindow = new sirikata.ui.window(
             '#' + ishmaelWindowId(),
                 {
	             autoOpen: true,
	             height: 'auto',
	             width: 300,
                     height: 600,
                     position: 'right'
                 }
             );
         inputWindow.show();
         
         ishmaelRedraw = function(nearbyObjs,scriptedObjs)
         {
             redrawNearby(nearbyObjs);
             redrawScriptedObjs(scriptedObjs);
         };

         
         /**
          \param {object: <string (visibleId): string (visibleId)>}
          nearbyObjs -- all objects that are in vicinity.
          */
         function redrawNearby(nearbyObjs)
         {
             var newHtml = '<b>Nearby presences</b><br/>';
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
             var newHtml = '<b>Scripted presences</b><br/>';
             for (var s in scriptedObjs)
             {
                 newHtml += '<div id="' + generateScriptedDivId(s) + '">';
                 newHtml += s;
                 newHtml += '</div>';
             }
             $('#' + scriptedListId()).html(newHtml);

             
             for (var s in scriptedObjs)
             {
                 $('#' + generateScriptedDivId(s)).click(
                     function()
                     {
                         sirikata.log('error','\\\n\\\nClicked on scripted\\\n\\\n');
                     });
             }
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
