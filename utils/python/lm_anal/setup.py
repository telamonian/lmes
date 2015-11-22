#!/usr/bin/env python
from setuptools import Extension, find_packages, setup

setup(
    author = 'Max Klein',
    description = 'python package for the analysis of Lattice Microbes input and output',
    license = 'UOISL',
    name = 'lm_anal',
    packages = find_packages(exclude=('lm_anal/example',
                                      'lm_anal/python_protobuf',
                                      'lm_anal/script')),
    # scripts = ['robertslab/md/src/rmsd/spark/rmsdSpark.py']
)