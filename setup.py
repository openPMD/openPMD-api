import os
import platform
import re
import subprocess
import sys

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def run(self):
        from packaging.version import parse

        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError(
                "CMake 3.22.0+ must be installed to build the following " +
                "extensions: " +
                ", ".join(e.name for e in self.extensions))

        cmake_version = parse(re.search(
            r'version\s*([\d.]+)',
            out.decode()
        ).group(1))
        if cmake_version < parse('3.22.0'):
            raise RuntimeError("CMake >= 3.22.0 is required")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(
            self.get_ext_fullpath(ext.name)
        ))
        # required for auto-detection of auxiliary "native" libs
        if not extdir.endswith(os.path.sep):
            extdir += os.path.sep

        pyv = sys.version_info
        cmake_args = [
            # Python: use the calling interpreter in CMake
            # https://cmake.org/cmake/help/latest/module/FindPython.html#hints
            # https://cmake.org/cmake/help/latest/command/find_package.html#config-mode-version-selection
            '-DPython_ROOT_DIR=' + sys.prefix,
            f'-DPython_FIND_VERSION={pyv.major}.{pyv.minor}.{pyv.micro}',
            '-DPython_FIND_VERSION_EXACT=TRUE',
            '-DPython_FIND_STRATEGY=LOCATION',
            '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' +
            os.path.join(extdir, "openpmd_api"),
            # '-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=' + extdir,
            '-DopenPMD_PYTHON_OUTPUT_DIRECTORY=' + extdir,
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
            # Unix: rpath to current dir when packaged
            #       needed for shared (here non-default) builds
            '-DCMAKE_BUILD_WITH_INSTALL_RPATH:BOOL=ON',
            '-DCMAKE_INSTALL_RPATH_USE_LINK_PATH:BOOL=OFF',
            # Windows: has no RPath concept, all `.dll`s must be in %PATH%
            #          or same dir as calling executable
        ]
        if HDF5_USE_STATIC_LIBRARIES is not None:
            cmake_args.append('-DHDF5_USE_STATIC_LIBRARIES:BOOL=' +
                              HDF5_USE_STATIC_LIBRARIES)
        if ZLIB_USE_STATIC_LIBS is not None:
            cmake_args.append('-DZLIB_USE_STATIC_LIBS:BOOL=' +
                              ZLIB_USE_STATIC_LIBS)
        if CMAKE_INTERPROCEDURAL_OPTIMIZATION is not None:
            cmake_args.append('-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=' +
                              CMAKE_INTERPROCEDURAL_OPTIMIZATION)
        if sys.platform == "darwin":
            cmake_args.append('-DCMAKE_INSTALL_RPATH=@loader_path')
        else:
            # values: linux*, aix, freebsd, ...
            #   just as well win32 & cygwin (although Windows has no RPaths)
            cmake_args.append('-DCMAKE_INSTALL_RPATH=$ORIGIN')

        cmake_args += extra_cmake_args

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        # Assumption: Windows builds are always multi-config (MSVC VS)
        if platform.system() == "Windows":
            cmake_args += [
                '-DopenPMD_BUILD_NO_CFG_SUBPATH:BOOL=ON',
                '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(
                    cfg.upper(),
                    os.path.join(extdir, "openpmd_api")
                )
            ]
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j2']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(
            env.get('CXXFLAGS', ''),
            self.distribution.get_version()
        )
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(
            ['cmake', ext.sourcedir] + cmake_args,
            cwd=self.build_temp,
            env=env
        )
        subprocess.check_call(
            ['cmake', '--build', '.'] + build_args,
            cwd=self.build_temp
        )
        # note that this does not call install;
        # we pick up artifacts directly from the build output dirs


with open('./README.md', encoding='utf-8') as f:
    long_description = f.read()

# Allow to control options via environment vars.
# Work-around for https://github.com/pypa/setuptools/issues/1712
# note: changed default for SHARED, MPI, TESTING and EXAMPLES
openPMD_USE_MPI = os.environ.get('openPMD_USE_MPI', 'OFF')
HDF5_USE_STATIC_LIBRARIES = os.environ.get('HDF5_USE_STATIC_LIBRARIES', None)
ZLIB_USE_STATIC_LIBS = os.environ.get('ZLIB_USE_STATIC_LIBS', None)
# deprecated: backwards compatibility to <= 0.13.*
BUILD_SHARED_LIBS = os.environ.get('BUILD_SHARED_LIBS', 'OFF')
BUILD_TESTING = os.environ.get('BUILD_TESTING', 'OFF')
BUILD_EXAMPLES = os.environ.get('BUILD_EXAMPLES', 'OFF')
# end deprecated
BUILD_SHARED_LIBS = os.environ.get('openPMD_BUILD_SHARED_LIBS',
                                   BUILD_SHARED_LIBS)
BUILD_TESTING = os.environ.get('openPMD_BUILD_TESTING',
                               BUILD_TESTING)
BUILD_EXAMPLES = os.environ.get('openPMD_BUILD_EXAMPLES',
                                BUILD_EXAMPLES)
CMAKE_INTERPROCEDURAL_OPTIMIZATION = os.environ.get(
    'CMAKE_INTERPROCEDURAL_OPTIMIZATION', None)

# extra CMake arguments
extra_cmake_args = []
for k, v in os.environ.items():
    extra_cmake_args_prefix = "openPMD_CMAKE_"
    if k.startswith(extra_cmake_args_prefix) and \
       len(k) > len(extra_cmake_args_prefix):
        extra_cmake_args.append("-D{0}={1}".format(
            k[len(extra_cmake_args_prefix):],
            v))

# https://cmake.org/cmake/help/v3.0/command/if.html
if openPMD_USE_MPI.upper() in ['1', 'ON', 'TRUE', 'YES']:
    openPMD_USE_MPI = "ON"
else:
    openPMD_USE_MPI = "OFF"

# Get the package requirements from the requirements.txt file
with open('./requirements.txt') as f:
    install_requires = [line.strip('\n') for line in f.readlines()]
    if openPMD_USE_MPI == "ON":
        install_requires.append('mpi4py>=2.1.0')

# keyword reference:
#   https://packaging.python.org/guides/distributing-packages-using-setuptools
setup(
    name='openPMD-api',
    # note PEP-440 syntax: x.y.zaN but x.y.z.devN
    version='0.17.0.dev',
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
        'Programming Language :: Python :: 3.13',
        ('License :: OSI Approved :: '
         'GNU Lesser General Public License v3 or later (LGPLv3+)'),
    ],
)
