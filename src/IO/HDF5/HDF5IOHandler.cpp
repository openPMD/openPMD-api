/* Copyright 2017-2021 Fabian Koller
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
#include "openPMD/IO/HDF5/HDF5IOHandler.hpp"
#include "openPMD/IO/HDF5/HDF5IOHandlerImpl.hpp"
#include "openPMD/auxiliary/Environment.hpp"

#if openPMD_HAVE_HDF5
#include "openPMD/Datatype.hpp"
#include "openPMD/IO/HDF5/HDF5Auxiliary.hpp"
#include "openPMD/IO/HDF5/HDF5FilePosition.hpp"
#include "openPMD/IO/IOTask.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/backend/Attribute.hpp"

#include <hdf5.h>
#endif

#include <complex>
#include <cstring>
#include <future>
#include <iostream>
#include <stack>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace openPMD
{
#if openPMD_HAVE_HDF5
#if openPMD_USE_VERIFY
#define VERIFY(CONDITION, TEXT)                                                \
    {                                                                          \
        if (!(CONDITION))                                                      \
            throw std::runtime_error((TEXT));                                  \
    }
#else
#define VERIFY(CONDITION, TEXT)                                                \
    do                                                                         \
    {                                                                          \
        (void)sizeof(CONDITION);                                               \
    } while (0)
#endif

HDF5IOHandlerImpl::HDF5IOHandlerImpl(
    AbstractIOHandler *handler, nlohmann::json config)
    : AbstractIOHandlerImpl(handler)
    , m_datasetTransferProperty{H5P_DEFAULT}
    , m_fileAccessProperty{H5P_DEFAULT}
    , m_H5T_BOOL_ENUM{H5Tenum_create(H5T_NATIVE_INT8)}
    , m_H5T_CFLOAT{H5Tcreate(H5T_COMPOUND, sizeof(float) * 2)}
    , m_H5T_CDOUBLE{H5Tcreate(H5T_COMPOUND, sizeof(double) * 2)}
    , m_H5T_CLONG_DOUBLE{H5Tcreate(H5T_COMPOUND, sizeof(long double) * 2)}
{
    // create a h5py compatible bool type
    VERIFY(
        m_H5T_BOOL_ENUM >= 0,
        "[HDF5] Internal error: Failed to create bool enum");
    std::string t{"TRUE"};
    std::string f{"FALSE"};
    int64_t tVal = 1;
    int64_t fVal = 0;
    herr_t status;
    status = H5Tenum_insert(m_H5T_BOOL_ENUM, t.c_str(), &tVal);
    VERIFY(
        status == 0, "[HDF5] Internal error: Failed to insert into HDF5 enum");
    status = H5Tenum_insert(m_H5T_BOOL_ENUM, f.c_str(), &fVal);
    VERIFY(
        status == 0, "[HDF5] Internal error: Failed to insert into HDF5 enum");

    // create h5py compatible complex types
    VERIFY(
        m_H5T_CFLOAT >= 0,
        "[HDF5] Internal error: Failed to create complex float");
    VERIFY(
        m_H5T_CDOUBLE >= 0,
        "[HDF5] Internal error: Failed to create complex double");
    VERIFY(
        m_H5T_CLONG_DOUBLE >= 0,
        "[HDF5] Internal error: Failed to create complex long double");
    H5Tinsert(m_H5T_CFLOAT, "r", 0, H5T_NATIVE_FLOAT);
    H5Tinsert(m_H5T_CFLOAT, "i", sizeof(float), H5T_NATIVE_FLOAT);
    H5Tinsert(m_H5T_CDOUBLE, "r", 0, H5T_NATIVE_DOUBLE);
    H5Tinsert(m_H5T_CDOUBLE, "i", sizeof(double), H5T_NATIVE_DOUBLE);
    H5Tinsert(m_H5T_CLONG_DOUBLE, "r", 0, H5T_NATIVE_LDOUBLE);
    H5Tinsert(m_H5T_CLONG_DOUBLE, "i", sizeof(long double), H5T_NATIVE_LDOUBLE);

    m_chunks = auxiliary::getEnvString("OPENPMD_HDF5_CHUNKS", "auto");
    // JSON option can overwrite env option:
    if (config.contains("hdf5"))
    {
        m_config = std::move(config["hdf5"]);

        // check for global dataset configs
        if (m_config.json().contains("dataset"))
        {
            auto datasetConfig = m_config["dataset"];
            if (datasetConfig.json().contains("chunks"))
            {
                m_chunks = datasetConfig["chunks"].json().get<std::string>();
            }
        }
        if (m_chunks != "auto" && m_chunks != "none")
        {
            std::cerr << "Warning: HDF5 chunking option set to an invalid "
                         "value '"
                      << m_chunks << "'. Reset to 'auto'." << std::endl;
            m_chunks = "auto";
        }

        // unused params
        auto shadow = m_config.invertShadow();
        if (shadow.size() > 0)
        {
            std::cerr << "Warning: parts of the JSON configuration for "
                         "HDF5 remain unused:\n"
                      << shadow << std::endl;
        }
    }

#if H5_VERSION_GE(1, 10, 0) && openPMD_HAVE_MPI
    auto const hdf5_collective_metadata =
        auxiliary::getEnvString("OPENPMD_HDF5_COLLECTIVE_METADATA", "ON");
    if (hdf5_collective_metadata == "ON")
        m_hdf5_collective_metadata = 1;
    else
        m_hdf5_collective_metadata = 0;
#endif
}

HDF5IOHandlerImpl::~HDF5IOHandlerImpl()
{
    herr_t status;
    status = H5Tclose(m_H5T_BOOL_ENUM);
    if (status < 0)
        std::cerr << "[HDF5] Internal error: Failed to close bool enum\n";
    status = H5Tclose(m_H5T_CFLOAT);
    if (status < 0)
        std::cerr
            << "[HDF5] Internal error: Failed to close complex float type\n";
    status = H5Tclose(m_H5T_CDOUBLE);
    if (status < 0)
        std::cerr
            << "[HDF5] Internal error: Failed to close complex double type\n";
    status = H5Tclose(m_H5T_CLONG_DOUBLE);
    if (status < 0)
        std::cerr << "[HDF5] Internal error: Failed to close complex long "
                     "double type\n";

    while (!m_openFileIDs.empty())
    {
        auto file = m_openFileIDs.begin();
        status = H5Fclose(*file);
        if (status < 0)
            std::cerr << "[HDF5] Internal error: Failed to close HDF5 file "
                         "(serial)\n";
        m_openFileIDs.erase(file);
    }
    if (m_datasetTransferProperty != H5P_DEFAULT)
    {
        status = H5Pclose(m_datasetTransferProperty);
        if (status < 0)
            std::cerr << "[HDF5] Internal error: Failed to close HDF5 dataset "
                         "transfer property\n";
    }
    if (m_fileAccessProperty != H5P_DEFAULT)
    {
        status = H5Pclose(m_fileAccessProperty);
        if (status < 0)
            std::cerr << "[HDF5] Internal error: Failed to close HDF5 file "
                         "access property\n";
    }
}

void HDF5IOHandlerImpl::createFile(
    Writable *writable, Parameter<Operation::CREATE_FILE> const &parameters)
{
    if (m_handler->m_backendAccess == Access::READ_ONLY)
        throw std::runtime_error(
            "[HDF5] Creating a file in read-only mode is not possible.");

    if (!writable->written)
    {
        if (!auxiliary::directory_exists(m_handler->directory))
        {
            bool success = auxiliary::create_directories(m_handler->directory);
            VERIFY(
                success,
                "[HDF5] Internal error: Failed to create directories during "
                "HDF5 file creation");
        }

        std::string name = m_handler->directory + parameters.name;
        if (!auxiliary::ends_with(name, ".h5"))
            name += ".h5";
        unsigned flags;
        if (m_handler->m_backendAccess == Access::CREATE)
            flags = H5F_ACC_TRUNC;
        else
            flags = H5F_ACC_EXCL;
        hid_t id =
            H5Fcreate(name.c_str(), flags, H5P_DEFAULT, m_fileAccessProperty);
        VERIFY(id >= 0, "[HDF5] Internal error: Failed to create HDF5 file");

        writable->written = true;
        writable->abstractFilePosition =
            std::make_shared<HDF5FilePosition>("/");

        m_fileNames[writable] = name;
        m_fileNamesWithID[std::move(name)] = id;
        m_openFileIDs.insert(id);
    }
}

void HDF5IOHandlerImpl::createPath(
    Writable *writable, Parameter<Operation::CREATE_PATH> const &parameters)
{
    if (m_handler->m_backendAccess == Access::READ_ONLY)
        throw std::runtime_error(
            "[HDF5] Creating a path in a file opened as read only is not "
            "possible.");

    hid_t gapl = H5Pcreate(H5P_GROUP_ACCESS);
#if H5_VERSION_GE(1, 10, 0) && openPMD_HAVE_MPI
    if (m_hdf5_collective_metadata)
    {
        H5Pset_all_coll_metadata_ops(gapl, true);
    }
#endif

    herr_t status;

    if (!writable->written)
    {
        /* Sanitize path */
        std::string path = parameters.path;
        if (auxiliary::starts_with(path, '/'))
            path = auxiliary::replace_first(path, "/", "");
        if (!auxiliary::ends_with(path, '/'))
            path += '/';

        /* Open H5Object to write into */
        Writable *position;
        if (writable->parent)
            position = writable->parent;
        else
            position = writable; /* root does not have a parent but might still
                                    have to be written */
        File file = getFile(position).get();
        hid_t node_id =
            H5Gopen(file.id, concrete_h5_file_position(position).c_str(), gapl);
        VERIFY(
            node_id >= 0,
            "[HDF5] Internal error: Failed to open HDF5 group during path "
            "creation");

        /* Create the path in the file */
        std::stack<hid_t> groups;
        groups.push(node_id);
        for (std::string const &folder : auxiliary::split(path, "/", false))
        {
            // avoid creation of paths that already exist
            htri_t const found =
                H5Lexists(groups.top(), folder.c_str(), H5P_DEFAULT);
            if (found > 0)
                continue;

            hid_t group_id = H5Gcreate(
                groups.top(),
                folder.c_str(),
                H5P_DEFAULT,
                H5P_DEFAULT,
                H5P_DEFAULT);
            VERIFY(
                group_id >= 0,
                "[HDF5] Internal error: Failed to create HDF5 group during "
                "path creation");
            groups.push(group_id);
        }

        /* Close the groups */
        while (!groups.empty())
        {
            status = H5Gclose(groups.top());
            VERIFY(
                status == 0,
                "[HDF5] Internal error: Failed to close HDF5 group during path "
                "creation");
            groups.pop();
        }

        writable->written = true;
        writable->abstractFilePosition =
            std::make_shared<HDF5FilePosition>(path);

        m_fileNames[writable] = file.name;
    }

    status = H5Pclose(gapl);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close HDF5 property during path "
        "creation");
}

