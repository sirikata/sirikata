#!/usr/bin/env python

import argparse
import BaseHTTPServer
import threading, signal, time, sys
import urlparse, random

parser = argparse.ArgumentParser(description='Runs an HTTP server for handling requests mapping ServerIDs to IP addresses and ports.')

parser.add_argument('--external-file', dest='external_file', action='store',
                    type=str, default=None,
                    help='Use the specified file to map external IDs to IPs and ports')
parser.add_argument('--external-dynamic', dest='external_dynamic', action='store_true',
                    help='Make external IPs dynamic, accepting requests from space servers to update their values')
parser.add_argument('--external-path', dest='external_path', action='store',
                    type=str, default='/external',
                    help='Respond to requests at the given URL path with external IPs.')
parser.add_argument('--external-update-path', dest='external_update_path', action='store',
                    type=str, default='/update-external',
                    help='URL path to respond to update requests from space servers on if using dynamic mapping')
parser.add_argument('--external-host', dest='external_host', action='store',
                    type=str, default='',
                    help='Host to listen on for external requests. Can be same as host for internal requests.')
parser.add_argument('--external-port', dest='external_port', action='store',
                    type=int, default=80,
                    help='Port to listen on for external requests. Can be same as port for internal requests.')


parser.add_argument('--internal-file', dest='internal_file', action='store',
                    type=str, default=None,
                    help='Use the specified file to map internal IDs to IPs and ports')
parser.add_argument('--internal-dynamic', dest='internal_dynamic', action='store_true',
                    help='Make internal IPs dynamic, accepting requests from space servers to update their values')
parser.add_argument('--internal-path', dest='internal_path', action='store',
                    type=str, default='/internal',
                    help='Respond to requests at the given URL path with internal IPs.')
parser.add_argument('--internal-update-path', dest='internal_update_path', action='store',
                    type=str, default='/update-internal',
                    help='URL path to respond to update requests from space servers on if using dynamic mapping')
parser.add_argument('--internal-host', dest='internal_host', action='store',
                    type=str, default='',
                    help='Host to listen on for internal requests. Can be same as host for external requests.')
parser.add_argument('--internal-port', dest='internal_port', action='store',
                    type=int, default=80,
                    help='Port to listen on for internal requests. Can be same as port for external requests.')


args = parser.parse_args()

external_thread = None
internal_thread = None

# Data we actually serve up, for either dynamic or pre-set from file
external_data = {}
internal_data = {}
# Servers we're running, used to ensure we only serve back valid requests when running on separate ports
external_server = None
internal_server = None

class LookupHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(self):
        # Figure out which server the request came from and parse the basic request info
        is_internal_server = (self.server is internal_server)
        is_external_server = (self.server is external_server)
        req = urlparse.urlparse( 'http://localhost' + self.path)

        if req.path == args.internal_path and is_internal_server:
            data = internal_data
        elif req.path == args.external_path and is_external_server:
            data = external_data
        else:
            self.send_response(404)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write("Invalid request for this server -- unknown path.")
            return

        query = urlparse.parse_qs(req.query)
        server = None
        if 'server' not in query:
            # Select a random server
            server = random.choice(data.values())
        else:
            if len(query['server']) != 1:
                self.send_response(403)
                self.send_header("Content-Type", "text/plain")
                self.end_headers()
                self.wfile.write("Invalid request -- server improperly specified")
            server_idx = int(query['server'][0])
            if server_idx in data:
                server = data[server_idx]
        if not server:
            self.send_response(404)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write("Requested server ID not found.")

        self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.end_headers()
        self.wfile.write(server)

    def do_POST(self): # For updates only
        # Figure out which server the request came from and parse the basic request info
        is_internal_server = (self.server is internal_server)
        is_external_server = (self.server is external_server)
        req = urlparse.urlparse( 'http://localhost' + self.path)

        if req.path == args.internal_update_path and is_internal_server:
            data = internal_data
            dynamic = args.internal_dynamic
        elif req.path == args.external_update_path and is_external_server:
            data = external_data
            dynamic = args.external_dynamic
        else:
            self.send_response(404)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write("Invalid request for this server -- unknown path.")
            return

        if not dynamic:
            self.send_response(404)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write("Invalid update request -- this server map is not dynamic.")
            return

        query = urlparse.parse_qs(req.query)
        if ('server' not in query or len(query['server']) != 1 or
            'host' not in query or len(query['host']) != 1 or
            'port' not in query or len(query['port']) != 1):
            self.send_response(404)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write("Invalid request for this server -- not all update parameters were properly specified.")
            return

        server_idx = int(query['server'][0])
        server_host = query['host'][0]
        server_port = int(query['port'][0])
        data[server_idx] = server_host + ':' + str(server_port)

        self.send_response(200)
        self.end_headers()

def run_httpd(host, port, is_external, is_internal):
    global internal_server, external_server
    httpd = BaseHTTPServer.HTTPServer((host, port), LookupHandler)
    if is_external: external_server = httpd
    if is_internal: internal_server = httpd
    httpd.serve_forever()

# Figure out how many servers we need
need_internal = args.internal_dynamic or args.internal_file
need_external = args.external_dynamic or args.external_file
same_server = ((args.internal_host == args.external_host) and (args.internal_port == args.external_port))

# Load data if we need it
def load_servermap_file(filename, data_out, port_type):
    with open(filename, 'r') as fp:
        for idx, line in enumerate(fp.readlines()):
            parts = line.split(':')
            if len(parts) != 3:
                print "Couldn't parse servermap file %s, it has an improperly formatted line." % (filename)
                sys.exit(1)
            ip, internal_port, external_port = tuple(parts)
            if port_type == 'internal': data_out[idx+1] = ip + ':' + internal_port
            if port_type == 'external': data_out[idx+1] = ip + ':' + external_port

if args.external_file: load_servermap_file(args.external_file, external_data, 'external')
if args.internal_file: load_servermap_file(args.internal_file, internal_data, 'internal')

# Setup http server args, 1 or 2 servers with specific paths to serve up
server_args = []
if (need_internal and need_external and same_server):
    # Both in one server
    server_args.append([ args.external_host,
                         args.external_port,
                         True, True # External + internal
                         ])
else:
    # Otherwise, add them individually if necessary
    if need_external:
        server_args.append([ args.external_host,
                             args.external_port,
                             True, False # External
                             ])
    if need_internal:
        server_args.append([ args.internal_host,
                             args.internal_port,
                             False, True # Internal
                             ])

if len(server_args) == 0:
    print "No HTTP servers were started. You need to specify at least one server for internal or external and with a file or using dynamic requests."
    sys.exit(1)

# Now start the list we generated
threads = []
for server in server_args:
    thread = threading.Thread(target=lambda: run_httpd(*server) )
    thread.daemon = True
    thread.start()
    threads.append(thread)

# And block until signal requests exit. We set threads into daemon mode, so
# we'll exit as soon as the main thread exits
quit_requested = False
def signal_handler(signum, frame):
    global quit_requested
    quit_requested = True
signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)
if hasattr(signal, 'CTRL_C_EVENT'):
    signal.signal(signal.CTRL_C_EVENT, signal_handler)
else:
    signal.signal(signal.SIGHUP, signal_handler)
while not quit_requested:
    time.sleep(1)
