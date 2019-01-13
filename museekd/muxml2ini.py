#!/usr/bin/env python2

from __future__ import print_function

import os
import sys
import xml.etree.ElementTree as ET

def main():
    path = str(os.path.expanduser("~/.museekd/config.xml"))
    if len(sys.argv) > 2:
        program_name = os.path.basename(sys.argv[0])
        print("Usage: %s [config.xml]" % program_name)
        print("Convert museekd config.xml file to ini format")
        sys.exit(1)

    if len(sys.argv) == 2:
        path = sys.argv[1]

    tree = ET.parse(path)
    root = tree.getroot()
    if root.tag != 'museekd':
        raise Exception("Root element '%s' must be 'museekd'" % (root.tag))

    for d in root:
        if d.tag != 'domain':
            print("'domain' element expected but '%s' found" % (d.tag), file=sys.stderr)
            continue

        domain = d.attrib.get('id')
        if not id:
            print("'domain' element doesn't have 'id' attribute", file=sys.stderr)
            continue

        print("[%s]" % domain)
        for e in d:
            if e.tag != 'key':
                print("'key' element expected but '%s' found in '%s'" % (e.tag, domain), file=sys.stderr)
                continue
            key = e.attrib.get('id')
            if not key:
                print("'key' element doesn't have 'id' attribute in '%s'" % (domain), file=sys.stderr)
                continue
            val = e.text
            if not val:
                val = ''
            print('%s=%s' % (key, val))
        print('')

if __name__ == "__main__":
    main()
