---
name: Bug report
about: Create a report to help us improve
labels: bug
---

**Describe the bug**
A clear and concise description of what the bug is.

**To Reproduce**
Compile-able/executable code example to reproduce the problem:

Python:
```python
import openpmd_api

# ...
```
or C++
```C++
#include <openPMD/openPMD.hpp>

// ...
```

or command line script, example:
```bash
# in directory $HOME/src/openPMD-api
mkdir build && cd build

cmake ..
# fails with:
#   bash: cmake: command not found
```

**Expected behavior**
A clear and concise description of what you expected to happen.

**Software Environment**
 - version of openPMD-api: [X.Y.Z-abc]
 - installed openPMD-api via: [conda-forge, spack, pip, brew, yggdrasil, from source, module system, ...]
 - operating system: [name and version]
 - machine: [Are you running on a public cluster? It's likely we compute on it as well!]
 - name and version of Python implementation: [e.g. CPython 3.9]
 - version of HDF5: [e.g. 1.12.0]
 - version of ADIOS1: [e.g. 1.13.1]
 - version of ADIOS2: [e.g. 2.7.1]
 - name and version of MPI: [e.g. OpenMPI 4.1.1]

**Additional context**
Add any other context about the problem here.
