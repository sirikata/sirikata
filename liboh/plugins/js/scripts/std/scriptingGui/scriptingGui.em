system.require('std/core/simpleInput.em');


(function()
 {

     /**
      @param {std.ScriptingGui.Controller} controller

      @param {std.client.Default} simulator
      
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
         function(controller,simulator,nearbyVisMap,
                  scriptedVisMap,actionMap,console)
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
         this.console.setScriptingGui(this);
         
         this.nameMap          = {};
         this.nameMap[system.self.toString()] = 'self';

         
         this.guiMod = simulator.addGUITextModule(
             guiName(),
             getGuiText(),
             std.core.bind(guiInitFunc,undefined,this));
         this.hasInited = false;
         this.isReady = false;
         
         //if get message that we should redraw with a particular
         //selected, then change heldSelected to not be undefined
         this.heldSelected = undefined;
         this.heldForce    = undefined;
     };

     system.require('scriptingGuiUtil.em');


     /**
      @param {String: vis/presId} selected (optional).  If called with
      this field, then gui should actually change currentlySelectedVisible

      @param {bool} force (optional).  If called with this field as
      true, then forces the gui window to be "shown".  
      */
     std.ScriptingGui.prototype.redraw = function(selected,force)
     {
         //have to wait for ace libraries to load.  after they do, js
         //will send an amReady event back to emerson code.  emerson
         //code will redraw with the last not undefined heldSelected.
         if ((!this.hasInited) || (!this.isReady))
         {
             if (typeof(selected) != 'undefined')
                 this.heldSelected =selected;                     

             if (typeof(force) != 'undefined')
                 this.heldForce = force;
             
             return;
         }


         
         //trigger redraw call
         this.guiMod.call(
             'ishmaelRedraw',toHtmlNearbyMap(this),toHtmlScriptedMap(this),
             toHtmlActionMap(this),toHtmlFileMap(this),toHtmlNameMap(this),
             toHtmlConsoleMap(this),selected,force);
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
             this.redraw();
             return;
         }


         if (!(visId in this.nearbyVisMap))
         {
             //visible is no longer available.
             this.console.guiEvent();
             this.redraw();
             return;
         }

         //redraw called from inside of addVisible.
         this.controller.addVisible(this.nearbyVisMap[visId]);
     };

     std.ScriptingGui.prototype.hRemoveVisible = function(visId)
     {
         //FIXME: should finish this call
         throw new Error('FIXME: must finish hRemoveVisible call ' +
                         'in scriptingGui');
     };

     std.ScriptingGui.prototype.hRenameVisible = function (visId,visName)
     {
         var userQuery =
             'Enter new name for visible previously named ' + visName +
             ' and with id: ' + visId;
         
         var newInput = std.core.SimpleInput(
             std.core.SimpleInput.ENTER_TEXT,
             userQuery,
             std.core.bind(renameVisibleInputCB,undefined,this,visId));
     };


     function renameVisibleInputCB(scriptingGui,visId,newName)
     {
         scriptingGui.nameMap[visId] = newName;
         scriptingGui.redraw();
     }
         

     /**
      @param{String} text -- gets displayed as a warning message to
      user.
      */
     std.ScriptingGui.prototype.hWarn = function (text)
     {
         var newInput = std.core.SimpleInput(
             std.core.SimpleInput.NO_INPUT,
             text,function(){});
     };

     
     /**
      @param {String?} actId -- Should be parsedInt to get an index
      into actionMap
      @param {String} newText -- What action with actId should set as
      its text.
      */
     std.ScriptingGui.prototype.hSaveAction = function(actId,newText)
     {
         this.controller.editAction(parseInt(actId),newText);
     };

     /**
      @param {String?} actId -- Should be parsedInt to get an index
      into actionMap
      @param {String} newText -- What action with actId should set as
      its text.
      @param {String} visId   -- Id of visible to execute action on.
      */
     std.ScriptingGui.prototype.hSaveAndExecuteAction =
         function(actId,newText, visId)
     {
         this.hSaveAction(actId,newText);
         this.controller.execAction(actId,visId);
     };

     /**
      Prompt user for the name of the new action using
      std.core.SimpleInput.
      */
     std.ScriptingGui.prototype.hNewAction =
         function()
     {
         var newInput = std.core.SimpleInput(
             std.core.SimpleInput.ENTER_TEXT,
             'Enter new action\'s name',
             std.core.bind(addActionInputCB,undefined,this));
     };

     /**
      @param {std.ScriptingGui} scriptingGui
      @param {String} userResp_actionName - The name of the new action
      that the user wants.
      */
     function addActionInputCB(scriptingGui,userResp_actionName)
     {
         //new action won't have any text in it.
         scriptingGui.controller.addAction(userResp_actionName,'');
         scriptingGui.redraw();
     }

     /**
      @param {String?} actId -- Should be parsedInt to get an index
      into actionMap
      */
     std.ScriptingGui.prototype.hRemoveAction =
         function(actId)
     {
         this.controller.removeAction(parseInt(actId));
         this.redraw();
     };
     

     std.ScriptingGui.prototype.hAddFile =
         function(visId)
     {
         var newInput = std.core.SimpleInput(
             std.core.SimpleInput.ENTER_TEXT,
             'Enter new file\'s name',
             std.core.bind(addFileInputCB,undefined,this,visId));
     };

     
     function addFileInputCB(scriptingGui,visId,userResp_filename)
     {
         scriptingGui.controller.addExistingFileIfCan(
             visId,userResp_filename);
         scriptingGui.redraw();
     }


     //reread file first, then send it to visible.
     std.ScriptingGui.prototype.hUpdateAndSendFile =
         function(visId,filename)
     {
         this.controller.rereadFile(visId,filename);
         this.controller.updateFile(visId,filename);
     };

     std.ScriptingGui.prototype.hUpdateAndSendAllFiles =
         function(visId)
     {
         this.controller.rereadAllFiles(visId);
         this.controller.updateAll(visId);
     };


     std.ScriptingGui.prototype.hRemoveFile =
         function(visId,filename)
     {
         this.controller.removeFile(visId,filename);
         this.redraw();
     };


     std.ScriptingGui.prototype.hExecScript =
         function(visId,toExec)
     {
         this.controller.execScriptAction(visId,toExec);
     };

     std.ScriptingGui.prototype.hAmReady =
         function()
     {
         this.isReady =true;
         this.redraw(this.heldSelected,this.heldForce);
         this.heldSelected  = undefined;
         this.heldForce     = undefined;
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

         scriptingGui.guiMod.bind(
             'renameVisible',
             std.core.bind(scriptingGui.hRenameVisible,scriptingGui));
         
         //when a user updates a particular action, and clicks to save
         //the new action text.
         scriptingGui.guiMod.bind(
             'saveAction',
             std.core.bind(scriptingGui.hSaveAction,scriptingGui));


         //saves and executes action to visible.
         scriptingGui.guiMod.bind(
             'saveAndExecuteAction',
             std.core.bind(scriptingGui.hSaveAndExecuteAction,scriptingGui));


         scriptingGui.guiMod.bind(
             'newAction',
             std.core.bind(scriptingGui.hNewAction,scriptingGui));

         scriptingGui.guiMod.bind(
             'removeAction',
             std.core.bind(scriptingGui.hRemoveAction,scriptingGui));

         scriptingGui.guiMod.bind(
             'addFile',
             std.core.bind(scriptingGui.hAddFile,scriptingGui));

         scriptingGui.guiMod.bind(
             'updateAndSendFile',
             std.core.bind(scriptingGui.hUpdateAndSendFile,scriptingGui));

         scriptingGui.guiMod.bind(
             'updateAndSendAllFiles',
             std.core.bind(scriptingGui.hUpdateAndSendAllFiles,scriptingGui));

         scriptingGui.guiMod.bind(
             'removeFile',
             std.core.bind(scriptingGui.hRemoveFile,scriptingGui));

         scriptingGui.guiMod.bind(
             'execScript',
             std.core.bind(scriptingGui.hExecScript,scriptingGui));


         scriptingGui.guiMod.bind(
             'amReady',
             std.core.bind(scriptingGui.hAmReady,scriptingGui));


         scriptingGui.guiMod.bind(
             'warn',
             std.core.bind(scriptingGui.hWarn,scriptingGui));

         
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
         //for now, we recognize that for a large scene, this approach
         //may be infeasible, and instead display a message to that
         //effect.
         return 'Because of the size of large scenes, we are '         +
             'not displaying all nearby visibles.  <br/>To re-enable ' +
             'this feature, please change std/scriptingGui/scriptingGui.em';
         
         // var returner = { };
         // for (var s in scriptingGui.nearbyVisMap)
         //     returner[s] = s;
         // return returner;
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


     function toHtmlFileMap(scriptingGui)
     {
         return scriptingGui.controller.htmlFileMap();
     }

     function toHtmlNameMap(scriptingGui)
     {
         return scriptingGui.nameMap;
     }

     function toHtmlConsoleMap(scriptingGui)
     {
         return scriptingGui.console.toHtmlMap();
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


        $LAB
            .script("../ace/build/src/ace-uncompressed.js")
            .script("../ace/build/src/theme-dawn.js")
            .script("../ace/build/src/mode-javascript.js").wait(
                function()
                {

	            var newcsslink =
                        $("<link />").attr({rel:'stylesheet', type:'text/css', href:'../scripting/prompt.css'});
	            $("head").append(newcsslink);



         /**
          Visible ids are displayed in nearby tab as well as in
          scripted objs tab.  It's a little overwhelming to get a full
          32-bit identifier, so this change restricts us to only the
          first so many characters.
          */
         function maxVisIdDispDigits()
         {
             return 6;
         }
                    
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

         function garbageNearbyVal()
         {
             return 'ishmael__garbageVal';
         }
                    

         function actionListId()
         {
             return 'ishmael__actionListID__';
         }

         function actionTareaId()
         {
             return 'ishmael__actionEditor__';
         }
         
         function saveActionButtonId()
         {
             return 'ishmael__saveActionButton__';
         }

         function execActionButtonId()
         {
             return 'ishmael__execActionButton__';
         }

         function newActionButtonId()
         {
             return 'ishmael__newActionButton__';
         }

         function removeActionButtonId()
         {
             return 'ishmael__removeActionButton__';
         }

         function fileSelectId()
         {
             return 'ishmael__fileSelectId__';
         }


         function addFileButtonId()
         {
             return 'ishmael__addFileButtonId__';
         }

         function updateAndSendFileButtonId()
         {
             return 'ishmael__updateAndSendFileButtonId__';
         }

         function updateAndSendAllFilesButtonId()
         {
             return 'ishmael__updateAndSendAllFilesButtonId__';
         }

         function removeFileButtonId()
         {
             return 'ishmael__removeFileButtonId__';
         }
         

         function renameVisibleButtonId()
         {
             return 'ishmael__renameVisibleId__';
         }

         function consoleId()
         {
             return 'ishmael__consoleId__';
         }

         function actionDivId()
         {
             return 'ishmael__actionDivId__';
         }

         function fileDivId()
         {
             return 'ishmael__fileDivId__';
         }

         function actionFileTabId()
         {
             return 'ishmael__actionFileTabId__';
         }

         function nearbyScriptedTabId()
         {
             return 'ishmael__nearbyScriptedTabId__';
         }

         function scriptConsoleDivId()
         {
             return 'ishmael__scriptConsoleDivId__';
         }
         
         function execScriptButtonId()
         {
             return 'ishmael__execScriptButtonId__';
         }


         function execTareaId()
         {
             return 'ishmael__execTareaId__';
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



         $('<div style="font-size:.9em">'   +
           
           //which presences are available
           '<div id="' + nearbyScriptedTabId() + '">' +
           '<ul>' +
		'<li><a href="#' + scriptedListId() +'">Scripted</a></li>' +
		'<li><a href="#' + nearbyListId() +'">Nearby</a></li>' +
	   '</ul>' +


              '<select id="'     + scriptedListId() + '" size=5 style="width:300px;overflow:auto">' +
              '</select>'   +

              '<select id="'     + nearbyListId() + '" size=5 style="min-width:300px;overflow:auto;">'   +
              '</select>'   +

           '<br/>' +

              '<button id="' + renameVisibleButtonId() + '">' +
              'rename' +
              '</button>'    +
 
           '</div>' + //closes scripted/nearby tab div

           
           //action file gui
           '<div id="' + actionFileTabId() + '">' +

           '<ul>' +
		'<li><a href="#' + actionDivId() +'">Actions</a></li>' +
		'<li><a href="#' + fileDivId() +'">Files</a></li>' +
                '<li><a href="#' + scriptConsoleDivId() + '"> Instant script</a></li>' +
	   '</ul>' +
           
              //action gui
              '<div id="'+actionDivId() + '">' + 
              '<table><tr><td>'+
              '<select id="'     + actionListId() + '" size=5 style="min-width:100px">'   +
              '</select>'        +
              '</td><td>' +
           
              '<div id="'   + actionTareaId()+ '"  style="min-width:400px;min-height:100px;max-width:400px;position:relative;margin:0;padding:0;">' +
              '</div>'      + //closes actionTareaDiv
              '</td></tr></table>' +
           
              '<button id="'     + saveActionButtonId()    + '">' +
              'save action'      +
              '</button>'        +

              '<button id="'     + execActionButtonId()    + '">' +
              'exec&save action' +
              '</button>'        +

              '<button id="'     + newActionButtonId()     + '">' +
              'new action'       +
              '</button>'        +

              '<button id="'     + removeActionButtonId()  + '">' +
              'remove action'    +
              '</button>'        +
              '</div>'   + //closes action div


              //file gui
              '<div id="' +fileDivId() + '">' +
              '<select id="'     + fileSelectId() + '" size=5 style="min-width:200px">'   +
              '</select><br/>'        +

              '<button id="'+ addFileButtonId() + '">' +
              'add file' +
              '</button>'+

              '<button id="'+ updateAndSendFileButtonId() + '">' +
              'update and send file' +
              '</button>'+

              '<button id="'+ updateAndSendAllFilesButtonId() + '">' +
              'update and send all files' +
              '</button>'+

              '<button id="' + removeFileButtonId() + '">' +
              'remove file' +
              '</button>'   +

              '</div>' + //closes file div

              //script console
              '<div id="' + scriptConsoleDivId() + '">' +

                 '<div id="'   + execTareaId()+ '"  style="min-width:400px;min-height:100px;max-width:400px;position:relative;margin:0;padding:0;">' +
                 '</div>'      + //closes execTareaDiv
           
                 '<button id="' + execScriptButtonId() + '">' +
                 'run' +
                 '</button>' +
              '</div>' +
           

           '</div>' + //closes tab div
           
           //console
           '<b>Console</b><br/>' +
           '<div id="' + consoleId() + '" style="min-width:500px;max-width:550px;min-height:200px;position:relative;margin:0;padding:0;">'  +
           '</div>' +

           
           '</div>' //end div at top.
          ).attr({id:ishmaelWindowId(),title:'ishmael'}).appendTo('body');


         var jsMode = require('ace/mode/javascript').Mode;
         var actionEditor = ace.edit(actionTareaId());
         actionEditor.setTheme('ace/theme/dawn');
         actionEditor.getSession().setMode(new jsMode());
         actionEditor.renderer.setShowGutter(false);
                    
         var consoleEditor = ace.edit(consoleId());
         consoleEditor.setTheme('ace/theme/dawn');
         consoleEditor.getSession().setMode(new jsMode());
         consoleEditor.renderer.setShowGutter(false);
         consoleEditor.setReadOnly(true);

         var execEditor = ace.edit(execTareaId());
         execEditor.setTheme('ace/theme/dawn');
         execEditor.getSession().setMode(new jsMode());
         execEditor.renderer.setShowGutter(false);
         
         
         var $tabs = $('#' +actionFileTabId());
         $tabs.tabs();

         $tabs = $('#' + nearbyScriptedTabId());
         $tabs.tabs();
         
         
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

         var currentlySelectedFile = undefined;
         var allFiles = undefined;

         var allConsoleHistories = undefined;
         
         $('#' + scriptedListId()).change(
             function()
             {
                 //loads the visible id 
                 var val = $('#' + scriptedListId()).val();

                 //updates currentlySelectedVisible and the display of
                 //the files that should be associated with it.
                 changeCurrentlySelectedVisible(val);

                 //if we change which visible we're scripting from the
                 //scripting selection menu, we want that change to be
                 //reflected in the nearby selection list.
                 updateNearbySelection(val.toString());
             });
         
         //set a listener for action list.  whenever select an option,
         //should communicate that to emerson gui, so that can keep
         //track of selected action.
         $('#' + actionListId()).change(
             function()
             {
                 var val = 
                     $('#' + actionListId()).val();

                 //if nothing is selected, then parseInt(val) will
                 //return NaN, which won't be in actionList, and will
                 //have no effect in changeActionText.
                 changeActionText(parseInt(val));
             });

                    
         
         $('#' + nearbyListId()).change(
             function()
             {
                 var val = $('#'+nearbyListId()).val();

                 //we ran into a problem where we don't want
                 //to actually display all nearby visibles.
                 //code now can display an error message to this
                 //effect.  if the user clicks on the error messsage,
                 //then the condition below will be true, and we'll do
                 //nothing.
                 if (val == garbageNearbyVal())
                     return;
                 
                 //updates currentlySelectedVisible and the display of
                 //the files that should be associated with it.
                 changeCurrentlySelectedVisible(val);
                 sirikata.event('addVisible',val);
             });


         /**
          \param {String} newVisible -- id of the new visible to set
          currentlySelectedVisible to.

          This function changes currentlySelectedVisible as well as
          updating the file fields that should be associated with that
          visible.
          */
         function changeCurrentlySelectedVisible(newVisible)
         {
             currentlySelectedVisible = newVisible;
             //ensures that file list gets updated as well.
             redrawFileSelect(allFiles);
             redrawConsole(allConsoleHistories);
         }


         $('#' + renameVisibleButtonId()).click(
             function()
             {
                 if (typeof(currentlySelectedVisible)== 'undefined')
                 {
                     sirikata.event('warn','Please select visible first.');
                     return;
                 }


                 var currentName =
                     $('#' + generateScriptedDivId(currentlySelectedVisible)).html();
                 
                 sirikata.event(
                     'renameVisible',currentlySelectedVisible,currentName);
             });
         
         //when hit save, sends the action text through to controller
         //to save it.
         $('#' + saveActionButtonId()).click(
             function()
             {
                 //no action is selected
                 if ((typeof(currentlySelectedAction) == 'undefined') ||
                     (currentlySelectedAction === null))
                 {
                     sirikata.event('warn','Please select visible and action first.');
                     return;
                 }

                 //var toSaveText = $('#' + actionTareaId()).val();
                 var toSaveText = actionEditor.getSession().getValue();
                 
                 //saving action does not force a redraw.  must
                 //preserve new text in allActions on our end. (takes
                 //care of failure case where save an action, then
                 //click on another action, then return to current
                 //action).
                 allActions[currentlySelectedAction].text = toSaveText;
                 
                 sirikata.event(
                     'saveAction',currentlySelectedAction,toSaveText);
             });


         //saves and executes current action
         $('#' + execActionButtonId()).click(
             function()
             {
                 if ((typeof(currentlySelectedAction)  == 'undefined') ||
                     (typeof(currentlySelectedVisible) == 'undefined'))
                 {
                     sirikata.event('warn','Please select visible or action first.');
                     return;
                 }

                 //see comments in click handler for saveActionButton.
                 var toSaveText = actionEditor.getSession().getValue();
                 
                 allActions[currentlySelectedAction].text = toSaveText;
                 sirikata.event(
                     'saveAndExecuteAction',currentlySelectedAction,
                     toSaveText,currentlySelectedVisible);
             });


         //user asks for new action: we pass event down to
         //scriptingGui, which creates a simpleInput asking for new
         //action name.
         $('#' + newActionButtonId()).click(
             function()
             {
                 sirikata.event('newAction');
             });
         
         $('#' + removeActionButtonId()).click(
             function()
             {
                 if (typeof(currentlySelectedAction) == 'undefined')
                 {
                     sirikata.event('warn', 'Please select action to remove first.');
                     return;                         
                 }

                 
                 sirikata.event('removeAction',currentlySelectedAction);
                 currentlySelectedAction = undefined;
             });


         $('#' + fileSelectId()).change(
             function()
             {
                 currentlySelectedFile = $('#' + fileSelectId()).val();
             });
         

         $('#' + addFileButtonId()).click(
             function()
             {
                 if (typeof(currentlySelectedVisible)=='undefined')
                 {
                     sirikata.event('warn','Please select visible first.');
                     return;                         
                 }


                 sirikata.event('addFile',currentlySelectedVisible);
             });

         $('#' + updateAndSendFileButtonId()).click(
             function()
             {
                 if ((typeof(currentlySelectedVisible) == 'undefined') ||
                     (typeof(currentlySelectedFile) == 'undefined'))
                 {
                     sirikata.event('warn','Please select visible and file first.');                     
                     return;
                 }
                 sirikata.event(
                     'updateAndSendFile',currentlySelectedVisible,
                     currentlySelectedFile);
             });


         $('#' + updateAndSendAllFilesButtonId()).click(
             function()
             {
                 if (typeof(currentlySelectedVisible) == 'undefined')
                 {
                     sirikata.event('warn','Please select visible first.');
                     return;
                 }


                 sirikata.event(
                     'updateAndSendAllFiles',currentlySelectedVisible);
             });

         $('#' + removeFileButtonId()).click(
             function()
             {
                 if ((typeof(currentlySelectedVisible) == 'undefined') ||
                     (typeof(currentlySelectedFile) == 'undefined'))
                 {
                     sirikata.event('warn','Please select visible and file first.');                     
                     return;
                 }

                 sirikata.event(
                     'removeFile',currentlySelectedVisible,currentlySelectedFile);

             });


         $('#' + execScriptButtonId()).click(
             function()
             {
                 if (typeof(currentlySelectedVisible) == 'undefined')
                 {
                     sirikata.event('warn','Please select visible first.');
                     return;
                 }

                 
                 var toExec = execEditor.getSession().getValue();
                 execEditor.getSession().setValue('');
                 sirikata.event(
                     'execScript',currentlySelectedVisible,toExec);
             });

         
         
         var inputWindow = new sirikata.ui.window(
             '#' + ishmaelWindowId(),
                 {
	             autoOpen: true,
	             height: 'auto',
	             width: 600,
                     height: 650,
                     position: 'right'
                 }
             );
         inputWindow.show();


         

         /**
          changes selection in updateNearby to the one associated with
          visibleId (if it's available).  If it's not, then we just
          unselect.

          Right now, we take a performance hit, because the selection
          change that we do here will trigger the .change handler
          associated with nearbyList.  Should still maintain semantics
          though.
          */
         function updateNearbySelection(visibleId)
         {
             var nearbyOption = $('#' + generateNearbyDivId(visibleId));
             if (nearbyOption.size())
             {
                 //means that the element did exist.  now want it to
                 //appear selected in nearbyList.
                 var nearbyVal = nearbyOption.val();
                 $('#' + nearbyListId()).val(nearbyVal);
             }
             else
             {
                 var errMsg =
                     'In updatenearbyselection attempting '+
                     'to set selector to none.';
                 
                 sirikata.log('error',errMsg);
                 $('#' + nearbyListId()).val(null);                     
             }
         }


         /**
          \param {int or undefined} idActSelected.  If an int, then
          looks through record of allActions, and sets action text
          area to that.  If undefined, then clears action text area.
          */
         function changeActionText(idActSelected)
         {
             //takes care of case where you may have stray clicks in
             //action selector
             if (typeof(allActions) == 'undefined')
                 return;
             
             currentlySelectedAction = idActSelected;
             var textToSetTo = '';

             if (typeof(idActSelected) !='undefined')
             {
                 if (! (idActSelected in allActions))
                 {                     
                     sirikata.log('error','action ids out of '      +
                                  'sync with actions in scripting ' +
                                  'gui, or received stray action click.');
                     return;
                 }
                 textToSetTo = allActions[idActSelected].text;
             }
             
             //actually update textarea with correct text
             actionEditor.getSession().setValue(textToSetTo);
         }

         
         /**
          \param {object: <int:std.ScriptingGui.Action>} actionMap

          
          \param {object: <string (visibleId):
              object: <string(filename):string(filename)>>} fileMap --
          keyed by visible id, elements are maps of files that each
          remote visible has on it.

          \param {object: <string visId: actual name>} nameMap

          \param {object: <string visId: array of console history>}
          consoleMap.

          \param {String (visId or presId)} selected (optional).  If
          not undefined, then change currentlySelectedVsible to this
          field before continuing.

          \param {bool} force (optional).  If true, then means that we
          should call .show on ishmael window
          */
         ishmaelRedraw = function(
             nearbyObjs,scriptedObjs,actionMap,fileMap,
             nameMap,consoleMap,selected,force)
         {
             //do not need to use changeCurrentlySelected function,
             //because know that the associated redraws of console +
             //files will be automatically handled in following redraw
             //call.  danger in calling changeCurrentlySelected is that
             //may not have a record of this visible in allFiles yet,
             //causing a problem.
             if (typeof(selected) != 'undefined')
                 currentlySelectedVisible = selected;                     

             redrawNearby(nearbyObjs,nameMap);
             redrawScriptedObjs(scriptedObjs,nameMap);
             redrawActionList(actionMap);
             redrawFileSelect(fileMap);
             redrawConsole(consoleMap);

             if (force)
                 inputWindow.show();
         };

         
         /**
          \param {object: <string (visibleId): string (visibleId)>}
          nearbyObjs -- all objects that are in vicinity.  (can also
          be a string if instead want to write a message explaining
          that displaying all nearby visibles is too onerous).

          \param {object: <string (visId): string (name)>} nameMap
          */
         function redrawNearby(nearbyObjs, nameMap)
         {
             var newHtml = '';

             if (typeof(nearbyObjs) == 'string')
             {
                 //case where we aren't displaying any nearby
                 //visibles, and instead displaying a message to that
                 //effect.
                 newHtml += '<option selected ';
                 newHtml += 'value="' + garbageNearbyVal() + '" ';
                 newHtml += 'id="' + generateNearbyDivId('garbage') + '">';
                 newHtml += nearbyObjs;
                 newHtml += '</option>';
             }
             else
             {
                 //we have a small enough scene that we don't mind the
                 //cost of displaying all nearby visibles.
                 for (var s in nearbyObjs)
                 {
                     if (s===currentlySelectedVisible)
                         newHtml += '<option selected ';
                     else
                         newHtml += '<option ';

                     newHtml += 'value="' + s + '" ';
                     newHtml += 'id="' + generateNearbyDivId(s) + '">';
                     if (s in nameMap)
                         newHtml += nameMap[s];
                     else
                         newHtml += s.substr(0,maxVisIdDispDigits());
                     newHtml += '</option>';
                 }
             }

             $('#' + nearbyListId()).html(newHtml);
         }


         /**
          \param {object: <string (visibleId): string(visibleId)>}
          scriptedObjs -- all objects that we have a scripting
          relationship with.
          */
         function redrawScriptedObjs(scriptedObjs,nameMap)
         {
             var newHtml = '';
             for (var s in scriptedObjs)
             {
                 if (s === currentlySelectedVisible)
                     newHtml += '<option selected ';
                 else
                     newHtml += '<option ';
                 
                 newHtml += 'value="' +s +  '" ';
                 newHtml += 'id="' + generateScriptedDivId(s) + '">';
                 
                 if (s in nameMap)
                     newHtml += nameMap[s];
                 else
                     newHtml += s.substr(0,maxVisIdDispDigits());
                 newHtml += '</option>';
             }
             $('#' + scriptedListId()).html(newHtml);
         }



         function redrawActionList(actionMap)
         {
             var prevCurAct = null;
             if (typeof(currentlySelectedAction) != 'undefined')
             {
                 prevCurAct = allActions[currentlySelectedAction];
                 //update with text that had entered in tarea.
                 //prevCurAct.text = $('#' + actionTareaId()).val();
                 prevCurAct.text = actionEditor.getSession().getValue();
             }

             allActions = actionMap;
             //preserves edits that scripter was potentially making
             //when other action triggered redraw (eg. a new nearby
             //object).
             if (prevCurAct !== null)
                 allActions[prevCurAct.id] = prevCurAct;

             
             var newHtml = '';
             for (var s in actionMap)
             {
                 if (parseInt(s) === parseInt(currentlySelectedAction))
                     newHtml += '<option selected ';
                 else
                     newHtml += '<option ';

                 newHtml += 'value="' + s + '">';
                 newHtml += actionMap[s].name ;
                 newHtml += '</option>';
             }

             $('#' + actionListId()).html(newHtml);
         }

         /**
          \param {object: <string (visibleId):
              object: <string(filename):string(filename)>>} fileMap --
          keyed by visible id, elements are maps of files that each
          remote visible has on it.

          (fileMap can also be undefined if called from
          changeCurrentlySelectedVisible.)
          
          Sets currentlySelectedFile to undefined if have no
          currentlySelectedFile that exists in list of files under
          currentlySelectedVisible.  This ensures that
          changeCurrentlySelectedVisible can call this function
          whenever want to update files after having updated
          currentlySelectedVisible.
          */
         function redrawFileSelect(fileMap)
         {
             if (typeof(allFiles) == 'undefined')
                 allFiles = fileMap;

             //no work to do.
             if(typeof(allFiles) == 'undefined')
                 return;
             
             //no visible selected, and hence no list of files to
             //show.
             if (typeof(currentlySelectedVisible) == 'undefined')
             {
                 currentlySelectedFile = undefined;
                 sirikata.event('warn','Please select visible first.');
                 return;                     
             }


             var visFiles = fileMap[currentlySelectedVisible];
             var newHtml = '';
             var haveSelectedFile = false;
             for (var s in visFiles)
             {
                 if (s === currentlySelectedFile)
                 {
                     newHtml += '<option selected ';
                     haveSelectedFile = true;
                 }
                 else
                     newHtml += '<option ';

                 newHtml += 'value="' + s + '">';
                 newHtml += visFiles[s];
                 newHtml += '</option>';
             }

             if (!haveSelectedFile)
                 currentlySelectedFile = undefined;
             
             $('#' + fileSelectId()).html(newHtml);
         }
         


         /**         
         can be called from ishmaelRedraw (using new consoleHistories)
         or from changeCurrentlySelectedVisible (using old
         allConsoleHistories).
          */
         function redrawConsole(consoleMap)
         {
             allConsoleHistories = consoleMap;
             
             if ((typeof(currentlySelectedVisible) == 'undefined') ||
                (!(currentlySelectedVisible in allConsoleHistories))) 
             {
                 consoleEditor.getSession().setValue('');
                 sirikata.event('warn','Please select visible first.');
                 return;
             }

             
             var consoleEntry =
                 allConsoleHistories[currentlySelectedVisible];
             var consMsg = '';
             
             for (var s in consoleEntry)
             {
                 consMsg += consoleEntry[s];
                 consMsg += '\\n\\n';
             }
             consoleEditor.getSession().setValue(consMsg);
         }


                    //shortcut keybinding: shift+enter executes
                    //instant script if you're focus is on instant
                    //scripter.
                    var handleInstantScriptingTareaKeyUp = function(evt)
                    {
                        //13 represents keycode for enter, submits whatever's in
                        //the text box if user hits enter.
                        if ((evt.keyCode == 13) && (evt.shiftKey))
                            $('#' + execScriptButtonId()).click();
                    };

                    document.getElementById(execTareaId()).onkeyup = handleInstantScriptingTareaKeyUp;


                    //tells the emerson controlling code that ace
                    //libraries are loaded and can now begin doing
                    //handling script input, etc.
                    sirikata.event('amReady');
                    
                }); //closes function waiting for ace to load through lab

         
         @;
         
         returner += '});';         
         return returner;
     }
 })();


//do all further imports for file and gui control.
system.require('action.em');
system.require('console.em');
system.require('controller.em');
system.require('fileManagerElement.em');