void HDF5IOHandlerImpl::createDataset(
    Writable *writable, Parameter<Operation::CREATE_DATASET> const &parameters)
{
    if (m_handler->m_backendAccess == Access::READ_ONLY)
        throw std::runtime_error(
            "[HDF5] Creating a dataset in a file opened as read only is not "
            "possible.");

    if (!writable->written)
    {
        /* Sanitize name */
        std::string name = parameters.name;
        if (auxiliary::starts_with(name, '/'))
            name = auxiliary::replace_first(name, "/", "");
        if (auxiliary::ends_with(name, '/'))
            name = auxiliary::replace_last(name, "/", "");

        auto config = nlohmann::json::parse(parameters.options);

        // general
        bool is_resizable_dataset = false;
        if (config.contains("resizable"))
        {
            is_resizable_dataset = config.at("resizable").get<bool>();
        }

        // HDF5 specific
        if (config.contains("hdf5") && config["hdf5"].contains("dataset"))
        {
            auxiliary::TracingJSON datasetConfig{config["hdf5"]["dataset"]};

            /*
             * @todo Read more options from config here.
             */
            auto shadow = datasetConfig.invertShadow();
            if (shadow.size() > 0)
            {
                std::cerr << "Warning: parts of the JSON configuration for "
                             "HDF5 dataset '"
                          << name << "' remain unused:\n"
                          << shadow << std::endl;
            }
        }

        hid_t gapl = H5Pcreate(H5P_GROUP_ACCESS);
#if H5_VERSION_GE(1, 10, 0) && openPMD_HAVE_MPI
        if (m_hdf5_collective_metadata)
        {
            H5Pset_all_coll_metadata_ops(gapl, true);
        }
#endif

        /* Open H5Object to write into */
        auto res = getFile(writable);
        File file = res ? res.get() : getFile(writable->parent).get();
        hid_t node_id =
            H5Gopen(file.id, concrete_h5_file_position(writable).c_str(), gapl);
        VERIFY(
            node_id >= 0,
            "[HDF5] Internal error: Failed to open HDF5 group during dataset "
            "creation");

        Datatype d = parameters.dtype;
        if (d == Datatype::UNDEFINED)
        {
            // TODO handle unknown dtype
            std::cerr << "[HDF5] Datatype::UNDEFINED caught during dataset "
                         "creation (serial HDF5)"
                      << std::endl;
            d = Datatype::BOOL;
        }

        // note: due to a C++17 issue with ICC 19.1.2 we write the
        //       T value to variant conversion explicitly
        //       https://github.com/openPMD/openPMD-api/pull/...
        // Attribute a(0);
        Attribute a(static_cast<Attribute::resource>(0));
        a.dtype = d;
        std::vector<hsize_t> dims;
        std::uint64_t num_elements = 1u;
        for (auto const &val : parameters.extent)
        {
            dims.push_back(static_cast<hsize_t>(val));
            num_elements *= val;
        }

        std::vector<hsize_t> max_dims(dims.begin(), dims.end());
        if (is_resizable_dataset)
            max_dims.assign(dims.size(), H5F_UNLIMITED);

        hid_t space = H5Screate_simple(
            static_cast<int>(dims.size()), dims.data(), max_dims.data());
        VERIFY(
            space >= 0,
            "[HDF5] Internal error: Failed to create dataspace during dataset "
            "creation");

        /* enable chunking on the created dataspace */
        hid_t datasetCreationProperty = H5Pcreate(H5P_DATASET_CREATE);

        H5Pset_fill_time(datasetCreationProperty, H5D_FILL_TIME_NEVER);

        if (num_elements != 0u && m_chunks != "none")
        {
            //! @todo add per dataset chunk control from JSON config

            // get chunking dimensions
            std::vector<hsize_t> chunk_dims =
                getOptimalChunkDims(dims, toBytes(d));

            //! @todo allow overwrite with user-provided chunk size
            // for( auto const& val : parameters.chunkSize )
            //     chunk_dims.push_back(static_cast< hsize_t >(val));

            herr_t status = H5Pset_chunk(
                datasetCreationProperty, chunk_dims.size(), chunk_dims.data());
            VERIFY(
                status == 0,
                "[HDF5] Internal error: Failed to set chunk size during "
                "dataset creation");
        }

        std::string const &compression = parameters.compression;
        if (!compression.empty())
            std::cerr
                << "[HDF5] Compression not yet implemented in HDF5 backend."
                << std::endl;
        /*
        {
            std::vector< std::string > args = auxiliary::split(compression,
        ":"); std::string const& format = args[0]; if( (format == "zlib" ||
        format == "gzip" || format == "deflate")
                && args.size() == 2 )
            {
                status = H5Pset_deflate(datasetCreationProperty,
        std::stoi(args[1])); VERIFY(status == 0, "[HDF5] Internal error: Failed
        to set deflate compression during dataset creation"); } else if( format
        == "szip" || format == "nbit" || format == "scaleoffset" ) std::cerr <<
        "[HDF5] Compression format " << format
                          << " not yet implemented. Data will not be
        compressed!"
                          << std::endl;
            else
                std::cerr << "[HDF5] Compression format " << format
                          << " unknown. Data will not be compressed!"
                          << std::endl;
        }
         */

        std::string const &transform = parameters.transform;
        if (!transform.empty())
            std::cerr << "[HDF5] Custom transform not yet implemented in HDF5 "
                         "backend."
                      << std::endl;

        GetH5DataType getH5DataType({
            {typeid(bool).name(), m_H5T_BOOL_ENUM},
            {typeid(std::complex<float>).name(), m_H5T_CFLOAT},
            {typeid(std::complex<double>).name(), m_H5T_CDOUBLE},
            {typeid(std::complex<long double>).name(), m_H5T_CLONG_DOUBLE},
        });
        hid_t datatype = getH5DataType(a);
        VERIFY(
            datatype >= 0,
            "[HDF5] Internal error: Failed to get HDF5 datatype during dataset "
            "creation");
        hid_t group_id = H5Dcreate(
            node_id,
            name.c_str(),
            datatype,
            space,
            H5P_DEFAULT,
            datasetCreationProperty,
            H5P_DEFAULT);
        VERIFY(
            group_id >= 0,
            "[HDF5] Internal error: Failed to create HDF5 group during dataset "
            "creation");

        herr_t status;
        status = H5Dclose(group_id);
        VERIFY(
            status == 0,
            "[HDF5] Internal error: Failed to close HDF5 dataset during "
            "dataset creation");
        status = H5Tclose(datatype);
        VERIFY(
            status == 0,
            "[HDF5] Internal error: Failed to close HDF5 datatype during "
            "dataset creation");
        status = H5Pclose(datasetCreationProperty);
        VERIFY(
            status == 0,
            "[HDF5] Internal error: Failed to close HDF5 dataset creation "
            "property during dataset creation");
        status = H5Sclose(space);
        VERIFY(
            status == 0,
            "[HDF5] Internal error: Failed to close HDF5 dataset space during "
            "dataset creation");
        status = H5Gclose(node_id);
        VERIFY(
            status == 0,
            "[HDF5] Internal error: Failed to close HDF5 group during dataset "
            "creation");
        status = H5Pclose(gapl);
        VERIFY(
            status == 0,
            "[HDF5] Internal error: Failed to close HDF5 property during "
            "dataset creation");

        writable->written = true;
        writable->abstractFilePosition =
            std::make_shared<HDF5FilePosition>(name);

        m_fileNames[writable] = file.name;
    }
}

