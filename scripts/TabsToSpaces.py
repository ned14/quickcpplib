#!/usr/bin/python

from __future__ import print_function

spaces_per_tab = 4

import os, sys

for dirpath, dirnames, filenames in os.walk('.'):
	for filename in filenames:
		if filename[-4:]=='.hpp' or filename[-4:]=='.cpp' or filename[-4:]=='.ipp' or filename[-4:]=='.txt' or filename[-3:]=='.md':
			path=os.path.join(dirpath, filename)
			if os.path.exists(path+'.orig'):
				os.remove(path+'.orig')
			os.rename(path, path+'.orig')
			with open(path, 'wb') as oh:
				with open(path+".orig", 'rb') as ih:
					print("Expanding tabs in", path, "...")
					for line in ih:
						oh.write(line.expandtabs(spaces_per_tab))
