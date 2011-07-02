


if (typeof(std) === 'undefined')
    std = { };


(function()
 {

     std.http = {    };

     
     /**
      @param {string} url If doesn't include a port, automatically
      append 80 to it.
      @param {function} cb Callback to execute on http success or
      failure.  Takes two arguments: bool indicating whether the
      request was successful or not and a data object that contains
      response headers and data.  On failure data obj is undefined.
      See system.basicHttpGet for full fields of object.
      @param {object} (optional) Each index of object corresponds to
      another field to send in http header.  The values of object
      should be strings and should correspond to the value that
      corresponds to the http request header's field.  Eg:
      additionalHeaders = {'user-agent': 'mozilla'}; Sends a header
      with user-agent: mozilla.  basicGet takes care of appending \r\n
      to headers.
      */
     std.http.basicGet = function(url, cb, additionalHeaders)
     {
         var decodedURL = decodeWebURL(url);
         if (decodedURL == null)
             throw new Error('Error in get: could not decode url passed in as first argument.');

         var headers ="GET  "+ decodedURL.resource + "  HTTP/1.1\r\n";
         headers += "Host: " + decodedURL.host +" \r\n";
         headers += "Accept: */*\r\n";
         headers += "Connection: close\r\n\r\n";

         if (typeof(additionalHeaders) != 'undefined')
         {
             for (var s in additionalHeaders)
                 headers += s +': ' + additionalHeaders[s] + '\r\n' ;                     
         }
                  
         system.basicHttpGet('get', decodedURL.protoHostPort(), headers,cb);
     };


     function URL(proto, host, resource, port)
     {
         this.proto = proto;
         this.host = host;
         this.resource = resource;
         this.port = port;
         
     }

     URL.prototype.protoHostPort = function()
     {
         return this.proto + '://' + this.host + ':' + this.port;
     };
     
     URL.prototype.printParts = function()
     {
         system.print('\n');
         system.print('Proto:' + this.proto);
         system.print('\n');
         system.print('Host:' + this.host);
         system.print('\n');
         system.print('Resource:' + this.resource);
         system.print('\n');
         system.print('Port:' + this.port);
         system.print('\n\n');
     };



     function decodeWebURL(urlIn)
     {
         
         //to get protocol: take everything to the left of ://.  If ://
         //does not exist, return null
         var port = '80';
         var resource = '/';
         
         var protSep = '://';
         var indexProtoEnd =  urlIn.search(protSep);
         if (indexProtoEnd == -1)
             return null;

         var proto = urlIn.substr(0,indexProtoEnd);
         urlIn = urlIn.substr(indexProtoEnd + protSep.length);


         var host = urlIn;
         var havePort = false;
         //if we have a port separator, everything to the left is the host;
         var portSep = ':';
         var indexPortBegin = urlIn.search(portSep);
         if (indexPortBegin != -1)
         {
             havePort = true;
             host = urlIn.substr(0,indexPortBegin);
             urlIn = urlIn.substr(indexPortBegin + portSep.length);
         }

    
         var resourceSep = '/';
         var resourceBeginIndex = urlIn.search(resourceSep);
         if (resourceBeginIndex != -1)
         {
             //port is everything to left of resourceSep
             if (havePort)
                 port = urlIn.substr(0,resourceBeginIndex);
             else
                 host = urlIn.substr(0,resourceBeginIndex);
             
             resource += urlIn.substr(resourceBeginIndex + resourceSep.length);
         }
         else
         {
             if (havePort)
                 port = urlIn;
         }
         return new URL(proto, host, resource, port);
     }

     
 })();

