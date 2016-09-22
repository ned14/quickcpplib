#!/usr/bin/python
# Merge a junit XML file into a CTest XML file
#
# (C) 2016 Niall Douglas http://www.nedproductions.biz/
# File created: July 2016

from __future__ import print_function
import os, sys
import xml.etree.ElementTree as ET

def main(argv):
  if len(argv)!=3:
    print("Usage: "+argv[0]+" <output> <input>")
    sys.exit(1)
  ctesttree = ET.parse(argv[1])
  testing = ctesttree.find('.//Testing')
  testlist = testing.find('TestList')

  # Remove all existing children from the first <Test> onwards
  # to be readded after
  postamble = ET.Element('Testing')
  removing = False
  for child in testing.getchildren():
    if not removing and child.tag == 'Test':
      removing = True
    if removing:
      postamble.append(child)
  for child in postamble.getchildren():
    testing.remove(child)

  def Elem(name, text):
    e = ET.Element(name)
    #e.tail = '\n'
    e.text = text
    return e
  def NamedMeasurement(name, type, value):
    if name is None:
      e = ET.Element('Measurement')
    else:
      e = ET.Element('NamedMeasurement', type=type, name=name)
    e.tail = '\n'
    e.append(Elem('Value', value))
    return e
  junittree = ET.parse(argv[2])
  for testsuite in junittree.findall('testsuite'):
    name = testsuite.get('name')
    for testcase in testsuite.findall('testcase'):
      test = ET.Element('Test')
      test.tail = '\n'

      thisname = name+' '+testcase.get('name')
      testlist.append(Elem('Test', thisname))
      test.append(Elem('Name', thisname))

      results = ET.Element('Results')
      results.append(NamedMeasurement('Execution Time', 'numeric/double', testcase.get('time')))
      results.append(NamedMeasurement('Completion Status', 'text/string', 'JUnit'))
      error = testcase.find('failure')
      skipped = testcase.find('skipped')
      if error is not None:
        test.set('Status', 'failed')
        results.append(NamedMeasurement(None, 'text/string', error.text))        
      elif skipped is not None:
        test.set('Status', 'notrun')
        results.append(NamedMeasurement(None, 'text/string', skipped.text))        
      else:
        test.set('Status', 'passed')
      systemout = testcase.find('system-out')
      systemerr = testcase.find('system-err')
      if systemout is not None:
        results.append(NamedMeasurement('Standard Out', 'text/string', systemout.text))
      if systemerr is not None:
        results.append(NamedMeasurement('Standard Error', 'text/string', systemerr.text))

      test.append(results)
      testing.append(test)
  # TODO FIXME: I should really strip out any <Test> for any binaries
  # already reported in detail above
  testing.extend(postamble)
  #ET.dump(testing)
  ctesttree.write(argv[1])

#main(['foo', 'Test.xml', 'junit.xml'])
if __name__ == '__main__':
  main(sys.argv)
