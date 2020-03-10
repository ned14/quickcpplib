#!/usr/bin/python3
# Test installing a quickcpplib project under https://github.com/cpp-pm/hunter
#
# (C) 2020 Niall Douglas http://www.nedproductions.biz/
# File created: Mar 2020

from __future__ import print_function
import os, sys, shutil, subprocess
from git import Repo

resourcepath = os.path.join(os.path.dirname(__file__), 'cpp-pm')

if len(sys.argv) < 5:
    print(sys.argv[0], " <git url> <test-cpp> <subrepo> <cmake-link-target>", file=sys.stderr)
    sys.exit(1)
giturl = sys.argv[1]
testcpppath = sys.argv[2]
subreponame = sys.argv[3]
linktarget = sys.argv[4]
#giturl = 'https://github.com/ned14/outcome'
#testcpppath = resourcepath + '/../test_outcome.cpp'
#subreponame = 'outcome'
#linktarget = 'outcome::hl'

def copytree(src, dst, symlinks=False):
    names = os.listdir(src)
    os.makedirs(dst, exist_ok = True)
    for name in names:
        srcname = os.path.join(src, name)
        dstname = os.path.join(dst, name)
        if os.path.isdir(srcname):
            copytree(srcname, dstname, symlinks)
        else:
            shutil.copy2(srcname, dstname)

repo = Repo.init('test_cpp-pm_install')
copytree(resourcepath, 'test_cpp-pm_install')
shutil.copy(testcpppath, 'test_cpp-pm_install/test.cpp')
if os.path.exists('test_cpp-pm_install/build'):
    shutil.rmtree('test_cpp-pm_install/build')
if os.path.exists('test_cpp-pm_install/' + subreponame):
    print('Updating subrepo to latest ...')
    repo.submodules[0].update(to_latest_revision = True, force = True)
else:
    print('Adding subrepo ...')
    subrepo = repo.create_submodule(subreponame, subreponame, url = giturl)

os.makedirs('test_cpp-pm_install/cmake/Hunter', exist_ok = True)
with open('test_cpp-pm_install/cmake/Hunter/config.cmake', 'w') as oh:
    oh.write('hunter_config(' + subreponame + ' GIT_SUBMODULE "' + subreponame + '")\n')

with open('test_cpp-pm_install/CMakeLists.txt', 'a') as oh:
    oh.write('hunter_add_package(' + subreponame + ')\nfind_package(' + subreponame + ' CONFIG REQUIRED)\ntarget_link_libraries(test ' + linktarget + ')\n')

repo.git.add('.')
try:
    repo.git.commit('--all', '--no-edit', '-m', '"A commit"')
except:
    pass
