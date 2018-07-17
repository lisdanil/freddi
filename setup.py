from glob import glob

import numpy as np
from setuptools import setup, Extension
from Cython.Build import cythonize

cpp_source = glob('src/*.cpp')
cpp_source.remove('src/main.cpp')

extensions = [
    Extension('freddi.freddi',
              cpp_source + ['freddi/freddi.pyx'],
              extra_compile_args=['-std=c++11'],
              include_dirs=['src/'] + [np.get_include()],
              libraries=['boost_program_options'])
]

setup(ext_modules=cythonize(extensions))