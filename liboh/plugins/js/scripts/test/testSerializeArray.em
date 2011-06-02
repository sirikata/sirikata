

// var tmpObj ={
//     0: ['c',
//         {
//             '__someData': 'inner',
//             '__otherData': 'inner2'
//         }],
//     '__someData'  : 'outer',
//     '__otherData' : 'outer2',
//     'final': 'final'
// };

// var tmpObj ={
//     0: ['c',
//         {
//             '__otherData': 'inner2'
//         }],
//     '__otherData' : 'outer2'
// };


// var tmpObj ={
//     0: ['c',
//         {
//             '__otherData': 'inner2'
//         }]
// };

// var tmpObj ={
//     'a': ['c',
//         {
//             '__otherData': 'inner2'
//         }]
// };


// var tmpObj ={
//     'a': {'c':'c',
//         'd': {
//             '__otherData': 'inner2'
//         }}
// };


// var tmpObj ={
//     'a': ['c',
//          {
//              'm':'m'
//         }]
// };


// var tmpObj ={
//     'msja': ['d',
//         {
//             '__otherData': 'in'
//         }]
// };


var tmpObj ={
    'msja': [
        {
            '__otherData': 'in'
        },'a']
};


// var tmpObj ={
//     'msja': {
//         0: {
//             '__otherData': 'in'
//         },
//         1: 'a'}
// };




system.prettyprint(tmpObj);
system.print('\n------------\n');
var tmp = system.serialize(tmpObj);
var da = system.deserialize(tmp);
system.prettyprint(da);
/////////////////////////


