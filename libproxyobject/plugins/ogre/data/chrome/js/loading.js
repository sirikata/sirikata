(function() {
     var win_height = $(window).height(), win_width = $(window).width();
     var load_panel = $('<div id="loadpanel"></div>');
     load_panel.css(
         {
             'background-color' : '#000',
             'position' : 'absolute',
             'top' : '0px',
             'left' : '0px',
             'height' : win_height,
             'width' : win_width,
             'z-index' : 9999
         }
     );
     load_panel.appendTo('body');

     var load_text = $('<div style="width: 500px;">Loading...</div>');
     load_text.appendTo(load_panel);
     load_text.css(
         {
             "color" : "#FFF",
             "position" :"absolute",
             "top" : ((win_height - load_text.outerHeight()) / 2) + $(window).scrollTop() + "px",
             "left" : ((win_width - load_text.outerWidth()) / 2) + $(window).scrollLeft() + "px",
             'text-align' : 'center'
         }
     );

     hideLoadScreen = function() {
         $('#loadpanel').remove();
         $('body').css({'background-color':'transparent'});

         // Clear out hideLoadingScreen so we'll GC everything in here
         // and since the loading screen won't be pulled up again
         delete hideLoadingScreen;
     };
 }
)();
