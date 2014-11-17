#!/usr/bin/python3

import sys, itertools

def prnt(a): sys.stdout.write(a)

if len(sys.argv)<=1:
  print("Usage: gen_guard_matrix.py PREFIX CPPMACRO1 CPPMACRO2 CPPMACRO3 ...", file=sys.stderr)
  sys.exit(1)

prefix=sys.argv[1]
print('#undef %s' % prefix)
print('#undef %s_DESCRIPTION' % prefix)
macros=sys.argv[2:]
possibilities=pow(2, len(macros))
for x in range(0, possibilities):
  if x==0: prnt('#if ')
  else: prnt('#elif ')
  binary=''
  desc=''
  for y in range(0, len(macros)):
    if y!=0: prnt(' && ')
    if not x & 1<<y: prnt('!')
    binary+='1' if x & 1<<y else '0'
    desc+=' '+macros[y]+'='+('1' if x & 1<<y else '0')
    prnt(macros[y])
  print()
  print('# ifndef %s_%s' % (prefix, binary))
  print(('#  define %s_DESCRIPTION "'+desc[1:]+'"') % prefix)
  print('#  define %s_%s' % (prefix, binary))
  print('#  define %s 1' % prefix)
  print('# endif')
print('#endif')
