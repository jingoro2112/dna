# Copyright: (c) 2013 by Curt Hartung
# This work is released under the Creating Commons 3.0 license
# found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
# and in the LICENCE.txt file included with this distribution

from distutils.core import setup, Extension

import platform

if (platform.system() == "Darwin"):
    libdirs = ['../dnausb/OSX']
    extraLink = ['-framework IOKit -framework CoreFoundation -framework CoreServices']
elif (platform.system() == "Linux"):
    libdirs = ['../dnausb/Linux']
    extraLink = None
else:
    libdirs = None
    
module1 = Extension('dna',
        sources=['pydna.cpp','../splice/splice.cpp'],
        extra_link_args=extraLink,
        library_dirs=libdirs,
        libraries=['dnausb'])

setup(name = 'dna',
      version = '1.00',
      description = 'DNA API',
      ext_modules = [module1])

