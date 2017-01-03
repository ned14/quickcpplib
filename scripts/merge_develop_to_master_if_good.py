#!/usr/bin/python
# Check a project's CDash if develop branch passed all its
# tests, if so merge to master and push
#
# (C) 2016 Niall Douglas http://www.nedproductions.biz/
# File created: December 2016

from __future__ import print_function
import os, sys, re, subprocess, urllib, json

#os.chdir('../../outcome')

def indented_print(s):
    s = s.splitlines()
    for line in s:
        print('  ', line)

if len(sys.argv)>1:
    os.chdir(sys.argv[1])
if not os.path.exists('.git'):
    print("Project is missing a .git file, is this a git repo?", file=sys.stderr)
    sys.exit(1)
if not os.path.exists('CTestConfig.cmake'):
    print("Project is missing a CTestConfig.cmake file, is this a CDash linked project?", file=sys.stderr)
    sys.exit(1)
ctestconfig = {}
with open('CTestConfig.cmake', 'rt') as ih:
    for line in ih.readlines():
        r = re.match('set\\((.*) "(.*)"\\)', line)
        if r:
            ctestconfig[r.group(1)] = r.group(2)
cdashurl = ctestconfig['CTEST_DROP_METHOD'] + '://' + ctestconfig['CTEST_DROP_SITE'] + ctestconfig['CTEST_DROP_LOCATION'].replace('submit.php', 'api/v1/index.php')
print('This is project', ctestconfig['CTEST_PROJECT_NAME'], 'with dashboard at', cdashurl)
print()

develop_shas = subprocess.check_output(['git', 'log', 'develop', '-n', '50', '--pretty=oneline'])
for develop_sha in develop_shas.splitlines():
    sha = develop_sha[:40]
    desc = develop_sha[41:]
    print('Asking CDash about status of develop branch SHA', sha, '(' + desc + ') ... ', end = '')
    results = urllib.urlopen(cdashurl + '&method=build&task=revisionstatus&revision=' + sha).read()
    if results.startswith('[]') or results.startswith('null'):
        print('NO DATA')
        continue
    try:
        results = json.loads(results)
    except:
        print('PARSE FAILED')
        continue
    failed = 0
    for result in results:
        if 'configureerrors' in result:
            failed += int(result['configureerrors'])
        if 'builderrors' in result:
            failed += int(result['builderrors'])
        if 'testfailed' in result:
            failed += int(result['testfailed'])
    if failed != 0:
        print('%d FAILED' % failed)
        continue
    print('ALL GREEN, MERGING')
    print('\nChecking out master branch ...')
    indented_print(subprocess.check_output(['git', 'checkout', 'master']))
    print('\nMerging develop branch ...')
    indented_print(subprocess.check_output(['git', 'merge', 'develop', '--no-ff', '-m', 'Merged from develop branch as CDash reports all green']))
    print('\nPushing master branch ...')
    pushoutput = subprocess.check_output(['git', 'push'], stderr=subprocess.STDOUT)
    indented_print(pushoutput)
    if '->' in pushoutput:
        sys.exit(0)
    sys.exit(1)

