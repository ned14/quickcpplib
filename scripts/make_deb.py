#!/usr/bin/python3
# Makes a debian source package for upload to launchpad
#
# (C) 2016 Niall Douglas http://www.nedproductions.biz/
# File created: Nov 2016

from __future__ import print_function
import os, sys, subprocess, shutil, tarfile

if len(sys.argv)<3:
  print("Usage", os.path.basename(sys.argv[0]), "<git http> <tempdir>", file=sys.stderr)
  print("  Make SURE nothing in <tempdir> is important!", file=sys.stderr)
  sys.exit(1)
  
githttpurl=sys.argv[1]
tempdir=sys.argv[2]
if not os.path.exists(tempdir):
  os.mkdir(tempdir)
os.chdir(tempdir)
gitrepodir=os.path.basename(githttpurl)
if gitrepodir.endswith('.git'):
  gitrepodir=gitrepodir[:-4]
# We need a totally 100% clean clone
#print("Cleaning", gitrepodir)
#while os.path.exists(gitrepodir):
#  shutil.rmtree(gitrepodir)

if not os.path.exists(gitrepodir):
  print("Cloning", githttpurl, "into", gitrepodir)
  if subprocess.run(['git', 'clone', '--recursive-submodules', githttpurl]).returncode!=0:
    print("Git clone of", githttpurl ,"failed", file=sys.stderr)
    sys.exit(1)
# Directory debuild -us -uc uses must be named same as Source:
controlpath=os.path.join(gitrepodir, "debian", "control")
changelogpath=os.path.join(gitrepodir, "debian", "changelog")
if not os.path.exists(controlpath) or not os.path.exists(changelogpath):
  print("FATAL: Git repo", githttpurl, "is missing a debian/changelog, debian/control file", file=sys.stderr)
  sys.exit(1)
sourcedir=None
with open(controlpath, "rt") as ih:
  for line in ih.readlines():
    if line.startswith("Source: "):
      sourcedir=line[8:].strip()
      break
if sourcedir is None:
  print("FATAL: debian/control file doesn't have a Source:", file=sys.stderr)
  sys.exit(1)
with open(changelogpath, "rt") as ih:
  for line in ih.readlines():
    changelogfirstline=line.split(' ')
    break
# changelogfirstline has format of
# boost-outcome (1.0-1) UNRELEASED; urgency=low
tarballname=changelogfirstline[0]+'_'+changelogfirstline[1][1:-1]
tarballname=tarballname[:tarballname.rfind('-')]
tarballname+='.orig.tar.xz'

# We need a totally 100% clean debuild directory
print("Cleaning debuild directory", sourcedir)
while os.path.exists(sourcedir):
  shutil.rmtree(sourcedir)
os.mkdir(sourcedir)
# Copy the debian metadata
shutil.copytree(os.path.join(gitrepodir, "debian"), os.path.join(sourcedir, "debian"))
# Tar up the git clone into a tarball
print("Tarballing", sourcedir, "into", tarballname)
with tarfile.open(os.path.join(sourcedir, tarballname), 'w:xz') as oh:
  for dirpath, dirnames, filenames in os.walk(gitrepodir):
    if '.git' not in dirpath:
      for filename in filenames:
        path=os.path.join(dirpath, filename)
        path2=os.path.join(sourcedir, path[len(gitrepodir):])
        oh.add(path, path2)
