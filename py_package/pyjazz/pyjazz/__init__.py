from sys import version_info
import os

try:
	os.unlink('pyjazz/_jazz_blocks.so')
except OSError:
	pass

if version_info[0] < 3: os.symlink(os.path.abspath('pyjazz/_jazz_blocks2.so'), 'pyjazz/_jazz_blocks.so')
else:                   os.symlink(os.path.abspath('pyjazz/_jazz_blocks3.so'), 'pyjazz/_jazz_blocks.so')

from pyjazz.get_jazz_version import get_jazz_version
