/* Copyright 2017-2021 Fabian Koller, Axel Huebl
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
#include <algorithm>
#include <complex>
#include <string>
#include <tuple>

void CommonADIOS1IOHandlerImpl::close(int64_t fd)
{
    int status;
    status = adios_close(fd);
    VERIFY(
        status == err_no_error,
        "[ADIOS1] Internal error: Failed to close ADIOS file (open_write)");
}

void CommonADIOS1IOHandlerImpl::close(ADIOS_FILE *f)
{
    int status;
    status = adios_read_close(f);
    VERIFY(
        status == err_no_error,
        "[ADIOS1] Internal error: Failed to close ADIOS file (open_read)");
}

void CommonADIOS1IOHandlerImpl::flush_attribute(
    int64_t group, std::string const &name, Attribute const &att)
{
    auto dtype = att.dtype;
    // https://github.com/ComputationalRadiationPhysics/picongpu/pull/1756
    if (dtype == Datatype::BOOL)
        dtype = Datatype::UCHAR;

    int nelems = 0;
    switch (dtype)
    {
        using DT = Datatype;
    case DT::VEC_CHAR:
        nelems = att.get<std::vector<char> >().size();
        break;
    case DT::VEC_SHORT:
        nelems = att.get<std::vector<short> >().size();
        break;
    case DT::VEC_INT:
        nelems = att.get<std::vector<int> >().size();
        break;
    case DT::VEC_LONG:
        nelems = att.get<std::vector<long> >().size();
        break;
    case DT::VEC_LONGLONG:
        nelems = att.get<std::vector<long long> >().size();
        break;
    case DT::VEC_UCHAR:
        nelems = att.get<std::vector<unsigned char> >().size();
        break;
    case DT::VEC_USHORT:
        nelems = att.get<std::vector<unsigned short> >().size();
        break;
    case DT::VEC_UINT:
        nelems = att.get<std::vector<unsigned int> >().size();
        break;
    case DT::VEC_ULONG:
        nelems = att.get<std::vector<unsigned long> >().size();
        break;
    case DT::VEC_ULONGLONG:
        nelems = att.get<std::vector<unsigned long long> >().size();
        break;
    case DT::VEC_FLOAT:
        nelems = att.get<std::vector<float> >().size();
        break;
    case DT::VEC_DOUBLE:
        nelems = att.get<std::vector<double> >().size();
        break;
    case DT::VEC_LONG_DOUBLE:
        nelems = att.get<std::vector<long double> >().size();
        break;
    case DT::VEC_STRING:
        nelems = att.get<std::vector<std::string> >().size();
        break;
    case DT::ARR_DBL_7:
        nelems = 7;
        break;
    case DT::UNDEFINED:
    case DT::DATATYPE:
        throw std::runtime_error(
            "[ADIOS1] Unknown Attribute datatype (ADIOS1 Attribute flush)");
    default:
        nelems = 1;
    }

    auto values = auxiliary::allocatePtr(dtype, nelems);
    switch (att.dtype)
    {
        using DT = Datatype;
    case DT::CHAR: {
        auto ptr = reinterpret_cast<char *>(values.get());
        *ptr = att.get<char>();
        break;
    }
    case DT::UCHAR: {
        auto ptr = reinterpret_cast<unsigned char *>(values.get());
        *ptr = att.get<unsigned char>();
        break;
    }
    case DT::SHORT: {
        auto ptr = reinterpret_cast<short *>(values.get());
        *ptr = att.get<short>();
        break;
    }
    case DT::INT: {
        auto ptr = reinterpret_cast<int *>(values.get());
        *ptr = att.get<int>();
        break;
    }
    case DT::LONG: {
        auto ptr = reinterpret_cast<long *>(values.get());
        *ptr = att.get<long>();
        break;
    }
    case DT::LONGLONG: {
        auto ptr = reinterpret_cast<long long *>(values.get());
        *ptr = att.get<long long>();
        break;
    }
    case DT::USHORT: {
        auto ptr = reinterpret_cast<unsigned short *>(values.get());
        *ptr = att.get<unsigned short>();
        break;
    }
    case DT::UINT: {
        auto ptr = reinterpret_cast<unsigned int *>(values.get());
        *ptr = att.get<unsigned int>();
        break;
    }
    case DT::ULONG: {
        auto ptr = reinterpret_cast<unsigned long *>(values.get());
        *ptr = att.get<unsigned long>();
        break;
    }
    case DT::ULONGLONG: {
        auto ptr = reinterpret_cast<unsigned long long *>(values.get());
        *ptr = att.get<unsigned long long>();
        break;
    }
    case DT::FLOAT: {
        auto ptr = reinterpret_cast<float *>(values.get());
        *ptr = att.get<float>();
        break;
    }
    case DT::DOUBLE: {
        auto ptr = reinterpret_cast<double *>(values.get());
        *ptr = att.get<double>();
        break;
    }
    case DT::LONG_DOUBLE: {
        auto ptr = reinterpret_cast<long double *>(values.get());
        *ptr = att.get<long double>();
        break;
    }
    case DT::CFLOAT: {
        auto ptr = reinterpret_cast<std::complex<float> *>(values.get());
        *ptr = att.get<std::complex<float> >();
        break;
    }
    case DT::CDOUBLE: {
        auto ptr = reinterpret_cast<std::complex<double> *>(values.get());
        *ptr = att.get<std::complex<double> >();
        break;
    }
    case DT::CLONG_DOUBLE: {
        throw std::runtime_error(
            "[ADIOS1] Unknown Attribute datatype (CLONG_DOUBLE)");
        break;
    }
    case DT::STRING: {
        auto const &v = att.get<std::string>();
        values = auxiliary::allocatePtr(Datatype::CHAR, v.length() + 1u);
        strcpy((char *)values.get(), v.c_str());
        break;
    }
    case DT::VEC_CHAR: {
        auto ptr = reinterpret_cast<char *>(values.get());
        auto const &vec = att.get<std::vector<char> >();
        for (size_t i = 0; i < vec.size(); ++i)
            ptr[i] = vec[i];
        break;
    }
    case DT::VEC_SHORT: {
        auto ptr = reinterpret_cast<short *>(values.get());
        auto const &vec = att.get<std::vector<short> >();
        for (size_t i = 0; i < vec.size(); ++i)
            ptr[i] = vec[i];
        break;
    }
    case DT::VEC_INT: {
        auto ptr = reinterpret_cast<int *>(values.get());
        auto const &vec = att.get<std::vector<int> >();
        for (size_t i = 0; i < vec.size(); ++i)
            ptr[i] = vec[i];
        break;
    }
    case DT::VEC_LONG: {
        auto ptr = reinterpret_cast<long *>(values.get());
        auto const &vec = att.get<std::vector<long> >();
        for (size_t i = 0; i < vec.size(); ++i)
            ptr[i] = vec[i];
        break;
    }
    case DT::VEC_LONGLONG: {
        auto ptr = reinterpret_cast<long long *>(values.get());
        auto const &vec = att.get<std::vector<long long> >();
        for (size_t i = 0; i < vec.size(); ++i)
            ptr[i] = vec[i];
        break;
    }
    case DT::VEC_UCHAR: {
        auto ptr = reinterpret_cast<unsigned char *>(values.get());
        auto const &vec = att.get<std::vector<unsigned char> >();
        for (size_t i = 0; i < vec.size(); ++i)
            ptr[i] = vec[i];
        break;
    }
    case DT::VEC_USHORT: {
        auto ptr = reinterpret_cast<unsigned short *>(values.get());
        auto const &vec = att.get<std::vector<unsigned short> >();
        for (size_t i = 0; i < vec.size(); ++i)
            ptr[i] = vec[i];
        break;
    }
    case DT::VEC_UINT: {
        auto ptr = reinterpret_cast<unsigned int *>(values.get());
        auto const &vec = att.get<std::vector<unsigned int> >();
        for (size_t i = 0; i < vec.size(); ++i)
            ptr[i] = vec[i];
        break;
    }
    case DT::VEC_ULONG: {
        auto ptr = reinterpret_cast<unsigned long *>(values.get());
        auto const &vec = att.get<std::vector<unsigned long> >();
        for (size_t i = 0; i < vec.size(); ++i)
            ptr[i] = vec[i];
        break;
    }
    case DT::VEC_ULONGLONG: {
        auto ptr = reinterpret_cast<unsigned long long *>(values.get());
        auto const &vec = att.get<std::vector<unsigned long long> >();
        for (size_t i = 0; i < vec.size(); ++i)
            ptr[i] = vec[i];
        break;
    }
    case DT::VEC_FLOAT: {
        auto ptr = reinterpret_cast<float *>(values.get());
        auto const &vec = att.get<std::vector<float> >();
        for (size_t i = 0; i < vec.size(); ++i)
            ptr[i] = vec[i];
        break;
    }
    case DT::VEC_DOUBLE: {
        auto ptr = reinterpret_cast<double *>(values.get());
        auto const &vec = att.get<std::vector<double> >();
        for (size_t i = 0; i < vec.size(); ++i)
            ptr[i] = vec[i];
        break;
    }
    case DT::VEC_LONG_DOUBLE: {
        auto ptr = reinterpret_cast<long double *>(values.get());
        auto const &vec = att.get<std::vector<long double> >();
        for (size_t i = 0; i < vec.size(); ++i)
            ptr[i] = vec[i];
        break;
    }
    /* not supported by ADIOS 1.13.1:
     *   https://github.com/ornladios/ADIOS/issues/212
     */
    case DT::VEC_CFLOAT:
    case DT::VEC_CDOUBLE:
    case DT::VEC_CLONG_DOUBLE: {
        throw std::runtime_error(
            "[ADIOS1] Arrays of complex attributes are not supported");
        break;
    }
    case DT::VEC_STRING: {
        auto ptr = reinterpret_cast<char **>(values.get());
        auto const &vec = att.get<std::vector<std::string> >();
        for (size_t i = 0; i < vec.size(); ++i)
        {
            size_t size = vec[i].size() + 1;
            ptr[i] = new char[size];
            strncpy(ptr[i], vec[i].c_str(), size);
        }
        break;
    }
    case DT::ARR_DBL_7: {
        auto ptr = reinterpret_cast<double *>(values.get());
        auto const &arr = att.get<std::array<double, 7> >();
        for (size_t i = 0; i < 7; ++i)
            ptr[i] = arr[i];
        break;
    }
    case DT::BOOL: {
        auto ptr = reinterpret_cast<unsigned char *>(values.get());
        *ptr = static_cast<unsigned char>(att.get<bool>());
        break;
    }
    case DT::UNDEFINED:
    case DT::DATATYPE:
        throw std::runtime_error(
            "[ADIOS1] Unknown Attribute datatype (ADIOS1 Attribute flush)");
    default:
        throw std::runtime_error(
            "[ADIOS1] Datatype not implemented in ADIOS IO");
    }

    int status;
    status = adios_define_attribute_byvalue(
        group,
        name.c_str(),
        "",
        getBP1DataType(att.dtype),
        nelems,
        values.get());
    VERIFY(
        status == err_no_error,
        "[ADIOS1] Internal error: Failed to define ADIOS attribute by value");

    if (att.dtype == Datatype::VEC_STRING)
    {
        auto ptr = reinterpret_cast<char **>(values.get());
        for (int i = 0; i < nelems; ++i)
            delete[] ptr[i];
    }
}

