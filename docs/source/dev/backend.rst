.. _development-backend:

How to Write a Backend
======================

Adding support for additional types of file storage or data transportation is possible by creating a backend.
Backend design has been kept independent of the openPMD-specific logic that maintains all constraints within a file.
This should allow easy introduction of new file formats with only little knowledge about the rest of the system.


File Formats
------------
To get started, you should create a new file format in ``include/openPMD/IO/Format.hpp`` representing the new backend.
Note that this enumeration value will never be seen by users of openPMD-api, but should be kept short and concise to
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

.. code-block:: cpp

    std::function< bool(std::string const&) >
    matcher(std::string const& name, Format f)
    {
        switch( f )
        {
            case Format::JSON:
            {
                std::regex pattern(auxiliary::replace_last(name + ".json$", "%T", "[[:digit:]]+"));
                return [pattern](std::string const& filename) -> bool { return std::regex_search(filename, pattern); };
            }
        }
    }

Unless your file format imposes additional restrictions to the openPMD constraints, this is all you have to do in the
frontend section of the API.

IO Handler
----------
Now that the user can specify that the new backend is to be used, a concrete mechanism for handling IO interactions is
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
        JSONIOHandler(std::string const& path, Access);
        virtual ~JSONIOHandler();

        std::future< void > flush() override;
    }
    } // openPMD

.. code-block:: cpp

    /* file: src/IO/JSON/JSONIOHandler.cpp */
    #include "openPMD/IO/JSON/JSONIOHandler.hpp"

    namespace openPMD
    {
    JSONIOHandler::JSONIOHandler(std::string const& path, Access at)
            : AbstractIOHandler(path, at)
    { }

    JSONIOHandler::~JSONIOHandler()
    { }

    std::future< void >
    JSONIOHandler::flush()
    { return std::future< void >(); }
    } // openPMD

Familiarizing your backend with the rest of the API happens in just one place in ``src/IO/AbstractIOHandlerHelper.cpp``:

.. code-block:: cpp

    #if openPMD_HAVE_MPI
    std::shared_ptr< AbstractIOHandler >
    createIOHandler(
        std::string const& path,
        Access at,
        Format f,
        MPI_Comm comm
    )
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
    createIOHandler(
        std::string const& path,
        Access at,
        Format f
    )
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
Operations between the logical representation in this API and physical storage are  funneled through a queue ``m_work``
that is contained in the newly created IOHandler. Contained in this queue are ``IOTask`` s that have to be processed in
FIFO order (unless you can prove sequential execution guarantees for out-of-order execution) when
``AbstractIOHandler::flush()`` is called. A **recommended** skeleton is provided in ``AbstractIOHandlerImpl``. Note
that emptying the queue this way is not required and might not fit your IO scheme.

Using the provided skeleton involves
 - deriving an IOHandlerImpl for your IOHandler and
 - delegating all flush calls to the IOHandlerImpl:

.. code-block:: cpp

    /* file: include/openPMD/IO/JSON/JSONIOHandlerImpl.hpp */
    #include "openPMD/IO/AbstractIOHandlerImpl.hpp"

    namespace openPMD
    {
    class JSONIOHandlerImpl : public AbstractIOHandlerImpl
    {
    public:
        JSONIOHandlerImpl(AbstractIOHandler*);
        virtual ~JSONIOHandlerImpl();

        void createFile(Writable*, Parameter< Operation::CREATE_FILE > const&) override;
        void createPath(Writable*, Parameter< Operation::CREATE_PATH > const&) override;
        void createDataset(Writable*, Parameter< Operation::CREATE_DATASET > const&) override;
        void extendDataset(Writable*, Parameter< Operation::EXTEND_DATASET > const&) override;
        void openFile(Writable*, Parameter< Operation::OPEN_FILE > const&) override;
        void openPath(Writable*, Parameter< Operation::OPEN_PATH > const&) override;
        void openDataset(Writable*, Parameter< Operation::OPEN_DATASET > &) override;
        void deleteFile(Writable*, Parameter< Operation::DELETE_FILE > const&) override;
        void deletePath(Writable*, Parameter< Operation::DELETE_PATH > const&) override;
        void deleteDataset(Writable*, Parameter< Operation::DELETE_DATASET > const&) override;
        void deleteAttribute(Writable*, Parameter< Operation::DELETE_ATT > const&) override;
        void writeDataset(Writable*, Parameter< Operation::WRITE_DATASET > const&) override;
        void writeAttribute(Writable*, Parameter< Operation::WRITE_ATT > const&) override;
        void readDataset(Writable*, Parameter< Operation::READ_DATASET > &) override;
        void readAttribute(Writable*, Parameter< Operation::READ_ATT > &) override;
        void listPaths(Writable*, Parameter< Operation::LIST_PATHS > &) override;
        void listDatasets(Writable*, Parameter< Operation::LIST_DATASETS > &) override;
        void listAttributes(Writable*, Parameter< Operation::LIST_ATTS > &) override;
    }
    } // openPMD

