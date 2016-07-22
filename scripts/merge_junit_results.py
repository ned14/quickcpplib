#!/usr/bin/python
# Merge a wildcarded set of junit XML files into a single XML file,
# renaming the test suite to match the source XML file
#
# (C) 2016 Niall Douglas http://www.nedproductions.biz/
# File created: July 2016

from __future__ import print_function
import os, sys, glob
import xml.etree.ElementTree as ET

def merge_junits(outxmlfile, xmlfiles):
  outxml = ET.ElementTree(ET.Element('testsuites'))
  for xmlfile in xmlfiles:
    tree = ET.parse(xmlfile)
    testsuites = tree.getroot()
    xmlfilebase = os.path.splitext(os.path.basename(xmlfile))[0]
    testsuite = testsuites[0]
    testsuite.set('name', xmlfilebase)
    #ET.dump(testsuite)
    outxml.getroot().append(testsuite)
  outxml.write(outxmlfile)

def main(argv):
  if len(argv)<3:
    print("Usage: "+argv[0]+" <output> <inputs...>")
    sys.exit(1)
  xmlfiles=[]
  for arg in range(2, len(argv)):
    for f in glob.glob(argv[arg]):
      xmlfiles.append(f)
  merge_junits(argv[1], xmlfiles)

#main(['foo', 'combined.xml.out', '*.xml'])
if __name__ == '__main__':
  main(sys.argv)
