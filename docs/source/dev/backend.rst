.. _development-backend:

How to write a Backend
======================

Adding support for additional types of file storage or data transportation is possible by creating a backend.
Backend design has been kept independent of the openPMD-specific logic that maintains all constraints within a file.
This should allow easy introduction of new file formats with only little knowledge about the rest of the system.


File Formats
------------
To get started, you should introduce a new file format in ``include/openPMD/IO/Format.hpp`` representing the new backend. Note
that this enumeration value will never be seen by users of openPMD-api, but you should keep it short and concise to
improve readability.

.. code-block:: cpp

    enum class Format
    {
         JSON
    };

In order to use the file format through the API, you need to provide unique and characteristic filename extensions that
are associated with it. This happens in ``src/Series.cpp``:

.. code-block:: cpp

    Format
    determineFormat(std::string const& filename)
    {
        if( auxiliary::ends_with(filename, ".json") )
            return Format::JSON;
    }

.. code-block:: cpp

    std::string
    cleanFilename(std::string const& filename, Format f)
    {
        switch( f )
        {
            case Format::JSON:
                return auxiliary::replace_last(filename, ".json", "");
        }
    }

Unless your file format imposes additional restrictions to the openPMD constraints, this is all you have to do in the
frontend section of the API.

IO Handler
----------
Now that the user can specify that your new backend is to be used, a concrete mechanism for handling IO interactions is
required. We call this an ``IOHandler``. It is not concerned with any logic or constraints enforced by openPMD, but
merely offers a small set of elementary IO operations.

On the very basic level, you will need to derive a class from ``AbstractIOHandler``:

.. code-block:: cpp

    /* file: include/openPMD/IO/JSON/JSONIOHandler.hpp */
    #include "openPMD/IO/AbstractIOHandler.hpp"

    namespace openPMD
    {
    class JSONIOHandler : public AbstractIOHandler
    {
    public:
        JSONIOHandler(std::string const& path, AccessType);
        virtual ~JSONIOHandler();

        std::future< void > flush() override;
    }
    } // openPMD

.. code-block:: cpp

    /* file: src/IO/JSON/JSONIOHandler.cpp */
    #include "openPMD/IO/JSON/JSONIOHandler.hpp"

    namespace openPMD
    {
    JSONIOHandler::JSONIOHandler(std::string const& path, AccessType at)
            : AbstractIOHandler(path, at)
    { }

    JSONIOHandler::~JSONIOHandler()
    { }

    std::future< void >
    JSONIOHandler::flush()
    { return std::future< void >(); }
    } // openPMD

Familiarizing your backend with the rest of the API happens in just one place in ``src/IO/AbstractIOHandler.cpp``:

.. code-block:: cpp

    #if openPMD_HAVE_MPI
    std::shared_ptr< AbstractIOHandler >
    AbstractIOHandler::createIOHandler(std::string const& path,
                                       AccessType at,
                                       Format f,
                                       MPI_Comm comm)
    {
        switch( f )
        {
            case Format::JSON:
                std::cerr << "No MPI-aware JSON backend available. "
                             "Falling back to the serial backend! "
                             "Possible failure and degraded performance!" << std::endl;
                return std::make_shared< JSONIOHandler >(path, at);
        }
    }
    #endif

    std::shared_ptr< AbstractIOHandler >
    AbstractIOHandler::createIOHandler(std::string const& path,
                                       AccessType at,
                                       Format f)
    {
        switch( f )
        {
            case Format::JSON:
                return std::make_shared< JSONIOHandler >(path, at);
        }
    }

In this state, the backend will do no IO operations and just act as a dummy that ignores all queries.

IO Task Queue
-------------