void HDF5IOHandlerImpl::extendDataset(
    Writable *writable, Parameter<Operation::EXTEND_DATASET> const &parameters)
{
    if (m_handler->m_backendAccess == Access::READ_ONLY)
        throw std::runtime_error(
            "[HDF5] Extending a dataset in a file opened as read only is not "
            "possible.");

    if (!writable->written)
        throw std::runtime_error(
            "[HDF5] Extending an unwritten Dataset is not possible.");

    auto res = getFile(writable);
    if (!res)
        res = getFile(writable->parent);
    hid_t dataset_id = H5Dopen(
        res.get().id, concrete_h5_file_position(writable).c_str(), H5P_DEFAULT);
    VERIFY(
        dataset_id >= 0,
        "[HDF5] Internal error: Failed to open HDF5 dataset during dataset "
        "extension");

    // Datasets may only be extended if they have chunked layout, so let's see
    // whether this one does
    {
        hid_t dataset_space = H5Dget_space(dataset_id);
        int ndims = H5Sget_simple_extent_ndims(dataset_space);
        VERIFY(
            ndims >= 0,
            "[HDF5]: Internal error: Failed to retrieve dimensionality of "
            "dataset during dataset read.");
        hid_t propertyList = H5Dget_create_plist(dataset_id);
        std::vector<hsize_t> chunkExtent(ndims, 0);
        int chunkDimensionality =
            H5Pget_chunk(propertyList, ndims, chunkExtent.data());
        if (chunkDimensionality < 0)
        {
            throw std::runtime_error(
                "[HDF5] Cannot extend datasets unless written with chunked "
                "layout.");
        }
    }

    std::vector<hsize_t> size;
    for (auto const &val : parameters.extent)
        size.push_back(static_cast<hsize_t>(val));

    herr_t status;
    status = H5Dset_extent(dataset_id, size.data());
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to extend HDF5 dataset during dataset "
        "extension");

    status = H5Dclose(dataset_id);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close HDF5 dataset during dataset "
        "extension");
}

void HDF5IOHandlerImpl::availableChunks(
    Writable *writable, Parameter<Operation::AVAILABLE_CHUNKS> &parameters)
{
    auto fname = m_fileNames.find(writable);
    VERIFY(
        fname != m_fileNames.end(), "[HDF5] File name not found in writable");
    auto fid = m_fileNamesWithID.find(fname->second);
    VERIFY(
        fid != m_fileNamesWithID.end(),
        "[HDF5] File ID not found with file name");

    hid_t dataset_id = H5Dopen(
        fid->second, concrete_h5_file_position(writable).c_str(), H5P_DEFAULT);
    VERIFY(
        dataset_id >= 0,
        "[HDF5] Internal error: Failed to open HDF5 dataset during dataset "
        "read");
    hid_t dataset_space = H5Dget_space(dataset_id);
    int ndims = H5Sget_simple_extent_ndims(dataset_space);
    VERIFY(
        ndims >= 0,
        "[HDF5]: Internal error: Failed to retrieve dimensionality of "
        "dataset "
        "during dataset read.");

    // // now let's figure out whether this one has chunks
    // hid_t propertyList = H5Dget_create_plist( dataset_id );
    // std::vector< hsize_t > chunkExtent( ndims, 0 );
    // int chunkDimensionality =
    //     H5Pget_chunk( propertyList, ndims, chunkExtent.data() );

    // if( chunkDimensionality >= 0 )
    // {
    //     /*
    //      * so, the dataset indeed has chunks
    //      * alas, this backend doesn't write chunks, so for now, reading them
    //      * is unimplemented
    //      *
    //      * https://hdf5.io/develop/group___h5_d.html#gaccff213d3e0765b86f66d08dd9959807
    //      * May or may not be helpful if implementing this properly one day.
    //      */
    // }

    std::vector<hsize_t> dims(ndims, 0);
    // return value is equal to ndims
    H5Sget_simple_extent_dims(dataset_space, dims.data(), nullptr);

    Offset offset(ndims, 0);
    Extent extent;
    extent.reserve(ndims);
    for (auto e : dims)
    {
        extent.push_back(e);
    }
    parameters.chunks->push_back(
        WrittenChunkInfo(std::move(offset), std::move(extent)));
}

void HDF5IOHandlerImpl::openFile(
    Writable *writable, Parameter<Operation::OPEN_FILE> const &parameters)
{
    if (!auxiliary::directory_exists(m_handler->directory))
        throw no_such_file_error(
            "[HDF5] Supplied directory is not valid: " + m_handler->directory);

    std::string name = m_handler->directory + parameters.name;
    if (!auxiliary::ends_with(name, ".h5"))
        name += ".h5";

    // this may (intentionally) overwrite
    m_fileNames[writable] = name;

    // check if file already open
    auto search = m_fileNamesWithID.find(name);
    if (search != m_fileNamesWithID.end())
    {
        return;
    }

    unsigned flags;
    Access at = m_handler->m_backendAccess;
    if (at == Access::READ_ONLY)
        flags = H5F_ACC_RDONLY;
    else if (at == Access::READ_WRITE || at == Access::CREATE)
        flags = H5F_ACC_RDWR;
    else
        throw std::runtime_error("[HDF5] Unknown file Access");
    hid_t file_id;
    file_id = H5Fopen(name.c_str(), flags, m_fileAccessProperty);
    if (file_id < 0)
        throw no_such_file_error("[HDF5] Failed to open HDF5 file " + name);

    writable->written = true;
    writable->abstractFilePosition = std::make_shared<HDF5FilePosition>("/");

    m_fileNamesWithID.erase(name);
    m_fileNamesWithID.insert({std::move(name), file_id});
    m_openFileIDs.insert(file_id);
}

void HDF5IOHandlerImpl::closeFile(
    Writable *writable, Parameter<Operation::CLOSE_FILE> const &)
{
    auto optionalFile = getFile(writable);
    if (!optionalFile)
    {
        throw std::runtime_error(
            "[HDF5] Trying to close a file that is not "
            "present in the backend");
    }
    File file = optionalFile.get();
    H5Fclose(file.id);
    m_openFileIDs.erase(file.id);
    m_fileNames.erase(writable);

    m_fileNamesWithID.erase(file.name);
}

void HDF5IOHandlerImpl::openPath(
    Writable *writable, Parameter<Operation::OPEN_PATH> const &parameters)
{
    File file = getFile(writable->parent).get();
    hid_t node_id, path_id;

    hid_t gapl = H5Pcreate(H5P_GROUP_ACCESS);
#if H5_VERSION_GE(1, 10, 0) && openPMD_HAVE_MPI
    if (m_hdf5_collective_metadata)
    {
        H5Pset_all_coll_metadata_ops(gapl, true);
    }
#endif

    node_id = H5Gopen(
        file.id, concrete_h5_file_position(writable->parent).c_str(), gapl);
    VERIFY(
        node_id >= 0,
        "[HDF5] Internal error: Failed to open HDF5 group during path opening");

    /* Sanitize path */
    std::string path = parameters.path;
    if (!path.empty())
    {
        if (auxiliary::starts_with(path, '/'))
            path = auxiliary::replace_first(path, "/", "");
        if (!auxiliary::ends_with(path, '/'))
            path += '/';
        path_id = H5Gopen(node_id, path.c_str(), gapl);
        VERIFY(
            path_id >= 0,
            "[HDF5] Internal error: Failed to open HDF5 group during path "
            "opening");

        herr_t status;
        status = H5Gclose(path_id);
        VERIFY(
            status == 0,
            "[HDF5] Internal error: Failed to close HDF5 group during path "
            "opening");
    }

    herr_t status;
    status = H5Gclose(node_id);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close HDF5 group during path "
        "opening");
    status = H5Pclose(gapl);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close HDF5 property during path "
        "opening");

    writable->written = true;
    writable->abstractFilePosition = std::make_shared<HDF5FilePosition>(path);

    m_fileNames.erase(writable);
    m_fileNames.insert({writable, file.name});
}

