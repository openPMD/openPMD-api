/* Copyright 2017-2019 Franz Poeschel
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/auxiliary/Memory.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/backend/Writable.hpp"
#include "openPMD/Datatype.hpp"
#include "openPMD/IO/JSON/JSONIOHandlerImpl.hpp"


namespace openPMD
{
#if openPMD_USE_VERIFY
#   define VERIFY( CONDITION, TEXT ) { if(!(CONDITION)) throw std::runtime_error((TEXT)); }
#else
#   define VERIFY( CONDITION, TEXT ) do{ (void)sizeof(CONDITION); } while( 0 );
#endif

#define VERIFY_ALWAYS( CONDITION, TEXT ) { if(!(CONDITION)) throw std::runtime_error((TEXT)); }


    JSONIOHandlerImpl::JSONIOHandlerImpl( AbstractIOHandler * handler ) :
        AbstractIOHandlerImpl( handler )
    {}


    JSONIOHandlerImpl::~JSONIOHandlerImpl( )
    {
        flush( );
    }


    std::future< void > JSONIOHandlerImpl::flush( )
    {
        AbstractIOHandlerImpl::flush( );
        for( auto const & file: m_dirty )
        {
            putJsonContents(
                file,
                false
            );
        }
        m_dirty.clear( );
        return std::future< void >( );
    }


    void JSONIOHandlerImpl::createFile(
        Writable * writable,
        Parameter< Operation::CREATE_FILE > const & parameters
    )
    {
        VERIFY_ALWAYS( m_handler->accessTypeBackend != AccessType::READ_ONLY,
            "Creating a file in read-only mode is not possible." );

        if( !writable->written )
        {
            std::string name = parameters.name;
            if( !auxiliary::ends_with(
                name,
                ".json"
            ) )
            {
                name += ".json";
            }

            auto res_pair = getPossiblyExisting( name );
            File shared_name = File( name );
            VERIFY_ALWAYS( !( m_handler->accessTypeBackend == AccessType::READ_WRITE &&
                              ( !std::get< 2 >( res_pair ) ||
                                auxiliary::file_exists( fullPath( std::get< 0 >( res_pair ) ) ) ) ),
                "Can only overwrite existing file in CREATE mode." );

            if( !std::get< 2 >( res_pair ) )
            {
                auto file = std::get< 0 >( res_pair );
                m_dirty.erase( file );
                m_jsonVals.erase( file );
                file.invalidate( );
            }

            std::string const dir( m_handler->directory );
            if( !auxiliary::directory_exists( dir ) )
            {
                auto success = auxiliary::create_directories( dir );
                VERIFY( success,
                    "Could not create directory." );
            }

            associateWithFile(
                writable,
                shared_name
            );
            this->m_dirty
                .emplace( shared_name );
            // make sure to overwrite!
            this->m_jsonVals[shared_name] =
                std::make_shared< nlohmann::json >( );


            writable->written = true;
            writable->abstractFilePosition =
                std::make_shared< JSONFilePosition >( );
        }
    }


    void JSONIOHandlerImpl::createPath(
        Writable * writable,
        Parameter< Operation::CREATE_PATH > const & parameter
    )
    {
        std::string path = parameter.path;
        /* Sanitize:
         * The JSON API does not like to have slashes in the end.
         */
        if( auxiliary::ends_with(
            path,
            "/"
        ) )
        {
            path = auxiliary::replace_last(
                path,
                "/",
                ""
            );
        }

        auto file = refreshFileFromParent( writable );

        auto * jsonVal = &*obtainJsonContents( file );
        if( !auxiliary::starts_with(
            path,
            "/"
        ) )
        { // path is relative
            auto filepos = setAndGetFilePosition(
                writable,
                false
            );

            jsonVal = &( *jsonVal )[filepos->id];
            ensurePath(
                jsonVal,
                path
            );
            path =
                filepos->id
                    .to_string( ) + "/" + path;
        }
        else
        {

            ensurePath(
                jsonVal,
                path
            );
        }

        m_dirty.emplace( file );
        writable->written = true;
        writable->abstractFilePosition =
            std::make_shared< JSONFilePosition >( nlohmann::json::json_pointer( path ) );
    }


    void JSONIOHandlerImpl::createDataset(
        Writable * writable,
        Parameter< Operation::CREATE_DATASET > const & parameter
    )
    {
        if( m_handler->accessTypeBackend == AccessType::READ_ONLY )
        {
            throw std::runtime_error( "Creating a dataset in a file opened as read only is not possible." );
        }
        if( !writable->written )
        {
            /* Sanitize name */
            std::string name = removeSlashes( parameter.name );

            auto file = refreshFileFromParent( writable );
            setAndGetFilePosition( writable );
            auto & jsonVal = obtainJsonContents( writable );
            // be sure to have a JSON object, not a list
            if( jsonVal.empty( ) )
            {
                jsonVal = nlohmann::json::object( );
            }
            setAndGetFilePosition(
                writable,
                name
            );
            auto & dset = jsonVal[name];
            dset["datatype"] = datatypeToString( parameter.dtype );
            dset["data"] = initializeNDArray( parameter.extent );
            writable->written = true;
            m_dirty.emplace( file );
        }
    }


    void JSONIOHandlerImpl::extendDataset(
        Writable * writable,
        Parameter< Operation::EXTEND_DATASET > const & parameters
    )
    {
        VERIFY_ALWAYS( m_handler->accessTypeBackend != AccessType::READ_ONLY,
            "Cannot extend a dataset in read-only mode." )
        refreshFileFromParent( writable );
        setAndGetFilePosition( writable );
        auto name = removeSlashes( parameters.name );
        auto & j = obtainJsonContents( writable )[name];

        try
        {
            auto datasetExtent = getExtent( j["data"] );
            VERIFY_ALWAYS( datasetExtent.size( ) ==
                           parameters.extent
                               .size( ),
                "Cannot change dimensionality of a dataset" )
            for( size_t currentdim = 0;
                currentdim <
                parameters.extent
                    .size( );
                currentdim++ )
            {
                VERIFY_ALWAYS( datasetExtent[currentdim] <=
                               parameters.extent[currentdim],
                    "Cannot shrink the extent of a dataset" )
            }
        } catch( json::basic_json::type_error & )
        {
            throw std::runtime_error( "The specified location contains no valid dataset" );
        }
        j["data"] = initializeNDArray( parameters.extent );
        writable->written = true;

    }


    void JSONIOHandlerImpl::openFile(
        Writable * writable,
        Parameter< Operation::OPEN_FILE > const & parameter
    )
    {
        if( !auxiliary::directory_exists( m_handler->directory ) )
        {
            throw no_such_file_error(
                "Supplied directory is not valid: " + m_handler->directory
            );
        }

        std::string name = parameter.name;
        if( !auxiliary::ends_with(
            name,
            ".json"
        ) )
        {
            name += ".json";
        }

        auto file = std::get< 0 >( getPossiblyExisting( name ) );

        associateWithFile(
            writable,
            file
        );

        writable->written = true;
        writable->abstractFilePosition =
            std::make_shared< JSONFilePosition >( );
    }


    void JSONIOHandlerImpl::openPath(
        Writable * writable,
        Parameter< Operation::OPEN_PATH > const & parameters
    )
    {
        auto file = refreshFileFromParent( writable );

        nlohmann::json * j = &obtainJsonContents( writable->parent );
        auto path = removeSlashes( parameters.path );
        path =
            path.empty( )
            ? filepositionOf( writable->parent )
            : filepositionOf( writable->parent ) + "/" + path;

        if( writable->abstractFilePosition )
        {
            *setAndGetFilePosition(
                writable,
                false
            ) = JSONFilePosition( json::json_pointer( path ) );
        }
        else
        {
            writable->abstractFilePosition =
                std::make_shared< JSONFilePosition >( json::json_pointer( path ) );
        }

        ensurePath(
            j,
            removeSlashes( parameters.path )
        );

        writable->written = true;
    }


    void JSONIOHandlerImpl::openDataset(
        Writable * writable,
        Parameter< Operation::OPEN_DATASET > & parameters
    )
    {
        refreshFileFromParent( writable );
        auto name = removeSlashes( parameters.name );
        auto & datasetJson = obtainJsonContents( writable->parent )[name];
        setAndGetFilePosition(
            writable,
            name
        );

        *parameters.dtype =
            Datatype( stringToDatatype( datasetJson["datatype"].get< std::string >( ) ) );
        *parameters.extent = getExtent( datasetJson["data"] );
        writable->written = true;

    }


    void JSONIOHandlerImpl::deleteFile(
        Writable * writable,
        Parameter< Operation::DELETE_FILE > const & parameters
    )
    {
        VERIFY_ALWAYS( m_handler->accessTypeBackend != AccessType::READ_ONLY,
            "Cannot delete files in read-only mode" )

        if( !writable->written )
        {
            return;
        }

        auto filename = auxiliary::ends_with(
            parameters.name,
            ".json"
        ) ? parameters.name : parameters.name + ".json";

        auto tuple = getPossiblyExisting( filename );
        if( !std::get< 2 >( tuple ) )
        {
            // file is already in the system
            auto file = std::get< 0 >( tuple );
            m_dirty.erase( file );
            m_jsonVals.erase( file );
            file.invalidate( );
        }

        std::remove( fullPath( filename ).c_str( ) );

        writable->written = false;
    }


    void JSONIOHandlerImpl::deletePath(
        Writable * writable,
        Parameter< Operation::DELETE_PATH > const & parameters
    )
    {
        VERIFY_ALWAYS( m_handler->accessTypeBackend != AccessType::READ_ONLY,
            "Cannot delete paths in read-only mode" )

        if( !writable->written )
        {
            return;
        }

        VERIFY_ALWAYS( !auxiliary::starts_with(
            parameters.path,
            '/'
        ),
            "Paths passed for deletion should be relative, the given path is absolute (starts with '/')" )
        auto file = refreshFileFromParent( writable );
        auto filepos = setAndGetFilePosition(
            writable,
            false
        );
        auto path = removeSlashes( parameters.path );
        VERIFY( !path.empty( ),
            "No path passed for deletion." )
        nlohmann::json * j;
        if( path == "." )
        {
            auto
                s =
                filepos->id
                    .to_string( );
            if( s == "/" )
            {
                throw std::runtime_error( "Cannot delete the root group" );
            }

            auto i = s.rfind( '/' );
            path = s;
            path.replace(
                0,
                i + 1,
                ""
            );
            // path should now be equal to the name of the current group
            // go up one group

            // go to parent directory
            // parent exists since we have verified that the current
            // directory is != root
            parentDir( s );
            j =
                &( *obtainJsonContents( file ) )[nlohmann::json::json_pointer( s )];
        }
        else
        {
            if( auxiliary::starts_with(
                path,
                "./"
            ) )
            {
                path = auxiliary::replace_first(
                    path,
                    "./",
                    ""
                );
            }
            j = &obtainJsonContents( writable );
        }
        nlohmann::json * lastPointer = j;
        bool needToDelete = true;
        auto splitPath = auxiliary::split(
            path,
            "/"
        );
        // be careful not to create the group by accident
        // the loop will execute at least once
        for( auto const & folder: splitPath )
        {
            auto it = j->find( folder );
            if( it == j->end( ) )
            {
                needToDelete = false;
                break;
            }
            else
            {
                lastPointer = j;
                j = &it.value( );
            }
        }
        if( needToDelete )
        {
            lastPointer->erase(
                splitPath[splitPath.size( ) - 1]
            );
        }

        putJsonContents( file );
        writable->abstractFilePosition
            .reset( );
        writable->written = false;
    }


    void JSONIOHandlerImpl::deleteDataset(
        Writable * writable,
        Parameter< Operation::DELETE_DATASET > const & parameters
    )
    {
        VERIFY_ALWAYS( m_handler->accessTypeBackend != AccessType::READ_ONLY,
            "Cannot delete datasets in read-only mode" )

        if( !writable->written )
        {
            return;
        }

        auto filepos = setAndGetFilePosition(
            writable,
            false
        );

        auto file = refreshFileFromParent( writable );
        auto dataset = removeSlashes( parameters.name );
        nlohmann::json * parent;
        if( dataset == "." )
        {
            auto
                s =
                filepos->id
                    .to_string( );
            if( s.empty( ) )
            {
                throw std::runtime_error( "Invalid position for a dataset in the JSON file." );
            }
            dataset = s;
            auto i = dataset.rfind( '/' );
            dataset.replace(
                0,
                i + 1,
                ""
            );

            parentDir( s );
            parent =
                &( *obtainJsonContents( file ) )[nlohmann::json::json_pointer( s )];
        }
        else
        {
            parent = &obtainJsonContents( writable );
        }
        parent->erase( dataset );
        putJsonContents( file );
        writable->written = false;
        writable->abstractFilePosition
            .reset( );
    }


    void JSONIOHandlerImpl::deleteAttribute(
        Writable * writable,
        Parameter< Operation::DELETE_ATT > const & parameters
    )
    {
        VERIFY_ALWAYS( m_handler->accessTypeBackend != AccessType::READ_ONLY,
            "Cannot delete attributes in read-only mode" )
        if( !writable->written )
        {
            return;
        }
        setAndGetFilePosition( writable );
        auto file = refreshFileFromParent( writable );
        auto & j = obtainJsonContents( writable );
        j.erase( parameters.name );
        putJsonContents( file );
    }


    void JSONIOHandlerImpl::writeDataset(
        Writable * writable,
        Parameter< Operation::WRITE_DATASET > const & parameters
    )
    {
        VERIFY_ALWAYS( m_handler->accessTypeBackend != AccessType::READ_ONLY,
            "Cannot write data in read-only mode." );

        auto pos = setAndGetFilePosition( writable );
        auto file = refreshFileFromParent( writable );
        auto & j = obtainJsonContents( writable );

        verifyDataset(
            parameters,
            j
        );


        DatasetWriter dw;
        switchType(
            parameters.dtype,
            dw,
            j,
            parameters
        );

        writable->written = true;
        putJsonContents( file );
    }


    void JSONIOHandlerImpl::writeAttribute(
        Writable * writable,
        Parameter< Operation::WRITE_ATT > const & parameter
    )
    {
        if( m_handler->accessTypeBackend == AccessType::READ_ONLY )
        {
            throw std::runtime_error( "Creating a dataset in a file opened as read only is not possible." );
        }

        /* Sanitize name */
        std::string name = removeSlashes( parameter.name );

        auto file = refreshFileFromParent( writable );
        auto jsonVal = obtainJsonContents( file );
        auto filePosition = setAndGetFilePosition( writable );
        if( ( *jsonVal )[filePosition->id]["attributes"].empty( ) )
        {
            ( *jsonVal )[filePosition->id]["attributes"] =
                nlohmann::json::object( );
        }
        nlohmann::json value;
        AttributeWriter aw;
        switchType(
            parameter.dtype,
            aw,
            value,
            parameter.resource
        );
        ( *jsonVal )[filePosition->id]["attributes"][parameter.name] = {
            {
                "datatype",
                datatypeToString( parameter.dtype )
            },
            {
                "value",
                value
            }
        };
        writable->written = true;
        m_dirty.emplace( file );
    }


    void JSONIOHandlerImpl::readDataset(
        Writable * writable,
        Parameter< Operation::READ_DATASET > & parameters
    )
    {
        refreshFileFromParent( writable );
        setAndGetFilePosition( writable );
        auto & j = obtainJsonContents( writable );
        verifyDataset(
            parameters,
            j
        );

        try
        {
            DatasetReader dr;
            switchType(
                parameters.dtype,
                dr,
                j["data"],
                parameters
            );
        } catch( json::basic_json::type_error & )
        {
            throw std::runtime_error( "The given path does not contain a valid dataset." );
        }
    }


    void JSONIOHandlerImpl::readAttribute(
        Writable * writable,
        Parameter< Operation::READ_ATT > & parameters
    )
    {
        VERIFY_ALWAYS( writable->written,
            "Attributes have to be written before reading." )
        refreshFileFromParent( writable );
        auto name = removeSlashes( parameters.name );
        auto & jsonLoc = obtainJsonContents( writable )["attributes"];
        setAndGetFilePosition( writable );
        VERIFY_ALWAYS( hasKey(
            jsonLoc,
            name
        ),
            "No such attribute in the given location." )
        auto & j = jsonLoc[name];
        try
        {
            *parameters.dtype =
                Datatype( stringToDatatype( j["datatype"].get< std::string >( ) ) );
            AttributeReader ar;
            switchType(
                *parameters.dtype,
                ar,
                j["value"],
                parameters
            );
        } catch( json::type_error & )
        {
            throw std::runtime_error( "The given location does not contain a properly formatted attribute" );
        }
    }


    void JSONIOHandlerImpl::listPaths(
        Writable * writable,
        Parameter< Operation::LIST_PATHS > & parameters
    )
    {
        VERIFY_ALWAYS( writable->written,
            "Values have to be written before reading a directory" );
        auto & j = obtainJsonContents( writable );
        setAndGetFilePosition( writable );
        refreshFileFromParent( writable );
        parameters.paths
            ->clear( );
        for( auto it = j.begin( ); it != j.end( ); it++ )
        {
            if( isGroup( it ) )
            {
                parameters.paths
                    ->push_back( it.key( ) );
            }
        }
    }


    void JSONIOHandlerImpl::listDatasets(
        Writable * writable,
        Parameter< Operation::LIST_DATASETS > & parameters
    )
    {
        VERIFY_ALWAYS( writable->written,
            "Datasets have to be written before reading." )
        refreshFileFromParent( writable );
        auto filePosition = setAndGetFilePosition( writable );
        auto & j = obtainJsonContents( writable );
        parameters.datasets
            ->clear( );
        for( auto it = j.begin( ); it != j.end( ); it++ )
        {
            if( isDataset( it.value() ) )
            {
                parameters.datasets
                    ->push_back( it.key( ) );
            }
        }
    }


    void JSONIOHandlerImpl::listAttributes(
        Writable * writable,
        Parameter< Operation::LIST_ATTS > & parameters
    )
    {
        VERIFY_ALWAYS( writable->written,
            "Attributes have to be written before reading." )
        refreshFileFromParent( writable );
        auto filePosition = setAndGetFilePosition( writable );
        auto & j = obtainJsonContents( writable )["attributes"];
        for( auto it = j.begin( ); it != j.end( ); it++ )
        {
            parameters.attributes
                ->push_back( it.key( ) );
        }
    }


    std::shared_ptr< JSONIOHandlerImpl::FILEHANDLE >
    JSONIOHandlerImpl::getFilehandle(
        File fileName,
        AccessType accessType
    )
    {
        VERIFY_ALWAYS( fileName.valid( ),
            "Tried opening a file that has been overwritten or deleted." )
        auto path = fullPath( std::move( fileName ) );
        auto fs = std::make_shared< std::fstream >( );
        switch( accessType )
        {
            case AccessType::CREATE:
            case AccessType::READ_WRITE:
                fs->open(
                    path,
                    std::ios_base::out | std::ios_base::trunc
                );
                break;
            case AccessType::READ_ONLY:
                fs->open(
                    path,
                    std::ios_base::in
                );
                break;
        }
        VERIFY( fs->good( ),
            "Failed opening a file" );
        return fs;
    }


    std::string JSONIOHandlerImpl::fullPath( File fileName )
    {
        return fullPath( *fileName );
    }


    std::string JSONIOHandlerImpl::fullPath( std::string const & fileName )
    {
        if( auxiliary::ends_with(
            m_handler->directory,
            "/"
        ) )
        {
            return m_handler->directory + fileName;
        }
        else
        {
            return m_handler->directory + "/" + fileName;
        }
    }


    void JSONIOHandlerImpl::parentDir( std::string & s )
    {
        auto i = s.rfind( '/' );
        if( i != std::string::npos )
        {
            s.replace(
                i,
                s.size( ) - i,
                ""
            );
            s.shrink_to_fit( );
        }
    }


    std::string JSONIOHandlerImpl::filepositionOf( Writable * writable )
    {
        return std::dynamic_pointer_cast< JSONFilePosition >( writable->abstractFilePosition )->id
            .to_string( );
    }


    template<
        typename T,
        typename Visitor
    >
    void JSONIOHandlerImpl::syncMultidimensionalJson(
        nlohmann::json & j,
        Offset const & offset,
        Extent const & extent,
        Extent const & multiplicator,
        Visitor visitor,
        T * data,
        size_t currentdim
    )
    {
        // Offset only relevant for JSON, the array data is contiguous
        auto off = offset[currentdim];
        // maybe rewrite iteratively, using a stack that stores for each level the
        // current iteration value i

        if( currentdim == offset.size( ) - 1 )
        {
            for( std::size_t i = 0; i < extent[currentdim]; ++i )
            {
                visitor(
                    j[i + off],
                    data[i]
                );
            }
        }
        else
        {
            for( std::size_t i = 0; i < extent[currentdim]; ++i )
            {
                syncMultidimensionalJson<
                    T,
                    Visitor
                >(
                    j[i + off],
                    offset,
                    extent,
                    multiplicator,
                    visitor,
                    data + i * multiplicator[currentdim],
                    currentdim + 1
                );
            }
        }
    }


    // multiplicators: an array [m_0,...,m_n] s.t.
    // data[i_0]...[i_n] = data[m_0*i_0+...+m_n*i_n]
    // (m_n = 1)
    Extent JSONIOHandlerImpl::getMultiplicators( Extent const & extent )
    {
        Extent res( extent );
        Extent::value_type n = 1;
        size_t i = extent.size( );
        do
        {
            --i;
            res[i] = n;
            n *= extent[i];
        }
        while( i > 0 );
        return res;
    }


    nlohmann::json JSONIOHandlerImpl::initializeNDArray( Extent const & extent )
    {
        // idea: begin from the innermost shale and copy the result into the
        // outer shales
        nlohmann::json accum;
        nlohmann::json old;
        auto * accum_ptr = & accum;
        auto * old_ptr = & old;
        for( auto it = extent.rbegin( ); it != extent.rend( ); it++ )
        {
            std::swap(old_ptr, accum_ptr);
            *accum_ptr = nlohmann::json {};
            for( Extent::value_type i = 0; i < *it; i++ )
            {
                (*accum_ptr)[i] = *old_ptr; // copy boi
            }
        }
        return *accum_ptr;
    }


    Extent JSONIOHandlerImpl::getExtent( nlohmann::json & j )
    {
        Extent res;
        nlohmann::json * ptr = &j;
        while( ptr->is_array( ) )
        {
            res.push_back( ptr->size( ) );
            ptr = &( *ptr )[0];
        }
        return res;
    }


    std::string JSONIOHandlerImpl::removeSlashes( std::string s )
    {
        if( auxiliary::starts_with(
            s,
            '/'
        ) )
        {
            s = auxiliary::replace_first(
                s,
                "/",
                ""
            );
        }
        if( auxiliary::ends_with(
            s,
            '/'
        ) )
        {
            s = auxiliary::replace_last(
                s,
                "/",
                ""
            );
        }
        return s;
    }


    template< typename KeyT >
    bool JSONIOHandlerImpl::hasKey(
        nlohmann::json & j,
        KeyT && key
    )
    {
        return j.find( std::forward< KeyT >( key ) ) != j.end( );
    }


    void JSONIOHandlerImpl::ensurePath(
        nlohmann::json * jsonp,
        std::string path
    )
    {
        auto groups = auxiliary::split(
            path,
            "/"
        );
        for( std::string & group: groups )
        {
            // Enforce a JSON object
            // the library will automatically create a list if the first
            // key added to it is parseable as an int
            jsonp = &( *jsonp )[group];
            if (jsonp->is_null())
            {
                *jsonp = nlohmann::json::object();
            }
        }
    }


    std::tuple<
        File,
        std::unordered_map<
            Writable *,
            File
        >::iterator,
        bool
    > JSONIOHandlerImpl::getPossiblyExisting( std::string file )
    {

        auto it = std::find_if(
            m_files.begin( ),
            m_files.end( ),
            [file](
                std::unordered_map<
                    Writable *,
                    File
                >::value_type const & entry
            )
            {
                return *entry.second == file &&
                       entry.second
                           .valid( );
            }
        );

        bool newlyCreated;
        File name;
        if( it == m_files.end( ) )
        {
            name = file;
            newlyCreated = true;
        }
        else
        {
            name = it->second;
            newlyCreated = false;
        }
        return std::tuple<
            File,
            std::unordered_map<
                Writable *,
                File
            >::iterator,
            bool
        >(
            std::move( name ),
            it,
            newlyCreated
        );
    }


    std::shared_ptr< nlohmann::json >
    JSONIOHandlerImpl::obtainJsonContents( File file )
    {
        VERIFY_ALWAYS( file.valid( ),
            "File has been overwritten or deleted before reading" );
        auto it = m_jsonVals.find( file );
        if( it != m_jsonVals.end( ) )
        {
            return it->second;
        }
        // read from file
        auto fh = getFilehandle(
            file,
            AccessType::READ_ONLY
        );
        std::shared_ptr< nlohmann::json >
            res = std::make_shared< nlohmann::json >( );
        *fh >> *res;
        VERIFY( fh->good( ),
            "Failed reading from a file." );
        m_jsonVals.emplace(
            file,
            res
        );
        return res;
    }


    nlohmann::json &
    JSONIOHandlerImpl::obtainJsonContents( Writable * writable )
    {
        auto file = refreshFileFromParent( writable );
        auto filePosition = setAndGetFilePosition(
            writable,
            false
        );
        return ( *obtainJsonContents( file ) )[filePosition->id];
    }


    void JSONIOHandlerImpl::putJsonContents(
        File filename,
        bool unsetDirty // = true
    )
    {
        VERIFY_ALWAYS( filename.valid( ),
            "File has been overwritten/deleted before writing" );
        auto it = m_jsonVals.find( filename );
        if( it != m_jsonVals.end( ) )
        {
            auto fh = getFilehandle(
                filename,
                AccessType::CREATE
            );
            ( *it->second )["platform_byte_widths"] = platformSpecifics( );
            *fh << *it->second << std::endl;
            VERIFY( fh->good( ),
                "Failed writing data to disk." )
            m_jsonVals.erase( it );
            if( unsetDirty )
            {
                m_dirty.erase( filename );
            }
        }

    }


    std::shared_ptr< JSONFilePosition >
    JSONIOHandlerImpl::setAndGetFilePosition(
        Writable * writable,
        std::string extend
    )
    {
        std::string path;
        if( writable->abstractFilePosition )
        {
            // do NOT reuse the old pointer, we want to change the file position
            // only for the writable!
            path = filepositionOf( writable ) + "/" + extend;
        }
        else if( writable->parent )
        {
            path = filepositionOf( writable->parent ) + "/" + extend;
        }
        else
        { // we are root
            path = extend;
            if( !auxiliary::starts_with(
                path,
                "/"
            ) )
            {
                path = "/" + path;
            }
        }
        auto
            res =
            std::make_shared< JSONFilePosition >( json::json_pointer( path ) );

        writable->abstractFilePosition = res;

        return res;
    }


    std::shared_ptr< JSONFilePosition >
    JSONIOHandlerImpl::setAndGetFilePosition(
        Writable * writable,
        bool write
    )
    {
        std::shared_ptr< AbstractFilePosition > res;


        if( writable->abstractFilePosition )
        {
            res = writable->abstractFilePosition;
        }
        else if( writable->parent )
        {
            res =
                writable->parent
                    ->abstractFilePosition;
        }
        else
        { // we are root
            res = std::make_shared< JSONFilePosition >( );
        }
        if( write )
        {
            writable->abstractFilePosition = res;
        }
        return std::dynamic_pointer_cast< JSONFilePosition >( res );
    }


    File JSONIOHandlerImpl::refreshFileFromParent( Writable * writable )
    {
        if( writable->parent )
        {
            auto
                file =
                m_files.find( writable->parent )
                    ->second;
            associateWithFile(
                writable,
                file
            );
            return file;
        }
        else
        {
            return m_files.find( writable )
                ->second;
        }
    }


    void JSONIOHandlerImpl::associateWithFile(
        Writable * writable,
        File file
    )
    {
        // make sure to overwrite
        m_files[writable] = std::move( file );
    }


    bool JSONIOHandlerImpl::isDataset( nlohmann::json const & j )
    {
        if( !j.is_object( ) )
        {
            return false;
        }
        auto i = j.find( "data" );
        return i != j.end( ) && i.value( ).is_array();
    }


    bool JSONIOHandlerImpl::isGroup( nlohmann::json::const_iterator it )
    {
        auto & j = it.value();
        if( it.key() == "attributes" || it.key() == "platform_byte_widths" || !j.is_object( ) )
        {
            return false;
        }
        auto i = j.find( "data" );
        return i == j.end( ) || !i.value( ).is_array();
    }


    template< typename Param >
    void JSONIOHandlerImpl::verifyDataset(
        Param const & parameters,
        nlohmann::json & j
    )
    {
        VERIFY_ALWAYS( isDataset(j),
            "Specified dataset does not exist or is not a dataset." );

        try
        {
            auto datasetExtent = getExtent( j["data"] );
            VERIFY_ALWAYS( datasetExtent.size( ) ==
                           parameters.extent
                               .size( ),
                "Read/Write request does not fit the dataset's dimension" );
            for( unsigned int dimension = 0;
                dimension <
                parameters.extent
                    .size( );
                dimension++ )
            {
                VERIFY_ALWAYS( parameters.offset[dimension] +
                               parameters.extent[dimension] <=
                               datasetExtent[dimension],
                    "Read/Write request exceeds the dataset's size" );
            }
            Datatype
                dt = stringToDatatype( j["datatype"].get< std::string >( ) );
            VERIFY_ALWAYS( dt == parameters.dtype,
                "Read/Write request does not fit the dataset's type" );
        } catch( json::basic_json::type_error & )
        {
            throw std::runtime_error( "The given path does not contain a valid dataset." );
        }
    }


    nlohmann::json JSONIOHandlerImpl::platformSpecifics( )
    {
        nlohmann::json res;
        static Datatype datatypes[] = {
            Datatype::CHAR,
            Datatype::UCHAR,
            Datatype::SHORT,
            Datatype::INT,
            Datatype::LONG,
            Datatype::LONGLONG,
            Datatype::USHORT,
            Datatype::UINT,
            Datatype::ULONG,
            Datatype::ULONGLONG,
            Datatype::FLOAT,
            Datatype::DOUBLE,
            Datatype::LONG_DOUBLE,
            Datatype::BOOL
        };
        for( auto it = std::begin( datatypes );
            it != std::end( datatypes );
            it++ )
        {
            res[datatypeToString( *it )] = toBytes( *it );
        }
        return res;
    }


    template< typename T >
    void JSONIOHandlerImpl::DatasetWriter::operator()(
        nlohmann::json & json,
        const Parameter< Operation::WRITE_DATASET > & parameters
    )
    {
        CppToJSON< T > ctj;
        syncMultidimensionalJson(
            json["data"],
            parameters.offset,
            parameters.extent,
            getMultiplicators( parameters.extent ),
            [&ctj](
                nlohmann::json & j,
                T const & data
            )
            {
                j = ctj( data );
            },
            static_cast<T const *>(parameters.data
                .get( ))
        );
    }


    template< int n >
    void JSONIOHandlerImpl::DatasetWriter::operator()(
        nlohmann::json &,
        const Parameter< Operation::WRITE_DATASET > &
    )
    {
        throw std::runtime_error( "Unknown datatype given for writing." );
    }


    template< typename T >
    void JSONIOHandlerImpl::DatasetReader::operator()(
        nlohmann::json & json,
        Parameter< Operation::READ_DATASET > & parameters
    )
    {
        JsonToCpp<
            T
        > jtc;
        syncMultidimensionalJson(
            json,
            parameters.offset,
            parameters.extent,
            getMultiplicators( parameters.extent ),
            [&jtc](
                nlohmann::json & j,
                T & data
            )
            {
                data = jtc( j );
            },
            static_cast<T *>(parameters.data
                .get( ))
        );
    }


    template< int n >
    void JSONIOHandlerImpl::DatasetReader::operator()(
        nlohmann::json &,
        Parameter< Operation::READ_DATASET > &
    )
    {
        throw std::runtime_error( "Unknown datatype while reading a dataset." );
    }


    template< typename T >
    void JSONIOHandlerImpl::AttributeWriter::operator()(
        nlohmann::json & value,
        Attribute::resource const & resource
    )
    {
        CppToJSON< T > ctj;
        value = ctj( variantSrc::get< T >( resource ) );
    }


    template< int n >
    void JSONIOHandlerImpl::AttributeWriter::operator()(
        nlohmann::json &,
        Attribute::resource const &
    )
    {
        throw std::runtime_error( "Unknown datatype in attribute writing." );
    }


    template< int n >
    void JSONIOHandlerImpl::AttributeReader::operator()(
        nlohmann::json &,
        Parameter< Operation::READ_ATT > &
    )
    {
        throw std::runtime_error( "Unknown datatype while reading attribute." );
    }


    template< typename T >
    void JSONIOHandlerImpl::AttributeReader::operator()(
        nlohmann::json & json,
        Parameter< Operation::READ_ATT > & parameters
    )
    {
        JsonToCpp<
            T
        > jtc;
        *parameters.resource = jtc(
            json
        );
    }


    template< typename T >
    nlohmann::json
    JSONIOHandlerImpl::CppToJSON< T >::operator()( const T & val )
    {
        return nlohmann::json( val );
    }


    template< typename T >
    nlohmann::json
    JSONIOHandlerImpl::CppToJSON< std::vector< T > >::operator()( const std::vector< T > & v )
    {
        nlohmann::json j;
        CppToJSON< T > ctj;
        for( auto const & a: v )
        {
            j.emplace_back( ctj( a ) );
        }
        return j;
    }


    template< typename T, int n >
    nlohmann::json JSONIOHandlerImpl::CppToJSON<
        std::array<
            T,
            n
        >
    >::operator()(
        const std::array<
            T,
            n
        > & v
    )
    {
        nlohmann::json j;
        CppToJSON< T > ctj;
        for( auto const & a: v )
        {
            j.emplace_back( ctj( a ) );
        }
        return j;
    }


    template<
        typename T,
        typename Dummy
    >
    T JSONIOHandlerImpl::JsonToCpp<
        T,
        Dummy
    >::operator()( nlohmann::json const & json )
    { return json.get< T >( ); }


    template< typename T >
    std::vector< T >
    JSONIOHandlerImpl::JsonToCpp< std::vector< T > >::operator()( nlohmann::json const & json )
    {
        std::vector< T > v;
        JsonToCpp< T > jtp;
        for( auto const & j: json )
        {
            v.emplace_back( jtp( j ) );
        }
        return v;
    }


    template< typename T, int n >
    std::array<
        T,
        n
    > JSONIOHandlerImpl::JsonToCpp<
        std::array<
            T,
            n
        >
    >::operator()( nlohmann::json const & json )
    {
        std::array<
            T,
            n
        > a;
        JsonToCpp< T > jtp;
        size_t i = 0;
        for( auto const & j: json )
        {
            a[i] = jtp( j );
            i++;
        }
        return a;
    }


    template<
        typename T
    >
    T JSONIOHandlerImpl::JsonToCpp<
        T,
        typename std::enable_if<
            std::is_floating_point<
                T
            >::value
        >::type
    >::operator()( nlohmann::json const & j ) {
        try {
            return j.get<T>();
        } catch (...) {
            return std::numeric_limits<T>::quiet_NaN();
        }
    }

} // namespace openPMD
