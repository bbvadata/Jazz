from sys import version_info
import os
import site

packs_pat = site.getsitepackages()[0]

try:
	os.unlink(packs_pat + '/pyjazz/_jazz_blocks.so')
except OSError:
	pass

if version_info[0] < 3:	os.symlink(os.path.abspath(packs_pat + '/pyjazz/_jazz_blocks2.so'), packs_pat + '/pyjazz/_jazz_blocks.so')
else:					os.symlink(os.path.abspath(packs_pat + '/pyjazz/_jazz_blocks3.so'), packs_pat + '/pyjazz/_jazz_blocks.so')

from pyjazz.get_jazz_version import get_jazz_version
