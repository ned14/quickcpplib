#!/usr/bin/python
# Convert a whole git history of a standalone library into a Boost library
#
# (C) 2019 Niall Douglas http://www.nedproductions.biz/
# File created: Jan 2019

from __future__ import print_function
import os, sys, subprocess, shutil, time
from git import Repo
from email import utils as emailutils

# There is always an empty tree object in every git repo
empty_tree_sha = "4b825dc642cb6eb9a060e54bf8d69288fbee4904"
mydir = os.path.dirname(sys.argv[0])

if len(sys.argv) < 4:
    print(sys.argv[0], " <destrepo> <srcrepo> <srcreposhaprefix>", file=sys.stderr)
    sys.exit(1)
destpath = sys.argv[1]
srcpath = sys.argv[2]
srcshaprefix = sys.argv[3] # "ned14/outcome@"

def clean():
    """Delete all files in the repo except .git and .gitattributes"""
    for root, dirs, files in os.walk(destpath, topdown = False):
        for file in files:
            if '.git' not in root:
                if root != destpath or file != '.gitattributes':
                    os.remove(os.path.join(root, file))
        for file in dirs:
            if '.git' not in root and file != '.git':
                os.rmdir(os.path.join(root, file))

def do_convert(destpath, srcpath):
    try:
        subprocess.check_output([sys.executable, os.path.join(mydir, 'boostify.py'), os.path.abspath(destpath), os.path.abspath(srcpath)])
        #subprocess.check_output(['c:/python27/pythonw.exe', os.path.join(mydir, 'boostify.py'), os.path.abspath(destpath), os.path.abspath(srcpath)], stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as ex:
        print(ex.returncode, ex.output, file = sys.stderr)
        raise

if False and os.path.exists(destpath):
    for n in range(0, 10):
        try:
            shutil.rmtree(destpath)
            break
        except:
            if n == 9:
                raise
            else:
                time.sleep(1)

srcrepo = Repo(srcpath)
if os.path.exists(destpath):
    destrepo = Repo(destpath)
else:
    # Copy the existing repo
    os.mkdir(destpath)
    shutil.copytree(os.path.join(srcpath, '.git'), os.path.join(destpath, '.git'))
    # Rename master and develop branches and delete all other branches and tags
    destrepo = Repo.init(destpath)
    if destrepo.tags:
        destrepo.tags[0].delete(destrepo, destrepo.tags)
    todelete = []  ## refs to delete
    for head in destrepo.heads:
        if head.name == 'develop' or head.name == 'master':
            head.rename('orig-' + head.name)
        else:
            todelete.append(destrepo)
    for remote in destrepo.remotes:
        for ref in remote.refs:
            todelete.append(ref)
    if todelete:
        todelete[0].delete(destrepo, *todelete)
    for head in destrepo.heads:
        if head.name == 'orig-develop' or head.name == 'orig-master':
            # Create orphaned, empty branch
            destrepo.git.checkout(head.name[5:], orphan = True)
            destrepo.git.rm('.', '-r', force = True)
            clean()
            # Make a first commit
            shutil.copy(os.path.join(srcpath, '.gitattributes'), destpath)
            destrepo.git.add('.gitattributes')
            destrepo.git.commit(message = "First commit")
            # Merge the 5th commit ago
            destrepo.git.merge(head.name + '~5', '--no-commit', '--allow-unrelated-histories')
            srccommit = destrepo.commit(head.name + '~5')
            srcrepo.git.checkout(srccommit.hexsha, force = True)
            do_convert(destpath, srcpath)
            destrepo.git.add('.')
            destrepo.git.commit('--all', '--no-edit')

# Fetch updates
for remote in destrepo.remotes:
    remote.fetch()

for branch in ['develop', 'master']:
    if branch not in destrepo.heads:
        continue
    destrepo.git.checkout(branch, force = True)

    # Find out what in srcbranch has not yet been merged into destbranch
    destcommit = destrepo.heads[branch].commit
    while True:
        found = False
        commits_to_merge = []
        if len(destcommit.parents) > 1:
            # Find the merge parent in srcbranch
            count = 0
            srccommit = destrepo.heads['orig-' + branch].commit
            while True:
                count = count + 1
                commits_to_merge.append(srccommit)
                if destcommit.parents[1] == srccommit:
                    found = True
                    break
                if count > 100 or not srccommit.parents:
                    break
                srccommit = srccommit.parents[0]
        if found:
            commits_to_merge.reverse()
            print(branch + ": Last source commit merged was", commits_to_merge[0])
            commits_to_merge = commits_to_merge[1:]
            print("Commits to merge now:")
            for commit in commits_to_merge:
                print(commit)
            print()
            break
        if not destcommit.parents:
            break
        destcommit = destcommit.parents[0]
            
    for commit in commits_to_merge:
        print('Merging commit', commit, commit.message)
        try:
            destrepo.git.merge(commit.hexsha, '--no-commit')
        except: pass
        destrepo.git.rm('.', '-r', force = True)
        clean()
        srcrepo.git.checkout(commit.hexsha, force = True)
        try:
            srcrepo.git.submodule('update', '--init', '--force')
        except: pass
        do_convert(destpath, srcpath)
        destrepo.git.add('.', '-A')
        destrepo = Repo(destpath)
        destrepo.index.commit('Merging commit ' + srcshaprefix + commit.hexsha + ':\n\n' + commit.message,
          parent_commits = (destrepo.head.commit, commit),
          author = commit.author,
          author_date = emailutils.formatdate(time.mktime(commit.authored_datetime.timetuple()))
        )
