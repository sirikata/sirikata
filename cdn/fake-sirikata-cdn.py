import os
import posixpath
import sys
import argparse
import hashlib
import re
import gzip
from StringIO import StringIO
import BaseHTTPServer
import SocketServer

class MultiThreadedHTTPServer(SocketServer.ThreadingMixIn, BaseHTTPServer.HTTPServer):
    pass

def get_gzip(str):
    gzbuff = StringIO()
    gzfile = gzip.GzipFile(fileobj=gzbuff, mode='wb')
    gzfile.write(str)
    gzfile.close()
    return gzbuff.getvalue()

def makeHandler(docroot, cachedir):

    sha256re = re.compile("[a-f0-9]{64}")

    class SirikataHTTPHandler(BaseHTTPServer.BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"
        
        def done_headers(self):
            if self.close_connection:
                self.send_header("Connection", "close")
            self.end_headers()
        
        def return_error(self, code, message):
            self.send_error(code, message)
        
        def do_HEAD(self):
            #normalize the path to get rid of // or ..
            request_path = posixpath.normpath(self.path)
            assert(request_path[0] == "/")
            #stip off leading slash(es)
            while request_path[0] == "/":
                request_path = request_path[1:]
            
            #check for HEAD request of actual file hash
            if len(request_path) == 64 and sha256re.match(request_path) is not None:
                self.handle_file(just_head=True)
                return
            
            #turn into native system path
            native_path = ""
            while request_path != "":
                (request_path, tail) = posixpath.split(request_path)
                native_path = os.path.join(tail, native_path)
            
            #join it with the doc root
            native_path = os.path.join(docroot, native_path)
            #collapse any intermediate .. or .
            native_path = os.path.normpath(native_path)
            #find the common prefix between calculated request and docroot
            request_root = os.path.commonprefix([native_path, docroot])

            #if request goes above docroot, do not serve
            if request_root != docroot:
                return self.return_error(403, "Forbidden")
                
            if not os.path.isfile(native_path):
                return self.return_error(404, "Not Found")
            
            f = open(native_path, "rb")
            buffer = f.read()
            hash = hashlib.sha256(buffer).hexdigest()
            
            cachefile = os.path.join(cachedir, hash)
            print "cachefile = %s" % cachefile
            if not os.path.isfile(cachefile):
                print "writing it out"
                f = open(cachefile, "w")
                f.write(native_path)
                f.close()
            
            self.send_response(200, "OK")
            self.send_header("File-Size", str(len(buffer)))
            self.send_header("Accept-Ranges", "bytes")
            self.send_header("Hash", hash)
            self.done_headers()
        
        def handle_file(self, just_head=False):
            #normalize the path to get rid of // or ..
            request_path = posixpath.normpath(self.path)
            assert(request_path[0] == "/")
            #stip off leading slash(es)
            while request_path[0] == "/":
                request_path = request_path[1:]
            
            if len(request_path) != 64 or sha256re.match(request_path) is None:
                return self.return_error(404, "Not Found")
            
            cachefile = os.path.join(cachedir, request_path)
            if not os.path.isfile(cachefile):
                return self.return_error(404, "Not Found")
            f = open(cachefile, "r")
            actual_file = f.read()
            
            if not os.path.isfile(actual_file):
                return self.return_error(500, "Internal Server Error")
            
            f = open(actual_file, "rb")
            buffer = f.read()
            
            gzipped = False
            if "Accept-Encoding" in self.headers and \
                "gzip" in self.headers["Accept-Encoding"]:
                    gzipped = True
            
            if "Range" in self.headers:
                rangestr = self.headers["Range"]
                
                parts = rangestr.split("=")
                if len(parts) != 2 or parts[0] != "bytes":
                    return self.return_error(400, "Bad Request")
                
                locs = parts[1].split("-")
                if len(locs) != 2:
                    return self.return_error(400, "Bad Request")
                
                try:
                    start = int(locs[0])
                    end = int(locs[1])
                except ValueError:
                    return self.return_error(400, "Bad Request")
                
                if start < 0 or end > len(buffer) or end <= start:
                    return self.return_error(400, "Bad Request")
                
                sliced = buffer[start:end+1]
                if gzipped:
                    sliced = get_gzip(sliced)
                
                self.send_response(200, "OK")
                self.send_header("Accept-Ranges", "bytes")
                self.send_header("Content-Range", "bytes %d-%d/%d" % (start,end,len(buffer)))
                if gzipped:
                    self.send_header("Content-Encoding", "gzip")
                self.send_header("Content-Length", str(len(sliced)))
                self.done_headers()
                if not just_head: self.wfile.write(sliced)
                
            else:
                if gzipped:
                    buffer = get_gzip(buffer)
                    
                self.send_response(200, "OK")
                self.send_header("Accept-Ranges", "bytes")
                if gzipped:
                    self.send_header("Content-Encoding", "gzip")
                self.send_header("Content-Length", str(len(buffer)))
                self.done_headers()
                if not just_head: self.wfile.write(buffer)
        
        def do_GET(self):
            self.handle_file()
            
            
    return SirikataHTTPHandler

def main():
    parser = argparse.ArgumentParser(
        description='Runs a fake sirikata cdn web server on localhost based out of a directory of files.')
    parser.add_argument('directory', help='Root directory of the web server')
    parser.add_argument('--port', '-p', type=int, default=8081, action='store',
                        help='Port the web server should run on. Defaults to 8081.')
    
    args = parser.parse_args()
    
    if not os.path.isdir(args.directory):
        parser.error("Given path '%s' is not a directory." % args.directory)
        
    docroot = os.path.abspath(args.directory)
    
    cachedir = os.path.join(docroot, "_hashcache")
    if not os.path.isdir(cachedir):
        os.mkdir(cachedir)
    
    httpd = MultiThreadedHTTPServer(("localhost", args.port), makeHandler(docroot, cachedir))
    print "Starting web server on localhost:%d with document root '%s'" % (args.port, docroot)
    
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print
        print "exiting"
        sys.exit(0)

if __name__ == '__main__':
    main()
