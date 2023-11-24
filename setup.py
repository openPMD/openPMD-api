import os

from skbuild import setup

# Environment variables and default values
openPMD_USE_MPI = os.environ.get("openPMD_USE_MPI", "OFF")

# Define the CMake arguments
cmake_args = [
    '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' +
    os.path.join(extdir, "openpmd_api"),
    # '-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=' + extdir,
    '-DopenPMD_PYTHON_OUTPUT_DIRECTORY=' + extdir,
    '-DPython_EXECUTABLE=' + sys.executable,
    '-DopenPMD_USE_PYTHON:BOOL=ON',
    # variants
    '-DopenPMD_USE_MPI:BOOL=' + openPMD_USE_MPI,
    # skip building cli tools, examples & tests
    #   note: CLI tools provided as console scripts
    '-DopenPMD_BUILD_CLI_TOOLS:BOOL=OFF',
    '-DopenPMD_BUILD_EXAMPLES:BOOL=' + BUILD_EXAMPLES,
    '-DopenPMD_BUILD_TESTING:BOOL=' + BUILD_TESTING,
    # static/shared libs
    '-DopenPMD_BUILD_SHARED_LIBS:BOOL=' + BUILD_SHARED_LIBS,
    '-DHDF5_USE_STATIC_LIBRARIES:BOOL=' + HDF5_USE_STATIC_LIBRARIES,
    '-DADIOS_USE_STATIC_LIBS:BOOL=' + ADIOS_USE_STATIC_LIBS,
    # Unix: rpath to current dir when packaged
    #       needed for shared (here non-default) builds and ADIOS1
    #       wrapper libraries
    '-DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=ON',
    '-DCMAKE_INSTALL_RPATH_USE_LINK_PATH:BOOL=OFF',
    # Windows: has no RPath concept, all `.dll`s must be in %PATH%
    #          or same dir as calling executable
]

setup(
    name='openPMD-api',
    # note PEP-440 syntax: x.y.zaN but x.y.z.devN
    version='0.16.0.dev',
    description='open standard for particle-mesh data files',
    long_description=open('README.rst').read(),
    name='openPMD-api',
    author='Axel Huebl, Franz Poeschel, Fabian Koller, Junmin Gu',
    author_email='axelhuebl@lbl.gov, f.poeschel@hzdr.de',
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
    ext_modules=[CMakeExtension('openpmd_api_cxx')],
    cmdclass=dict(build_ext=CMakeBuild),
    # scripts=['openpmd-ls'],
    zip_safe=False,
    python_requires='>=3.8',
    # tests_require=['pytest'],
    install_requires=install_requires,
    # see: src/bindings/python/cli
    entry_points={
        'console_scripts': [
            'openpmd-ls = openpmd_api.ls.__main__:main',
            'openpmd-pipe = openpmd_api.pipe.__main__:main'
        ]
    },
    # we would like to use this mechanism, but pip / setuptools do not
    # influence the build and build_ext with it.
    # therefore, we use environment vars to control.
    # ref: https://github.com/pypa/setuptools/issues/1712
    # extras_require={
    #     'mpi': ['mpi4py>=2.1.0'],
    # },
    # cmdclass={'test': PyTest},
    # platforms='any',
    classifiers=[
        'Development Status :: 4 - Beta',
        'Natural Language :: English',
        'Environment :: Console',
        'Intended Audience :: Science/Research',
        'Operating System :: OS Independent',
        'Topic :: Scientific/Engineering',
        'Topic :: Database :: Front-Ends',
        'Programming Language :: C++',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Programming Language :: Python :: 3.12',
        ('License :: OSI Approved :: '
         'GNU Lesser General Public License v3 or later (LGPLv3+)'),
    ],
    packages=['openpmd_api'],
    cmake_install_dir='openpmd_api',
    cmake_args=cmake_args,
    python_requires='>=3.8',
)