void CommonADIOS1IOHandlerImpl::createFile(
    Writable *writable, Parameter<Operation::CREATE_FILE> const &parameters)
{
    if (m_handler->m_backendAccess == Access::READ_ONLY)
        throw std::runtime_error(
            "[ADIOS1] Creating a file in read-only mode is not possible.");

    if (!writable->written)
    {
        if (!auxiliary::directory_exists(m_handler->directory))
        {
            bool success = auxiliary::create_directories(m_handler->directory);
            VERIFY(
                success,
                "[ADIOS1] Internal error: Failed to create directories during "
                "ADIOS file creation");
        }

        std::string name = m_handler->directory + parameters.name;
        if (!auxiliary::ends_with(name, ".bp"))
            name += ".bp";

        writable->written = true;
        writable->abstractFilePosition =
            std::make_shared<ADIOS1FilePosition>("/");

        m_filePaths[writable] = std::make_shared<std::string>(name);

        /* our control flow allows for more than one open file handle
         * if multiple files are opened with the same group, data might be lost
         */

        /* defer actually opening the file handle until the first
         * Operation::WRITE_DATASET occurs */
        m_existsOnDisk[m_filePaths[writable]] = false;

        GetFileHandle(writable);
    }
}

void CommonADIOS1IOHandlerImpl::createPath(
    Writable *writable, Parameter<Operation::CREATE_PATH> const &parameters)
{
    if (m_handler->m_backendAccess == Access::READ_ONLY)
        throw std::runtime_error(
            "[ADIOS1] Creating a path in a file opened as read only is not "
            "possible.");

    if (!writable->written)
    {
        /* Sanitize path */
        std::string path = parameters.path;
        if (auxiliary::starts_with(path, '/'))
            path = auxiliary::replace_first(path, "/", "");
        if (!auxiliary::ends_with(path, '/'))
            path += '/';

        /* ADIOS has no concept for explicitly creating paths.
         * They are implicitly created with the paths of variables/attributes.
         */

        writable->written = true;
        writable->abstractFilePosition =
            std::make_shared<ADIOS1FilePosition>(path);

        Writable *position;
        if (writable->parent)
            position = writable->parent;
        else
            position = writable; /* root does not have a parent but might still
                                    have to be written */
        auto res = m_filePaths.find(position);

        m_filePaths[writable] = res->second;
    }
}

