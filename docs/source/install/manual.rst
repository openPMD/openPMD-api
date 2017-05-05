.. install-manual:

Manual
======

Manual install from source.

.. code-block:: bash

   mkdir src build

   cd src
   git clone https://github.com/ComputationalRadiationPhysics/libopenPMD.git

   cd ../build
   cmake ../src/libopenPMD

   make poc
   make poc_HDF5Writer
   make poc_HDF5Reader

   make libopenpmdCoreTests
   make libopenpmdAuxiliaryTests
