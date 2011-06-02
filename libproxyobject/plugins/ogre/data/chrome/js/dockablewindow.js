/* Sirikata
 * dockablewindow.js
 *
 * Modified from:
 *
 * jQuery UI Dialog @VERSION
 *
 * Copyright 2010, AUTHORS.txt (http://jqueryui.com/about)
 * Dual licensed under the MIT or GPL Version 2 licenses.
 * http://jquery.org/license
 *
 * http://docs.jquery.com/UI/Dialog
 *
 * Depends:
 *      jquery.ui.core.js
 *      jquery.ui.widget.js
 *  jquery.ui.button.js
 *      jquery.ui.draggable.js
 *      jquery.ui.mouse.js
 *      jquery.ui.position.js
 *      jquery.ui.resizable.js
 */


(function( $, undefined ) {

// Note that we continue to use these classes so we get theming for free
var uiDockableWindowClasses =
        'ui-dockablewindow ' +
        'ui-widget ' +
        'ui-widget-content ' +
        'ui-corner-all ';

$.widget("ui.dockablewindow", {
        options: {
                autoOpen: true,
                buttons: {},
                closeOnEscape: true,
                closeText: 'close',
                dialogClass: '',
                docked: false,
                dockText: 'dock',
                draggable: true,
                hide: null,
                height: 'auto',
                maxHeight: false,
                maxWidth: false,
                minHeight: 150,
                minWidth: 150,
                modal: false,
                parent: document.body,
                position: {
                        my: 'center',
                        at: 'center',
                        of: window,
                        collision: 'fit',
                        // ensure that the titlebar is never outside the document
                        using: function(pos) {
                                var topOffset = $(this).css(pos).offset().top;
                                if (topOffset < 0) {
                                        $(this).css('top', pos.top - topOffset);
                                }
                        }
                },
                resizable: true,
                show: null,
                stack: true,
                title: '',
                width: 300,
                zIndex: 1000
        },

        _create: function() {
                this.originalTitle = this.element.attr('title');
                // #5742 - .attr() might return a DOMElement
                if ( typeof this.originalTitle !== "string" ) {
                        this.originalTitle = "";
                }

                this.options.title = this.options.title || this.originalTitle;
                var self = this,
                        options = self.options,

                        title = options.title || '&#160;',
                        titleId = $.ui.dockablewindow.getTitleId(self.element),

                        uiDockableWindow = (self.uiDockableWindow = $('<div></div>'))
                                .appendTo(self._getParent())
                                .hide()
                                .addClass(uiDockableWindowClasses + options.dialogClass)
                                .css({
                                        zIndex: options.zIndex
                                })
                                // setting tabIndex makes the div focusable
                                // setting outline to 0 prevents a border on focus in Mozilla
                                .attr('tabIndex', -1).css('outline', 0).keydown(function(event) {
                                        if (options.closeOnEscape && event.keyCode &&
                                                event.keyCode === $.ui.keyCode.ESCAPE) {

                                                self.close(event);
                                                event.preventDefault();
                                        }
                                })
                                .attr({
                                        role: 'dialog',
                                        'aria-labelledby': titleId
                                })
                                .mousedown(function(event) {
                                        self.moveToTop(false, event);
                                }),

                        uiDockableWindowContent = self.element
                                .show()
                                .removeAttr('title')
                                .addClass(
                                        'ui-dockablewindow-content ' +
                                        'ui-widget-content')
                                .appendTo(uiDockableWindow),

                        uiDockableWindowTitlebar = (self.uiDockableWindowTitlebar = $('<div></div>'))
                                .addClass(
                                        'ui-dockablewindow-titlebar ' +
                                        'ui-widget-header ' +
                                        'ui-corner-all ' +
                                        'ui-helper-clearfix'
                                )
                                .prependTo(uiDockableWindow),

                        uiDockableWindowTitlebarClose = $('<a href="#"></a>')
                                .addClass(
                                        'ui-dockablewindow-titlebar-close ' +
                                        'ui-corner-all'
                                )
                                .attr('role', 'button')
                                .hover(
                                        function() {
                                                uiDockableWindowTitlebarClose.addClass('ui-state-hover');
                                        },
                                        function() {
                                                uiDockableWindowTitlebarClose.removeClass('ui-state-hover');
                                        }
                                )
                                .focus(function() {
                                        uiDockableWindowTitlebarClose.addClass('ui-state-focus');
                                })
                                .blur(function() {
                                        uiDockableWindowTitlebarClose.removeClass('ui-state-focus');
                                })
                                .click(function(event) {
                                        self.close(event);
                                        return false;
                                })
                                .appendTo(uiDockableWindowTitlebar),

                        uiDockableWindowTitlebarCloseText = (self.uiDockableWindowTitlebarCloseText = $('<span></span>'))
                                .addClass(
                                        'ui-icon ' +
                                        'ui-icon-closethick'
                                )
                                .text(options.closeText)
                                .appendTo(uiDockableWindowTitlebarClose),


                        uiDockableWindowTitlebarDock = $('<a href="#"></a>')
                                .addClass(
                                        'ui-dockablewindow-titlebar-dock ' +
                                        'ui-corner-all'
                                )
                                .attr('role', 'button')
                                .hover(
                                        function() {
                                                uiDockableWindowTitlebarDock.addClass('ui-state-hover');
                                        },
                                        function() {
                                                uiDockableWindowTitlebarDock.removeClass('ui-state-hover');
                                        }
                                )
                                .focus(function() {
                                        uiDockableWindowTitlebarDock.addClass('ui-state-focus');
                                })
                                .blur(function() {
                                        uiDockableWindowTitlebarDock.removeClass('ui-state-focus');
                                })
                                .click(function(event) {
                                        self.toggleDock(event);
                                        return false;
                                })
                                .appendTo(uiDockableWindowTitlebar),

                        uiDockableWindowTitlebarDockText = (self.uiDockableWindowTitlebarDockText = $('<span></span>'))
                                .addClass(
                                        'ui-icon ' +
                                        'ui-icon-pin-s'
                                )
                                .text(options.dockText)
                                .appendTo(uiDockableWindowTitlebarDock),

                        uiDockableWindowTitle = $('<span></span>')
                                .addClass('ui-dockablewindow-title')
                                .attr('id', titleId)
                                .html(title)
                                .prependTo(uiDockableWindowTitlebar);

                //handling of deprecated beforeclose (vs beforeClose) option
                //Ticket #4669 http://dev.jqueryui.com/ticket/4669
                //TODO: remove in 1.9pre
                if ($.isFunction(options.beforeclose) && !$.isFunction(options.beforeClose)) {
                        options.beforeClose = options.beforeclose;
                }

                uiDockableWindowTitlebar.find("*").add(uiDockableWindowTitlebar).disableSelection();

                self._createButtons(options.buttons);
                self._isOpen = false;

                if ($.fn.bgiframe) {
                        uiDockableWindow.bgiframe();
                }
        },

        _init: function() {
            this._updateDock();

                if ( this.options.autoOpen ) {
                        this.open();
                }
        },

        destroy: function() {
                var self = this;

                if (self.overlay) {
                        self.overlay.destroy();
                }
                self.uiDockableWindow.hide();
                self.element
                        .unbind('.dialog')
                        .removeData('dialog')
                        .removeClass('ui-dockablewindow-content ui-widget-content')
                        .hide().appendTo('body');
                self.uiDockableWindow.remove();

                if (self.originalTitle) {
                        self.element.attr('title', self.originalTitle);
                }

                return self;
        },

        widget: function() {
                return this.uiDockableWindow;
        },

        close: function(event) {
                var self = this,
                        maxZ;

                if (false === self._trigger('beforeClose', event)) {
                        return;
                }

                if (self.overlay) {
                        self.overlay.destroy();
                }
                self.uiDockableWindow.unbind('keypress.ui-dockablewindow');

                self._isOpen = false;

                if (self.options.hide) {
                        self.uiDockableWindow.hide(self.options.hide, function() {
                                self._trigger('close', event);
                        });
                } else {
                        self.uiDockableWindow.hide();
                        self._trigger('close', event);
                }

                $.ui.dockablewindow.overlay.resize();

                // adjust the maxZ to allow other modal dialogs to continue to work (see #4309)
                if (self.options.modal) {
                        maxZ = 0;
                        $('.ui-dockablewindow').each(function() {
                                if (this !== self.uiDockableWindow[0]) {
                                        maxZ = Math.max(maxZ, $(this).css('z-index'));
                                }
                        });
                        $.ui.dockablewindow.maxZ = maxZ;
                }

                return self;
        },

        isOpen: function() {
                return this._isOpen;
        },

        // the force parameter allows us to move modal dialogs to their correct
        // position on open
        moveToTop: function(force, event) {
                var self = this,
                        options = self.options,
                        saveScroll;

                if ((options.modal && !force) ||
                        (!options.stack && !options.modal)) {
                        return self._trigger('focus', event);
                }

                if (options.zIndex > $.ui.dockablewindow.maxZ) {
                        $.ui.dockablewindow.maxZ = options.zIndex;
                }
                if (self.overlay) {
                        $.ui.dockablewindow.maxZ += 1;
                        self.overlay.$el.css('z-index', $.ui.dockablewindow.overlay.maxZ = $.ui.dockablewindow.maxZ);
                }

                //Save and then restore scroll since Opera 9.5+ resets when parent z-Index is changed.
                //  http://ui.jquery.com/bugs/ticket/3193
                saveScroll = { scrollTop: self.element.attr('scrollTop'), scrollLeft: self.element.attr('scrollLeft') };
                $.ui.dockablewindow.maxZ += 1;
                self.uiDockableWindow.css('z-index', $.ui.dockablewindow.maxZ);
                self.element.attr(saveScroll);
                self._trigger('focus', event);

                return self;
        },

        open: function() {
                if (this._isOpen) { return; }

                var self = this,
                        options = self.options,
                        uiDockableWindow = self.uiDockableWindow;

                self.overlay = options.modal ? new $.ui.dockablewindow.overlay(self) : null;
                if (uiDockableWindow.next().length) {
                        uiDockableWindow.appendTo(this._getParent());
                }
                self._size();
                self._position(options.position);
                uiDockableWindow.show(options.show);
                self.moveToTop(true);

                // prevent tabbing out of modal dialogs
                if (options.modal) {
                        uiDockableWindow.bind('keypress.ui-dockablewindow', function(event) {
                                if (event.keyCode !== $.ui.keyCode.TAB) {
                                        return;
                                }

                                var tabbables = $(':tabbable', this),
                                        first = tabbables.filter(':first'),
                                        last  = tabbables.filter(':last');

                                if (event.target === last[0] && !event.shiftKey) {
                                        first.focus(1);
                                        return false;
                                } else if (event.target === first[0] && event.shiftKey) {
                                        last.focus(1);
                                        return false;
                                }
                        });
                }

                // set focus to the first tabbable element in the content area or the first button
                // if there are no tabbable elements, set focus on the dialog itself
                $(self.element.find(':tabbable').get().concat(
                        uiDockableWindow.find('.ui-dockablewindow-buttonpane :tabbable').get().concat(
                                uiDockableWindow.get()))).eq(0).focus();

                self._isOpen = true;
                self._trigger('open');

                return self;
        },

        _createButtons: function(buttons) {
                var self = this,
                        hasButtons = false,
                        uiDockableWindowButtonPane = $('<div></div>')
                                .addClass(
                                        'ui-dockablewindow-buttonpane ' +
                                        'ui-widget-content ' +
                                        'ui-helper-clearfix'
                                ),
                        uiButtonSet = $( "<div></div>" )
                                .addClass( "ui-dockablewindow-buttonset" )
                                .appendTo( uiDockableWindowButtonPane );

                // if we already have a button pane, remove it
                self.uiDockableWindow.find('.ui-dockablewindow-buttonpane').remove();

                if (typeof buttons === 'object' && buttons !== null) {
                        $.each(buttons, function() {
                                return !(hasButtons = true);
                        });
                }
                if (hasButtons) {
                        $.each(buttons, function(name, props) {
                                props = $.isFunction( props ) ?
                                        { click: props, text: name } :
                                        props;
                                var button = $('<button></button>', props)
                                        .unbind('click')
                                        .click(function() {
                                                props.click.apply(self.element[0], arguments);
                                        })
                                        .appendTo(uiButtonSet);
                                if ($.fn.button) {
                                        button.button();
                                }
                        });
                        uiDockableWindowButtonPane.appendTo(self.uiDockableWindow);
                }
        },

        _makeDraggable: function() {
                var self = this,
                        options = self.options,
                        doc = $(document),
                        heightBeforeDrag;

                function filteredUi(ui) {
                        return {
                                position: ui.position,
                                offset: ui.offset
                        };
                }

                self.uiDockableWindow.draggable({
                        cancel: '.ui-dockablewindow-content, .ui-dockablewindow-titlebar-dock, .ui-dockablewindow-titlebar-close',
                        handle: '.ui-dockablewindow-titlebar',
                        containment: 'parent',
                        start: function(event, ui) {
                                heightBeforeDrag = options.height === "auto" ? "auto" : $(this).height();
                                $(this).height($(this).height()).addClass("ui-dockablewindow-dragging");
                                self._trigger('dragStart', event, filteredUi(ui));
                        },
                        drag: function(event, ui) {
                                self._trigger('drag', event, filteredUi(ui));
                        },
                        stop: function(event, ui) {
                                options.position = [ui.position.left - doc.scrollLeft(),
                                        ui.position.top - doc.scrollTop()];
                                $(this).removeClass("ui-dockablewindow-dragging").height(heightBeforeDrag);
                                self._trigger('dragStop', event, filteredUi(ui));
                                $.ui.dockablewindow.overlay.resize();
                        }
                });

            this._isDraggable = true;
        },

        _unmakeDraggable: function() {
            if (this._isDraggable) {
                this._isDraggable = false;
                this.uiDockableWindow.draggable('destroy');
            }
        },

        _makeResizable: function(handles) {
                handles = (handles === undefined ? this.options.resizable : handles);
                var self = this,
                        options = self.options,
                        // .ui-resizable has position: relative defined in the stylesheet
                        // but dialogs have to use absolute or fixed positioning
                        position = self.uiDockableWindow.css('position'),
                        resizeHandles = (typeof handles === 'string' ?
                                handles :
                                'n,e,s,w,se,sw,ne,nw'
                        );

                function filteredUi(ui) {
                        return {
                                originalPosition: ui.originalPosition,
                                originalSize: ui.originalSize,
                                position: ui.position,
                                size: ui.size
                        };
                }

                self.uiDockableWindow.resizable({
                        cancel: '.ui-dockablewindow-content',
                        containment: 'parent',
                        alsoResize: self.element,
                        maxWidth: options.maxWidth,
                        maxHeight: options.maxHeight,
                        minWidth: options.minWidth,
                        minHeight: self._minHeight(),
                        handles: resizeHandles,
                        start: function(event, ui) {
                                $(this).addClass("ui-dockablewindow-resizing");
                                self._trigger('resizeStart', event, filteredUi(ui));
                        },
                        resize: function(event, ui) {
                                self._trigger('resize', event, filteredUi(ui));
                        },
                        stop: function(event, ui) {
                                $(this).removeClass("ui-dockablewindow-resizing");
                                options.height = $(this).height();
                                options.width = $(this).width();
                                self._trigger('resizeStop', event, filteredUi(ui));
                                $.ui.dockablewindow.overlay.resize();
                        }
                })
                .css('position', position)
                .find('.ui-resizable-se').addClass('ui-icon ui-icon-grip-diagonal-se');
        },

        _getParent: function() {
            if (this.options.docked)
                return this.options.parent;
            else
                return document.body;
        },

        toggleDock: function() {
            this._setOption('docked', !this.options.docked);
        },

        _updateDock: function() {
            // Make sure we have classes setup properly
            this.uiDockableWindow.removeClass("ui-dockablewindow-docked ui-dockablewindow-undocked");
            if (this.options.docked) {
                this.uiDockableWindow.addClass("ui-dockablewindow-docked");
                this._setOption('width', $(this._getParent()).width());
            }
            else
                this.uiDockableWindow.addClass("ui-dockablewindow-undocked");

            // NOTE: This is critical! The dragger ends up setting the
            // position attribute directly and if we don't clear it,
            // then it will get stuck in absolute positioning.
            this.uiDockableWindow.css('position', '');

            // Make sure we've got parents setup properly for drag + resize
            if (this.options.draggable && $.fn.draggable) {
                this._unmakeDraggable();
                // Only re-enable if they are undocked.
                if (!this.options.docked)
                    this._makeDraggable();
            }
            if (this.options.resizable && $.fn.resizable) {
                this._makeResizable();
            }

            // Reparent and update position
            if (self.overlay) {
                self.overlay.appendTo(this._getParent());
            }
            this.uiDockableWindow.appendTo(this._getParent());
            this._position(this.options.position);
        },

        _minHeight: function() {
                var options = this.options;

                if (options.height === 'auto') {
                        return options.minHeight;
                } else {
                        return Math.min(options.minHeight, options.height);
                }
        },

        _position: function(position) {
            // If we're working within a parent container, handle this differently than normal dialogs
            if (this._getParent() !== document.body) {
                this.uiDockableWindow.css({ top: 0, left: 0 });
                return;
            }

                var myAt = [],
                        offset = [0, 0],
                        isVisible;

                if (position) {
                        // deep extending converts arrays to objects in jQuery <= 1.3.2 :-(
        //              if (typeof position == 'string' || $.isArray(position)) {
        //                      myAt = $.isArray(position) ? position : position.split(' ');

                        if (typeof position === 'string' || (typeof position === 'object' && '0' in position)) {
                                myAt = position.split ? position.split(' ') : [position[0], position[1]];
                                if (myAt.length === 1) {
                                        myAt[1] = myAt[0];
                                }

                                $.each(['left', 'top'], function(i, offsetPosition) {
                                        if (+myAt[i] === myAt[i]) {
                                                offset[i] = myAt[i];
                                                myAt[i] = offsetPosition;
                                        }
                                });

                                position = {
                                        my: myAt.join(" "),
                                        at: myAt.join(" "),
                                        offset: offset.join(" ")
                                };
                        }

                        position = $.extend({}, $.ui.dockablewindow.prototype.options.position, position);
                } else {
                        position = $.ui.dockablewindow.prototype.options.position;
                }

                // need to show the dialog to get the actual offset in the position plugin
                isVisible = this.uiDockableWindow.is(':visible');
                if (!isVisible) {
                        this.uiDockableWindow.show();
                }
                this.uiDockableWindow
                        // workaround for jQuery bug #5781 http://dev.jquery.com/ticket/5781
                        .css({ top: 0, left: 0 })
                        .position(position);
                if (!isVisible) {
                        this.uiDockableWindow.hide();
                }
        },

        _setOption: function(key, value){
                var self = this,
                        uiDockableWindow = self.uiDockableWindow,
                        isResizable = uiDockableWindow.is(':data(resizable)'),
                        resize = false,
                        docked = false;

                switch (key) {
                        //handling of deprecated beforeclose (vs beforeClose) option
                        //Ticket #4669 http://dev.jqueryui.com/ticket/4669
                        //TODO: remove in 1.9pre
                        case "beforeclose":
                                key = "beforeClose";
                                break;
                        case "buttons":
                                self._createButtons(value);
                                resize = true;
                                break;
                        case "closeText":
                                // convert whatever was passed in to a string, for text() to not throw up
                                self.uiDockableWindowTitlebarCloseText.text("" + value);
                                break;
                        case "dialogClass":
                                uiDockableWindow
                                        .removeClass(self.options.dialogClass)
                                        .addClass(uiDockableWindowClasses + value);
                                break;
                        case "disabled":
                                if (value) {
                                        uiDockableWindow.addClass('ui-dockablewindow-disabled');
                                } else {
                                        uiDockableWindow.removeClass('ui-dockablewindow-disabled');
                                }
                                break;
                        case "docked":
                                docked = true;
                                break;
                        case "draggable":
                                if (value) {
                                        self._makeDraggable();
                                } else {
                                        self._unmakeDraggable();
                                }
                                self.options.draggable = value;
                                break;
                        case "height":
                                resize = true;
                                break;
                        case "maxHeight":
                                if (isResizable) {
                                        uiDockableWindow.resizable('option', 'maxHeight', value);
                                }
                                resize = true;
                                break;
                        case "maxWidth":
                                if (isResizable) {
                                        uiDockableWindow.resizable('option', 'maxWidth', value);
                                }
                                resize = true;
                                break;
                        case "minHeight":
                                if (isResizable) {
                                        uiDockableWindow.resizable('option', 'minHeight', value);
                                }
                                resize = true;
                                break;
                        case "minWidth":
                                if (isResizable) {
                                        uiDockableWindow.resizable('option', 'minWidth', value);
                                }
                                resize = true;
                                break;
                        case "position":
                                self._position(value);
                                break;
                        case "resizable":
                                // currently resizable, becoming non-resizable
                                if (isResizable && !value) {
                                        uiDockableWindow.resizable('destroy');
                                }

                                // currently resizable, changing handles
                                if (isResizable && typeof value === 'string') {
                                        uiDockableWindow.resizable('option', 'handles', value);
                                }

                                // currently non-resizable, becoming resizable
                                if (!isResizable && value !== false) {
                                        self._makeResizable(value);
                                }
                                break;
                        case "title":
                                // convert whatever was passed in o a string, for html() to not throw up
                                $(".ui-dockablewindow-title", self.uiDockableWindowTitlebar).html("" + (value || '&#160;'));
                                break;
                        case "width":
                                resize = true;
                                break;
                }

                $.Widget.prototype._setOption.apply(self, arguments);
                if (resize) {
                        self._size();
                }
            if (docked)
                self._updateDock();
        },

        _size: function() {
                /* If the user has resized the dialog, the .ui-dockablewindow and .ui-dockablewindow-content
                 * divs will both have width and height set, so we need to reset them
                 */
                var options = this.options,
                        nonContentHeight;

                // reset content sizing
                // hide for non content measurement because height: 0 doesn't work in IE quirks mode (see #4350)
                this.element.css({
                        width: 'auto',
                        minHeight: 0,
                        height: 0
                });

                if (options.minWidth > options.width) {
                        options.width = options.minWidth;
                }

                // reset wrapper sizing
                // determine the height of all the non-content elements
                nonContentHeight = this.uiDockableWindow.css({
                                height: 'auto',
                                width: options.width
                        })
                        .height();

                this.element
                        .css(options.height === 'auto' ? {
                                        minHeight: Math.max(options.minHeight - nonContentHeight, 0),
                                        height: $.support.minHeight ? 'auto' :
                                                Math.max(options.minHeight - nonContentHeight, 0)
                                } : {
                                        minHeight: 0,
                                        height: Math.max(options.height - nonContentHeight, 0)
                        })
                        .show();

                if (this.uiDockableWindow.is(':data(resizable)')) {
                        this.uiDockableWindow.resizable('option', 'minHeight', this._minHeight());
                }
        }
});

$.extend($.ui.dockablewindow, {
        version: "@VERSION",

        uuid: 0,
        maxZ: 0,

        getTitleId: function($el) {
                var id = $el.attr('id');
                if (!id) {
                        this.uuid += 1;
                        id = this.uuid;
                }
                return 'ui-dockablewindow-title-' + id;
        },

        overlay: function(dockablewindow) {
                this.$el = $.ui.dockablewindow.overlay.create(dockablewindow);
        }
});

$.extend($.ui.dockablewindow.overlay, {
        instances: [],
        // reuse old instances due to IE memory leak with alpha transparency (see #5185)
        oldInstances: [],
        maxZ: 0,
        events: $.map('focus,mousedown,mouseup,keydown,keypress,click'.split(','),
                function(event) { return event + '.dialog-overlay'; }).join(' '),
        create: function(dockablewindow) {
                if (this.instances.length === 0) {
                        // prevent use of anchors and inputs
                        // we use a setTimeout in case the overlay is created from an
                        // event that we're going to be cancelling (see #2804)
                        setTimeout(function() {
                                // handle $(el).dialog().dialog('close') (see #4065)
                                if ($.ui.dockablewindow.overlay.instances.length) {
                                        $(document).bind($.ui.dockablewindow.overlay.events, function(event) {
                                                // stop events if the z-index of the target is < the z-index of the overlay
                                                // we cannot return true when we don't want to cancel the event (#3523)
                                                if ($(event.target).zIndex() < $.ui.dockablewindow.overlay.maxZ) {
                                                        return false;
                                                }
                                        });
                                }
                        }, 1);

                        // allow closing by pressing the escape key
                        $(document).bind('keydown.dialog-overlay', function(event) {
                                if (dockablewindow.options.closeOnEscape && event.keyCode &&
                                        event.keyCode === $.ui.keyCode.ESCAPE) {

                                        dockablewindow.close(event);
                                        event.preventDefault();
                                }
                        });

                        // handle window resize
                        $(window).bind('resize.dialog-overlay', $.ui.dockablewindow.overlay.resize);
                }

                var $el = (this.oldInstances.pop() || $('<div></div>').addClass('ui-widget-overlay'))
                        .appendTo(this._getParent())
                        .css({
                                width: this.width(),
                                height: this.height()
                        });

                if ($.fn.bgiframe) {
                        $el.bgiframe();
                }

                this.instances.push($el);
                return $el;
        },

        destroy: function($el) {
                this.oldInstances.push(this.instances.splice($.inArray($el, this.instances), 1)[0]);

                if (this.instances.length === 0) {
                        $([document, window]).unbind('.dialog-overlay');
                }

                $el.remove();

                // adjust the maxZ to allow other modal dialogs to continue to work (see #4309)
                var maxZ = 0;
                $.each(this.instances, function() {
                        maxZ = Math.max(maxZ, this.css('z-index'));
                });
                this.maxZ = maxZ;
        },

        height: function() {
                var scrollHeight,
                        offsetHeight;
                // handle IE 6
                if ($.browser.msie && $.browser.version < 7) {
                        scrollHeight = Math.max(
                                document.documentElement.scrollHeight,
                                document.body.scrollHeight
                        );
                        offsetHeight = Math.max(
                                document.documentElement.offsetHeight,
                                document.body.offsetHeight
                        );

                        if (scrollHeight < offsetHeight) {
                                return $(window).height() + 'px';
                        } else {
                                return scrollHeight + 'px';
                        }
                // handle "good" browsers
                } else {
                        return $(document).height() + 'px';
                }
        },

        width: function() {
                var scrollWidth,
                        offsetWidth;
                // handle IE 6
                if ($.browser.msie && $.browser.version < 7) {
                        scrollWidth = Math.max(
                                document.documentElement.scrollWidth,
                                document.body.scrollWidth
                        );
                        offsetWidth = Math.max(
                                document.documentElement.offsetWidth,
                                document.body.offsetWidth
                        );

                        if (scrollWidth < offsetWidth) {
                                return $(window).width() + 'px';
                        } else {
                                return scrollWidth + 'px';
                        }
                // handle "good" browsers
                } else {
                        return $(document).width() + 'px';
                }
        },

        resize: function() {
                /* If the dialog is draggable and the user drags it past the
                 * right edge of the window, the document becomes wider so we
                 * need to stretch the overlay. If the user then drags the
                 * dialog back to the left, the document will become narrower,
                 * so we need to shrink the overlay to the appropriate size.
                 * This is handled by shrinking the overlay before setting it
                 * to the full document size.
                 */
                var $overlays = $([]);
                $.each($.ui.dockablewindow.overlay.instances, function() {
                        $overlays = $overlays.add(this);
                });

                $overlays.css({
                        width: 0,
                        height: 0
                }).css({
                        width: $.ui.dockablewindow.overlay.width(),
                        height: $.ui.dockablewindow.overlay.height()
                });
        }
});

$.extend($.ui.dockablewindow.overlay.prototype, {
        destroy: function() {
                $.ui.dockablewindow.overlay.destroy(this.$el);
        }
});

}(jQuery));