void CommonADIOS1IOHandlerImpl::createDataset(
    Writable *writable, Parameter<Operation::CREATE_DATASET> const &parameters)
{
    if (m_handler->m_backendAccess == Access::READ_ONLY)
        throw std::runtime_error(
            "[ADIOS1] Creating a dataset in a file opened as read only is not "
            "possible.");

    if (!writable->written)
    {
        /* ADIOS variable definitions require the file to be (re-)opened to take
         * effect/not cause errors */
        auto res = m_filePaths.find(writable);
        if (res == m_filePaths.end())
            res = m_filePaths.find(writable->parent);

        int64_t group = m_groups[res->second];

        /* Sanitize name */
        std::string name = parameters.name;
        if (auxiliary::starts_with(name, '/'))
            name = auxiliary::replace_first(name, "/", "");
        if (auxiliary::ends_with(name, '/'))
            name = auxiliary::replace_last(name, "/", "");

        std::string path = concrete_bp1_file_position(writable) + name;

        size_t ndims = parameters.extent.size();

        std::vector<std::string> chunkSize(ndims, "");
        std::vector<std::string> chunkOffset(ndims, "");
        int64_t id;
        for (size_t i = 0; i < ndims; ++i)
        {
            chunkSize[i] = "/tmp" + path + "_chunkSize" + std::to_string(i);
            id = adios_define_var(
                group,
                chunkSize[i].c_str(),
                "",
                adios_unsigned_long,
                "",
                "",
                "");
            VERIFY(
                id != 0,
                "[ADIOS1] Internal error: Failed to define ADIOS variable "
                "during Dataset creation");
            chunkOffset[i] = "/tmp" + path + "_chunkOffset" + std::to_string(i);
            id = adios_define_var(
                group,
                chunkOffset[i].c_str(),
                "",
                adios_unsigned_long,
                "",
                "",
                "");
            VERIFY(
                id != 0,
                "[ADIOS1] Internal error: Failed to define ADIOS variable "
                "during Dataset creation");
        }

        std::string chunkSizeParam = auxiliary::join(chunkSize, ",");
        std::string globalSize = getBP1Extent(parameters.extent);
        std::string chunkOffsetParam = auxiliary::join(chunkOffset, ",");
        id = adios_define_var(
            group,
            path.c_str(),
            "",
            getBP1DataType(parameters.dtype),
            chunkSizeParam.c_str(),
            globalSize.c_str(),
            chunkOffsetParam.c_str());
        VERIFY(
            id != 0,
            "[ADIOS1] Internal error: Failed to define ADIOS variable during "
            "Dataset creation");

        if (!parameters.compression.empty())
            std::cerr << "Custom compression not compatible with ADIOS1 "
                         "backend. Use transform instead."
                      << std::endl;

        if (!parameters.transform.empty())
        {
            int status;
            status = adios_set_transform(id, parameters.transform.c_str());
            VERIFY(
                status == err_no_error,
                "[ADIOS1] Internal error: Failed to set ADIOS transform during "
                "Dataset cretaion");
        }

        writable->written = true;
        writable->abstractFilePosition =
            std::make_shared<ADIOS1FilePosition>(name);

        m_filePaths[writable] = res->second;
    }
}

void CommonADIOS1IOHandlerImpl::extendDataset(
    Writable *, Parameter<Operation::EXTEND_DATASET> const &)
{
    throw std::runtime_error(
        "[ADIOS1] Dataset extension not implemented in ADIOS backend");
}

void CommonADIOS1IOHandlerImpl::openFile(
    Writable *writable, Parameter<Operation::OPEN_FILE> const &parameters)
{
    if (!auxiliary::directory_exists(m_handler->directory))
        throw no_such_file_error(
            "[ADIOS1] Supplied directory is not valid: " +
            m_handler->directory);

    std::string name = m_handler->directory + parameters.name;
    if (!auxiliary::ends_with(name, ".bp"))
        name += ".bp";

    std::shared_ptr<std::string> filePath;
    auto it = std::find_if(
        m_filePaths.begin(),
        m_filePaths.end(),
        [name](std::unordered_map<Writable *, std::shared_ptr<std::string> >::
                   value_type const &entry) { return *entry.second == name; });
    if (it == m_filePaths.end())
        filePath = std::make_shared<std::string>(name);
    else
        filePath = it->second;

    if (m_handler->m_backendAccess == Access::CREATE)
    {
        // called at Series::flush for iterations that has been flushed before
        // this is to make sure to point the Series.m_writer points to this
        // iteration so when call Series.flushAttribute(), the attributes can be
        // flushed to the iteration level file.
        m_filePaths[writable] = filePath;
        writable->written = true;
        writable->abstractFilePosition =
            std::make_shared<ADIOS1FilePosition>("/");
        return;
    }
    /* close the handle that corresponds to the file we want to open */
    if (m_openWriteFileHandles.find(filePath) != m_openWriteFileHandles.end())
    {
        close(m_openWriteFileHandles[filePath]);
        m_openWriteFileHandles.erase(filePath);
    }

    if (m_groups.find(filePath) == m_groups.end())
        m_groups[filePath] = initialize_group(name);

    if (m_openReadFileHandles.find(filePath) == m_openReadFileHandles.end())
    {
        ADIOS_FILE *f = open_read(name);
        m_openReadFileHandles[filePath] = f;
    }

    writable->written = true;
    writable->abstractFilePosition = std::make_shared<ADIOS1FilePosition>("/");

    m_filePaths[writable] = filePath;
    m_existsOnDisk[filePath] = true;
}

void CommonADIOS1IOHandlerImpl::closeFile(
    Writable *writable, Parameter<Operation::CLOSE_FILE> const &)
{
    auto myFile = m_filePaths.find(writable);
    if (myFile == m_filePaths.end())
    {
        return;
    }

    // finish write operations
    auto myGroup = m_groups.find(myFile->second);
    if (myGroup != m_groups.end())
    {
        auto attributeWrites = m_attributeWrites.find(myGroup->second);
        if (this->m_handler->m_backendAccess != Access::READ_ONLY &&
            attributeWrites != m_attributeWrites.end())
        {
            for (auto &att : attributeWrites->second)
            {
                flush_attribute(myGroup->second, att.first, att.second);
            }
            m_attributeWrites.erase(attributeWrites);
        }
        m_groups.erase(myGroup);
    }

    auto handle_write = m_openWriteFileHandles.find(myFile->second);
    if (handle_write != m_openWriteFileHandles.end())
    {
        close(handle_write->second);
        m_openWriteFileHandles.erase(handle_write);
    }

    // finish read operations
    auto handle_read = m_openReadFileHandles.find(myFile->second);
    if (handle_read != m_openReadFileHandles.end())
    {
        auto scheduled = m_scheduledReads.find(handle_read->second);
        if (scheduled != m_scheduledReads.end())
        {
            auto status = adios_perform_reads(scheduled->first, 1);
            VERIFY(
                status == err_no_error,
                "[ADIOS1] Internal error: Failed to perform ADIOS reads during "
                "dataset reading");

            for (auto &sel : scheduled->second)
                adios_selection_delete(sel.selection);
            m_scheduledReads.erase(scheduled);
        }
        close(handle_read->second);
        m_openReadFileHandles.erase(handle_read);
    }
    m_existsOnDisk.erase(myFile->second);
    m_filePaths.erase(myFile);
}