void HDF5IOHandlerImpl::openDataset(
    Writable *writable, Parameter<Operation::OPEN_DATASET> &parameters)
{
    File file = getFile(writable->parent).get();
    hid_t node_id, dataset_id;

    hid_t gapl = H5Pcreate(H5P_GROUP_ACCESS);
#if H5_VERSION_GE(1, 10, 0) && openPMD_HAVE_MPI
    if (m_hdf5_collective_metadata)
    {
        H5Pset_all_coll_metadata_ops(gapl, true);
    }
#endif

    node_id = H5Gopen(
        file.id, concrete_h5_file_position(writable->parent).c_str(), gapl);
    VERIFY(
        node_id >= 0,
        "[HDF5] Internal error: Failed to open HDF5 group during dataset "
        "opening");

    /* Sanitize name */
    std::string name = parameters.name;
    if (auxiliary::starts_with(name, '/'))
        name = auxiliary::replace_first(name, "/", "");
    if (!auxiliary::ends_with(name, '/'))
        name += '/';

    dataset_id = H5Dopen(node_id, name.c_str(), H5P_DEFAULT);
    VERIFY(
        dataset_id >= 0,
        "[HDF5] Internal error: Failed to open HDF5 dataset during dataset "
        "opening");

    hid_t dataset_type, dataset_space;
    dataset_type = H5Dget_type(dataset_id);
    dataset_space = H5Dget_space(dataset_id);

    H5S_class_t dataset_class = H5Sget_simple_extent_type(dataset_space);

    using DT = Datatype;
    Datatype d;
    if (dataset_class == H5S_SIMPLE || dataset_class == H5S_SCALAR ||
        dataset_class == H5S_NULL)
    {
        if (H5Tequal(dataset_type, H5T_NATIVE_CHAR))
            d = DT::CHAR;
        else if (H5Tequal(dataset_type, H5T_NATIVE_UCHAR))
            d = DT::UCHAR;
        else if (H5Tequal(dataset_type, H5T_NATIVE_SHORT))
            d = DT::SHORT;
        else if (H5Tequal(dataset_type, H5T_NATIVE_INT))
            d = DT::INT;
        else if (H5Tequal(dataset_type, H5T_NATIVE_LONG))
            d = DT::LONG;
        else if (H5Tequal(dataset_type, H5T_NATIVE_LLONG))
            d = DT::LONGLONG;
        else if (H5Tequal(dataset_type, H5T_NATIVE_FLOAT))
            d = DT::FLOAT;
        else if (H5Tequal(dataset_type, H5T_NATIVE_DOUBLE))
            d = DT::DOUBLE;
        else if (H5Tequal(dataset_type, H5T_NATIVE_LDOUBLE))
            d = DT::LONG_DOUBLE;
        else if (H5Tequal(dataset_type, m_H5T_CFLOAT))
            d = DT::CFLOAT;
        else if (H5Tequal(dataset_type, m_H5T_CDOUBLE))
            d = DT::CDOUBLE;
        else if (H5Tequal(dataset_type, m_H5T_CLONG_DOUBLE))
            d = DT::CLONG_DOUBLE;
        else if (H5Tequal(dataset_type, H5T_NATIVE_USHORT))
            d = DT::USHORT;
        else if (H5Tequal(dataset_type, H5T_NATIVE_UINT))
            d = DT::UINT;
        else if (H5Tequal(dataset_type, H5T_NATIVE_ULONG))
            d = DT::ULONG;
        else if (H5Tequal(dataset_type, H5T_NATIVE_ULLONG))
            d = DT::ULONGLONG;
        else if (H5Tget_class(dataset_type) == H5T_STRING)
            d = DT::STRING;
        else
            throw std::runtime_error("[HDF5] Unknown dataset type");
    }
    else
        throw std::runtime_error("[HDF5] Unsupported dataset class");

    auto dtype = parameters.dtype;
    *dtype = d;

    int ndims = H5Sget_simple_extent_ndims(dataset_space);
    std::vector<hsize_t> dims(ndims, 0);
    std::vector<hsize_t> maxdims(ndims, 0);

    H5Sget_simple_extent_dims(dataset_space, dims.data(), maxdims.data());
    Extent e;
    for (auto const &val : dims)
        e.push_back(val);
    auto extent = parameters.extent;
    *extent = e;

    herr_t status;
    status = H5Sclose(dataset_space);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close HDF5 dataset space during "
        "dataset opening");
    status = H5Tclose(dataset_type);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close HDF5 dataset type during "
        "dataset opening");
    status = H5Dclose(dataset_id);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close HDF5 dataset during dataset "
        "opening");
    status = H5Gclose(node_id);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close HDF5 group during dataset "
        "opening");
    status = H5Pclose(gapl);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close HDF5 property during dataset "
        "opening");

    writable->written = true;
    writable->abstractFilePosition = std::make_shared<HDF5FilePosition>(name);

    m_fileNames[writable] = file.name;
}

void HDF5IOHandlerImpl::deleteFile(
    Writable *writable, Parameter<Operation::DELETE_FILE> const &parameters)
{
    if (m_handler->m_backendAccess == Access::READ_ONLY)
        throw std::runtime_error(
            "[HDF5] Deleting a file opened as read only is not possible.");

    if (writable->written)
    {
        hid_t file_id = getFile(writable).get().id;
        herr_t status = H5Fclose(file_id);
        VERIFY(
            status == 0,
            "[HDF5] Internal error: Failed to close HDF5 file during file "
            "deletion");

        std::string name = m_handler->directory + parameters.name;
        if (!auxiliary::ends_with(name, ".h5"))
            name += ".h5";

        if (!auxiliary::file_exists(name))
            throw std::runtime_error("[HDF5] File does not exist: " + name);

        auxiliary::remove_file(name);

        writable->written = false;
        writable->abstractFilePosition.reset();

        m_openFileIDs.erase(file_id);
        m_fileNames.erase(writable);
        m_fileNamesWithID.erase(name);
    }
}

void HDF5IOHandlerImpl::deletePath(
    Writable *writable, Parameter<Operation::DELETE_PATH> const &parameters)
{
    if (m_handler->m_backendAccess == Access::READ_ONLY)
        throw std::runtime_error(
            "[HDF5] Deleting a path in a file opened as read only is not "
            "possible.");

    if (writable->written)
    {
        /* Sanitize path */
        std::string path = parameters.path;
        if (auxiliary::starts_with(path, '/'))
            path = auxiliary::replace_first(path, "/", "");
        if (!auxiliary::ends_with(path, '/'))
            path += '/';

        /* Open H5Object to delete in
         * Ugly hack: H5Ldelete can't delete "."
         *            Work around this by deleting from the parent
         */
        auto res = getFile(writable);
        File file = res ? res.get() : getFile(writable->parent).get();
        hid_t node_id = H5Gopen(
            file.id,
            concrete_h5_file_position(writable->parent).c_str(),
            H5P_DEFAULT);
        VERIFY(
            node_id >= 0,
            "[HDF5] Internal error: Failed to open HDF5 group during path "
            "deletion");

        path += static_cast<HDF5FilePosition *>(
                    writable->abstractFilePosition.get())
                    ->location;
        herr_t status = H5Ldelete(node_id, path.c_str(), H5P_DEFAULT);
        VERIFY(
            status == 0, "[HDF5] Internal error: Failed to delete HDF5 group");

        status = H5Gclose(node_id);
        VERIFY(
            status == 0,
            "[HDF5] Internal error: Failed to close HDF5 group during path "
            "deletion");

        writable->written = false;
        writable->abstractFilePosition.reset();

        m_fileNames.erase(writable);
    }
}

void HDF5IOHandlerImpl::deleteDataset(
    Writable *writable, Parameter<Operation::DELETE_DATASET> const &parameters)
{
    if (m_handler->m_backendAccess == Access::READ_ONLY)
        throw std::runtime_error(
            "[HDF5] Deleting a path in a file opened as read only is not "
            "possible.");

    if (writable->written)
    {
        /* Sanitize name */
        std::string name = parameters.name;
        if (auxiliary::starts_with(name, '/'))
            name = auxiliary::replace_first(name, "/", "");
        if (!auxiliary::ends_with(name, '/'))
            name += '/';

        /* Open H5Object to delete in
         * Ugly hack: H5Ldelete can't delete "."
         *            Work around this by deleting from the parent
         */
        auto res = getFile(writable);
        File file = res ? res.get() : getFile(writable->parent).get();
        hid_t node_id = H5Gopen(
            file.id,
            concrete_h5_file_position(writable->parent).c_str(),
            H5P_DEFAULT);
        VERIFY(
            node_id >= 0,
            "[HDF5] Internal error: Failed to open HDF5 group during dataset "
            "deletion");

        name += static_cast<HDF5FilePosition *>(
                    writable->abstractFilePosition.get())
                    ->location;
        herr_t status = H5Ldelete(node_id, name.c_str(), H5P_DEFAULT);
        VERIFY(
            status == 0, "[HDF5] Internal error: Failed to delete HDF5 group");

        status = H5Gclose(node_id);
        VERIFY(
            status == 0,
            "[HDF5] Internal error: Failed to close HDF5 group during dataset "
            "deletion");

        writable->written = false;
        writable->abstractFilePosition.reset();

        m_fileNames.erase(writable);
    }
}

void HDF5IOHandlerImpl::deleteAttribute(
    Writable *writable, Parameter<Operation::DELETE_ATT> const &parameters)
{
    if (m_handler->m_backendAccess == Access::READ_ONLY)
        throw std::runtime_error(
            "[HDF5] Deleting an attribute in a file opened as read only is not "
            "possible.");

    if (writable->written)
    {
        std::string name = parameters.name;

        /* Open H5Object to delete in */
        auto res = getFile(writable);
        File file = res ? res.get() : getFile(writable->parent).get();
        hid_t node_id = H5Oopen(
            file.id, concrete_h5_file_position(writable).c_str(), H5P_DEFAULT);
        VERIFY(
            node_id >= 0,
            "[HDF5] Internal error: Failed to open HDF5 group during attribute "
            "deletion");

        herr_t status = H5Adelete(node_id, name.c_str());
        VERIFY(
            status == 0,
            "[HDF5] Internal error: Failed to delete HDF5 attribute");

        status = H5Oclose(node_id);
        VERIFY(
            status == 0,
            "[HDF5] Internal error: Failed to close HDF5 group during "
            "attribute deletion");
    }
}

