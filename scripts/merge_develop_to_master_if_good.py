#!/usr/bin/python
# Check a project's CDash if develop branch passed all its
# tests, if so merge to master and push
#
# (C) 2016 Niall Douglas http://www.nedproductions.biz/
# File created: December 2016

from __future__ import print_function
import os, sys, re, subprocess, json
try:
    from urllib.request import urlopen
except ImportError:  # Python 2
    from urllib import urlopen

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

# decode: subprocess returns bytes on Python 3, and this script is run by
# /usr/bin/python which is python3 on current images.
develop_shas = subprocess.check_output(['git', 'log', 'develop', '-n', '50', '--pretty=oneline']).decode('utf8', 'replace')
for develop_sha in develop_shas.splitlines():
    sha = develop_sha[:40]
    desc = develop_sha[41:]
    print('Asking CDash about status of develop branch SHA', sha, '(' + desc + ') ... ', end = '')
    raw = urlopen(cdashurl + '&method=build&task=revisionstatus&revision=' + sha).read()
    if not isinstance(raw, str):
        raw = raw.decode('utf8', 'replace')
    if raw.startswith('[]') or raw.startswith('null'):
        print('NO DATA')
        continue
    try:
        results = json.loads(raw)
    except Exception:
        print('PARSE FAILED')
        continue
    # FAIL CLOSED. The only payload this script may merge on is a NON-EMPTY LIST
    # of build records it actually understood. A dict payload (CDash returns
    # {"error": ...} on a bad query), a string, a number or an empty list used
    # to fall through the loop below with failed == 0 and then merge and PUSH
    # master - the gate failing open, which is how untested code reached master
    # (reviewer P0 r2:SD-2).
    if not isinstance(results, list) or len(results) == 0:
        print('UNRECOGNISED PAYLOAD (%s) - NOT MERGING' % type(results).__name__)
        continue
    failed = 0
    understood = 0
    for result in results:
        if not isinstance(result, dict):
            # A record we cannot read is not a record that passed.
            print('UNRECOGNISED RECORD (%s)' % type(result).__name__)
            failed += 1
            continue
        understood += 1
        if 'configureerrors' in result:
            failed += int(result['configureerrors'])
        if 'builderrors' in result:
            failed += int(result['builderrors'])
        if 'testfailed' in result:
            failed += int(result['testfailed'])
    if failed != 0:
        print('%d FAILED' % failed)
        continue
    # Merge the SHA that CDash actually tested, NOT develop HEAD. The loop above
    # walks develop's recent history, so a green report can be several commits
    # behind HEAD; merging `develop` then merged and pushed code that no
    # dashboard had ever validated (reviewer P0 r3:SD-1).
    if subprocess.call(['git', 'merge-base', '--is-ancestor', sha, 'develop']) != 0:
        print('GREEN BUT NOT AN ANCESTOR OF DEVELOP, NOT MERGING')
        continue
    print('ALL GREEN, MERGING ' + sha)
    print('\nChecking out master branch ...')
    indented_print(subprocess.check_output(['git', 'checkout', 'master']).decode('utf8', 'replace'))
    print('\nMerging the tested revision ' + sha + ' ...')
    indented_print(subprocess.check_output(['git', 'merge', sha, '--no-ff', '-m', 'Merged ' + sha + ' from develop branch as CDash reports all green']).decode('utf8', 'replace'))
    print('\nPushing master branch ...')
    pushoutput = subprocess.check_output(['git', 'push'], stderr=subprocess.STDOUT).decode('utf8', 'replace')
    indented_print(pushoutput)
    if '->' in pushoutput:
        sys.exit(0)
    sys.exit(1)