void CommonADIOS1IOHandlerImpl::availableChunks(
    Writable *writable, Parameter<Operation::AVAILABLE_CHUNKS> &params)
{
    ADIOS_FILE *f;
    f = m_openReadFileHandles.at(m_filePaths.at(writable));
    std::string name = concrete_bp1_file_position(writable);
    VERIFY(
        std::strcmp(f->path, m_filePaths.at(writable)->c_str()) == 0,
        "[ADIOS1] Internal Error: Invalid ADIOS read file handle");
    ADIOS_VARINFO *varinfo = adios_inq_var(f, name.c_str());
    VERIFY(
        adios_errno == err_no_error,
        "[ADIOS1] Internal error: Failed to inquire ADIOS variable while "
        "querying available chunks.");
    int err = adios_inq_var_blockinfo(f, varinfo);
    VERIFY(
        err == 0,
        "[ADIOS1] Internal error: Failed to obtain ADIOS varinfo while "
        "querying available chunks.");
    int nblocks = varinfo->nblocks[0]; // we don't use steps, so index 0 is fine
    int ndim = varinfo->ndim;
    auto &table = *params.chunks;
    table.reserve(nblocks);
    for (int block = 0; block < nblocks; ++block)
    {
        ADIOS_VARBLOCK &varblock = varinfo->blockinfo[block];
        Offset offset(ndim);
        Extent extent(ndim);
        for (int i = 0; i < ndim; ++i)
        {
            offset[i] = varblock.start[i];
            extent[i] = varblock.count[i];
        }
        table.emplace_back(offset, extent, int(varblock.process_id));
    }
    adios_free_varinfo(varinfo);
}

void CommonADIOS1IOHandlerImpl::openPath(
    Writable *writable, Parameter<Operation::OPEN_PATH> const &parameters)
{
    /* Sanitize path */
    std::string path = parameters.path;
    if (!path.empty())
    {
        if (auxiliary::starts_with(path, '/'))
            path = auxiliary::replace_first(path, "/", "");
        if (!auxiliary::ends_with(path, '/'))
            path += '/';
    }

    writable->written = true;
    writable->abstractFilePosition = std::make_shared<ADIOS1FilePosition>(path);

    auto res = writable->parent ? m_filePaths.find(writable->parent)
                                : m_filePaths.find(writable);

    m_filePaths[writable] = res->second;
}

void CommonADIOS1IOHandlerImpl::openDataset(
    Writable *writable, Parameter<Operation::OPEN_DATASET> &parameters)
{
    ADIOS_FILE *f;
    auto res = m_filePaths.find(writable);
    if (res == m_filePaths.end())
        res = m_filePaths.find(writable->parent);
    f = m_openReadFileHandles.at(res->second);

    /* Sanitize name */
    std::string name = parameters.name;
    if (auxiliary::starts_with(name, '/'))
        name = auxiliary::replace_first(name, "/", "");

    std::string datasetname = writable->abstractFilePosition
        ? concrete_bp1_file_position(writable)
        : concrete_bp1_file_position(writable) + name;

    ADIOS_VARINFO *vi;
    vi = adios_inq_var(f, datasetname.c_str());
    std::string error_string("[ADIOS1] Internal error: ");
    error_string.append("Failed to inquire about ADIOS variable '")
        .append(datasetname)
        .append("' during dataset opening");
    VERIFY(adios_errno == err_no_error, error_string);
    VERIFY(vi != nullptr, error_string);

    Datatype dtype;

    // note the ill-named fixed-byte adios_... types
    // https://github.com/ornladios/ADIOS/issues/187
    switch (vi->type)
    {
        using DT = Datatype;
    case adios_byte:
        dtype = DT::CHAR;
        break;
    case adios_short:
        if (sizeof(short) == 2u)
            dtype = DT::SHORT;
        else if (sizeof(int) == 2u)
            dtype = DT::INT;
        else if (sizeof(long) == 2u)
            dtype = DT::LONG;
        else if (sizeof(long long) == 2u)
            dtype = DT::LONGLONG;
        else
            throw unsupported_data_error(
                "[ADIOS1] No native equivalent for Datatype adios_short "
                "found.");
        break;
    case adios_integer:
        if (sizeof(short) == 4u)
            dtype = DT::SHORT;
        else if (sizeof(int) == 4u)
            dtype = DT::INT;
        else if (sizeof(long) == 4u)
            dtype = DT::LONG;
        else if (sizeof(long long) == 4u)
            dtype = DT::LONGLONG;
        else
            throw unsupported_data_error(
                "[ADIOS1] No native equivalent for Datatype adios_integer "
                "found.");
        break;
    case adios_long:
        if (sizeof(short) == 8u)
            dtype = DT::SHORT;
        else if (sizeof(int) == 8u)
            dtype = DT::INT;
        else if (sizeof(long) == 8u)
            dtype = DT::LONG;
        else if (sizeof(long long) == 8u)
            dtype = DT::LONGLONG;
        else
            throw unsupported_data_error(
                "[ADIOS1] No native equivalent for Datatype adios_long found.");
        break;
    case adios_unsigned_byte:
        dtype = DT::UCHAR;
        break;
    case adios_unsigned_short:
        if (sizeof(unsigned short) == 2u)
            dtype = DT::USHORT;
        else if (sizeof(unsigned int) == 2u)
            dtype = DT::UINT;
        else if (sizeof(unsigned long) == 2u)
            dtype = DT::ULONG;
        else if (sizeof(unsigned long long) == 2u)
            dtype = DT::ULONGLONG;
        else
            throw unsupported_data_error(
                "[ADIOS1] No native equivalent for Datatype "
                "adios_unsigned_short found.");
        break;
    case adios_unsigned_integer:
        if (sizeof(unsigned short) == 4u)
            dtype = DT::USHORT;
        else if (sizeof(unsigned int) == 4u)
            dtype = DT::UINT;
        else if (sizeof(unsigned long) == 4u)
            dtype = DT::ULONG;
        else if (sizeof(unsigned long long) == 4u)
            dtype = DT::ULONGLONG;
        else
            throw unsupported_data_error(
                "[ADIOS1] No native equivalent for Datatype "
                "adios_unsigned_integer found.");
        break;
    case adios_unsigned_long:
        if (sizeof(unsigned short) == 8u)
            dtype = DT::USHORT;
        else if (sizeof(unsigned int) == 8u)
            dtype = DT::UINT;
        else if (sizeof(unsigned long) == 8u)
            dtype = DT::ULONG;
        else if (sizeof(unsigned long long) == 8u)
            dtype = DT::ULONGLONG;
        else
            throw unsupported_data_error(
                "[ADIOS1] No native equivalent for Datatype "
                "adios_unsigned_long found.");
        break;
    case adios_real:
        dtype = DT::FLOAT;
        break;
    case adios_double:
        dtype = DT::DOUBLE;
        break;
    case adios_long_double:
        dtype = DT::LONG_DOUBLE;
        break;
    case adios_complex:
        dtype = DT::CFLOAT;
        break;
    case adios_double_complex:
        dtype = DT::CDOUBLE;
        break;

    case adios_string:
    case adios_string_array:
    default:
        throw unsupported_data_error(
            "[ADIOS1] Datatype not implemented for ADIOS dataset writing");
    }
    *parameters.dtype = dtype;

    Extent e;
    e.resize(vi->ndim);
    for (int i = 0; i < vi->ndim; ++i)
        e[i] = vi->dims[i];
    *parameters.extent = e;

    writable->written = true;
    if (!writable->abstractFilePosition)
    {
        writable->abstractFilePosition =
            std::make_shared<ADIOS1FilePosition>(name);
    }

    m_openReadFileHandles[res->second] = f;
    m_filePaths[writable] = res->second;
}

