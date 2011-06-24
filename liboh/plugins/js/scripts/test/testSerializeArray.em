
// var tmpObj ={
//     'msja': [
//         {
//             '__otherData': 'in'
//         },'a']
// };

var tmpObj = {
    'masterAddr': 'mAddr'
};


system.prettyprint(tmpObj);
system.print('\n------------\n');
var tmp = system.serialize(tmpObj);
var da = system.deserialize(tmp);
system.prettyprint(da);
/////////////////////////


