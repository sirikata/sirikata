
system.require('std/http/http.em');


function callback(success, data)
{
    if (success)
    {
        system.print('\nhttp get successful\n');
        system.prettyprint(data);
    }
    else
        system.print('\nhttp get unsuccessful\n');
}


var url = 'http://www.transcendentalists.com/emerson_quotes.htm';
std.http.basicGet(url,callback);