void CommonADIOS1IOHandlerImpl::deleteFile(
    Writable *writable, Parameter<Operation::DELETE_FILE> const &parameters)
{
    if (m_handler->m_backendAccess == Access::READ_ONLY)
        throw std::runtime_error(
            "[ADIOS1] Deleting a file opened as read only is not possible.");

    if (writable->written)
    {
        auto path = m_filePaths.at(writable);
        if (m_openReadFileHandles.find(path) != m_openReadFileHandles.end())
        {
            close(m_openReadFileHandles.at(path));
            m_openReadFileHandles.erase(path);
        }
        if (m_openWriteFileHandles.find(path) != m_openWriteFileHandles.end())
        {
            close(m_openWriteFileHandles.at(path));
            m_openWriteFileHandles.erase(path);
        }

        std::string name = m_handler->directory + parameters.name;
        if (!auxiliary::ends_with(name, ".bp"))
            name += ".bp";

        if (!auxiliary::file_exists(name))
            throw std::runtime_error("[ADIOS1] File does not exist: " + name);

        auxiliary::remove_file(name);

        writable->written = false;
        writable->abstractFilePosition.reset();

        m_filePaths.erase(writable);
    }
}

void CommonADIOS1IOHandlerImpl::deletePath(
    Writable *, Parameter<Operation::DELETE_PATH> const &)
{
    throw std::runtime_error(
        "[ADIOS1] Path deletion not implemented in ADIOS backend");
}

void CommonADIOS1IOHandlerImpl::deleteDataset(
    Writable *, Parameter<Operation::DELETE_DATASET> const &)
{
    throw std::runtime_error(
        "[ADIOS1] Dataset deletion not implemented in ADIOS backend");
}

void CommonADIOS1IOHandlerImpl::deleteAttribute(
    Writable *, Parameter<Operation::DELETE_ATT> const &)
{
    throw std::runtime_error(
        "[ADIOS1] Attribute deletion not implemented in ADIOS backend");
}

int64_t CommonADIOS1IOHandlerImpl::GetFileHandle(Writable *writable)
{
    auto res = m_filePaths.find(writable);
    if (res == m_filePaths.end())
        res = m_filePaths.find(writable->parent);
    int64_t fd;

    if (m_openWriteFileHandles.find(res->second) ==
        m_openWriteFileHandles.end())
    {
        std::string name = *(res->second);
        m_groups[m_filePaths[writable]] = initialize_group(name);

        fd = open_write(writable);
        m_openWriteFileHandles[res->second] = fd;
    }
    else
        fd = m_openWriteFileHandles.at(res->second);

    return fd;
}
void CommonADIOS1IOHandlerImpl::writeDataset(
    Writable *writable, Parameter<Operation::WRITE_DATASET> const &parameters)
{
    if (m_handler->m_backendAccess == Access::READ_ONLY)
        throw std::runtime_error(
            "[ADIOS1] Writing into a dataset in a file opened as read-only is "
            "not possible.");

    int64_t fd = GetFileHandle(writable);

    std::string name = concrete_bp1_file_position(writable);

    size_t ndims = parameters.extent.size();

    std::string chunkSize;
    std::string chunkOffset;
    int status;
    for (size_t i = 0; i < ndims; ++i)
    {
        chunkSize = "/tmp" + name + "_chunkSize" + std::to_string(i);
        status = adios_write(fd, chunkSize.c_str(), &parameters.extent[i]);
        VERIFY(
            status == err_no_error,
            "[ADIOS1] Internal error: Failed to write ADIOS variable during "
            "Dataset writing");
        chunkOffset = "/tmp" + name + "_chunkOffset" + std::to_string(i);
        status = adios_write(fd, chunkOffset.c_str(), &parameters.offset[i]);
        VERIFY(
            status == err_no_error,
            "[ADIOS1] Internal error: Failed to write ADIOS variable during "
            "Dataset writing");
    }

    status = adios_write(fd, name.c_str(), parameters.data.get());
    VERIFY(
        status == err_no_error,
        "[ADIOS1] Internal error: Failed to write ADIOS variable during "
        "Dataset writing");
}

void CommonADIOS1IOHandlerImpl::writeAttribute(
    Writable *writable, Parameter<Operation::WRITE_ATT> const &parameters)
{
    if (m_handler->m_backendAccess == Access::READ_ONLY)
        throw std::runtime_error(
            "[ADIOS1] Writing an attribute in a file opened as read only is "
            "not possible.");

    std::string name = concrete_bp1_file_position(writable);
    if (!auxiliary::ends_with(name, '/'))
        name += '/';
    name += parameters.name;

    auto res = m_filePaths.find(writable);
    if (res == m_filePaths.end())
        res = m_filePaths.find(writable->parent);
    GetFileHandle(writable);

    int64_t group = m_groups[res->second];

    auto &attributes = m_attributeWrites[group];
    attributes.erase(name);
    attributes.emplace(name, parameters.resource);
}

void CommonADIOS1IOHandlerImpl::readDataset(
    Writable *writable, Parameter<Operation::READ_DATASET> &parameters)
{
    switch (parameters.dtype)
    {
        using DT = Datatype;
    case DT::DOUBLE:
    case DT::FLOAT:
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
            "[ADIOS1] Unknown Attribute datatype (ADIOS1 Dataset read)");
    case DT::DATATYPE:
        throw std::runtime_error("[ADIOS1] Meta-Datatype leaked into IO");
    default:
        throw std::runtime_error(
            "[ADIOS1] Datatype not implemented in ADIOS1 IO");
    }

    ADIOS_FILE *f;
    f = m_openReadFileHandles.at(m_filePaths.at(writable));
    VERIFY(
        std::strcmp(f->path, m_filePaths.at(writable)->c_str()) == 0,
        "[ADIOS1] Internal Error: Invalid ADIOS read file handle");

    ADIOS_SELECTION *sel;
    sel = adios_selection_boundingbox(
        parameters.extent.size(),
        parameters.offset.data(),
        parameters.extent.data());
    VERIFY(
        sel != nullptr,
        "[ADIOS1] Internal error: Failed to select ADIOS bounding box during "
        "dataset reading");
    VERIFY(
        adios_errno == err_no_error,
        "[ADIOS1] Internal error: Failed to select ADIOS bounding box during "
        "dataset reading");

    std::string varname = concrete_bp1_file_position(writable);
    void *data = parameters.data.get();

    int status;
    status = adios_schedule_read(f, sel, varname.c_str(), 0, 1, data);
    VERIFY(
        status == err_no_error,
        "[ADIOS1] Internal error: Failed to schedule ADIOS read during dataset "
        "reading");
    VERIFY(
        adios_errno == err_no_error,
        "[ADIOS1] Internal error: Failed to schedule ADIOS read during dataset "
        "reading");

    m_scheduledReads[f].push_back({sel, parameters.data});
}

