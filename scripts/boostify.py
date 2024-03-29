#!/usr/bin/python
# Convert a standalone library into a Boost library
#
# (C) 2017 Niall Douglas http://www.nedproductions.biz/
# File created: Sept 2017

from __future__ import print_function
import os, sys, re, shutil, time, datetime, subprocess, io

if len(sys.argv) < 2:
    print(sys.argv[0], " <dest> [<src>]", file=sys.stderr)
    sys.exit(1)
    #os.chdir('outcome')
    #destpath = '../boost-outcome'
else:
    destpath = sys.argv[1]
    if len(sys.argv) > 2:
        os.chdir(sys.argv[2])

configpath = ".boostify"
if not os.path.exists(configpath):
    print("ERROR: Source directory needs to have a .boostify config file", file=sys.stderr)
with open(configpath) as f:
    code = compile(f.read(), configpath, 'exec')
    exec(code)

for dirpath, dirnames, filenames in os.walk(destpath, topdown=False):
    for filename in filenames:
        path = os.path.join(dirpath, filename)
        if '.git' not in dirpath:
            os.remove(path)
    for dirname in dirnames:
        path = os.path.join(dirpath, dirname)
        if '.git' not in dirpath and dirname != '.git':
            try:
                os.rmdir(path)
            except:
                time.sleep(1)
                os.rmdir(path)
                
    

def do_transform(srcpath, destpath):
    for dirpath, dirnames, filenames in os.walk(srcpath):
        for filename in filenames:
            path = os.path.join(dirpath, filename).replace('\\', '/')
            if path.startswith('./') or path.startswith('.\\'):
                path = path[2:]
            excluded = False
            for exclusion in exclude_files:
                if re.match(exclusion, path):
                    excluded = True
                    break
            if excluded:
                continue
            included = False
            destpath2 = destpath + '/' + path
            for inclusion in include_files:
                if isinstance(inclusion, str):
                    if re.match(inclusion, path):
                        included = True
                        break
                else:
                    if re.match(inclusion[0], path):
                        included = True
                        destpath2 = re.sub(inclusion[0], inclusion[1], destpath2)
                        break
            if not included:
                continue
            print(path, inclusion, destpath2)
            if not os.path.exists(os.path.dirname(destpath2)):
                os.makedirs(os.path.dirname(destpath2))
            need_transform = {}
            for transform in transforms:
                if re.match(transform, path):
                    print('   | ', transform)
                    need_transform.update(transforms[transform])
            if need_transform:
                need_transform = sorted(need_transform.items())
                with io.open(path, 'rt', encoding='utf-8') as ih:
                    lines = ih.readlines()
                with io.open(destpath2, 'wt', encoding='utf-8') as oh:
                    for line in lines:
                        for transform, repl in need_transform:
                            if isinstance(repl, str):
                                #if '/include' in line:
                                #    print("in", line)
                                #    print("re", transform)
                                #    print("out", re.sub(transform, repl, line))
                                line = re.sub(transform, repl, line)
                            else:
                                m = re.match(transform, line)
                                if m:
                                    line = repl(m)
                        oh.write(line)
            else:
                shutil.copyfile(path, destpath2)
            shutil.copystat(path, destpath2)
        
do_transform('.', destpath)
for src, dest in overlay_files:
    for dirpath, dirnames, filenames in os.walk(src):
        for filename in filenames:
            srcpath2 = os.path.join(dirpath, filename).replace('\\', '/')
            destpath2 = os.path.join(destpath, srcpath2[len(src)+1:])
            if not os.path.exists(os.path.dirname(destpath2)):
                os.makedirs(os.path.dirname(destpath2))
            shutil.copyfile(srcpath2, destpath2)
            shutil.copystat(srcpath2, destpath2)


print("\n\nReplacing licences ...")
extensions = ['.cpp', '.hpp', '.ipp', '.c', '.h']

licence = '''Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
'''

replace = [ licence,
'''Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License in the accompanying file
Licence.txt or at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


Distributed under the Boost Software License, Version 1.0.
    (See accompanying file Licence.txt or copy at
          http://www.boost.org/LICENSE_1_0.txt)
''',
'''Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License in the accompanying file
Licence.txt or at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


Distributed under the Boost Software License, Version 1.0.
(See accompanying file Licence.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
'''
]


# Most of my code has the following header:
# /* bitfield.hpp
# Yet another C++ 11 constexpr bitfield
# (C) 2016 Niall Douglas http://www.nedprod.com/
# File Created: Aug 2016
#
#
# Boost Software License - Version 1.0 - August 17th, 2003
# ...
# */
#
# Some code omits the licence, but has some custom notes you'd want to preserve
# Newer code omts the file name at the top
# Really old code stops just before the "File Created:"