void HDF5IOHandlerImpl::writeDataset(
    Writable *writable, Parameter<Operation::WRITE_DATASET> const &parameters)
{
    if (m_handler->m_backendAccess == Access::READ_ONLY)
        throw std::runtime_error(
            "[HDF5] Writing into a dataset in a file opened as read only is "
            "not possible.");

    auto res = getFile(writable);
    File file = res ? res.get() : getFile(writable->parent).get();

    hid_t dataset_id, filespace, memspace;
    herr_t status;
    dataset_id = H5Dopen(
        file.id, concrete_h5_file_position(writable).c_str(), H5P_DEFAULT);
    VERIFY(
        dataset_id >= 0,
        "[HDF5] Internal error: Failed to open HDF5 dataset during dataset "
        "write");

    std::vector<hsize_t> start;
    for (auto const &val : parameters.offset)
        start.push_back(static_cast<hsize_t>(val));
    std::vector<hsize_t> stride(start.size(), 1); /* contiguous region */
    std::vector<hsize_t> count(start.size(), 1); /* single region */
    std::vector<hsize_t> block;
    for (auto const &val : parameters.extent)
        block.push_back(static_cast<hsize_t>(val));
    memspace =
        H5Screate_simple(static_cast<int>(block.size()), block.data(), nullptr);
    filespace = H5Dget_space(dataset_id);
    status = H5Sselect_hyperslab(
        filespace,
        H5S_SELECT_SET,
        start.data(),
        stride.data(),
        count.data(),
        block.data());
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to select hyperslab during dataset "
        "write");

    std::shared_ptr<void const> data = parameters.data;

    GetH5DataType getH5DataType({
        {typeid(bool).name(), m_H5T_BOOL_ENUM},
        {typeid(std::complex<float>).name(), m_H5T_CFLOAT},
        {typeid(std::complex<double>).name(), m_H5T_CDOUBLE},
        {typeid(std::complex<long double>).name(), m_H5T_CLONG_DOUBLE},
    });

    // TODO Check if parameter dtype and dataset dtype match
    Attribute a(0);
    a.dtype = parameters.dtype;
    hid_t dataType = getH5DataType(a);
    VERIFY(
        dataType >= 0,
        "[HDF5] Internal error: Failed to get HDF5 datatype during dataset "
        "write");
    switch (a.dtype)
    {
        using DT = Datatype;
    case DT::LONG_DOUBLE:
    case DT::DOUBLE:
    case DT::FLOAT:
    case DT::CLONG_DOUBLE:
    case DT::CDOUBLE:
    case DT::CFLOAT:
    case DT::SHORT:
    case DT::INT:
    case DT::LONG:
    case DT::LONGLONG:
    case DT::USHORT:
    case DT::UINT:
    case DT::ULONG:
    case DT::ULONGLONG:
    case DT::CHAR:
    case DT::UCHAR:
    case DT::BOOL:
        status = H5Dwrite(
            dataset_id,
            dataType,
            memspace,
            filespace,
            m_datasetTransferProperty,
            data.get());
        VERIFY(
            status == 0,
            "[HDF5] Internal error: Failed to write dataset " +
                concrete_h5_file_position(writable));
        break;
    case DT::UNDEFINED:
        throw std::runtime_error("[HDF5] Undefined Attribute datatype");
    case DT::DATATYPE:
        throw std::runtime_error("[HDF5] Meta-Datatype leaked into IO");
    default:
        throw std::runtime_error("[HDF5] Datatype not implemented in HDF5 IO");
    }
    status = H5Tclose(dataType);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close dataset datatype during "
        "dataset write");
    status = H5Sclose(filespace);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close dataset file space during "
        "dataset write");
    status = H5Sclose(memspace);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close dataset memory space during "
        "dataset write");
    status = H5Dclose(dataset_id);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close dataset " +
            concrete_h5_file_position(writable) + " during dataset write");

    m_fileNames[writable] = file.name;
}

void HDF5IOHandlerImpl::writeAttribute(
    Writable *writable, Parameter<Operation::WRITE_ATT> const &parameters)
{
    if (m_handler->m_backendAccess == Access::READ_ONLY)
        throw std::runtime_error(
            "[HDF5] Writing an attribute in a file opened as read only is not "
            "possible.");

    auto res = getFile(writable);
    File file = res ? res.get() : getFile(writable->parent).get();
    hid_t node_id, attribute_id;

    hid_t fapl = H5Pcreate(H5P_LINK_ACCESS);
#if H5_VERSION_GE(1, 10, 0) && openPMD_HAVE_MPI
    if (m_hdf5_collective_metadata)
    {
        H5Pset_all_coll_metadata_ops(fapl, true);
    }
#endif

    node_id =
        H5Oopen(file.id, concrete_h5_file_position(writable).c_str(), fapl);
    VERIFY(
        node_id >= 0,
        "[HDF5] Internal error: Failed to open HDF5 object during attribute "
        "write");
    Attribute const att(parameters.resource);
    Datatype dtype = parameters.dtype;
    herr_t status;
    GetH5DataType getH5DataType({
        {typeid(bool).name(), m_H5T_BOOL_ENUM},
        {typeid(std::complex<float>).name(), m_H5T_CFLOAT},
        {typeid(std::complex<double>).name(), m_H5T_CDOUBLE},
        {typeid(std::complex<long double>).name(), m_H5T_CLONG_DOUBLE},
    });
    hid_t dataType = getH5DataType(att);
    VERIFY(
        dataType >= 0,
        "[HDF5] Internal error: Failed to get HDF5 datatype during attribute "
        "write");
    std::string name = parameters.name;
    if (H5Aexists(node_id, name.c_str()) == 0)
    {
        hid_t dataspace = getH5DataSpace(att);
        VERIFY(
            dataspace >= 0,
            "[HDF5] Internal error: Failed to get HDF5 dataspace during "
            "attribute write");
        attribute_id = H5Acreate(
            node_id,
            name.c_str(),
            dataType,
            dataspace,
            H5P_DEFAULT,
            H5P_DEFAULT);
        VERIFY(
            node_id >= 0,
            "[HDF5] Internal error: Failed to create HDF5 attribute during "
            "attribute write");
        status = H5Sclose(dataspace);
        VERIFY(
            status == 0,
            "[HDF5] Internal error: Failed to close HDF5 dataspace during "
            "attribute write");
    }
    else
    {
        attribute_id = H5Aopen(node_id, name.c_str(), H5P_DEFAULT);
        VERIFY(
            node_id >= 0,
            "[HDF5] Internal error: Failed to open HDF5 attribute during "
            "attribute write");
    }

    using DT = Datatype;
    switch (dtype)
    {
    case DT::CHAR: {
        char c = att.get<char>();
        status = H5Awrite(attribute_id, dataType, &c);
        break;
    }
    case DT::UCHAR: {
        auto u = att.get<unsigned char>();
        status = H5Awrite(attribute_id, dataType, &u);
        break;
    }
    case DT::SHORT: {
        auto i = att.get<short>();
        status = H5Awrite(attribute_id, dataType, &i);
        break;
    }
    case DT::INT: {
        int i = att.get<int>();
        status = H5Awrite(attribute_id, dataType, &i);
        break;
    }
    case DT::LONG: {
        long i = att.get<long>();
        status = H5Awrite(attribute_id, dataType, &i);
        break;
    }
    case DT::LONGLONG: {
        auto i = att.get<long long>();
        status = H5Awrite(attribute_id, dataType, &i);
        break;
    }
    case DT::USHORT: {
        auto u = att.get<unsigned short>();
        status = H5Awrite(attribute_id, dataType, &u);
        break;
    }
    case DT::UINT: {
        auto u = att.get<unsigned int>();
        status = H5Awrite(attribute_id, dataType, &u);
        break;
    }
    case DT::ULONG: {
        auto u = att.get<unsigned long>();
        status = H5Awrite(attribute_id, dataType, &u);
        break;
    }
    case DT::ULONGLONG: {
        auto u = att.get<unsigned long long>();
        status = H5Awrite(attribute_id, dataType, &u);
        break;
    }
    case DT::FLOAT: {
        auto f = att.get<float>();
        status = H5Awrite(attribute_id, dataType, &f);
        break;
    }
    case DT::DOUBLE: {
        auto d = att.get<double>();
        status = H5Awrite(attribute_id, dataType, &d);
        break;
    }
    case DT::LONG_DOUBLE: {
        auto d = att.get<long double>();
        status = H5Awrite(attribute_id, dataType, &d);
        break;
    }
    case DT::CFLOAT: {
        std::complex<float> f = att.get<std::complex<float> >();
        status = H5Awrite(attribute_id, dataType, &f);
        break;
    }
    case DT::CDOUBLE: {
        std::complex<double> d = att.get<std::complex<double> >();
        status = H5Awrite(attribute_id, dataType, &d);
        break;
    }
    case DT::CLONG_DOUBLE: {
        std::complex<long double> d = att.get<std::complex<long double> >();
        status = H5Awrite(attribute_id, dataType, &d);
        break;
    }
    case DT::STRING:
        status =
            H5Awrite(attribute_id, dataType, att.get<std::string>().c_str());
        break;
    case DT::VEC_CHAR:
        status = H5Awrite(
            attribute_id, dataType, att.get<std::vector<char> >().data());
        break;
    case DT::VEC_SHORT:
        status = H5Awrite(
            attribute_id, dataType, att.get<std::vector<short> >().data());
        break;
    case DT::VEC_INT:
        status = H5Awrite(
            attribute_id, dataType, att.get<std::vector<int> >().data());
        break;
    case DT::VEC_LONG:
        status = H5Awrite(
            attribute_id, dataType, att.get<std::vector<long> >().data());
        break;
    case DT::VEC_LONGLONG:
        status = H5Awrite(
            attribute_id, dataType, att.get<std::vector<long long> >().data());
        break;
    case DT::VEC_UCHAR:
        status = H5Awrite(
            attribute_id,
            dataType,
            att.get<std::vector<unsigned char> >().data());
        break;
    case DT::VEC_USHORT:
        status = H5Awrite(
            attribute_id,
            dataType,
            att.get<std::vector<unsigned short> >().data());
        break;
    case DT::VEC_UINT:
        status = H5Awrite(
            attribute_id,
            dataType,
            att.get<std::vector<unsigned int> >().data());
        break;
    case DT::VEC_ULONG:
        status = H5Awrite(
            attribute_id,
            dataType,
            att.get<std::vector<unsigned long> >().data());
        break;
    case DT::VEC_ULONGLONG:
        status = H5Awrite(
            attribute_id,
            dataType,
            att.get<std::vector<unsigned long long> >().data());
        break;
    case DT::VEC_FLOAT:
        status = H5Awrite(
            attribute_id, dataType, att.get<std::vector<float> >().data());
        break;
    case DT::VEC_DOUBLE:
        status = H5Awrite(
            attribute_id, dataType, att.get<std::vector<double> >().data());
        break;
    case DT::VEC_LONG_DOUBLE:
        status = H5Awrite(
            attribute_id,
            dataType,
            att.get<std::vector<long double> >().data());
        break;
    case DT::VEC_CFLOAT:
        status = H5Awrite(
            attribute_id,
            dataType,
            att.get<std::vector<std::complex<float> > >().data());
        break;
    case DT::VEC_CDOUBLE:
        status = H5Awrite(
            attribute_id,
            dataType,
            att.get<std::vector<std::complex<double> > >().data());
        break;
    case DT::VEC_CLONG_DOUBLE:
        status = H5Awrite(
            attribute_id,
            dataType,
            att.get<std::vector<std::complex<long double> > >().data());
        break;
    case DT::VEC_STRING: {
        auto vs = att.get<std::vector<std::string> >();
        size_t max_len = 0;
        for (std::string const &s : vs)
            max_len = std::max(max_len, s.size());
        std::unique_ptr<char[]> c_str(new char[max_len * vs.size()]);
        for (size_t i = 0; i < vs.size(); ++i)
            strncpy(c_str.get() + i * max_len, vs[i].c_str(), max_len);
        status = H5Awrite(attribute_id, dataType, c_str.get());
        break;
    }
    case DT::ARR_DBL_7:
        status = H5Awrite(
            attribute_id, dataType, att.get<std::array<double, 7> >().data());
        break;
    case DT::BOOL: {
        bool b = att.get<bool>();
        status = H5Awrite(attribute_id, dataType, &b);
        break;
    }
    case DT::UNDEFINED:
    case DT::DATATYPE:
        throw std::runtime_error(
            "[HDF5] Unknown Attribute datatype (HDF5 Attribute write)");
    default:
        throw std::runtime_error("[HDF5] Datatype not implemented in HDF5 IO");
    }
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to write attribute " + name + " at " +
            concrete_h5_file_position(writable));

    status = H5Tclose(dataType);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close HDF5 datatype during Attribute "
        "write");

    status = H5Aclose(attribute_id);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close attribute " + name + " at " +
            concrete_h5_file_position(writable) + " during attribute write");
    status = H5Oclose(node_id);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close " +
            concrete_h5_file_position(writable) + " during attribute write");
    status = H5Pclose(fapl);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close HDF5 property during attribute "
        "write");

    m_fileNames[writable] = file.name;
}

