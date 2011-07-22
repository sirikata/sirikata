
var sender = 'a';
var receiver = 'receiver';
var msgObj = "oooooo  message object";

//sender : msgObj -> receiver;
sender # msgObj >> receiver;

//msgObj -> receiver;
msgObj >> receiver;

//sender:msgObj->receiver;
sender#msgObj>>receiver;

//msgObj->receiver;
msgObj>>receiver;

//sender: { 'a':'object literal' } -> receiver;
sender# { 'a':'object literal' } >> receiver;

var toDo = 'todo';
sender # msgObj >> receiver >> toDo;