void CommonADIOS1IOHandlerImpl::readAttribute(
    Writable *writable, Parameter<Operation::READ_ATT> &parameters)
{
    if (!writable->written)
        throw std::runtime_error(
            "[ADIOS1] Internal error: Writable not marked written during "
            "attribute reading");

    ADIOS_FILE *f;
    f = m_openReadFileHandles.at(m_filePaths.at(writable));

    std::string attrname = concrete_bp1_file_position(writable);
    if (!auxiliary::ends_with(attrname, '/'))
        attrname += "/";
    attrname += parameters.name;

    ADIOS_DATATYPES datatype = adios_unknown;
    int size = 0;
    void *data = nullptr;

    int status;
    status = adios_get_attr(f, attrname.c_str(), &datatype, &size, &data);
    VERIFY(
        status == 0,
        "[ADIOS1] Internal error: Failed to get ADIOS1 attribute during "
        "attribute read");
    VERIFY(
        datatype != adios_unknown,
        "[ADIOS1] Internal error: Read unknown ADIOS1 datatype during "
        "attribute read");
    VERIFY(size != 0, "[ADIOS1] Internal error: ADIOS1 read 0-size attribute");

    // size is returned in number of allocated bytes
    // note the ill-named fixed-byte adios_... types
    // https://github.com/ornladios/ADIOS/issues/187
    switch (datatype)
    {
    case adios_byte:
        break;
    case adios_short:
        size /= 2;
        break;
    case adios_integer:
        size /= 4;
        break;
    case adios_long:
        size /= 8;
        break;
    case adios_unsigned_byte:
        break;
    case adios_unsigned_short:
        size /= 2;
        break;
    case adios_unsigned_integer:
        size /= 4;
        break;
    case adios_unsigned_long:
        size /= 8;
        break;
    case adios_real:
        size /= 4;
        break;
    case adios_double:
        size /= 8;
        break;
    case adios_long_double:
        size /= sizeof(long double);
        break;
    case adios_complex:
        size /= 8;
        break;
    case adios_double_complex:
        size /= 16;
        break;
    case adios_string:
        break;
    case adios_string_array:
        size /= sizeof(char *);
        break;

    default:
        throw unsupported_data_error(
            "[ADIOS1] readAttribute: Unsupported ADIOS1 attribute datatype '" +
            std::to_string(datatype) + "' in size check");
    }

    Datatype dtype;
    Attribute a(0);
    if (size == 1)
    {
        switch (datatype)
        {
            using DT = Datatype;
        case adios_byte:
            dtype = DT::CHAR;
            a = Attribute(*reinterpret_cast<char *>(data));
            break;
        case adios_short:
            if (sizeof(short) == 2u)
            {
                dtype = DT::SHORT;
                a = Attribute(*reinterpret_cast<short *>(data));
            }
            else if (sizeof(int) == 2u)
            {
                dtype = DT::INT;
                a = Attribute(*reinterpret_cast<int *>(data));
            }
            else if (sizeof(long) == 2u)
            {
                dtype = DT::LONG;
                a = Attribute(*reinterpret_cast<long *>(data));
            }
            else if (sizeof(long long) == 2u)
            {
                dtype = DT::LONGLONG;
                a = Attribute(*reinterpret_cast<long long *>(data));
            }
            else
                throw unsupported_data_error(
                    "[ADIOS1] No native equivalent for Datatype adios_short "
                    "found.");
            break;
        case adios_integer:
            if (sizeof(short) == 4u)
            {
                dtype = DT::SHORT;
                a = Attribute(*reinterpret_cast<short *>(data));
            }
            else if (sizeof(int) == 4u)
            {
                dtype = DT::INT;
                a = Attribute(*reinterpret_cast<int *>(data));
            }
            else if (sizeof(long) == 4u)
            {
                dtype = DT::LONG;
                a = Attribute(*reinterpret_cast<long *>(data));
            }
            else if (sizeof(long long) == 4u)
            {
                dtype = DT::LONGLONG;
                a = Attribute(*reinterpret_cast<long long *>(data));
            }
            else
                throw unsupported_data_error(
                    "[ADIOS1] No native equivalent for Datatype adios_integer "
                    "found.");
            break;
        case adios_long:
            if (sizeof(short) == 8u)
            {
                dtype = DT::SHORT;
                a = Attribute(*reinterpret_cast<short *>(data));
            }
            else if (sizeof(int) == 8u)
            {
                dtype = DT::INT;
                a = Attribute(*reinterpret_cast<int *>(data));
            }
            else if (sizeof(long) == 8u)
            {
                dtype = DT::LONG;
                a = Attribute(*reinterpret_cast<long *>(data));
            }
            else if (sizeof(long long) == 8u)
            {
                dtype = DT::LONGLONG;
                a = Attribute(*reinterpret_cast<long long *>(data));
            }
            else
                throw unsupported_data_error(
                    "[ADIOS1] No native equivalent for Datatype adios_long "
                    "found.");
            break;
        case adios_unsigned_byte:
            dtype = DT::UCHAR;
            a = Attribute(*reinterpret_cast<unsigned char *>(data));
            break;
        case adios_unsigned_short:
            if (sizeof(unsigned short) == 2u)
            {
                dtype = DT::USHORT;
                a = Attribute(*reinterpret_cast<unsigned short *>(data));
            }
            else if (sizeof(unsigned int) == 2u)
            {
                dtype = DT::UINT;
                a = Attribute(*reinterpret_cast<unsigned int *>(data));
            }
            else if (sizeof(unsigned long) == 2u)
            {
                dtype = DT::ULONG;
                a = Attribute(*reinterpret_cast<unsigned long *>(data));
            }
            else if (sizeof(unsigned long long) == 2u)
            {
                dtype = DT::ULONGLONG;
                a = Attribute(*reinterpret_cast<unsigned long long *>(data));
            }
            else
                throw unsupported_data_error(
                    "[ADIOS1] No native equivalent for Datatype "
                    "adios_unsigned_short found.");
            break;
        case adios_unsigned_integer:
            if (sizeof(unsigned short) == 4u)
            {
                dtype = DT::USHORT;
                a = Attribute(*reinterpret_cast<unsigned short *>(data));
            }
            else if (sizeof(unsigned int) == 4u)
            {
                dtype = DT::UINT;
                a = Attribute(*reinterpret_cast<unsigned int *>(data));
            }
            else if (sizeof(unsigned long) == 4u)
            {
                dtype = DT::ULONG;
                a = Attribute(*reinterpret_cast<unsigned long *>(data));
            }
            else if (sizeof(unsigned long long) == 4u)
            {
                dtype = DT::ULONGLONG;
                a = Attribute(*reinterpret_cast<unsigned long long *>(data));
            }
            else
                throw unsupported_data_error(
                    "[ADIOS1] No native equivalent for Datatype "
                    "adios_unsigned_integer found.");
            break;
        case adios_unsigned_long:
            if (sizeof(unsigned short) == 8u)
            {
                dtype = DT::USHORT;
                a = Attribute(*reinterpret_cast<unsigned short *>(data));
            }
            else if (sizeof(unsigned int) == 8u)
            {
                dtype = DT::UINT;
                a = Attribute(*reinterpret_cast<unsigned int *>(data));
            }
            else if (sizeof(unsigned long) == 8u)
            {
                dtype = DT::ULONG;
                a = Attribute(*reinterpret_cast<unsigned long *>(data));
            }
            else if (sizeof(unsigned long long) == 8u)
            {
                dtype = DT::ULONGLONG;
                a = Attribute(*reinterpret_cast<unsigned long long *>(data));
            }
            else
                throw unsupported_data_error(
                    "[ADIOS1] No native equivalent for Datatype "
                    "adios_unsigned_long found.");
            break;
        case adios_real:
            dtype = DT::FLOAT;
            a = Attribute(*reinterpret_cast<float *>(data));
            break;
        case adios_double:
            dtype = DT::DOUBLE;
            a = Attribute(*reinterpret_cast<double *>(data));
            break;
        case adios_long_double:
            dtype = DT::LONG_DOUBLE;
            a = Attribute(*reinterpret_cast<long double *>(data));
            break;
        case adios_complex:
            dtype = DT::CFLOAT;
            a = Attribute(*reinterpret_cast<std::complex<float> *>(data));
            break;
        case adios_double_complex:
            dtype = DT::CDOUBLE;
            a = Attribute(*reinterpret_cast<std::complex<double> *>(data));
            break;
        case adios_string: {
            dtype = DT::STRING;
            auto c = reinterpret_cast<char *>(data);
            a = Attribute(
                auxiliary::strip(std::string(c, std::strlen(c)), {'\0'}));
            break;
        }
        case adios_string_array: {
            dtype = DT::VEC_STRING;
            auto c = reinterpret_cast<char **>(data);
            std::vector<std::string> vs;
            vs.resize(size);
            for (int i = 0; i < size; ++i)
            {
                vs[i] = auxiliary::strip(
                    std::string(c[i], std::strlen(c[i])), {'\0'});
                /** @todo pointer should be freed, but this causes memory
                 * corruption */
                // free(c[i]);
            }
            a = Attribute(vs);
            break;
        }
        default:
            throw unsupported_data_error(
                "[ADIOS1] readAttribute: Unsupported ADIOS1 attribute datatype "
                "'" +
                std::to_string(datatype) + "' in scalar branch");
        }
    }
    else
    {
        switch (datatype)
        {
            using DT = Datatype;
        case adios_byte: {
            dtype = DT::VEC_CHAR;
            auto c = reinterpret_cast<char *>(data);
            std::vector<char> vc;
            vc.resize(size);
            for (int i = 0; i < size; ++i)
                vc[i] = c[i];
            a = Attribute(vc);
            break;
        }
        case adios_short: {
            if (sizeof(short) == 2u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<short>(data, size),
                    DT::VEC_SHORT);
            else if (sizeof(int) == 2u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<int>(data, size), DT::VEC_INT);
            else if (sizeof(long) == 2u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<long>(data, size),
                    DT::VEC_LONG);
            else if (sizeof(long long) == 2u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<long long>(data, size),
                    DT::VEC_LONGLONG);
            else
                throw unsupported_data_error(
                    "[ADIOS1] No native equivalent for Datatype adios_short "
                    "found.");
            break;
        }
        case adios_integer: {
            if (sizeof(short) == 4u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<short>(data, size),
                    DT::VEC_SHORT);
            else if (sizeof(int) == 4u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<int>(data, size), DT::VEC_INT);
            else if (sizeof(long) == 4u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<long>(data, size),
                    DT::VEC_LONG);
            else if (sizeof(long long) == 4u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<long long>(data, size),
                    DT::VEC_LONGLONG);
            else
                throw unsupported_data_error(
                    "[ADIOS1] No native equivalent for Datatype adios_integer "
                    "found.");
            break;
        }
        case adios_long: {
            if (sizeof(short) == 8u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<short>(data, size),
                    DT::VEC_SHORT);
            else if (sizeof(int) == 8u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<int>(data, size), DT::VEC_INT);
            else if (sizeof(long) == 8u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<long>(data, size),
                    DT::VEC_LONG);
            else if (sizeof(long long) == 8u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<long long>(data, size),
                    DT::VEC_LONGLONG);
            else
                throw unsupported_data_error(
                    "[ADIOS1] No native equivalent for Datatype adios_long "
                    "found.");
            break;
        }
        case adios_unsigned_byte: {
            dtype = DT::VEC_UCHAR;
            auto uc = reinterpret_cast<unsigned char *>(data);
            std::vector<unsigned char> vuc;
            vuc.resize(size);
            for (int i = 0; i < size; ++i)
                vuc[i] = uc[i];
            a = Attribute(vuc);
            break;
        }
        case adios_unsigned_short: {
            if (sizeof(unsigned short) == 2u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<unsigned short>(data, size),
                    DT::VEC_USHORT);
            else if (sizeof(unsigned int) == 2u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<unsigned int>(data, size),
                    DT::VEC_UINT);
            else if (sizeof(unsigned long) == 2u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<unsigned long>(data, size),
                    DT::VEC_ULONG);
            else if (sizeof(unsigned long long) == 2u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<unsigned long long>(data, size),
                    DT::VEC_ULONGLONG);
            else
                throw unsupported_data_error(
                    "[ADIOS1] No native equivalent for Datatype "
                    "adios_unsigned_short found.");
            break;
        }
        case adios_unsigned_integer: {
            if (sizeof(unsigned short) == 4u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<unsigned short>(data, size),
                    DT::VEC_USHORT);
            else if (sizeof(unsigned int) == 4u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<unsigned int>(data, size),
                    DT::VEC_UINT);
            else if (sizeof(unsigned long) == 4u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<unsigned long>(data, size),
                    DT::VEC_ULONG);
            else if (sizeof(unsigned long long) == 4u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<unsigned long long>(data, size),
                    DT::VEC_ULONGLONG);
            else
                throw unsupported_data_error(
                    "[ADIOS1] No native equivalent for Datatype "
                    "adios_unsigned_integer found.");
            break;
        }
        case adios_unsigned_long: {
            if (sizeof(unsigned short) == 8u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<unsigned short>(data, size),
                    DT::VEC_USHORT);
            else if (sizeof(unsigned int) == 8u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<unsigned int>(data, size),
                    DT::VEC_UINT);
            else if (sizeof(unsigned long) == 8u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<unsigned long>(data, size),
                    DT::VEC_ULONG);
            else if (sizeof(unsigned long long) == 8u)
                std::tie(a, dtype) = std::make_tuple(
                    readVectorAttributeInternal<unsigned long long>(data, size),
                    DT::VEC_ULONGLONG);
            else
                throw unsupported_data_error(
                    "[ADIOS1] No native equivalent for Datatype "
                    "adios_unsigned_long found.");
            break;
        }
        case adios_real: {
            dtype = DT::VEC_FLOAT;
            auto f4 = reinterpret_cast<float *>(data);
            std::vector<float> vf;
            vf.resize(size);
            for (int i = 0; i < size; ++i)
                vf[i] = f4[i];
            a = Attribute(vf);
            break;
        }
        case adios_double: {
            dtype = DT::VEC_DOUBLE;
            auto d8 = reinterpret_cast<double *>(data);
            std::vector<double> vd;
            vd.resize(size);
            for (int i = 0; i < size; ++i)
                vd[i] = d8[i];
            a = Attribute(vd);
            break;
        }
        case adios_long_double: {
            dtype = DT::VEC_LONG_DOUBLE;
            auto ld = reinterpret_cast<long double *>(data);
            std::vector<long double> vld;
            vld.resize(size);
            for (int i = 0; i < size; ++i)
                vld[i] = ld[i];
            a = Attribute(vld);
            break;
        }
        /* not supported by ADIOS 1.13.1: VEC_CFLOAT, VEC_CDOUBLE,
         * VEC_CLONG_DOUBLE https://github.com/ornladios/ADIOS/issues/212
         */
        case adios_string: {
            dtype = DT::STRING;
            a = Attribute(auxiliary::strip(
                std::string(reinterpret_cast<char *>(data), size), {'\0'}));
            break;
        }
        case adios_string_array: {
            dtype = DT::VEC_STRING;
            auto c = reinterpret_cast<char **>(data);
            std::vector<std::string> vs;
            vs.resize(size);
            for (int i = 0; i < size; ++i)
            {
                vs[i] = auxiliary::strip(
                    std::string(c[i], std::strlen(c[i])), {'\0'});
                /** @todo pointer should be freed, but this causes memory
                 * corruption */
                // free(c[i]);
            }
            a = Attribute(vs);
            break;
        }

        default:
            throw unsupported_data_error(
                "[ADIOS1] readAttribute: Unsupported ADIOS1 attribute datatype "
                "'" +
                std::to_string(datatype) + "' in vector branch");
        }
    }

    free(data);

    *parameters.dtype = dtype;
    *parameters.resource = a.getResource();
}

