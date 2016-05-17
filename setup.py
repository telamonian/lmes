from setuptools import setup
from distutils.sysconfig import get_python_lib
import glob
import os
import sys

setup(
    name = "testytest",
    package_dir = {'': 'src/cython'},
    data_files = [(get_python_lib(), glob.glob('src/cython/*.so'))],
    author = 'Max Klein',
    description = 'python libraries for interfacing with Lattice Microbes',
    license = 'UIOSL',
    zip_safe = False,
    )
