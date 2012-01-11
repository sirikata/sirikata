sirikata.ui("propertybox", function() {

/*
function setMesh() {
    var setMeshUrl = "meerkat:///jterrace/Insect-Bird.dae/optimized/0/Insect-Bird.dae";
    console.log("Setting mesh to: " + setMeshUrl);
    chrome.send("setmesh", setMeshUrl);
    chrome.send("updateProperties");
    //sirikata.event("setmesh", setMeshUrl);
}
*/

updateProperties = function(xp, yp, zp, xv, yv, zv, xo, yo, zo, wo, xov, yov, zov, wov, scale, distance, meshname, id) {
    $( "#xloc" ).text(function() {
        var type = "X: ";
        return type + xp;
    });
    $( "#yloc" ).text(function() {
        var type = "Y: ";
        return type + yp;
    });
    $( "#zloc" ).text(function() {
        var type = "Z: ";
        return type + zp;
    });
    $( "#xvel" ).text(function() {
        var type = "X: ";
        return type + xv;
    });
    $( "#yvel" ).text(function() {
        var type = "Y: ";
        return type + yv;
    });
    $( "#zvel" ).text(function() {
        var type = "Z: ";
        return type + zv;
    });
    $( "#xori" ).text(function() {
        var type = "X: ";
        return type + xo;
    });
    $( "#yori" ).text(function() {
        var type = "Y: ";
        return type + yo;
    });
    $( "#zori" ).text(function() {
        var type = "Z: ";
        return type + zo;
    });
    $( "#wori" ).text(function() {
        var type = "W: ";
        return type + wo;
    });
    $( "#xorivel" ).text(function() {
        var type = "X: ";
        return type + xov;
    });
    $( "#yorivel" ).text(function() {
        var type = "Y: ";
        return type + yov;
    });
    $( "#zorivel" ).text(function() {
        var type = "Z: ";
        return type + zov;
    });
    $( "#worivel" ).text(function() {
        var type = "W: ";
        return type + wov;
    });    
    $( "#scale" ).text(function() {
        var type = "Scale: ";
        return type + scale;
    });
    $( "#distance" ).text(function() {
        var type = "Distance from Self: ";
        return type + distance;
    });
    $( "#meshval" ).text(function() {
        var type = "mesh: ";
        return type + meshname;
    });
    $( "#propid" ).text(function() {
        var type = "id: ";
        return type + id;
    });
};

    $('<div id="property-box" title="Presence Properties">' +
      '  <div id="propid"/>' +
      '  <br/> ' + 
      '  Position: ' +
      '  <div id="xloc"/>' +
      '  <div id="yloc"/>' +
      '  <div id="zloc"/>' +
      '  <br/> ' +
      '  Velocity: ' +
      '  <div id="xvel"/>' +
      '  <div id="yvel"/>' +
      '  <div id="zvel"/>' +
      '  <br/> ' +
      '  Orientation: ' +
      '  <div id="xori"/>' +
      '  <div id="yori"/>' +
      '  <div id="zori"/>' +
      '  <div id="wori"/>' +
      '  <br/> ' +
      '  Orientation Velocity: ' +
      '  <div id="xorivel"/>' +
      '  <div id="yorivel"/>' +
      '  <div id="zorivel"/>' +
      '  <div id="worivel"/>' +
      '  <br/>' +
      '  <div id="scale"/>' +
      '  <div id="distance"/>' +
      '  <div id="meshval"/>' +
      '</div>').appendTo('body');
    $( "#property-box" ).dialog({
        width: 430,
        height: 'auto',
        modal: false,
        autoOpen: false
    });
    updateProperties(0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,'meshname','idnum');

    //$('#set-mesh-button').click(setMesh);
    var newcsslink = $("<link />").attr({rel:'stylesheet', type:'text/css', href:'../propertybox/propertybox.css'})
    $("head").append(newcsslink);

});