http_server_map.py
------------------

This is a simple HTTP server for acting as the ServerIDMap server with the
core-http plugin's http ServerIDMap implementation. It can read a
fixed set of servers from a file or accept requests from space servers
to set IP addresses dynamically (e.g. to support failover or just
loading up servers on a set of servers with dynamic IPs).

Example
=======

Start using files to specify the mapping of ServerIDs to IP address
and port:

    http_server_map.py --external-file=/path/to/external_server_map --internal-file=/path/to/internal_server_map

Start using a file to specify the mapping of ServerIDs to IP address
and port only for external addresses:

    http_server_map.py --external-file=/path/to/external_server_map

Start using dynamic addresses to specify the mapping of ServerIDs to
IP address and port (note that this currently isn't secure...)

    http_server_map.py --external-dynamic

Get help about http_server_map.py, including more options:

    http_server_map.py --help

Requirements
============

You need at least Python 2.7.