.. code-block:: cpp

    /* file: include/openPMD/IO/JSON/JSONIOHandler.hpp */
    #include "openPMD/IO/AbstractIOHandler.hpp"
    #include "openPMD/IO/JSON/JSONIOHandlerImpl.hpp"

    namespace openPMD
    {
    class JSONIOHandler : public AbstractIOHandler
    {
    public:
        /* ... */
    private:
        JSONIOHandlerImpl m_impl;
    }
    } // openPMD

.. code-block:: cpp

    /* file: src/IO/JSON/JSONIOHandler.cpp */
    #include "openPMD/IO/JSON/JSONIOHandler.hpp"

    namespace openPMD
    {
    /*...*/
    std::future< void >
    JSONIOHandler::flush()
    {
        return m_impl->flush();
    }
    } // openPMD

Each IOTask contains a pointer to a ``Writable`` that corresponds to one object in the openPMD hierarchy. This object
may be a group or a dataset. When processing certain types of IOTasks in the queue, you will have to assign unique
FilePositions to these objects to identify the logical object in your physical storage. For this, you need to derive
a concrete FilePosition for your backend from ``AbstractFilePosition``. There is no requirement on how to identify your
objects, but ids from your IO library and positional strings are good candidates.

.. code-block:: cpp

    /* file: include/openPMD/IO/JSON/JSONFilePosition.hpp */
    #include "openPMD/IO/AbstractFilePosition.hpp"

    namespace openPMD
    {
    struct JSONFilePosition : public AbstractFilePosition
    {
        JSONFilePosition(uint64_t id)
            : id{id}
        { }

        uint64_t id;
    };
    } // openPMD

From this point, all that is left to do is implement the elementary IO operations provided in the IOHandlerImpl. The
``Parameter`` structs contain both input parameters (from storage to API) and output parameters (from API to storage).
The easy way to distinguish between the two parameter sets is their C++ type: Input parameters are
``std::shared_ptr`` s that allow you to pass the requested data to their destination. Output parameters are all objects
that are *not* ``std::shared_ptr`` s. The contract of each function call is outlined in
``include/openPMD/IO/AbstractIOHandlerImpl``.

.. code-block:: cpp

    /* file: src/IO/JSON/JSONIOHandlerImpl.cpp */
    #include "openPMD/IO/JSONIOHandlerImpl.hpp"

    namespace openPMD
    {
    void
    JSONIOHandlerImpl::createFile(Writable* writable,
                                  Parameter< Operation::CREATE_FILE > const& parameters)
    {
        if( !writable->written )
        {
            path dir(m_handler->directory);
            if( !exists(dir) )
                create_directories(dir);

            std::string name = m_handler->directory + parameters.name;
            if( !auxiliary::ends_with(name, ".json") )
                name += ".json";

            uint64_t id = /*...*/
            VERIFY(id >= 0, "Internal error: Failed to create JSON file");

            writable->written = true;
            writable->abstractFilePosition = std::make_shared< JSONFilePosition >(id);
        }
    }
    /*...*/
    } // openPMD

Note that you might have to keep track of open file handles if they have to be closed explicitly during destruction of
the IOHandlerImpl (prominent in C-style frameworks).
