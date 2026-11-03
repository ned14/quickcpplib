#!/usr/bin/python
# Merge a wildcarded set of junit XML files into a single XML file,
# renaming the test suite to match the source XML file
#
# (C) 2016 Niall Douglas http://www.nedproductions.biz/
# File created: July 2016

from __future__ import print_function
import os, sys, glob, platform, datetime, copy
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
  appended = 0
  for xmlfile in xmlfiles:
    tree = ET.parse(xmlfile)
    testsuites = tree.getroot()
    xmlfilebase = os.path.basename(xmlfile)
    xmlfilebase = xmlfilebase[:xmlfilebase.rfind('.junit.xml')]
    # JUnit files carry MORE than one <testsuite> in practice (Boost.Test emits
    # one per test case), so taking only testsuites[0] silently dropped every
    # test outside the first suite - including failing ones (reviewer P0
    # r9:SD-1). Keep them all, naming each so they stay distinguishable.
    suites = [c for c in testsuites if c.tag == 'testsuite']
    for testsuite in suites:
      if len(suites) == 1:
        testsuite.set('name', xmlfilebase)
      else:
        testsuite.set('name', '%s [%s]' % (xmlfilebase, testsuite.get('name')))
      testsuite.set('hostname', hostname)
      testsuite.set('timestamp', timestamp)
      testsuite.insert(0, copy.deepcopy(properties))
      #ET.dump(testsuite)
      outxml.getroot().append(testsuite)
      appended += 1
  outxml.write(outxmlfile)
  return appended

def main(argv):
  if len(argv)<3:
    print("Usage: "+argv[0]+" <output> <inputs...>")
    sys.exit(1)
  xmlfiles=[]
  for arg in range(2, len(argv)):
    for f in glob.glob(argv[arg]):
      xmlfiles.append(f)
  if len(xmlfiles)==0:
    # An unmatched pattern used to write an empty <testsuites/> and exit 0, so
    # CI went green having merged no results at all (reviewer P0 r1:SD-3).
    print("ERROR: no junit XML files matched any of the patterns: "+" ".join(argv[2:]), file=sys.stderr)
    sys.exit(1)
  if merge_junits(argv[1], xmlfiles)==0:
    print("ERROR: the matched junit files contained no <testsuite> elements: "+" ".join(xmlfiles), file=sys.stderr)
    sys.exit(1)

#main(['foo', 'combined.xml.out', '*.xml'])
if __name__ == '__main__':
  main(sys.argv)
