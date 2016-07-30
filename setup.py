from setuptools import setup, Extension
import sys

base_module = Extension(
    'atomic', ['atomic_module.c', 
               'atomic_integer.c',
               'atomic_reference.c',
               'atomic_markable_reference.c'],
    extra_compile_args=['-fno-strict-aliasing'])

setup(
    name='atomic',
    version='0.1',
    description='Module providing types supporting atomic operations',
    author='Omar Sandoval',
    author_email='osandov@osandov.com',
    url='https://github.com/osandov/python-atomic',
    ext_modules=[base_module],
    test_suite='tests')
