#!/usr/bin/python
# Merge a wildcarded set of junit XML files into a single XML file,
# renaming the test suite to match the source XML file
#
# (C) 2016 Niall Douglas http://www.nedproductions.biz/
# File created: July 2016

from __future__ import print_function
import os, sys, glob, platform, datetime
import xml.etree.ElementTree as ET

hostname = platform.node()
timestamp = datetime.datetime.utcnow().isoformat()+'Z'
properties = ET.Element('properties')
def add_property(name, value):
  global properties
  property = ET.Element('property', attrib={'name' : name, 'value' : value})
  properties.append(property)
add_property('os.name', platform.system())
add_property('os.platform', platform.processor())
add_property('os.release', platform.release())
add_property('os.version', platform.version())
# Is there a file called CMakeCXXCompiler.cmake anywhere in a CMakeFiles?
if os.path.exists('CMakeFiles'):
  files = glob.glob('CMakeFiles/*/CMakeCXXCompiler.cmake')
  if len(files)>0:
    with open(files[0], 'rt') as ih:
      text = ih.read()
      idx = text.find('CMAKE_CXX_COMPILER')
      if idx!=-1:
        idx = text.find('"', idx)
        add_property('compiler.name', text[idx+1:text.find('"', idx+1)])
      idx = text.find('CMAKE_CXX_COMPILER_VERSION')
      if idx!=-1:
        idx = text.find('"', idx)
        add_property('compiler.version', text[idx+1:text.find('"', idx+1)])

def merge_junits(outxmlfile, xmlfiles):
  outxml = ET.ElementTree(ET.Element('testsuites'))
  for xmlfile in xmlfiles:
    tree = ET.parse(xmlfile)
    testsuites = tree.getroot()
    xmlfilebase = os.path.basename(xmlfile)
    xmlfilebase = xmlfilebase[:xmlfilebase.rfind('.junit.xml')]
    if len(testsuites) > 0:
      testsuite = testsuites[0]
      testsuite.set('name', xmlfilebase)
      testsuite.set('hostname', hostname)
      testsuite.set('timestamp', timestamp)
      testsuite.insert(0, properties)
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
