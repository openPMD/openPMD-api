.. _usage-serial:

Serial Examples
===============

The serial API provides sequential, one-process read and write access.
Most users will use this for exploration and processing of their data.

Reading
-------

C++
^^^

.. literalinclude:: 2_read_serial.cpp
   :language: cpp
   :lines: 21-

An extended example can be found in ``examples/6_dump_filebased_series.cpp``.

Python
^^^^^^

.. literalinclude:: 2_read_serial.py
   :language: python3
   :lines: 9-

Writing
-------

C++
^^^

.. literalinclude:: 3_write_serial.cpp
   :language: cpp
   :lines: 21-

An extended example can be found in ``examples/7_extended_write_serial.cpp``.

Python
^^^^^^

.. literalinclude:: 3_write_serial.py
   :language: python3
   :lines: 9-