void HDF5IOHandlerImpl::readDataset(
    Writable *writable, Parameter<Operation::READ_DATASET> &parameters)
{
    auto res = getFile(writable);
    File file = res ? res.get() : getFile(writable->parent).get();
    hid_t dataset_id, memspace, filespace;
    herr_t status;
    dataset_id = H5Dopen(
        file.id, concrete_h5_file_position(writable).c_str(), H5P_DEFAULT);
    VERIFY(
        dataset_id >= 0,
        "[HDF5] Internal error: Failed to open HDF5 dataset during dataset "
        "read");

    std::vector<hsize_t> start;
    for (auto const &val : parameters.offset)
        start.push_back(static_cast<hsize_t>(val));
    std::vector<hsize_t> stride(start.size(), 1); /* contiguous region */
    std::vector<hsize_t> count(start.size(), 1); /* single region */
    std::vector<hsize_t> block;
    for (auto const &val : parameters.extent)
        block.push_back(static_cast<hsize_t>(val));
    memspace =
        H5Screate_simple(static_cast<int>(block.size()), block.data(), nullptr);
    filespace = H5Dget_space(dataset_id);
    status = H5Sselect_hyperslab(
        filespace,
        H5S_SELECT_SET,
        start.data(),
        stride.data(),
        count.data(),
        block.data());
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to select hyperslab during dataset "
        "read");

    void *data = parameters.data.get();

    Attribute a(0);
    a.dtype = parameters.dtype;
    switch (a.dtype)
    {
        using DT = Datatype;
    case DT::LONG_DOUBLE:
    case DT::DOUBLE:
    case DT::FLOAT:
    case DT::CLONG_DOUBLE:
    case DT::CDOUBLE:
    case DT::CFLOAT:
    case DT::SHORT:
    case DT::INT:
    case DT::LONG:
    case DT::LONGLONG:
    case DT::USHORT:
    case DT::UINT:
    case DT::ULONG:
    case DT::ULONGLONG:
    case DT::CHAR:
    case DT::UCHAR:
    case DT::BOOL:
        break;
    case DT::UNDEFINED:
        throw std::runtime_error(
            "[HDF5] Unknown Attribute datatype (HDF5 Dataset read)");
    case DT::DATATYPE:
        throw std::runtime_error("[HDF5] Meta-Datatype leaked into IO");
    default:
        throw std::runtime_error("[HDF5] Datatype not implemented in HDF5 IO");
    }
    GetH5DataType getH5DataType({
        {typeid(bool).name(), m_H5T_BOOL_ENUM},
        {typeid(std::complex<float>).name(), m_H5T_CFLOAT},
        {typeid(std::complex<double>).name(), m_H5T_CDOUBLE},
        {typeid(std::complex<long double>).name(), m_H5T_CLONG_DOUBLE},
    });
    hid_t dataType = getH5DataType(a);
    VERIFY(
        dataType >= 0,
        "[HDF5] Internal error: Failed to get HDF5 datatype during dataset "
        "read");
    status = H5Dread(
        dataset_id,
        dataType,
        memspace,
        filespace,
        m_datasetTransferProperty,
        data);
    VERIFY(status == 0, "[HDF5] Internal error: Failed to read dataset");

    status = H5Tclose(dataType);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close dataset datatype during "
        "dataset read");
    status = H5Sclose(filespace);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close dataset file space during "
        "dataset read");
    status = H5Sclose(memspace);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close dataset memory space during "
        "dataset read");
    status = H5Dclose(dataset_id);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close dataset during dataset read");
}

