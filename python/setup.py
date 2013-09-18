# Copyright: (c) 2013 by Curt Hartung
# This work is released under the Creating Commons 3.0 license
# found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
# and in the LICENCE.txt file included with this distribution

from distutils.core import setup, Extension

module1 = Extension('dna',
                    sources=['pydna.cpp','../splice/splice.cpp','../dnausb/dnausb_common.cpp','../dnausb/linux/dnausb.cpp'],
                    libraries=['rt', 'usb-1.0'])

setup(name = 'dna',
      version = '1.00',
      description = 'DNA API',
      ext_modules = [module1])

