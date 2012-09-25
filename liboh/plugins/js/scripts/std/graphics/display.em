// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

system.require('std/movement/movableremote.em');
system.require('std/core/repeatingTimer.em');

/** The DisplayProperties class allows you to modify current display
 *  properties.
 */
std.graphics.DisplayProperties = system.Class.extend({
    init: function(sim, init_cb) {
        this._sim = sim;

        this._sim.addGUITextModule(
            "DisplaySettings", this._guiCode,
            std.core.bind(function(ui) {
                this._ui = ui;
                this._ui.bind("requestDisplayUpdate", std.core.bind(this.requestDisplayUpdate, this));
                this._ui.bind("requestPropertyUpdate", std.core.bind(this.update, this));
                if (init_cb) init_cb();

                this._statsTimer = std.core.RepeatingTimer(1, std.core.bind(this.updateStats, this));
            }, this)
        );
    },

    onReset : function(reset_cb) {
        this._sim.addGUITextModule(
            "DisplaySettings", this._guiCode,
            std.core.bind(function(chat_gui) {
                if (reset_cb) reset_cb();
            }, this));
    },

    toggle: function() {
        this._ui.call('DisplaySettings.toggleVisible');
        this.update();
    },

    update: function() {
        this._ui.call('DisplaySettings.setUIInfo', this._sim.maxObjects(), this._sim.objectPrioritization());
    },

    updateStats: function() {
        var stats = this._sim.stats();
        this._ui.call('DisplaySettings.updateStats',
                      stats.objects.total, stats.objects.unloaded,
                      stats.objects.loading, stats.objects.loaded,
                      stats.assets.total, stats.assets.loading, stats.assets.loaded
                     );
    },

    requestDisplayUpdate: function(settings) {
        this._sim.setMaxObjects(settings.max_objects);
        this._sim.setObjectPrioritization(settings.object_prioritization);
    },

    _guiCode:
@
        sirikata.ui(
            "DisplaySettings",
            function() {
                DisplaySettings = {}; // External interface
                var window; // Our UI window

                var toggleVisible = function() {
                    window.toggle();
                    sirikata.event('requestPropertyUpdate');
                };
                DisplaySettings.toggleVisible = toggleVisible;

                // FIXME having to list these stinks, but we can't pass objects through currently
                var setUIInfo = function(max_objects, object_priority) {
                    // Clear all options
                    $('.object_prioritization input').removeAttr('checked');

                    // Set values
                    $('#max_objects_value').val(max_objects);
                    $('.object_prioritization #' + object_priority).attr('checked', 'checked');

                    // Refresh the UI based on new settings
                    $( "#object_prioritization" ).buttonset('refresh');

                    window.enable();
                };
                DisplaySettings.setUIInfo = setUIInfo;

                var setDisplayInfo = function() {
                    // Sends info from UI back to the script.
                    // Note that we use ui-state-active because the buttonset doesn't
                    // seem to update the underlying input radio's properly. Since
                    // that gets us the lable, we use the 'for' attr
                    var max_objects = parseInt($('#max_objects_value').val());
                    var object_prioritization = $('.object_prioritization label.ui-state-active').attr('for');
                    sirikata.event('requestDisplayUpdate',
                                   { 'max_objects' : max_objects,
                                     'object_prioritization' : object_prioritization
                                   }
                                  );
                    return false;
                };

                // FIXME object instead of params, as above...
                var updateStats = function(
                    total_objects, unloaded_objects, loading_objects, loaded_objects,
                    total_assets, loading_assets, loaded_assets
                ) {
                    $('#renderer_settings_total_objects').text(total_objects);
                    $('#renderer_settings_unloaded_objects').text(unloaded_objects);
                    $('#renderer_settings_loading_objects').text(loading_objects);
                    $('#renderer_settings_loaded_objects').text(loaded_objects);

                    $('#renderer_settings_total_assets').text(total_assets);
                    $('#renderer_settings_loading_assets').text(loading_assets);
                    $('#renderer_settings_loaded_assets').text(loaded_assets);
                };
                DisplaySettings.updateStats = updateStats;

                // UI setup
                $('<div id="display-settings-dialog">' +
                  ' <div id="max_objects">' +
                  ' <form id="max_objects_form">' +
	          '  <input type="number" id="max_objects_value" value="1000" /><label for="max_objects_value">Maximum # of Objects</label>' +
                  ' </form>' +
                  ' </div>' +
                  ' <div id="object_prioritization" class="object_prioritization">' +
                  ' <form>' +
	          '  <input type="radio" id="distance" name="radio" /><label for="distance">Distance</label>' +
	          '  <input type="radio" id="solid_angle" name="radio" /><label for="solid_angle">Solid Angle</label>' +
                  ' </form>' +
                  ' </div>' +
                  ' <div id="stats" class="stats">' +
                  ' <table>' +
                  '  <tr><td>Total Objects</td><td id="renderer_settings_total_objects"></td></tr>' +
                  '  <tr><td>Unloaded Objects</td><td id="renderer_settings_unloaded_objects"></td></tr>' +
                  '  <tr><td>Loading Objects</td><td id="renderer_settings_loading_objects"></td></tr>' +
                  '  <tr><td>Loaded Objects</td><td id="renderer_settings_loaded_objects"></td></tr>' +
                  '  <tr><td></td><td></td></tr>' +
                  '  <tr><td>Total Assets</td><td id="renderer_settings_total_assets"></td></tr>' +
                  '  <tr><td>Loading Assets</td><td id="renderer_settings_loading_assets"></td></tr>' +
                  '  <tr><td>Loaded Assets</td><td id="renderer_settings_loaded_assets"></td></tr>' +
                  ' </div>' +
                  '</div>').attr({title:'Display Settings'}).appendTo('body');

                window = new sirikata.ui.window(
                    '#display-settings-dialog',
                    {
	                height: 'auto',
	                width: 400,
                        autoOpen: false
                    }
                );

                $( "#object_prioritization" ).buttonset();

                $( '#object_prioritization, #collision-mesh' ).click(setDisplayInfo);
                $( '#max_objects_value' ).change(setDisplayInfo);
                $( '#max_objects_form' ).submit(setDisplayInfo);

                // Add Options -> Display Settings
                sirikata.ui.menu({
                    'id' : 'renderer-settings',
                    'text' : 'Renderer Settings',
                    'parent' : 'options-display',
                    'click' : function() {
                        toggleVisible();
                    }
                });

            });
@
});
