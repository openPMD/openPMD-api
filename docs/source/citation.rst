.. _acknowledgement:

Citation
========

openPMD (Open Standard for Particle-Mesh Data Files) is a community project with many people contributing to it.
If you use openPMD and/or openPMD related software in your work, please credit it when publishing and/or presenting work performed with it in order to give back to the community.

openPMD-standard
----------------

The central definition of **openPMD is the meta data schema** defined in `openPMD/openPMD-standard <https://github.com/openPMD/openPMD-standard>`_.
The most general reference to openPMD is:

.. tip::

   Axel Huebl, Remi Lehe, Jean-Luc Vay, David P. Grote, Ivo F. Sbalzarini, Stephan Kuschel, David Sagan, Christopher Mayes, Frederic Perez, Fabian Koller, Franz Poeschel, Carsten Fortmann-Grote, Angel Ferran Pousa, Juncheng E, Maxence Thevenet and Michael Bussmann.
   *"openPMD: A meta data standard for particle and mesh based data,"*
   `DOI:10.5281/zenodo.591699 <https://doi.org/10.5281/zenodo.591699>`_ (2015)

The equivalent BibTeX code is:

.. code-block:: bibtex

   @misc{openPMDstandard,
     author       = {Huebl, Axel and
                     Lehe, R{\'e}mi and
                     Vay, Jean-Luc and
                     Grote, David P. and
                     Sbalzarini, Ivo and
                     Kuschel, Stephan and
                     Sagan, David and
                     Mayes, Christopher and
                     P{\'e}rez, Fr{\'e}d{\'e}ric and
                     Koller, Fabian and
                     Poeschel, Franz and
                     Fortmann-Grote, Carsten and
                     Ferran Pousa, Angel and
                     E, Juncheng and
                     Th{\'e}venet, Maxence and
                     Bussmann, Michael},
     title        = {{openPMD: A meta data standard for particle and mesh based data}},
     year         = 2015,
     publisher    = {Zenodo},
     doi          = {10.5281/zenodo.591699},
     url          = {https://www.openPMD.org},
     howpublished = {https://github.com/openPMD}
   }

Since the openPMD-standard is an actively evolving meta data schema, a specific version of the openPMD standard might be used in your work.
You can select a version-specific DOI from the `release page <https://github.com/openPMD/openPMD-standard/releases>`_ and add the version number to the cited title, e.g.

.. note::

   [author list as above] ...
   *"openPMD 1.1.0: A meta data standard for particle and mesh based data,"*
   `DOI:10.5281/zenodo.1167843 <https://doi.org/10.5281/zenodo.1167843>`_ (2018)

openPMD-api
-----------

openPMD-api is a **software library** that provides a reference implementation of the openPMD-standard for popular data formats.
It targets both desktop as well as high-performance computing environments.

It is good scientific practice to document all used software, including transient dependencies, with versions in, e.g. a methods section of a publication.
As a software citation, you almost always want to refer to a *specific version* of openPMD-api in your work, as illustrated for version 0.14.3:

.. tip::

   Axel Huebl, Franz Poeschel, Fabian Koller, Junmin Gu, Michael Bussmann, Jean-Luc Vay and Kesheng Wu.
   *"openPMD-api 0.14.3: C++ & Python API for Scientific I/O with openPMD,"*
   `DOI:10.14278/rodare.1234 <https://doi.org/10.14278/rodare.1234>`_ (2021)

A list of all releases and DOIs can be found `on the release page <https://github.com/openPMD/openPMD-api/releases>`_.

We also provide a DOI that refers to all releases of openPMD-api:

.. note::

   Axel Huebl, Franz Poeschel, Fabian Koller, Junmin Gu, Michael Bussmann, Jean-Luc Vay and Kesheng Wu.
   *"openPMD-api: C++ & Python API for Scientific I/O with openPMD"*
   `DOI:10.14278/rodare.27 <https://doi.org/10.14278/rodare.27>`_ (2018)

The equivalent BibTeX code is:

.. code-block:: bibtex

   @misc{openPMDapi,
     author       = {Huebl, Axel and
                     Poeschel, Franz and
                     Koller, Fabian and
                     Gu, Junmin and
                     Bussmann, Michael and
                     Vay, Jean-Luc and
                     Wu, Kesheng},
     title        = {{openPMD-api: C++ \& Python API for Scientific I/O with openPMD}},
     month        = june,
     year         = 2018,
     doi          = {10.14278/rodare.27},
     url          = {https://github.com/openPMD/openPMD-api}
   }


Dependent Software
~~~~~~~~~~~~~~~~~~

The good way to control complex software environments is to install software through a :ref:`package manager (see installation) <install>`.
Furthermore, openPMD-api provides functionality to simplify the documentation of its version and enabled backends:

C++17
^^^^^

.. code-block:: cpp

   #include <openPMD/openPMD.hpp>
   #include <iostream>

   namespace io = openPMD;

   // ...
   std::cout << "openPMD-api: "
             << io::getVersion() << std::endl;
   std::cout << "openPMD-standard: "
             << io::getStandard() << std::endl;

   std::cout << "openPMD-api backend variants: " << std::endl;
   for( auto const & v : io::getVariants() )
       std::cout << "  " << v.first << ": "
                 << v.second << std::endl;

Python
^^^^^^

.. code-block:: python3

   import openpmd_api as io

   print("openPMD-api: {}"
         .format(io.__version__))
   print("openPMD-api backend variants: {}"
         .format(io.variants))
