#!/usr/bin/python3
# Makes a debian source package for upload to launchpad
#
# (C) 2016 Niall Douglas http://www.nedproductions.biz/
# File created: Nov 2016
#
# TODO FIXME: Can only package up header-only libraries right now

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
  if subprocess.call(['git', 'clone', '--recurse-submodules', githttpurl])!=0:
    print("Git clone of", githttpurl ,"failed", file=sys.stderr)
    sys.exit(1)
else:
  print("Pulling", githttpurl, "into", gitrepodir)
  os.chdir(gitrepodir)
  if subprocess.call(['git', 'pull'])!=0:
    print("Git pull of", githttpurl ,"failed", file=sys.stderr)
    sys.exit(1)
  if subprocess.call(['git', 'submodule', 'update'])!=0:
    print("Git pull of", githttpurl ,"failed", file=sys.stderr)
    sys.exit(1)
  os.chdir('..')

# Directory debuild uses must be named same as Source:
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
# Copy the include, src and debian metadata
# shutils.copytree() ignore facility appears to be bugged, so ...
ignored=[]
for dirpath, dirnames, filenames in os.walk(gitrepodir):
  skip=False
  for n in ignored:
    if n in dirpath:
      skip=True
      break
  if skip:
    continue
#  if '.git' in filenames or '.git' in dirnames:
#    for dirname in dirnames:
#      if dirname != 'include' and dirname != 'src' and dirname != 'cmake':
#        ignored.append(os.path.join(dirpath, dirname))
#    continue
  if ".git" in dirnames:
    ignored.append(os.path.join(dirpath, ".git"))
  if "doc" in dirnames:
    ignored.append(os.path.join(dirpath, "doc"))
  if "test" in dirnames:
    ignored.append(os.path.join(dirpath, "test"))
  if "attic" in dirnames:
    ignored.append(os.path.join(dirpath, "attic"))
  os.makedirs(sourcedir+dirpath[len(gitrepodir):], exist_ok=True)
  for filename in filenames:
    if filename.startswith(".git"):
      continue
    path1=os.path.join(dirpath, filename)
    path2=sourcedir+path1[len(gitrepodir):]
    print(path1)
    shutil.copy2(path1, path2)
#shutil.copytree(os.path.join(gitrepodir, "debian"), os.path.join(sourcedir, "debian"))
#shutil.copy2(os.path.join(gitrepodir, "CMakeLists.txt"), os.path.join(sourcedir, "CMakeLists.txt"))
with open(changelogpath, "rt") as ih:
  changelog=ih.readlines()
# Tar up the git clone into an original tarball
olddistro='unstable'
for distro in ['trusty', 'xenial']:
  install_file=[]
  print("\n\nTarring up for distro", distro, "...")
  changelog[0]=changelog[0].replace(olddistro, distro)
  olddistro=distro
  with open(changelogpath, "wt") as oh:
    oh.writelines(changelog)
  with tarfile.open(tarballname, 'w:xz') as oh:
    for dirpath, dirnames, filenames in os.walk(sourcedir):
      for filename in filenames:
        path=os.path.join(dirpath, filename)
        oh.add(path)
        installpath=path[len(sourcedir)+1:]
        if not installpath.startswith("debian"):
          install_file+=[installpath+' /usr/'+installpath+'\n']
  print("Running debuild ...")
  os.chdir(sourcedir)
  #with open("debian/install", "wt") as oh:
  #  oh.writelines(install_file)
  #print("The following files will be installed by the .deb:")
  #for line in install_file:
  #  print(line, end='')
  if subprocess.call(['debuild', '-S', '-sa', '-k65D611D2'])!=0:
    print("Debuild failed", file=sys.stderr)
    sys.exit(1)
  # TODO Need to clean upd/* of deb generated stuff
  # dput boost.outcome upd/boost-outcome_1.0-2_source.changes
  os.chdir("..")
  break
