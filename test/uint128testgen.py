#!/usr/bin/python
# Generate some uint128 test tables
# (C) 2017 Niall Douglas
# File created: Aug 2017

import random

random.seed(78);

def tohex32(v):
    s=hex(v)
    if s[-1]=='L':
        s=s[:-1]
    return s

def tohex128(v):
    s=hex(v)[2:]
    if s[-1]=='L':
        s=s[:-1]
    s=s.zfill(32)
    #return '{0x'+zeroify(s[2:10])+',0x'+zeroify(s[10:18])+',0x'+zeroify(s[18:24])+',0x'+zeroify(s[24:32])+'}'
    return '{0x'+s[24:32]+',0x'+s[16:24]+',0x'+s[8:16]+',0x'+s[0:8]+'}'

with open("uint128testdata1.h", "wt") as oh:
    oh.write('// "A",""B","addition","subtraction","divided","modulus"\n')
    for n in xrange(0, 32):
        num = random.getrandbits(32)
        den = random.getrandbits(16)
        #print hex(num), hex(num+den)
        oh.write('{'+tohex128(num)+','+tohex32(den)+','+tohex128(num+den)+','+tohex128(num-den)+','+tohex128(num/den)+','+tohex32(num%den)+'},\n')
with open("uint128testdata2.h", "wt") as oh:
    oh.write('// "A",""B","addition","subtraction","divided","modulus"\n')
    for n in xrange(0, 4096):
        num = random.getrandbits(127)
        den = random.getrandbits(32)
        #print hex(num), hex(num+den)
        oh.write('{'+tohex128(num)+','+tohex32(den)+','+tohex128(num+den)+','+tohex128(num-den)+','+tohex128(num/den)+','+tohex32(num%den)+'},\n')
        