void HDF5IOHandlerImpl::readAttribute(
    Writable *writable, Parameter<Operation::READ_ATT> &parameters)
{
    if (!writable->written)
        throw std::runtime_error(
            "[HDF5] Internal error: Writable not marked written during "
            "attribute read");

    auto res = getFile(writable);
    File file = res ? res.get() : getFile(writable->parent).get();

    hid_t obj_id, attr_id;
    herr_t status;

    hid_t fapl = H5Pcreate(H5P_LINK_ACCESS);
#if H5_VERSION_GE(1, 10, 0) && openPMD_HAVE_MPI
    if (m_hdf5_collective_metadata)
    {
        H5Pset_all_coll_metadata_ops(fapl, true);
    }
#endif

    obj_id =
        H5Oopen(file.id, concrete_h5_file_position(writable).c_str(), fapl);
    VERIFY(
        obj_id >= 0,
        std::string("[HDF5] Internal error: Failed to open HDF5 object '") +
            concrete_h5_file_position(writable).c_str() +
            "' during attribute read");
    std::string const &attr_name = parameters.name;
    attr_id = H5Aopen(obj_id, attr_name.c_str(), H5P_DEFAULT);
    VERIFY(
        attr_id >= 0,
        std::string("[HDF5] Internal error: Failed to open HDF5 attribute '") +
            attr_name + "' (" + concrete_h5_file_position(writable).c_str() +
            ") during attribute read");

    hid_t attr_type, attr_space;
    attr_type = H5Aget_type(attr_id);
    attr_space = H5Aget_space(attr_id);

    int ndims = H5Sget_simple_extent_ndims(attr_space);
    std::vector<hsize_t> dims(ndims, 0);
    std::vector<hsize_t> maxdims(ndims, 0);

    status = H5Sget_simple_extent_dims(attr_space, dims.data(), maxdims.data());
    VERIFY(
        status == ndims,
        "[HDF5] Internal error: Failed to get dimensions during attribute "
        "read");

    H5S_class_t attr_class = H5Sget_simple_extent_type(attr_space);
    Attribute a(0);
    if (attr_class == H5S_SCALAR ||
        (attr_class == H5S_SIMPLE && ndims == 1 && dims[0] == 1))
    {
        if (H5Tequal(attr_type, H5T_NATIVE_CHAR))
        {
            char c;
            status = H5Aread(attr_id, attr_type, &c);
            a = Attribute(c);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_UCHAR))
        {
            unsigned char u;
            status = H5Aread(attr_id, attr_type, &u);
            a = Attribute(u);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_SHORT))
        {
            short i;
            status = H5Aread(attr_id, attr_type, &i);
            a = Attribute(i);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_INT))
        {
            int i;
            status = H5Aread(attr_id, attr_type, &i);
            a = Attribute(i);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_LONG))
        {
            long i;
            status = H5Aread(attr_id, attr_type, &i);
            a = Attribute(i);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_LLONG))
        {
            long long i;
            status = H5Aread(attr_id, attr_type, &i);
            a = Attribute(i);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_USHORT))
        {
            unsigned short u;
            status = H5Aread(attr_id, attr_type, &u);
            a = Attribute(u);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_UINT))
        {
            unsigned int u;
            status = H5Aread(attr_id, attr_type, &u);
            a = Attribute(u);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_ULONG))
        {
            unsigned long u;
            status = H5Aread(attr_id, attr_type, &u);
            a = Attribute(u);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_ULLONG))
        {
            unsigned long long u;
            status = H5Aread(attr_id, attr_type, &u);
            a = Attribute(u);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_FLOAT))
        {
            float f;
            status = H5Aread(attr_id, attr_type, &f);
            a = Attribute(f);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_DOUBLE))
        {
            double d;
            status = H5Aread(attr_id, attr_type, &d);
            a = Attribute(d);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_LDOUBLE))
        {
            long double l;
            status = H5Aread(attr_id, attr_type, &l);
            a = Attribute(l);
        }
        else if (H5Tget_class(attr_type) == H5T_STRING)
        {
            if (H5Tis_variable_str(attr_type))
            {
                // refs.:
                // https://github.com/HDFGroup/hdf5/blob/hdf5-1_12_0/tools/src/h5dump/h5dump_xml.c
                hsize_t size =
                    H5Tget_size(attr_type); // not yet the actual string length
                std::vector<char> vc(size); // byte buffer to vlen strings
                status = H5Aread(attr_id, attr_type, vc.data());
                auto c_str = *((char **)vc.data()); // get actual string out
                a = Attribute(std::string(c_str));
                // free dynamically allocated vlen memory from H5Aread
                H5Dvlen_reclaim(attr_type, attr_space, H5P_DEFAULT, vc.data());
                // 1.12+:
                // H5Treclaim(attr_type, attr_space, H5P_DEFAULT, vc.data());
            }
            else
            {
                hsize_t size = H5Tget_size(attr_type);
                std::vector<char> vc(size);
                status = H5Aread(attr_id, attr_type, vc.data());
                a = Attribute(
                    auxiliary::strip(std::string(vc.data(), size), {'\0'}));
            }
        }
        else if (H5Tget_class(attr_type) == H5T_ENUM)
        {
            bool attrIsBool = false;
            if (H5Tget_nmembers(attr_type) == 2)
            {
                char *m0 = H5Tget_member_name(attr_type, 0);
                char *m1 = H5Tget_member_name(attr_type, 1);
                if (m0 != nullptr && m1 != nullptr)
                    if ((strncmp("TRUE", m0, 4) == 0) &&
                        (strncmp("FALSE", m1, 5) == 0))
                        attrIsBool = true;
                H5free_memory(m1);
                H5free_memory(m0);
            }

            if (attrIsBool)
            {
                int8_t enumVal;
                status = H5Aread(attr_id, attr_type, &enumVal);
                a = Attribute(static_cast<bool>(enumVal));
            }
            else
                throw unsupported_data_error(
                    "[HDF5] Unsupported attribute enumeration");
        }
        else if (H5Tget_class(attr_type) == H5T_COMPOUND)
        {
            bool isComplexType = false;
            if (H5Tget_nmembers(attr_type) == 2)
            {
                char *m0 = H5Tget_member_name(attr_type, 0);
                char *m1 = H5Tget_member_name(attr_type, 1);
                if (m0 != nullptr && m1 != nullptr)
                    if ((strncmp("r", m0, 1) == 0) &&
                        (strncmp("i", m1, 1) == 0))
                        isComplexType = true;
                H5free_memory(m1);
                H5free_memory(m0);
            }

            // re-implement legacy libSplash attributes for ColDim
            // see: include/splash/basetypes/ColTypeDim.hpp
            bool isLegacyLibSplashAttr =
                (H5Tget_nmembers(attr_type) == 3 &&
                 H5Tget_size(attr_type) == sizeof(hsize_t) * 3);
            if (isLegacyLibSplashAttr)
            {
                char *m0 = H5Tget_member_name(attr_type, 0);
                char *m1 = H5Tget_member_name(attr_type, 1);
                char *m2 = H5Tget_member_name(attr_type, 2);
                if (m0 == nullptr || m1 == nullptr || m2 == nullptr)
                    // clang-format off
                    isLegacyLibSplashAttr = false;  // NOLINT(bugprone-branch-clone)
                // clang-format on
                else if (
                    strcmp("x", m0) != 0 || strcmp("y", m1) != 0 ||
                    strcmp("z", m2) != 0)
                    isLegacyLibSplashAttr = false;
                H5free_memory(m2);
                H5free_memory(m1);
                H5free_memory(m0);
            }
            if (isLegacyLibSplashAttr)
            {
                std::vector<hsize_t> vc(3, 0);
                status = H5Aread(attr_id, attr_type, vc.data());
                a = Attribute(vc);
            }
            else if (isComplexType)
            {
                size_t complexSize = H5Tget_member_offset(attr_type, 1);
                if (complexSize == sizeof(float))
                {
                    std::complex<float> cf;
                    status = H5Aread(attr_id, attr_type, &cf);
                    a = Attribute(cf);
                }
                else if (complexSize == sizeof(double))
                {
                    std::complex<double> cd;
                    status = H5Aread(attr_id, attr_type, &cd);
                    a = Attribute(cd);
                }
                else if (complexSize == sizeof(long double))
                {
                    std::complex<long double> cld;
                    status = H5Aread(attr_id, attr_type, &cld);
                    a = Attribute(cld);
                }
                else
                    throw unsupported_data_error(
                        "[HDF5] Unknown complex type representation");
            }
            else
                throw unsupported_data_error(
                    "[HDF5] Compound attribute type not supported");
        }
        else
            throw std::runtime_error(
                "[HDF5] Unsupported scalar attribute type");
    }
    else if (attr_class == H5S_SIMPLE)
    {
        if (ndims != 1)
            throw std::runtime_error(
                "[HDF5] Unsupported attribute (array with ndims != 1)");

        if (H5Tequal(attr_type, H5T_NATIVE_CHAR))
        {
            std::vector<char> vc(dims[0], 0);
            status = H5Aread(attr_id, attr_type, vc.data());
            a = Attribute(vc);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_UCHAR))
        {
            std::vector<unsigned char> vu(dims[0], 0);
            status = H5Aread(attr_id, attr_type, vu.data());
            a = Attribute(vu);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_SHORT))
        {
            std::vector<short> vint16(dims[0], 0);
            status = H5Aread(attr_id, attr_type, vint16.data());
            a = Attribute(vint16);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_INT))
        {
            std::vector<int> vint32(dims[0], 0);
            status = H5Aread(attr_id, attr_type, vint32.data());
            a = Attribute(vint32);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_LONG))
        {
            std::vector<long> vint64(dims[0], 0);
            status = H5Aread(attr_id, attr_type, vint64.data());
            a = Attribute(vint64);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_LLONG))
        {
            std::vector<long long> vint64(dims[0], 0);
            status = H5Aread(attr_id, attr_type, vint64.data());
            a = Attribute(vint64);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_USHORT))
        {
            std::vector<unsigned short> vuint16(dims[0], 0);
            status = H5Aread(attr_id, attr_type, vuint16.data());
            a = Attribute(vuint16);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_UINT))
        {
            std::vector<unsigned int> vuint32(dims[0], 0);
            status = H5Aread(attr_id, attr_type, vuint32.data());
            a = Attribute(vuint32);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_ULONG))
        {
            std::vector<unsigned long> vuint64(dims[0], 0);
            status = H5Aread(attr_id, attr_type, vuint64.data());
            a = Attribute(vuint64);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_ULLONG))
        {
            std::vector<unsigned long long> vuint64(dims[0], 0);
            status = H5Aread(attr_id, attr_type, vuint64.data());
            a = Attribute(vuint64);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_FLOAT))
        {
            std::vector<float> vf(dims[0], 0);
            status = H5Aread(attr_id, attr_type, vf.data());
            a = Attribute(vf);
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_DOUBLE))
        {
            if (dims[0] == 7 && attr_name == "unitDimension")
            {
                std::array<double, 7> ad;
                status = H5Aread(attr_id, attr_type, &ad);
                a = Attribute(ad);
            }
            else
            {
                std::vector<double> vd(dims[0], 0);
                status = H5Aread(attr_id, attr_type, vd.data());
                a = Attribute(vd);
            }
        }
        else if (H5Tequal(attr_type, H5T_NATIVE_LDOUBLE))
        {
            std::vector<long double> vld(dims[0], 0);
            status = H5Aread(attr_id, attr_type, vld.data());
            a = Attribute(vld);
        }
        else if (H5Tequal(attr_type, m_H5T_CFLOAT))
        {
            std::vector<std::complex<float> > vcf(dims[0], 0);
            status = H5Aread(attr_id, attr_type, vcf.data());
            a = Attribute(vcf);
        }
        else if (H5Tequal(attr_type, m_H5T_CDOUBLE))
        {
            std::vector<std::complex<double> > vcd(dims[0], 0);
            status = H5Aread(attr_id, attr_type, vcd.data());
            a = Attribute(vcd);
        }
        else if (H5Tequal(attr_type, m_H5T_CLONG_DOUBLE))
        {
            std::vector<std::complex<long double> > vcld(dims[0], 0);
            status = H5Aread(attr_id, attr_type, vcld.data());
            a = Attribute(vcld);
        }
        else if (H5Tget_class(attr_type) == H5T_STRING)
        {
            std::vector<std::string> vs;
            if (H5Tis_variable_str(attr_type))
            {
                std::vector<char *> vc(dims[0]);
                status = H5Aread(attr_id, attr_type, vc.data());
                VERIFY(
                    status == 0,
                    "[HDF5] Internal error: Failed to read attribute " +
                        attr_name + " at " +
                        concrete_h5_file_position(writable));
                for (auto const &val : vc)
                    vs.push_back(auxiliary::strip(std::string(val), {'\0'}));
                status = H5Dvlen_reclaim(
                    attr_type, attr_space, H5P_DEFAULT, vc.data());
            }
            else
            {
                size_t length = H5Tget_size(attr_type);
                std::vector<char> c(dims[0] * length);
                status = H5Aread(attr_id, attr_type, c.data());
                for (hsize_t i = 0; i < dims[0]; ++i)
                    vs.push_back(auxiliary::strip(
                        std::string(c.data() + i * length, length), {'\0'}));
            }
            a = Attribute(vs);
        }
        else
            throw std::runtime_error(
                "[HDF5] Unsupported simple attribute type");
    }
    else
        throw std::runtime_error("[HDF5] Unsupported attribute class");
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to read attribute " + attr_name +
            " at " + concrete_h5_file_position(writable));

    status = H5Tclose(attr_type);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close attribute datatype during "
        "attribute read");
    status = H5Sclose(attr_space);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close attribute file space during "
        "attribute read");

    auto dtype = parameters.dtype;
    *dtype = a.dtype;
    auto resource = parameters.resource;
    *resource = a.getResource();

    status = H5Aclose(attr_id);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close attribute " + attr_name +
            " at " + concrete_h5_file_position(writable) +
            " during attribute read");
    status = H5Oclose(obj_id);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close " +
            concrete_h5_file_position(writable) + " during attribute read");
    status = H5Pclose(fapl);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close HDF5 attribute during "
        "attribute read");
}