void CommonADIOS1IOHandlerImpl::listPaths(
    Writable *writable, Parameter<Operation::LIST_PATHS> &parameters)
{
    if (!writable->written)
        throw std::runtime_error(
            "[ADIOS1] Internal error: Writable not marked written during path "
            "listing");

    ADIOS_FILE *f;
    f = m_openReadFileHandles.at(m_filePaths.at(writable));

    std::string name = concrete_bp1_file_position(writable);

    std::unordered_set<std::string> paths;
    std::unordered_set<std::string> variables;
    for (int i = 0; i < f->nvars; ++i)
    {
        char *str = f->var_namelist[i];
        std::string s(str, std::strlen(str));
        if (auxiliary::starts_with(s, name))
        {
            /* remove the writable's path from the name */
            s = auxiliary::replace_first(s, name, "");
            variables.emplace(s);
            if (std::any_of(
                    s.begin(), s.end(), [](char c) { return c == '/'; }))
            {
                /* there are more path levels after the current writable */
                s = s.substr(0, s.find_first_of('/'));
                paths.emplace(s);
            }
        }
    }
    for (int i = 0; i < f->nattrs; ++i)
    {
        char *str = f->attr_namelist[i];
        std::string s(str, std::strlen(str));
        if (auxiliary::starts_with(s, name))
        {
            /* remove the writable's path from the name */
            s = auxiliary::replace_first(s, name, "");
            if (std::any_of(
                    s.begin(), s.end(), [](char c) { return c == '/'; }))
            {
                /* remove the attribute name */
                s = s.substr(0, s.find_last_of('/'));
                if (!std::any_of(
                        variables.begin(),
                        variables.end(),
                        [&s](std::string const &var) {
                            return auxiliary::starts_with(var, s);
                        }))
                {
                    /* this is either a group or a constant scalar */
                    s = s.substr(0, s.find_first_of('/'));
                    paths.emplace(s);
                }
            }
        }
    }

    *parameters.paths = std::vector<std::string>(paths.begin(), paths.end());
}

