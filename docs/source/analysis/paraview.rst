.. _analysis-paraview:

3D Visualization: ParaView
==========================

openPMD data can be visualized by ParaView, an open source visualization and analysis software.
ParaView can be downloaded and installed from httpshttps://www.paraview.org.
Use the latest version for best results.

Tutorials
---------

ParaView is a powerful, general parallel rendering program.
If this is your first time using ParaView, consider starting with a tutorial.

* https://www.paraview.org/Wiki/The_ParaView_Tutorial
* https://www.youtube.com/results?search_query=paraview+introduction
* https://www.youtube.com/results?search_query=paraview+tutorial


openPMD
-------

openPMD files can be visualized with ParaView 5.9+, using 5.11+ is recommended.
ParaView supports ADIOS2 and HDF5 files, as it implements against the Python bindings of openPMD-api.

For openPMD output to be recognized, create a small textfile with ``.pmd`` ending per data series, which can be opened with ParaView:

.. code-block:: console

   $ cat paraview.pmd
   openpmd_%06T.bp

The file contains the same string as one would put in an openPMD ``Series("....")`` object.

.. tip::

   When you first open ParaView, adjust its global ``Settings`` (Linux: under menu item ``Edit``).
   ``General`` -> ``Advanced`` -> Search for ``data`` -> ``Data Processing Options``.
   Check the box ``Auto Convert Properties``.

   This will simplify application of filters, e.g., contouring of components of vector fields, without first adding a calculator that extracts a single component or magnitude.

.. warning::

   As of ParaView 5.11 and older, the axisLabel is not yet read for fields.
   See, e.g., `WarpX issue 21162 <https://github.com/ECP-WarpX/WarpX/issues/1803>`__.
   Please apply rotation of, e.g., ``0 -90 0`` to mesh data where needed.

.. warning::

   `ParaView issue 21837 <https://gitlab.kitware.com/paraview/paraview/-/issues/21837>`__:
   In order to visualize particle traces with the ``Temporal Particles To Pathlines``, you need to apply the ``Merge Blocks`` filter first.

   If you have multiple species, you may have to extract the species you want with ``Extract Block`` before applying ``Merge Blocks``.