void HDF5IOHandlerImpl::listPaths(
    Writable *writable, Parameter<Operation::LIST_PATHS> &parameters)
{
    if (!writable->written)
        throw std::runtime_error(
            "[HDF5] Internal error: Writable not marked written during path "
            "listing");

    auto res = getFile(writable);
    File file = res ? res.get() : getFile(writable->parent).get();

    hid_t gapl = H5Pcreate(H5P_GROUP_ACCESS);
#if H5_VERSION_GE(1, 10, 0) && openPMD_HAVE_MPI
    if (m_hdf5_collective_metadata)
    {
        H5Pset_all_coll_metadata_ops(gapl, true);
    }
#endif

    hid_t node_id =
        H5Gopen(file.id, concrete_h5_file_position(writable).c_str(), gapl);
    VERIFY(
        node_id >= 0,
        "[HDF5] Internal error: Failed to open HDF5 group during path listing");

    H5G_info_t group_info;
    herr_t status = H5Gget_info(node_id, &group_info);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to get HDF5 group info for " +
            concrete_h5_file_position(writable) + " during path listing");

    auto paths = parameters.paths;
    for (hsize_t i = 0; i < group_info.nlinks; ++i)
    {
        if (H5G_GROUP == H5Gget_objtype_by_idx(node_id, i))
        {
            ssize_t name_length = H5Gget_objname_by_idx(node_id, i, nullptr, 0);
            std::vector<char> name(name_length + 1);
            H5Gget_objname_by_idx(node_id, i, name.data(), name_length + 1);
            paths->push_back(std::string(name.data(), name_length));
        }
    }

    status = H5Gclose(node_id);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close HDF5 group " +
            concrete_h5_file_position(writable) + " during path listing");
    status = H5Pclose(gapl);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close HDF5 property during path "
        "listing");
}

void HDF5IOHandlerImpl::listDatasets(
    Writable *writable, Parameter<Operation::LIST_DATASETS> &parameters)
{
    if (!writable->written)
        throw std::runtime_error(
            "[HDF5] Internal error: Writable not marked written during dataset "
            "listing");

    auto res = getFile(writable);
    File file = res ? res.get() : getFile(writable->parent).get();

    hid_t gapl = H5Pcreate(H5P_GROUP_ACCESS);
#if H5_VERSION_GE(1, 10, 0) && openPMD_HAVE_MPI
    if (m_hdf5_collective_metadata)
    {
        H5Pset_all_coll_metadata_ops(gapl, true);
    }
#endif

    hid_t node_id =
        H5Gopen(file.id, concrete_h5_file_position(writable).c_str(), gapl);
    VERIFY(
        node_id >= 0,
        "[HDF5] Internal error: Failed to open HDF5 group during dataset "
        "listing");

    H5G_info_t group_info;
    herr_t status = H5Gget_info(node_id, &group_info);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to get HDF5 group info for " +
            concrete_h5_file_position(writable) + " during dataset listing");

    auto datasets = parameters.datasets;
    for (hsize_t i = 0; i < group_info.nlinks; ++i)
    {
        if (H5G_DATASET == H5Gget_objtype_by_idx(node_id, i))
        {
            ssize_t name_length = H5Gget_objname_by_idx(node_id, i, nullptr, 0);
            std::vector<char> name(name_length + 1);
            H5Gget_objname_by_idx(node_id, i, name.data(), name_length + 1);
            datasets->push_back(std::string(name.data(), name_length));
        }
    }

    status = H5Gclose(node_id);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close HDF5 group " +
            concrete_h5_file_position(writable) + " during dataset listing");
    status = H5Pclose(gapl);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close HDF5 property during dataset "
        "listing");
}

void HDF5IOHandlerImpl::listAttributes(
    Writable *writable, Parameter<Operation::LIST_ATTS> &parameters)
{
    if (!writable->written)
        throw std::runtime_error(
            "[HDF5] Internal error: Writable not marked written during "
            "attribute listing");

    auto res = getFile(writable);
    File file = res ? res.get() : getFile(writable->parent).get();
    hid_t node_id;

    hid_t fapl = H5Pcreate(H5P_LINK_ACCESS);
#if H5_VERSION_GE(1, 10, 0) && openPMD_HAVE_MPI
    if (m_hdf5_collective_metadata)
    {
        H5Pset_all_coll_metadata_ops(fapl, true);
    }
#endif

    node_id =
        H5Oopen(file.id, concrete_h5_file_position(writable).c_str(), fapl);
    VERIFY(
        node_id >= 0,
        "[HDF5] Internal error: Failed to open HDF5 group during attribute "
        "listing");

    herr_t status;
#if H5_VERSION_GE(1, 12, 0)
    H5O_info2_t object_info;
    status = H5Oget_info3(node_id, &object_info, H5O_INFO_NUM_ATTRS);
#else
    H5O_info_t object_info;
    status = H5Oget_info(node_id, &object_info);
#endif
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to get HDF5 object info for " +
            concrete_h5_file_position(writable) + " during attribute listing");

    auto attributes = parameters.attributes;
    for (hsize_t i = 0; i < object_info.num_attrs; ++i)
    {
        ssize_t name_length = H5Aget_name_by_idx(
            node_id,
            ".",
            H5_INDEX_CRT_ORDER,
            H5_ITER_INC,
            i,
            nullptr,
            0,
            H5P_DEFAULT);
        std::vector<char> name(name_length + 1);
        H5Aget_name_by_idx(
            node_id,
            ".",
            H5_INDEX_CRT_ORDER,
            H5_ITER_INC,
            i,
            name.data(),
            name_length + 1,
            H5P_DEFAULT);
        attributes->push_back(std::string(name.data(), name_length));
    }

    status = H5Oclose(node_id);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close HDF5 object during attribute "
        "listing");
    status = H5Pclose(fapl);
    VERIFY(
        status == 0,
        "[HDF5] Internal error: Failed to close HDF5 property during dataset "
        "listing");
}

auxiliary::Option<HDF5IOHandlerImpl::File>
HDF5IOHandlerImpl::getFile(Writable *writable)
{
    using namespace auxiliary;
    auto it = m_fileNames.find(writable);
    if (it == m_fileNames.end())
    {
        return Option<File>();
    }
    auto it2 = m_fileNamesWithID.find(it->second);
    if (it2 == m_fileNamesWithID.end())
    {
        return Option<File>();
    }
    File res;
    res.name = it->second;
    res.id = it2->second;
    return makeOption(std::move(res));
}
#endif

#if openPMD_HAVE_HDF5
HDF5IOHandler::HDF5IOHandler(std::string path, Access at, nlohmann::json config)
    : AbstractIOHandler(std::move(path), at)
    , m_impl{new HDF5IOHandlerImpl(this, std::move(config))}
{}

HDF5IOHandler::~HDF5IOHandler() = default;

std::future<void> HDF5IOHandler::flush(internal::FlushParams const &)
{
    return m_impl->flush();
}
#else
HDF5IOHandler::HDF5IOHandler(
    std::string path, Access at, nlohmann::json /* config */)
    : AbstractIOHandler(std::move(path), at)
{
    throw std::runtime_error("openPMD-api built without HDF5 support");
}

HDF5IOHandler::~HDF5IOHandler() = default;

std::future<void> HDF5IOHandler::flush(internal::FlushParams const &)
{
    return std::future<void>();
}
#endif
} // namespace openPMD