void CommonADIOS1IOHandlerImpl::listDatasets(
    Writable *writable, Parameter<Operation::LIST_DATASETS> &parameters)
{
    if (!writable->written)
        throw std::runtime_error(
            "[ADIOS1] Internal error: Writable not marked written during "
            "dataset listing");

    ADIOS_FILE *f;
    f = m_openReadFileHandles.at(m_filePaths.at(writable));

    std::string name = concrete_bp1_file_position(writable);

    std::unordered_set<std::string> paths;
    for (int i = 0; i < f->nvars; ++i)
    {
        char *str = f->var_namelist[i];
        std::string s(str, std::strlen(str));
        if (auxiliary::starts_with(s, name))
        {
            /* remove the writable's path from the name */
            s = auxiliary::replace_first(s, name, "");
            if (std::none_of(
                    s.begin(), s.end(), [](char c) { return c == '/'; }))
            {
                /* this is a dataset of the writable */
                paths.emplace(s);
            }
        }
    }

    *parameters.datasets = std::vector<std::string>(paths.begin(), paths.end());
}

void CommonADIOS1IOHandlerImpl::listAttributes(
    Writable *writable, Parameter<Operation::LIST_ATTS> &parameters)
{
    if (!writable->written)
        throw std::runtime_error(
            "[ADIOS1] Internal error: Writable not marked written during "
            "attribute listing");

    ADIOS_FILE *f;
    f = m_openReadFileHandles.at(m_filePaths.at(writable));

    std::string name = concrete_bp1_file_position(writable);

    if (!auxiliary::ends_with(name, '/'))
    {
        /* writable is a dataset and corresponds to an ADIOS variable */
        ADIOS_VARINFO *info;
        info = adios_inq_var(f, name.c_str());
        VERIFY(
            adios_errno == err_no_error,
            "[ADIOS1] Internal error: Failed to inquire ADIOS variable during "
            "attribute listing");
        VERIFY(
            info != nullptr,
            "[ADIOS1] Internal error: Failed to inquire ADIOS variable during "
            "attribute listing");

        name += '/';
        parameters.attributes->reserve(info->nattrs);
        for (int i = 0; i < info->nattrs; ++i)
        {
            char *c = f->attr_namelist[info->attr_ids[i]];
            parameters.attributes->push_back(auxiliary::replace_first(
                std::string(c, std::strlen(c)), name, ""));
        }

        adios_free_varinfo(info);
    }
    else
    {
        /* there is no ADIOS variable associated with the writable */
        std::unordered_set<std::string> attributes;
        for (int i = 0; i < f->nattrs; ++i)
        {
            char *str = f->attr_namelist[i];
            std::string s(str, std::strlen(str));
            if (auxiliary::starts_with(s, name))
            {
                /* remove the writable's path from the name */
                s = auxiliary::replace_first(s, name, "");
                if (std::none_of(
                        s.begin(), s.end(), [](char c) { return c == '/'; }))
                {
                    /* this is an attribute of the writable */
                    attributes.insert(s);
                }
            }
        }
        *parameters.attributes =
            std::vector<std::string>(attributes.begin(), attributes.end());
    }
}