class SourceFile(object):
    todayyear = datetime.datetime.now().year
    niallstrings = [ 'Niall Douglas', 'ned Productions Jenkins build bot', 'Jenkins nedprod CI' ]
    
    def __init__(self):
        self.title = None
        self.description = None
        self.startyear = 0
        self.endyear = None
        self.copyright = None
        self.created = None
        self.remainder = None
        self.matchedlen = 0
        self.history = []

    def refresh_history(self, path):
        path = path.replace('\\', '/')
        # git shortlog hangs if called with a stdin which is not a TTY, so hack the problem
        if sys.platform == 'win32':
            historyre = subprocess.check_output(['git', 'shortlog', '-sne', '--', path, '<', 'CON'], shell = True)
        else:
            historyre = subprocess.check_output(['git', 'shortlog', '-sne', '--', path, '<', '/dev/tty'], shell = True)
        # Format will be:
        #    203  Niall Douglas (s [underscore] sourceforge {at} nedprod [dot] com) <spamtrap@nedprod.com>
        historyre = historyre.split('\n')
        history_ = []
        for line in historyre:
            line = line.rstrip()
            if line:
                result = re.match(r'\s*(\d+)\s+(.+) <(.+)>', line)
                assert result
                history_.append((int(result.group(1)), result.group(2), result.group(3)))
        # Accumulate all Niall Douglas commits
        niallcommits = 0
        self.history = []
        for item in history_:
            for niall in self.niallstrings:
                if niall in item[1]:
                    niallcommits += item[0]
                    break
            else:
                self.history.append(item)
        self.history.append((niallcommits, 'Niall Douglas', 'http://www.nedproductions.biz/'))
        self.history.sort(reverse = True)


class CppSourceFile(SourceFile):
    def match_header(self, contents):
        """Match a comment header at the top of my C++ source files and extract stuff,
        returning False if failed to match"""
        result = re.match(r'/\* ([^\n]+)\n([^\n]+)\n\(C\) (\d\d\d\d)(-\d\d\d\d)? (Niall Douglas[^\n]+)\n([^\n*/]+)?(.*?\*/)', contents, flags = re.DOTALL)
        if not result:
            result = re.match(r'()/\* ([^\n]+)\n\(C\) (\d\d\d\d)(-\d\d\d\d)? (Niall Douglas[^\n]+)\n([^\n*/]+)?(.*?\*/)', contents, flags = re.DOTALL)
        if not result:
            return False
        self.title = result.group(1).lstrip().rstrip()
        self.description = result.group(2).lstrip().rstrip()
        self.startyear = int(result.group(3))
        self.endyear = result.group(4)
        if self.endyear is not None:
            self.endyear = int(self.endyear[1:])
        self.copyright = result.group(5).lstrip().rstrip()
        self.created = result.group(6)
        if self.created is not None:
            self.created = self.created.lstrip().rstrip()
        self.remainder = result.group(7).lstrip().rstrip()
        self.matchedlen = len(result.group(0))
        return True

    def gen_header(self):
        """Returns a regenerated header for C++ source"""
        ret = '/* ' + self.description + '\n(C) ' + str(self.startyear)
        if self.todayyear != self.startyear:
            ret += '-' + str(self.todayyear)
        ret += ' '
        if not self.history:
            ret += self.copyright
        else:
            for idx in range(0, len(self.history)):
                ret += self.history[idx][1] + ' <' + self.history[idx][2] + '> (' + str(self.history[idx][0])
                ret += ' commit)' if self.history[idx][0] == 1 else ' commits)'
                if idx == len(self.history) - 2:
                    ret += ' and '
                elif idx < len(self.history) - 2:
                    ret += ', '
        ret += '\n'
        if self.created:
            ret += self.created + '\n'
        ret += '\n\n'
        ret += licence
        if self.remainder != '*/':
            ret += '\n\n'
        ret += self.remainder
        return ret

for dirpath, dirnames, filenames in os.walk(destpath):
    for filename in filenames:
        process = False
        for ext in extensions:
            if filename[-len(ext):] == ext:
                process = True
                break
        if not process:
            continue
        path = os.path.join(dirpath, filename)
        if '.git' not in path:
            with io.open(path, 'r', encoding='utf-8') as ih:
                contents = ih.read()
            processor = CppSourceFile()
            if not processor.match_header(contents):
                print("NOTE: Did not match", path)
            else:
                # Remove any licence boilerplates from the header comment
                for r in replace:
                    idx = processor.remainder.find(r)
                    if idx != -1:
                        processor.remainder = processor.remainder[:idx] + processor.remainder[idx+len(r):]
                        processor.remainder = processor.remainder.lstrip()
                #print("\n\nMatched", path, "with:\n  title=", processor.title, "\n  description=", processor.description,
                #      "\n  startyear=", processor.startyear, "\n  endyear=", processor.endyear, "\n  copyright=", processor.copyright,
                #      "\n  created=", processor.created, "\n  remainder=", processor.remainder)
                #processor.refresh_history(path)
                replacement = processor.gen_header()
                #print("\n\nMatched", path, "with:\n\n" + replacement)
                contents2 = replacement + contents[processor.matchedlen:]
                if contents != contents2:
                    with io.open(path, 'wt', encoding='utf-8') as oh:
                        oh.write(contents2)
                    print("Updated", path)
                else:
                    print("No need to update", path)
                    
        


#print("\nRunning Hugo to generate docs ...")
#os.chdir(destpath+"/doc/src")
#subprocess.check_output(['hugo'], shell = True)
