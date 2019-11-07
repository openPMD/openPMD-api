import os
import re
import sys
import platform
import subprocess

from skbuild import setup


with open('./README.md', encoding='utf-8') as f:
    long_description = f.read()

# Work-around for https://github.com/pypa/setuptools/issues/1712
openPMD_USE_MPI = os.environ.get('openPMD_USE_MPI', 'OFF')
# https://cmake.org/cmake/help/v3.0/command/if.html
if openPMD_USE_MPI.upper() in ['1', 'ON', 'YES', 'TRUE', 'YES']:
    openPMD_USE_MPI = "ON"
else:
    openPMD_USE_MPI = "OFF"

# Get the package requirements from the requirements.txt file
with open('./requirements.txt') as f:
    install_requires = [line.strip('\n') for line in f.readlines()]
    if openPMD_USE_MPI == "ON":
        install_requires.append('mpi4py>=2.1.0')

# keyword reference:
#   https://scikit-build.readthedocs.io/en/latest/usage.html#setup-options
setup(
    name='openPMD-api',
    # note PEP-440 syntax: x.y.zaN but x.y.z.devN
    version='0.12.0.dev',
    author='Fabian Koller, Franz Poeschel, Axel Huebl',
    author_email='f.koller@hzdr.de, f.poeschel@hzdr.de, axelhuebl@lbl.gov',
    maintainer='Axel Huebl',
    maintainer_email='axelhuebl@lbl.gov',
    description='C++ & Python API for Scientific I/O with openPMD',
    long_description=long_description,
    long_description_content_type='text/markdown',
    keywords=('openPMD openscience hdf5 adios mpi hpc research '
              'file-format file-handling'),
    url='https://www.openPMD.org',
    project_urls={
        'Documentation': 'https://openpmd-api.readthedocs.io',
        'Doxygen': 'https://www.openpmd.org/openPMD-api',
        'Reference': 'https://doi.org/10.14278/rodare.27',
        'Source': 'https://github.com/openPMD/openPMD-api',
        'Tracker': 'https://github.com/openPMD/openPMD-api/issues',
    },
    zip_safe=False,
    python_requires='>=3.5, <3.9',
    # tests_require=['pytest'],
    install_requires=install_requires,
    cmake_minimum_required_version="3.11.0",
    cmake_args=[
        # note: changed default for MPI, TESTING and EXAMPLES
        #    '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
        #    '-DCMAKE_PYTHON_OUTPUT_DIRECTORY=' + extdir,
        '-DPYTHON_EXECUTABLE=%s' % sys.executable,
        # change variant search default
        '-DopenPMD_USE_MPI:BOOL=%s' % openPMD_USE_MPI,
        # skip building CLI tools, tests & examples
        '-DBUILD_CLI_TOOLS:BOOL=OFF',  # FIXME: solve via entry points instead?
        '-DBUILD_TESTING:BOOL=%s' % os.environ.get('BUILD_TESTING', 'OFF'),
        '-DBUILD_EXAMPLES:BOOL=%s' % os.environ.get('BUILD_EXAMPLES', 'OFF'),
        # static/shared libs
        '-DBUILD_SHARED_LIBS:BOOL=%s' % os.environ.get('BUILD_SHARED_LIBS', 'OFF'),
        '-DHDF5_USE_STATIC_LIBRARIES:BOOL=%s' % os.environ.get('HDF5_USE_STATIC_LIBRARIES', 'OFF'),
        '-DADIOS_USE_STATIC_LIBS:BOOL=%s' % os.environ.get('ADIOS_USE_STATIC_LIBS', 'OFF'),
        # Unix: rpath to current dir when packaged
        #       needed for shared (here non-default) builds and ADIOS1
        #       wrapper libraries
        '-DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=ON',
        '-DCMAKE_INSTALL_RPATH_USE_LINK_PATH:BOOL=OFF',
        # Windows: has no RPath concept, all `.dll`s must be in %PATH%
        #          or same dir as calling executable
        '-DCMAKE_INSTALL_RPATH={0}'.format(
            #"@loader_path" if sys.platform == "darwin" else "$ORIGIN"),
            # rpath to lib/: bin/../lib and pythonX.Z/site-packages/../..
            "@loader_path/../lib;@loader_path/../.." if sys.platform == "darwin" else "$ORIGIN/../lib:$ORIGIN/../.."),
    ],
#    entry_points={
#        'console_scripts': [
#            'openpmd-ls = openpmd_api:ls'
#        ]
#    },
    # cmake_languages=('C', 'CXX'),
    # we would like to use this mechanism, but pip / setuptools do not
    # influence the build and build_ext with it.
    # therefore, we use environment vars to control.
    # ref: https://github.com/pypa/setuptools/issues/1712
    # extras_require={
    #     'mpi': ['mpi4py>=2.1.0'],
    # },
    # cmdclass={'test': PyTest},
    # platforms=['any'],
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Natural Language :: English',
        'Environment :: Console',
        'Intended Audience :: Science/Research',
        'Operating System :: OS Independent',
        'Topic :: Scientific/Engineering',
        'Topic :: Database :: Front-Ends',
        'Programming Language :: C++',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        ('License :: OSI Approved :: '
         'GNU Lesser General Public License v3 or later (LGPLv3+)'),
    ],
)
