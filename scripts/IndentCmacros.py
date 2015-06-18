#!/usr/bin/python3

import sys

if(len(sys.argv))<2:
  print("Usage: "+sys.argv[0]+" <file>", file=sys.stderr)
  sys.exit(1)
with open(sys.argv[1], 'r') as h:
  lines=h.readlines();
print("Processing %s of %d lines" % (sys.argv[1], len(lines)))

indent=0
stack=[]
for n in range(0, len(lines)):
  trimmedline=lines[n].strip()
  if len(trimmedline) and trimmedline[0]=='#':
    trimmedline=trimmedline[1:].strip()
    while '/*_ ' in trimmedline and ' _*/' in trimmedline:
      trimmedline=trimmedline[:trimmedline.find('/*_ ')]+trimmedline[trimmedline.rfind(' _*/')+3:]
      trimmedline=trimmedline.strip()
    command=trimmedline.split()[0]
    thiscomment=''
    if command=='endif' or command=='elif' or command=='else':
      indent-=1
      if command=='else':
        thiscomment=stack[-1]
      else:
        thiscomment=stack.pop()
      if command=='elif':
        thiscomment=''
    if command=='else':
      thiscomment=stack[-1]
    if '//' in trimmedline or '/*' in trimmedline:
      thiscomment=''
    lines[n]='#'+(''.ljust(indent))+trimmedline+thiscomment+'\n'
    if command[0:2]=='if' or command=='elif' or command=='else':
      indent+=1
      if command=='ifdef':
        stack.append('  /*_ defined('+trimmedline.split()[1]+') _*/')
      elif command=='ifndef':
        stack.append('  /*_ !defined('+trimmedline.split()[1]+') _*/')
      elif command!='else':
        stack.append('  /*_ '+' '.join(trimmedline.split()[1:])+' _*/')

#with open('tempfile.hpp', 'w') as h:
with open(sys.argv[1], 'w') as h:
  h.writelines(lines)
  