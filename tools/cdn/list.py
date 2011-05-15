#!/usr/bin/env python
#
# list.py - Grabs listings from a CDN and prints them to stdout.
#
# list.py url [-n num_assets] [-r report_value]
#
# list.py is a utility for grabbing listings of content from a CDN
# using its JSON API and piping them to stdout. On its own, it isn't
# too useful; however, using it as a primitive in a pipeline of tools
# can be very powerful. For example, to validate recent models we
# might do something like
#
#  list.py http://mycdn.com | xargs -I {} sh -c "curl -s {} | OpenColladaValidator"
#
# This gets (the default) 10 items and uses xargs and CURL to download each and
# pipe it into the OpenColladaValidator.
#
#  -n num_assets - The number of asset listings to grab from the
#     CDN. Defaults to 10.
#
#  -r report_value - The value to report for each item. Options are:
#     model - URL for the mesh file [default]
#     zip - URL for the zip file
#     screenshot - URL for the screenshot file
#     thumbnail - URL for the thumbnail of the screenshot


import json
import sys
import urllib2

def usage():
    print 'Usage: list.py url [-n num_assets] [-r report_value]'

def grab_list(url, num, value):

    download_url = url + '/download'
    browse_url = url + '/api/browse'
    next_start = ''
    all_items = []

    while len(all_items) < num and next_start != None:
        models_js = json.load( urllib2.urlopen(browse_url + '/' + next_start) )

        next_start = models_js['next_start']
        # Get real list of items
        models_js = models_js['content_items']

        new_items = [ x['metadata']['types']['original'][value] for x in models_js ]
        new_items = [ download_url  + '/' + x for x in new_items ]
        all_items.extend(new_items)

    if len(all_items) > num:
        all_items = all_items[0:num]
    return all_items

def main():
    if len(sys.argv) < 2:
        usage()
        exit(-1)

    url = sys.argv[1]
    num = 10
    value = 'model'

    x = 2
    while x < len(sys.argv):
        if sys.argv[x] == '-n':
            x += 1
            num = int(sys.argv[x])
        elif sys.argv[x] == '-r':
            x += 1
            value = sys.argv[x]
        else:
            usage()
            exit(-1)
        x += 1

    value_field_names = {
        'model' : 'hash',
        'zip' : 'zip',
        'screenshot' : 'screenshot',
        'thumbnail' : 'thumbnail'
        }
    value = value_field_names[value]

    items = grab_list(url, num, value)
    for item in items: print item

if __name__ == "__main__":
    main()
