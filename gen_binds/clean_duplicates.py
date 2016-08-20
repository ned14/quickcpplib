#!/usr/bin/python
# Any entries in file 1 are removed from files 2 ... N

import os, sys
toremove={}
with open(sys.argv[1], 'r') as ih:
  for line in ih:
    if line[:8]=='// begin':
      toremove[line[9:]]=1
for n in xrange(2, len(sys.argv)):
  print("Renaming", sys.argv[n], "...")
  os.rename(sys.argv[n], 'temp999')
  try:
    with open(sys.argv[n], 'w') as oh:
      with open('temp999', 'r') as ih:
        copying=True
        for line in ih:
          if copying:
            if line[:8]=='// begin':
              if line[9:] in toremove:
                copying=False
                print("Removing", line[9:])
                continue
            oh.write(line)
          else:
            if line[:6]=='// end':
              copying=True
  finally:
    os.remove('temp999')
    