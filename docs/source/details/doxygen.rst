.. _details-doxygen:

C++
===

Our Doxygen page `provides an index of all C++ functionality <../_static/doxyhtml/index.html>`_.


Public Headers
--------------

``#include ...`` the following headers to use openPMD-api:

===========================  =======================================================
Include                      Description
===========================  =======================================================
``<openPMD/openPMD.hpp>``    Public facade header (serial and MPI)
``<openPMD/benchmark/...>``  Optional :ref:`benchmark <utilities-benchmark>` helpers
===========================  =======================================================


External Documentation
----------------------

If you want to link to the openPMD-api doxygen index `from an external documentation <http://www.doxygen.nl/manual/external.html>`_, you can find the `Doxygen tag file here <../_static/doxyhtml/openpmd-api-doxygen-web.tag.xml>`_.

If you want to use this tag file with e.g. `xeus-cling <https://xeus-cling.readthedocs.io/en/latest/inline_help.html>`_, add the following in its configuration directory:

.. code:: json

    {
        "url": "https://openpmd-api.readthedocs.io/en/<adjust-version-of-tag-file-here>/_static/doxyhtml/",
        "tagfile": "openpmd-api-doxygen-web.tag.xml"
    }
