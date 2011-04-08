#!/usr/bin/env python

from wsgiref.simple_server import make_server
import os, os.path, sys
import cgi

site_name = 'http://crashes.sirikata.com'
id_file = 'data/__id'

def server_file(path):
    return os.path.join( os.path.dirname(__file__), path )

def load_file(path):
    f = open(server_file(path), 'rb')
    r = f.read()
    f.close()
    return r

def save_file(path, data):
    f = open(server_file(path), 'wb')
    f.write(data)
    f.close()

def get_id():
    """Get a unique id to assign to a report."""
    if not os.path.exists(server_file(id_file)):
        id = 0
    else:
        id = int( load_file(id_file) )

    save_file(id_file, str(id+1))

    return id

def report(environ, start_response):
    status = '200 OK'
    headers = [('Content-type', 'text/plain')]
    start_response(status, headers)

    if environ['REQUEST_METHOD'].upper() != 'POST':
        return ''
    content_type = environ.get('CONTENT_TYPE', 'application/x-www-form-urlencoded')
    if not (content_type.startswith('application/x-www-form-urlencoded') or
            content_type.startswith('multipart/form-data')):
        return ''
    
    post_form = cgi.FieldStorage(fp=environ['wsgi.input'], environ=environ, keep_blank_values=True)
    if not post_form or not 'dump' in post_form or not 'dumpname' in post_form:
        return ''

    dump = post_form['dump'].value
    dump_name = post_form['dumpname'].value

    id = get_id()

    id_dir = 'data/' + str(id)
    os.makedirs(server_file(id_dir))
    save_file(id_dir + '/' + dump_name, dump)

    return [site_name + '/status/' + str(id)]

def status_page(environ, start_response, id):
    status = '200 OK'
    headers = [('Content-type', 'text/plain')]
    start_response(status, headers)

    id_dir = server_file('data/' + str(id))
    if not os.path.exists(id_dir):
        return 'Report' + str(id) + ' not found.'

    result = []
    dumps = [x for x in os.listdir(id_dir) if x.endswith('.dmp')]
    for x in dumps:
        result += ['Dump: ', x, '\n']
    return result

def edit_page(environ, start_response, id):
    status = '200 OK'
    headers = [('Content-type', 'text/plain')]
    start_response(status, headers)

    return 'edit'

def crashcollector_app(environ, start_response):
    url = environ.get("REDIRECT_TEMPLATEPAGE", environ.get("REDIRECT_URL", None))

    if url.startswith('/report'):
        return report(environ, start_response)
    elif url.startswith('/edit/'):
        url = url.replace('/edit/', '')
        return edit_page(environ, start_response, int(url))
    elif url.startswith('/status/'):
        url = url.replace('/status/', '')
        return status_page(environ, start_response, int(url))

    status = '200 OK'
    headers = [('Content-type', 'text/plain')]
    start_response(status, headers)

    return ['']

application = crashcollector_app

if __name__ == "__main__":
    httpd = make_server('', 8000, crashcollector_app)
    httpd.serve_forever()
