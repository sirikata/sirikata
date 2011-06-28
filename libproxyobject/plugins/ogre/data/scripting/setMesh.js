sirikata.ui(
    "SetMesh",
    function() {


SetMesh = {}; // External interface
var window; // Our UI window

/* toggles the window with the key combo ctrl + j */
var toggleVisible = function() {
    window.toggle();
};
SetMesh.toggleVisible = toggleVisible;

                  $('<div id="set-mesh" title="Select Mesh">' +
          '<div style="position: absolute; left: 10px; top: 10px;"> <img id="img0" src="http://open3dhub.com/download/988525fdc6d8a8baa1e18a9fd796bfe3325c6775540d8a26750f73c5109311a3"></img></div>' +
          '<div style="position: absolute; left: 110px; top: 10px;"><img id="img1" src="http://open3dhub.com/download/988525fdc6d8a8baa1e18a9fd796bfe3325c6775540d8a26750f73c5109311a3"></img></div>' +
          '<div style="position: absolute; left: 210px; top: 10px;"><img id="img2" src="http://open3dhub.com/download/988525fdc6d8a8baa1e18a9fd796bfe3325c6775540d8a26750f73c5109311a3"></img></div>' +
          '<div style="position: absolute; left: 10px; top: 120px;"> <img id="img3" src="http://open3dhub.com/download/988525fdc6d8a8baa1e18a9fd796bfe3325c6775540d8a26750f73c5109311a3"></img></div>' +
          '<div style="position: absolute; left: 110px; top: 120px;"><img id="img4" src="http://open3dhub.com/download/988525fdc6d8a8baa1e18a9fd796bfe3325c6775540d8a26750f73c5109311a3"></img></div>' +
          '<div style="position: absolute; left: 210px; top: 120px;"><img id="img5" src="http://open3dhub.com/download/988525fdc6d8a8baa1e18a9fd796bfe3325c6775540d8a26750f73c5109311a3"></img></div>' +
          '<div style="position: absolute; left: 40px; top: 220px;"><button id="prevButton">Prev</button>' + '<button id="nextButton">Next</button>' + '<input type = "checkbox" id="createNew">Create New Presence?</input>' +
         /* directions on how to use this tool */
          '<div id="meshDescriptionText">Select an object in the world, then click on a mesh thumbnail to set the mesh of selected object - or you may create a new mesh by first clicking the checkbox above.</div>' + '<input value="Enter Keyword" id="search"></input>' + '<button id="searchButton">search</button>' + '</div>' +
          '</div>').appendTo('body');
       
       
       window = new sirikata.ui.window(                     // (2)
           "#set-mesh",
           {
               autoOpen: false,
               width: 310,
               height: 360,
               modal: false
           }
        );

        /* Set Mesh Navigation buttons */
        sirikata.ui.button('#nextButton').click(loadNext);
        sirikata.ui.button('#prevButton').click(loadPrev);
        sirikata.ui.button('#searchButton').click(search);

        /* Set Mesh Thumbnail buttons */
        sirikata.ui.button('#img0').click(function() {
            setMesh('#img0', 0);
        });
        sirikata.ui.button('#img1').click(function() {
            setMesh('#img1', 1);
        });
        sirikata.ui.button('#img2').click(function() {
            setMesh('#img2', 2);
        });
        
        sirikata.ui.button('#img3').click(function() {
            setMesh('#img3', 3);
        });
        sirikata.ui.button('#img4').click(function() {
            setMesh('#img4', 4);
        });
        sirikata.ui.button('#img5').click(function() {
            setMesh('#img5', 5);
        });
        
        /*************************** variables ********************************/

        var nextStart = '';
        var currentMesh = null;
        var numChoices = 6;
        var searchResult = 0;
        displayArray = [];
        allMeshes = [];
 
        /* retrieving model information from open3dhub */
        while (true) {
            $.ajax({
                url: 'http://open3dhub.com/api/browse/' + nextStart, 
                type: 'GET',
                dataType: 'json',
                async: false,
                success: allMeshesFunc,
                crossDomain: true
            });

            if (nextStart == null) break;
        }

        /* Function: allMeshesFunc
         *************************
         * Callback function for ajax request, pushes each thumbnail, full path, and title into a 2-dimensional array
         */
        function allMeshesFunc(dataGotten, status, xhr) {
            nextStart = dataGotten.next_start;
            for (var s in dataGotten.content_items) {
                if (typeof (dataGotten.content_items[s].metadata.types.original.thumbnail) != 'undefined') {
                    allMeshes.push([dataGotten.content_items[s].metadata.types.original.thumbnail, dataGotten.content_items[s].full_path, dataGotten.content_items[s].metadata.title]);
                }
            }
            loadNext();
        }
        

        /* Function: setMesh
        ********************
        * if an object in the world is selected, the object takes on the mesh of the thumbnail clicked afterward. if the "create new presence" option is checked,
        * then clicking a thumbnail creates an object with that mesh.  
        */
        function setMesh(name, offset) {
            toggleBorder(offset);
            var string = (allMeshes[currentMesh + offset][1]);
            var str1 = string.substring(0, string.length - 2);
            var append = str1.substring(str1.lastIndexOf('/'), str1.length);
            var str2 = string.substring(string.length - 2, string.length);
            string = str1 + '/optimized' + str2 + append;
            string = "meerkat://" + string;
            
            if ($('#createNew').is(":checked")) {
                sirikata.event("createNewPres", string);
            } else sirikata.event("setmesh", string);
            
            var msg = "Mesh has been set to " + allMeshes[currentMesh + offset][2];
            $('#meshDescriptionText').text(msg);
        }
      
        /* Function: toggleBorder
         ************************
         * created a border around a thumbnail mesh, given what the offset of 
         * the thumbnail is relative to the first one shown */
        function toggleBorder(offset) {
            var prefix = "#img";
            for (var i = 0; i < numChoices; i++) {
                var name = prefix + i;
                if (i == offset) $(name).attr("border", 5);
                else $(name).attr("border", 0);
            }
        }
        
        /* Function: toggleBorder
         ************************
         * removes border when another button is clicked
         */
        function removeBorders() {
            var prefix = "#img";
            for (var i = 0; i < numChoices; i++) {
                var name = prefix + i;
                $(name).attr("border", 0);
            }
        }



        //Takes a string like this:
        // "d3308661de134382afafce26376c84c9d29a6bd319d9c3f3ac501d17f75f0e60"
        //turns it into this:
        //  http://open3dhub.com/download/d3308661de134382afafce26376c84c9d29a6bd319d9c3f3ac501d17f75f0e60
        function formSource(str) {
            return 'http://open3dhub.com/download/' + str;
        }

        /* Function: loadPrev
        *********************
        * loads the previous page of thumbnails
        */
        function loadPrev() {
            updateDisplayArray(0);
            removeBorders();
            var prefix = "#img";
            for (var i = 0; i < numChoices; i++) {
                var name = prefix + i;
                $(name).attr("src", formSource(allMeshes[(displayArray[i])][0]));
            }
        }
        
        /* Function: loadNext
         ********************
         * loads the next page of thumbnails
         */
        function loadNext() {
            updateDisplayArray(1);
            removeBorders();
            var prefix = "#img";
            for (var i = 0; i < numChoices; i++) {
                var name = prefix + i;
                $(name).attr("src", formSource(allMeshes[(displayArray[i])][0]));
            }
            
        }
        
        function searchTemp() {
            window.hide();
        }
        
    
    /* Function: levenshtein
     ***********************
     * Search function to find minimum edit distance 
     * source info below 
     */    
    function levenshtein (s1, s2) {
    // Calculate Levenshtein distance between two strings  
    // 
    // version: 1103.1210
    // discuss at: http://phpjs.org/functions/levenshtein
    // +            original by: Carlos R. L. Rodrigues (http://www.jsfromhell.com)
    // +            bugfixed by: Onno Marsman
    // +             revised by: Andrea Giammarchi (http://webreflection.blogspot.com)
    // + reimplemented by: Brett Zamir (http://brett-zamir.me)
    // + reimplemented by: Alexander M Beedie
    // *                example 1: levenshtein('Kevin van Zonneveld', 'Kevin van Sommeveld');
    // *                returns 1: 3
    if (s1 == s2) {
        return 0;
    }
 
    var s1_len = s1.length;
    var s2_len = s2.length;
    if (s1_len === 0) {
        return s2_len;
    }
    if (s2_len === 0) {
        return s1_len;
    }
 
    // BEGIN STATIC
    var split = false;
    try {
        split = !('0')[0];
    } catch (e) {
        split = true; // Earlier IE may not support access by string index
    }
    // END STATIC
    if (split) {
        s1 = s1.split('');
        s2 = s2.split('');
    }
 
    var v0 = new Array(s1_len + 1);
    var v1 = new Array(s1_len + 1);
 
    var s1_idx = 0,
        s2_idx = 0,
        cost = 0;
    for (s1_idx = 0; s1_idx < s1_len + 1; s1_idx++) {
        v0[s1_idx] = s1_idx;
    }
    var char_s1 = '',
        char_s2 = '';
    for (s2_idx = 1; s2_idx <= s2_len; s2_idx++) {
        v1[0] = s2_idx;
        char_s2 = s2[s2_idx - 1];
 
        for (s1_idx = 0; s1_idx < s1_len; s1_idx++) {
            char_s1 = s1[s1_idx];
            cost = (char_s1 == char_s2) ? 0 : 1;
            var m_min = v0[s1_idx + 1] + 1;
            var b = v1[s1_idx] + 1;
            var c = v0[s1_idx] + cost;
            if (b < m_min) {
                m_min = b;
            }
            if (c < m_min) {
                m_min = c;
            }
            v1[s1_idx + 1] = m_min;
        }
        var v_tmp = v0;
        v0 = v1;
        v1 = v_tmp;
    }
    return v0[s1_len];
    }

        /* Function: search
         ******************
         * finds the best search result and displays it
         */        
        function search() {
            var searchVal = ($('#search').val()).toLowerCase();
            var i = 0;
            searchResult = currentMesh;
            var min = searchVal.length;
            for (i = 0; i < allMeshes.length; i++) {
                var editDist = levenshtein(searchVal, allMeshes[i][2], 0);
                if (editDist < min && (searchVal.charAt(0) == (allMeshes[i][2].charAt(0)).toLowerCase())) {
                    min = editDist;
                    searchResult = i;
                }
            }
            $('#meshDescriptionText').text("Search result: " + allMeshes[searchResult][2]);
            updateDisplayArray(2);
            var prefix = "#img";
            for (var i = 0; i < numChoices; i++) {
                var name = prefix + i;
                $(name).attr("src", formSource(allMeshes[(displayArray[i])][0]));
            }
            toggleBorder(0);
            $('#search').val('');


        }
        
        /* Function: updateDisplayArray
         ******************************
         * updates the array of thumbnail indexes to be displayed
         */
        function updateDisplayArray(condition) {
            var max = allMeshes.length;
            if (condition == 1) {
                if (currentMesh == null || currentMesh > (max - (2 * numChoices))) currentMesh = 0;
                else currentMesh += numChoices;   
            } else if (condition == 0) {
                if (currentMesh == null || currentMesh < numChoices) currentMesh = max - numChoices;
                else currentMesh -= numChoices;
            } else if (condition == 2) currentMesh = searchResult;
            var count = 0;
            for (var i = currentMesh; i < currentMesh + numChoices; i++) {
               displayArray[count] = i;
               count++;
            } 
        }      
        
    }

);