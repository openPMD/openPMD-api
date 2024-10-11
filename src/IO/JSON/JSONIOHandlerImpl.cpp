/* Copyright 2017-2021 Franz Poeschel
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

#include "openPMD/IO/JSON/JSONIOHandlerImpl.hpp"
#include "openPMD/Datatype.hpp"
#include "openPMD/DatatypeHelpers.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/AbstractIOHandlerImpl.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/auxiliary/JSON_internal.hpp"
#include "openPMD/auxiliary/Memory.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/auxiliary/TypeTraits.hpp"
#include "openPMD/backend/Writable.hpp"

#include <iomanip>
#include <sstream>
#include <toml.hpp>

#include <algorithm>
#include <exception>
#include <iostream>
#include <optional>

namespace openPMD
{
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
    } while (0);
#endif

#define VERIFY_ALWAYS(CONDITION, TEXT)                                         \
    {                                                                          \
        if (!(CONDITION))                                                      \
            throw std::runtime_error((TEXT));                                  \
    }

namespace
{
    struct DefaultValue
    {
        template <typename T>
        static nlohmann::json call()
        {
            if constexpr (auxiliary::IsComplex_v<T>)
            {
                return typename T::value_type{};
            }
            else
            {
                return T{};
            }
#if defined(__INTEL_COMPILER)
/*
 * ICPC has trouble with if constexpr, thinking that return statements are
 * missing afterwards. Deactivate the warning.
 * Note that putting a statement here will not help to fix this since it will
 * then complain about unreachable code.
 * https://community.intel.com/t5/Intel-C-Compiler/quot-if-constexpr-quot-and-quot-missing-return-statement-quot-in/td-p/1154551
 */
#pragma warning(disable : 1011)
        }
#pragma warning(default : 1011)
#else
        }
#endif

        static constexpr char const *errorMsg = "JSON default value";
    };

    /*
     * If initializeWithDefaultValue contains a datatype, then the dataset ought
     * to be initialized with the zero value of that dataset.
     * Otherwise with null.
     */
    nlohmann::json initializeNDArray(
        Extent const &extent,
        std::optional<Datatype> initializeWithDefaultValue)
    {
        // idea: begin from the innermost shale and copy the result into the
        // outer shales
        nlohmann::json accum = initializeWithDefaultValue.has_value()
            ? switchNonVectorType<DefaultValue>(
                  initializeWithDefaultValue.value())
            : nlohmann::json();
        nlohmann::json old;
        auto *accum_ptr = &accum;
        auto *old_ptr = &old;
        for (auto it = extent.rbegin(); it != extent.rend(); it++)
        {
            std::swap(old_ptr, accum_ptr);
            *accum_ptr = nlohmann::json::array();
            for (Extent::value_type i = 0; i < *it; i++)
            {
                (*accum_ptr)[i] = *old_ptr; // copy boi
            }
        }
        return *accum_ptr;
    }
} // namespace

JSONIOHandlerImpl::JSONIOHandlerImpl(
    AbstractIOHandler *handler,
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    [[maybe_unused]] openPMD::json::TracingJSON config,
    FileFormat format,
    std::string originalExtension)
    : AbstractIOHandlerImpl(handler)
    , m_fileFormat{format}
    , m_originalExtension{std::move(originalExtension)}
{}

#if openPMD_HAVE_MPI
JSONIOHandlerImpl::JSONIOHandlerImpl(
    AbstractIOHandler *handler,
    MPI_Comm comm,
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    [[maybe_unused]] openPMD::json::TracingJSON config,
    FileFormat format,
    std::string originalExtension)
    : AbstractIOHandlerImpl(handler)
    , m_communicator{comm}
    , m_fileFormat{format}
    , m_originalExtension{std::move(originalExtension)}
{}
#endif

JSONIOHandlerImpl::~JSONIOHandlerImpl() = default;

std::future<void> JSONIOHandlerImpl::flush()
{
    AbstractIOHandlerImpl::flush();
    for (auto const &file : m_dirty)
    {
        putJsonContents(file, false);
    }
    m_dirty.clear();
    return std::future<void>();
}

void JSONIOHandlerImpl::createFile(
    Writable *writable, Parameter<Operation::CREATE_FILE> const &parameters)
{
    VERIFY_ALWAYS(
        access::write(m_handler->m_backendAccess),
        "[JSON] Creating a file in read-only mode is not possible.");

    if (!writable->written)
    {
        std::string name = parameters.name + m_originalExtension;

        auto res_pair = getPossiblyExisting(name);
        auto fullPathToFile = fullPath(std::get<0>(res_pair));
        File shared_name = File(name);
        VERIFY_ALWAYS(
            !(m_handler->m_backendAccess == Access::READ_WRITE &&
              (!std::get<2>(res_pair) ||
               auxiliary::file_exists(fullPathToFile))),
            "[JSON] Can only overwrite existing file in CREATE mode.");

        if (!std::get<2>(res_pair))
        {
            auto file = std::get<0>(res_pair);
            m_dirty.erase(file);
            m_jsonVals.erase(file);
            file.invalidate();
        }

        std::string const &dir(m_handler->directory);
        if (!auxiliary::directory_exists(dir))
        {
            auto success = auxiliary::create_directories(dir);
            VERIFY(success, "[JSON] Could not create directory.");
        }

        associateWithFile(writable, shared_name);
        this->m_dirty.emplace(shared_name);

        if (m_handler->m_backendAccess != Access::APPEND ||
            !auxiliary::file_exists(fullPathToFile))
        {
            // if in create mode: make sure to overwrite
            // if in append mode and the file does not exist: create an empty
            // dataset
            this->m_jsonVals[shared_name] = std::make_shared<nlohmann::json>();
        }
        // else: the JSON value is not available in m_jsonVals and will be
        // read from the file later on before overwriting

        writable->written = true;
        writable->abstractFilePosition = std::make_shared<JSONFilePosition>();
    }
}

void JSONIOHandlerImpl::checkFile(
    Writable *, Parameter<Operation::CHECK_FILE> &parameters)
{
    std::string name = parameters.name;
    if (!auxiliary::ends_with(name, ".json"))
    {
        name += ".json";
    }
    name = fullPath(name);
    using FileExists = Parameter<Operation::CHECK_FILE>::FileExists;
    *parameters.fileExists =
        (auxiliary::file_exists(name) || auxiliary::directory_exists(name))
        ? FileExists::Yes
        : FileExists::No;
}

void JSONIOHandlerImpl::createPath(
    Writable *writable, Parameter<Operation::CREATE_PATH> const &parameter)
{
    std::string path = parameter.path;
    /* Sanitize:
     * The JSON API does not like to have slashes in the end.
     */
    if (auxiliary::ends_with(path, "/"))
    {
        path = auxiliary::replace_last(path, "/", "");
    }

    auto file = refreshFileFromParent(writable);

    auto *jsonVal = &*obtainJsonContents(file);
    if (!auxiliary::starts_with(path, "/"))
    { // path is relative
        auto filepos = setAndGetFilePosition(writable, false);

        jsonVal = &(*jsonVal)[filepos->id];
        ensurePath(jsonVal, path);
        path = filepos->id.to_string() + "/" + path;
    }
    else
    {

        ensurePath(jsonVal, path);
    }

    m_dirty.emplace(file);
    writable->written = true;
    writable->abstractFilePosition =
        std::make_shared<JSONFilePosition>(nlohmann::json::json_pointer(path));
}

void JSONIOHandlerImpl::createDataset(
    Writable *writable, Parameter<Operation::CREATE_DATASET> const &parameter)
{
    if (access::readOnly(m_handler->m_backendAccess))
    {
        throw std::runtime_error(
            "[JSON] Creating a dataset in a file opened as read only is not "
            "possible.");
    }
    if (parameter.joinedDimension.has_value())
    {
        error::throwOperationUnsupportedInBackend(
            "JSON", "Joined Arrays currently only supported in ADIOS2");
    }

    if (!writable->written)
    {
        /* Sanitize name */
        std::string name = removeSlashes(parameter.name);

        auto file = refreshFileFromParent(writable);
        writable->abstractFilePosition.reset();
        setAndGetFilePosition(writable);
        auto &jsonVal = obtainJsonContents(writable);
        // be sure to have a JSON object, not a list
        if (jsonVal.empty())
        {
            jsonVal = nlohmann::json::object();
        }
        setAndGetFilePosition(writable, name);
        auto &dset = jsonVal[name];
        dset["datatype"] = datatypeToString(parameter.dtype);
        auto extent = parameter.extent;
        switch (parameter.dtype)
        {
        case Datatype::CFLOAT:
        case Datatype::CDOUBLE:
        case Datatype::CLONG_DOUBLE: {
            extent.push_back(2);
            break;
        }
        default:
            break;
        }
        // TOML does not support nulls, so initialize with zero
        dset["data"] = initializeNDArray(
            extent,
            m_fileFormat == FileFormat::Json ? std::optional<Datatype>()
                                             : parameter.dtype);
        writable->written = true;
        m_dirty.emplace(file);
    }
}

namespace
{
    void mergeInto(nlohmann::json &into, nlohmann::json &from);
    void mergeInto(nlohmann::json &into, nlohmann::json &from)
    {
        if (!from.is_array())
        {
            into = from; // copy
        }
        else
        {
            size_t size = from.size();
            for (size_t i = 0; i < size; ++i)
            {
                if (!from[i].is_null())
                {
                    mergeInto(into[i], from[i]);
                }
            }
        }
    }
} // namespace

void JSONIOHandlerImpl::extendDataset(
    Writable *writable, Parameter<Operation::EXTEND_DATASET> const &parameters)
{
    VERIFY_ALWAYS(
        access::write(m_handler->m_backendAccess),
        "[JSON] Cannot extend a dataset in read-only mode.")
    setAndGetFilePosition(writable);
    refreshFileFromParent(writable);
    auto &j = obtainJsonContents(writable);

    try
    {
        auto datasetExtent = getExtent(j);
        VERIFY_ALWAYS(
            datasetExtent.size() == parameters.extent.size(),
            "[JSON] Cannot change dimensionality of a dataset")
        for (size_t currentdim = 0; currentdim < parameters.extent.size();
             currentdim++)
        {
            VERIFY_ALWAYS(
                datasetExtent[currentdim] <= parameters.extent[currentdim],
                "[JSON] Cannot shrink the extent of a dataset")
        }
    }
    catch (json::basic_json::type_error &)
    {
        throw std::runtime_error(
            "[JSON] The specified location contains no valid dataset");
    }
    auto extent = parameters.extent;
    auto datatype = stringToDatatype(j["datatype"].get<std::string>());
    switch (datatype)
    {
    case Datatype::CFLOAT:
    case Datatype::CDOUBLE:
    case Datatype::CLONG_DOUBLE: {
        extent.push_back(2);
        break;
    }
    default:
        // nothing to do
        break;
    }
    // TOML does not support nulls, so initialize with zero
    nlohmann::json newData = initializeNDArray(
        extent,
        m_fileFormat == FileFormat::Json ? std::optional<Datatype>()
                                         : datatype);
    nlohmann::json &oldData = j["data"];
    mergeInto(newData, oldData);
    j["data"] = newData;
    writable->written = true;
}

namespace
{
    // pre-declare since this one is recursive
    ChunkTable chunksInJSON(nlohmann::json const &);
    ChunkTable chunksInJSON(nlohmann::json const &j)
    {
        /*
         * Idea:
         * Iterate (n-1)-dimensional hyperslabs line by line and query
         * their chunks recursively.
         * If two or more successive (n-1)-dimensional slabs return the
         * same chunktable, they can be merged as one chunk.
         *
         * Notice that this approach is simple, relatively easily
         * implemented, but not ideal, since chunks that overlap in some
         * dimensions may be ripped apart:
         *
         *      0123
         *    0 ____
         *    1 ____
         *    2 **__
         *    3 **__
         *    4 **__
         *    5 **__
         *    6 **__
         *    7 **_*
         *    8 ___*
         *    9 ___*
         *
         * Since both of the drawn chunks overlap on line 7, this approach
         * will return 4 chunks:
         * offset - extent
         * (2,0)  - (4,2)
         * (7,0)  - (1,2)
         * (7,3)  - (1,1)
         * (8,3)  - (2,1)
         *
         * Hence, in a second phase, the mergeChunks function below will
         * merge things back up.
         */
        if (!j.is_array())
        {
            return ChunkTable{WrittenChunkInfo(Offset{}, Extent{})};
        }
        ChunkTable res;
        size_t it = 0;
        size_t end = j.size();
        while (it < end)
        {
            // skip empty slots
            while (it < end && j[it].is_null())
            {
                ++it;
            }
            if (it == end)
            {
                break;
            }
            // get chunking at current position
            // and additionally, number of successive rows with the same
            // recursive results
            size_t const offset = it;
            ChunkTable referenceTable = chunksInJSON(j[it]);
            ++it;
            for (; it < end; ++it)
            {
                if (j[it].is_null())
                {
                    break;
                }
                ChunkTable currentTable = chunksInJSON(j[it]);
                if (currentTable != referenceTable)
                {
                    break;
                }
            }
            size_t const extent = it - offset; // sic! no -1
            // now we know the number of successive rows with same rec.
            // results, let's extend these results to include dimension 0
            for (auto const &chunk : referenceTable)
            {
                Offset o = {offset};
                Extent e = {extent};
                for (auto entry : chunk.offset)
                {
                    o.push_back(entry);
                }
                for (auto entry : chunk.extent)
                {
                    e.push_back(entry);
                }
                res.emplace_back(std::move(o), std::move(e), chunk.sourceID);
            }
        }
        return res;
    }

    /*
     * Check whether two chunks can be merged to form a large one
     * and optionally return that larger chunk
     */
    std::optional<WrittenChunkInfo>
    mergeChunks(WrittenChunkInfo const &chunk1, WrittenChunkInfo const &chunk2)
    {
        /*
         * Idea:
         * If two chunks can be merged into one, they agree on offsets and
         * extents in all but exactly one dimension dim.
         * At dimension dim, the offset of chunk 2 is equal to the offset
         * of chunk 1 plus its extent -- or vice versa.
         */
        unsigned dimensionality = chunk1.extent.size();
        for (unsigned dim = 0; dim < dimensionality; ++dim)
        {
            WrittenChunkInfo const *c1(&chunk1), *c2(&chunk2);
            // check if one chunk is the extension of the other at
            // dimension dim
            // first, let's put things in order
            if (c1->offset[dim] > c2->offset[dim])
            {
                std::swap(c1, c2);
            }
            // now, c1 begins at the lower of both offsets
            // next check, that both chunks border one another exactly
            if (c2->offset[dim] != c1->offset[dim] + c1->extent[dim])
            {
                continue;
            }
            // we've got a candidate
            // verify that all other dimensions have equal values
            auto equalValues = [dimensionality, dim, c1, c2]() {
                for (unsigned j = 0; j < dimensionality; ++j)
                {
                    if (j == dim)
                    {
                        continue;
                    }
                    if (c1->offset[j] != c2->offset[j] ||
                        c1->extent[j] != c2->extent[j])
                    {
                        return false;
                    }
                }
                return true;
            };
            if (!equalValues())
            {
                continue;
            }
            // we can merge the chunks
            Offset offset(c1->offset);
            Extent extent(c1->extent);
            extent[dim] += c2->extent[dim];
            return std::make_optional(WrittenChunkInfo(offset, extent));
        }
        return std::optional<WrittenChunkInfo>();
    }

    /*
     * Merge chunks in the chunktable until no chunks are left that can be
     * merged.
     */
    void mergeChunks(ChunkTable &table)
    {
        bool stillChanging;
        do
        {
            stillChanging = false;
            auto innerLoops = [&table]() {
                /*
                 * Iterate over pairs of chunks in the table.
                 * When a pair that can be merged is found, merge it,
                 * delete the original two chunks from the table,
                 * put the new one in and return.
                 */
                for (auto i = table.begin(); i < table.end(); ++i)
                {
                    for (auto j = i + 1; j < table.end(); ++j)
                    {
                        std::optional<WrittenChunkInfo> merged =
                            mergeChunks(*i, *j);
                        if (merged)
                        {
                            // erase order is important due to iterator
                            // invalidation
                            table.erase(j);
                            table.erase(i);
                            table.emplace_back(std::move(merged.value()));
                            return true;
                        }
                    }
                }
                return false;
            };
            stillChanging = innerLoops();
        } while (stillChanging);
    }
} // namespace

void JSONIOHandlerImpl::availableChunks(
    Writable *writable, Parameter<Operation::AVAILABLE_CHUNKS> &parameters)
{
    refreshFileFromParent(writable);
    auto filePosition = setAndGetFilePosition(writable);
    auto &j = obtainJsonContents(writable)["data"];
    *parameters.chunks = chunksInJSON(j);
    mergeChunks(*parameters.chunks);
}

void JSONIOHandlerImpl::openFile(
    Writable *writable, Parameter<Operation::OPEN_FILE> &parameter)
{
    if (!auxiliary::directory_exists(m_handler->directory))
    {
        throw error::ReadError(
            error::AffectedObject::File,
            error::Reason::Inaccessible,
            "JSON",
            "Supplied directory is not valid: " + m_handler->directory);
    }

    std::string name = parameter.name + m_originalExtension;

    auto file = std::get<0>(getPossiblyExisting(name));

    associateWithFile(writable, file);

    writable->written = true;
    writable->abstractFilePosition = std::make_shared<JSONFilePosition>();
}

void JSONIOHandlerImpl::closeFile(
    Writable *writable, Parameter<Operation::CLOSE_FILE> const &)
{
    auto fileIterator = m_files.find(writable);
    if (fileIterator != m_files.end())
    {
        auto it = putJsonContents(fileIterator->second);
        if (it != m_jsonVals.end())
        {
            m_jsonVals.erase(it);
        }
        m_dirty.erase(fileIterator->second);
        // do not invalidate the file
        // it still exists, it is just not open
        m_files.erase(fileIterator);
    }
}

void JSONIOHandlerImpl::openPath(
    Writable *writable, Parameter<Operation::OPEN_PATH> const &parameters)
{
    auto file = refreshFileFromParent(writable);

    nlohmann::json *j = &obtainJsonContents(writable->parent);
    auto path = removeSlashes(parameters.path);
    path = path.empty() ? filepositionOf(writable->parent)
                        : filepositionOf(writable->parent) + "/" + path;

    if (writable->abstractFilePosition)
    {
        *setAndGetFilePosition(writable, false) =
            JSONFilePosition(json::json_pointer(path));
    }
    else
    {
        writable->abstractFilePosition =
            std::make_shared<JSONFilePosition>(json::json_pointer(path));
    }

    ensurePath(j, removeSlashes(parameters.path));

    writable->written = true;
}

void JSONIOHandlerImpl::openDataset(
    Writable *writable, Parameter<Operation::OPEN_DATASET> &parameters)
{
    refreshFileFromParent(writable);
    auto name = removeSlashes(parameters.name);
    auto &datasetJson = obtainJsonContents(writable->parent)[name];
    /*
     * If the dataset has been opened previously, the path needs not be
     * set again.
     */
    if (!writable->abstractFilePosition)
    {
        setAndGetFilePosition(writable, name);
    }

    *parameters.dtype =
        Datatype(stringToDatatype(datasetJson["datatype"].get<std::string>()));
    *parameters.extent = getExtent(datasetJson);
    writable->written = true;
}

void JSONIOHandlerImpl::deleteFile(
    Writable *writable, Parameter<Operation::DELETE_FILE> const &parameters)
{
    VERIFY_ALWAYS(
        access::write(m_handler->m_backendAccess),
        "[JSON] Cannot delete files in read-only mode")

    if (!writable->written)
    {
        return;
    }

    auto filename = auxiliary::ends_with(parameters.name, ".json")
        ? parameters.name
        : parameters.name + ".json";

    auto tuple = getPossiblyExisting(filename);
    if (!std::get<2>(tuple))
    {
        // file is already in the system
        auto file = std::get<0>(tuple);
        m_dirty.erase(file);
        m_jsonVals.erase(file);
        file.invalidate();
    }

    std::remove(fullPath(filename).c_str());

    writable->written = false;
}

void JSONIOHandlerImpl::deletePath(
    Writable *writable, Parameter<Operation::DELETE_PATH> const &parameters)
{
    VERIFY_ALWAYS(
        access::write(m_handler->m_backendAccess),
        "[JSON] Cannot delete paths in read-only mode")

    if (!writable->written)
    {
        return;
    }

    VERIFY_ALWAYS(
        !auxiliary::starts_with(parameters.path, '/'),
        "[JSON] Paths passed for deletion should be relative, the given path "
        "is absolute (starts with '/')")
    auto file = refreshFileFromParent(writable);
    auto filepos = setAndGetFilePosition(writable, false);
    auto path = removeSlashes(parameters.path);
    VERIFY(!path.empty(), "[JSON] No path passed for deletion.")
    nlohmann::json *j;
    if (path == ".")
    {
        auto s = filepos->id.to_string();
        if (s == "/")
        {
            throw std::runtime_error("[JSON] Cannot delete the root group");
        }

        auto i = s.rfind('/');
        path = s;
        path.replace(0, i + 1, "");
        // path should now be equal to the name of the current group
        // go up one group

        // go to parent directory
        // parent exists since we have verified that the current
        // directory is != root
        parentDir(s);
        j = &(*obtainJsonContents(file))[nlohmann::json::json_pointer(s)];
    }
    else
    {
        if (auxiliary::starts_with(path, "./"))
        {
            path = auxiliary::replace_first(path, "./", "");
        }
        j = &obtainJsonContents(writable);
    }
    nlohmann::json *lastPointer = j;
    bool needToDelete = true;
    auto splitPath = auxiliary::split(path, "/");
    // be careful not to create the group by accident
    // the loop will execute at least once
    for (auto const &folder : splitPath)
    {
        auto it = j->find(folder);
        if (it == j->end())
        {
            needToDelete = false;
            break;
        }
        else
        {
            lastPointer = j;
            j = &it.value();
        }
    }
    if (needToDelete)
    {
        lastPointer->erase(splitPath[splitPath.size() - 1]);
    }

    putJsonContents(file);
    writable->abstractFilePosition.reset();
    writable->written = false;
}

void JSONIOHandlerImpl::deleteDataset(
    Writable *writable, Parameter<Operation::DELETE_DATASET> const &parameters)
{
    VERIFY_ALWAYS(
        access::write(m_handler->m_backendAccess),
        "[JSON] Cannot delete datasets in read-only mode")

    if (!writable->written)
    {
        return;
    }

    auto filepos = setAndGetFilePosition(writable, false);

    auto file = refreshFileFromParent(writable);
    auto dataset = removeSlashes(parameters.name);
    nlohmann::json *parent;
    if (dataset == ".")
    {
        auto s = filepos->id.to_string();
        if (s.empty())
        {
            throw std::runtime_error(
                "[JSON] Invalid position for a dataset in the JSON file.");
        }
        dataset = s;
        auto i = dataset.rfind('/');
        dataset.replace(0, i + 1, "");

        parentDir(s);
        parent = &(*obtainJsonContents(file))[nlohmann::json::json_pointer(s)];
    }
    else
    {
        parent = &obtainJsonContents(writable);
    }
    parent->erase(dataset);
    putJsonContents(file);
    writable->written = false;
    writable->abstractFilePosition.reset();
}

void JSONIOHandlerImpl::deleteAttribute(
    Writable *writable, Parameter<Operation::DELETE_ATT> const &parameters)
{
    VERIFY_ALWAYS(
        access::write(m_handler->m_backendAccess),
        "[JSON] Cannot delete attributes in read-only mode")
    if (!writable->written)
    {
        return;
    }
    setAndGetFilePosition(writable);
    auto file = refreshFileFromParent(writable);
    auto &j = obtainJsonContents(writable);
    j.erase(parameters.name);
    putJsonContents(file);
}

void JSONIOHandlerImpl::writeDataset(
    Writable *writable, Parameter<Operation::WRITE_DATASET> &parameters)
{
    VERIFY_ALWAYS(
        access::write(m_handler->m_backendAccess),
        "[JSON] Cannot write data in read-only mode.");

    auto pos = setAndGetFilePosition(writable);
    auto file = refreshFileFromParent(writable);
    auto &j = obtainJsonContents(writable);

    verifyDataset(parameters, j);

    switchType<DatasetWriter>(parameters.dtype, j, parameters);

    writable->written = true;
    putJsonContents(file);
}

void JSONIOHandlerImpl::writeAttribute(
    Writable *writable, Parameter<Operation::WRITE_ATT> const &parameter)
{
    if (parameter.changesOverSteps ==
        Parameter<Operation::WRITE_ATT>::ChangesOverSteps::Yes)
    {
        // cannot do this
        return;
    }
    if (access::readOnly(m_handler->m_backendAccess))
    {
        throw std::runtime_error(
            "[JSON] Creating a dataset in a file opened as read only is not "
            "possible.");
    }

    /* Sanitize name */
    std::string name = removeSlashes(parameter.name);

    auto file = refreshFileFromParent(writable);
    auto jsonVal = obtainJsonContents(file);
    auto filePosition = setAndGetFilePosition(writable);
    if ((*jsonVal)[filePosition->id]["attributes"].empty())
    {
        (*jsonVal)[filePosition->id]["attributes"] = nlohmann::json::object();
    }
    nlohmann::json value;
    switchType<AttributeWriter>(parameter.dtype, value, parameter.resource);
    (*jsonVal)[filePosition->id]["attributes"][parameter.name] = {
        {"datatype", datatypeToString(parameter.dtype)}, {"value", value}};
    writable->written = true;
    m_dirty.emplace(file);
}

void JSONIOHandlerImpl::readDataset(
    Writable *writable, Parameter<Operation::READ_DATASET> &parameters)
{
    refreshFileFromParent(writable);
    setAndGetFilePosition(writable);
    auto &j = obtainJsonContents(writable);
    verifyDataset(parameters, j);

    try
    {
        switchType<DatasetReader>(parameters.dtype, j["data"], parameters);
    }
    catch (json::basic_json::type_error &)
    {
        throw std::runtime_error(
            "[JSON] The given path does not contain a valid dataset.");
    }
}

void JSONIOHandlerImpl::readAttribute(
    Writable *writable, Parameter<Operation::READ_ATT> &parameters)
{
    VERIFY_ALWAYS(
        writable->written,
        "[JSON] Attributes have to be written before reading.")
    refreshFileFromParent(writable);
    auto name = removeSlashes(parameters.name);
    auto const &jsonContents = obtainJsonContents(writable);
    auto const &jsonLoc = jsonContents["attributes"];
    setAndGetFilePosition(writable);
    std::string error_msg("[JSON] No such attribute '");
    if (!hasKey(jsonLoc, name))
    {
        throw error::ReadError(
            error::AffectedObject::Attribute,
            error::Reason::NotFound,
            "JSON",
            "Tried looking up attribute '" + name +
                "' in object: " + jsonLoc.dump());
    }
    auto &j = jsonLoc[name];
    try
    {
        *parameters.dtype =
            Datatype(stringToDatatype(j["datatype"].get<std::string>()));
        switchType<AttributeReader>(*parameters.dtype, j["value"], parameters);
    }
    catch (json::type_error &)
    {
        throw error::ReadError(
            error::AffectedObject::Attribute,
            error::Reason::UnexpectedContent,
            "JSON",
            "No properly formatted attribute with name '" + name +
                "' found in object: " + jsonLoc.dump());
    }
}

void JSONIOHandlerImpl::listPaths(
    Writable *writable, Parameter<Operation::LIST_PATHS> &parameters)
{
    VERIFY_ALWAYS(
        writable->written,
        "[JSON] Values have to be written before reading a directory");
    auto &j = obtainJsonContents(writable);
    setAndGetFilePosition(writable);
    refreshFileFromParent(writable);
    parameters.paths->clear();
    for (auto it = j.begin(); it != j.end(); it++)
    {
        if (isGroup(it))
        {
            parameters.paths->push_back(it.key());
        }
    }
}

void JSONIOHandlerImpl::listDatasets(
    Writable *writable, Parameter<Operation::LIST_DATASETS> &parameters)
{
    VERIFY_ALWAYS(
        writable->written, "[JSON] Datasets have to be written before reading.")
    refreshFileFromParent(writable);
    auto filePosition = setAndGetFilePosition(writable);
    auto &j = obtainJsonContents(writable);
    parameters.datasets->clear();
    for (auto it = j.begin(); it != j.end(); it++)
    {
        if (isDataset(it.value()))
        {
            parameters.datasets->push_back(it.key());
        }
    }
}

void JSONIOHandlerImpl::listAttributes(
    Writable *writable, Parameter<Operation::LIST_ATTS> &parameters)
{
    VERIFY_ALWAYS(
        writable->written,
        "[JSON] Attributes have to be written before reading.")
    refreshFileFromParent(writable);
    auto filePosition = setAndGetFilePosition(writable);
    auto const &jsonContents = obtainJsonContents(writable);
    if (!jsonContents.contains("attributes"))
    {
        return;
    }
    auto const &j = jsonContents["attributes"];
    for (auto it = j.begin(); it != j.end(); it++)
    {
        parameters.attributes->push_back(it.key());
    }
}

void JSONIOHandlerImpl::deregister(
    Writable *writable, Parameter<Operation::DEREGISTER> const &)
{
    m_files.erase(writable);
}

void JSONIOHandlerImpl::touch(
    Writable *writable, Parameter<Operation::TOUCH> const &)
{
    auto file = refreshFileFromParent(writable);
    m_dirty.emplace(std::move(file));
}

auto JSONIOHandlerImpl::getFilehandle(File const &fileName, Access access)
    -> std::tuple<std::unique_ptr<FILEHANDLE>, std::istream *, std::ostream *>
{
    VERIFY_ALWAYS(
        fileName.valid(),
        "[JSON] Tried opening a file that has been overwritten or deleted.")
    auto path = fullPath(fileName);
    auto fs = std::make_unique<FILEHANDLE>();
    std::istream *istream = nullptr;
    std::ostream *ostream = nullptr;
    if (access::write(access))
    {
        /*
         * Always truncate when writing, we alway write entire JSON
         * datasets, never partial ones.
         * Within the JSON backend, APPEND and READ_WRITE mode are
         * equivalent, but the openPMD frontend exposes no reading
         * functionality in APPEND mode.
         */
        std::ios_base::openmode openmode =
            std::ios_base::out | std::ios_base::trunc;
        if (m_fileFormat == FileFormat::Toml)
        {
            openmode |= std::ios_base::binary;
        }
        fs->open(path, openmode);
        ostream =
            &(*fs << std::setprecision(
                  std::numeric_limits<double>::digits10 + 1));
    }
    else
    {
        std::ios_base::openmode openmode = std::ios_base::in;
        if (m_fileFormat == FileFormat::Toml)
        {
            openmode |= std::ios_base::binary;
        }
        fs->open(path, openmode);
        istream =
            &(*fs >>
              std::setprecision(std::numeric_limits<double>::digits10 + 1));
    }
    VERIFY(fs->good(), "[JSON] Failed opening a file '" + path + "'");
    return std::make_tuple(std::move(fs), istream, ostream);
}

std::string JSONIOHandlerImpl::fullPath(File const &fileName)
{
    return fullPath(*fileName);
}

std::string JSONIOHandlerImpl::fullPath(std::string const &fileName)
{
    if (auxiliary::ends_with(m_handler->directory, "/"))
    {
        return m_handler->directory + fileName;
    }
    else
    {
        return m_handler->directory + "/" + fileName;
    }
}

void JSONIOHandlerImpl::parentDir(std::string &s)
{
    auto i = s.rfind('/');
    if (i != std::string::npos)
    {
        s.replace(i, s.size() - i, "");
        s.shrink_to_fit();
    }
}

std::string JSONIOHandlerImpl::filepositionOf(Writable *writable)
{
    return std::dynamic_pointer_cast<JSONFilePosition>(
               writable->abstractFilePosition)
        ->id.to_string();
}

template <typename T, typename Visitor>
void JSONIOHandlerImpl::syncMultidimensionalJson(
    nlohmann::json &j,
    Offset const &offset,
    Extent const &extent,
    Extent const &multiplicator,
    Visitor visitor,
    T *data,
    size_t currentdim)
{
    // Offset only relevant for JSON, the array data is contiguous
    auto off = offset[currentdim];
    // maybe rewrite iteratively, using a stack that stores for each level the
    // current iteration value i

    if (currentdim == offset.size() - 1)
    {
        for (std::size_t i = 0; i < extent[currentdim]; ++i)
        {
            visitor(j[i + off], data[i]);
        }
    }
    else
    {
        for (std::size_t i = 0; i < extent[currentdim]; ++i)
        {
            syncMultidimensionalJson<T, Visitor>(
                j[i + off],
                offset,
                extent,
                multiplicator,
                visitor,
                data + i * multiplicator[currentdim],
                currentdim + 1);
        }
    }
}

// multiplicators: an array [m_0,...,m_n] s.t.
// data[i_0]...[i_n] = data[m_0*i_0+...+m_n*i_n]
// (m_n = 1)
Extent JSONIOHandlerImpl::getMultiplicators(Extent const &extent)
{
    Extent res(extent);
    Extent::value_type n = 1;
    size_t i = extent.size();
    do
    {
        --i;
        res[i] = n;
        n *= extent[i];
    } while (i > 0);
    return res;
}

Extent JSONIOHandlerImpl::getExtent(nlohmann::json &j)
{
    Extent res;
    nlohmann::json *ptr = &j["data"];
    while (ptr->is_array())
    {
        res.push_back(ptr->size());
        ptr = &(*ptr)[0];
    }
    switch (stringToDatatype(j["datatype"].get<std::string>()))
    {
    case Datatype::CFLOAT:
    case Datatype::CDOUBLE:
    case Datatype::CLONG_DOUBLE:
        // the last "dimension" is only the two entries for the complex
        // number, so remove that again
        res.erase(res.end() - 1);
        break;
    default:
        break;
    }
    return res;
}

std::string JSONIOHandlerImpl::removeSlashes(std::string s)
{
    if (auxiliary::starts_with(s, '/'))
    {
        s = auxiliary::replace_first(s, "/", "");
    }
    if (auxiliary::ends_with(s, '/'))
    {
        s = auxiliary::replace_last(s, "/", "");
    }
    return s;
}

template <typename KeyT>
bool JSONIOHandlerImpl::hasKey(nlohmann::json const &j, KeyT &&key)
{
    return j.find(std::forward<KeyT>(key)) != j.end();
}

void JSONIOHandlerImpl::ensurePath(
    nlohmann::json *jsonp, std::string const &path)
{
    auto groups = auxiliary::split(path, "/");
    for (std::string &group : groups)
    {
        // Enforce a JSON object
        // the library will automatically create a list if the first
        // key added to it is parseable as an int
        jsonp = &(*jsonp)[group];
        if (jsonp->is_null())
        {
            *jsonp = nlohmann::json::object();
        }
    }
}

std::tuple<File, std::unordered_map<Writable *, File>::iterator, bool>
JSONIOHandlerImpl::getPossiblyExisting(std::string const &file)
{

    auto it = std::find_if(
        m_files.begin(),
        m_files.end(),
        [file](std::unordered_map<Writable *, File>::value_type const &entry) {
            return *entry.second == file && entry.second.valid();
        });

    bool newlyCreated;
    File name;
    if (it == m_files.end())
    {
        name = file;
        newlyCreated = true;
    }
    else
    {
        name = it->second;
        newlyCreated = false;
    }
    return std::
        tuple<File, std::unordered_map<Writable *, File>::iterator, bool>(
            std::move(name), it, newlyCreated);
}

std::shared_ptr<nlohmann::json>
JSONIOHandlerImpl::obtainJsonContents(File const &file)
{
    VERIFY_ALWAYS(
        file.valid(),
        "[JSON] File has been overwritten or deleted before reading");
    auto it = m_jsonVals.find(file);
    if (it != m_jsonVals.end())
    {
        return it->second;
    }
    // read from file
    auto serialImplementation = [&file, this]() {
        auto [fh, fh_with_precision, _] =
            getFilehandle(file, Access::READ_ONLY);
        (void)_;
        std::shared_ptr<nlohmann::json> res =
            std::make_shared<nlohmann::json>();
        switch (m_fileFormat)
        {
        case FileFormat::Json:
            *fh_with_precision >> *res;
            break;
        case FileFormat::Toml:
            *res = openPMD::json::tomlToJson(
                toml::parse(*fh_with_precision, *file));
            break;
        }
        VERIFY(fh->good(), "[JSON] Failed reading from a file.");
        return res;
    };
#if openPMD_HAVE_MPI
    auto parallelImplementation = [&file, this](MPI_Comm comm) {
        auto path = fullPath(*file);
        std::string collectivelyReadRawData =
            auxiliary::collective_file_read(path, comm);
        std::shared_ptr<nlohmann::json> res =
            std::make_shared<nlohmann::json>();
        switch (m_fileFormat)
        {
        case FileFormat::Json:
            *res = nlohmann::json::parse(collectivelyReadRawData);
            break;
        case FileFormat::Toml:
            std::istringstream istream(
                collectivelyReadRawData.c_str(),
                std::ios_base::binary | std::ios_base::in);
            auto as_toml = toml::parse(
                istream >> std::setprecision(
                               std::numeric_limits<double>::digits10 + 1),
                *file);
            *res = openPMD::json::tomlToJson(as_toml);
            break;
        }
        return res;
    };
    std::shared_ptr<nlohmann::json> res;
    if (m_communicator.has_value())
    {
        res = parallelImplementation(m_communicator.value());
    }
    else
    {
        res = serialImplementation();
    }

#else
    auto res = serialImplementation();
#endif

    m_jsonVals.emplace(file, res);
    return res;
}

nlohmann::json &JSONIOHandlerImpl::obtainJsonContents(Writable *writable)
{
    auto file = refreshFileFromParent(writable);
    auto filePosition = setAndGetFilePosition(writable, false);
    return (*obtainJsonContents(file))[filePosition->id];
}

auto JSONIOHandlerImpl::putJsonContents(
    File const &filename,
    bool unsetDirty // = true
    ) -> decltype(m_jsonVals)::iterator
{
    VERIFY_ALWAYS(
        filename.valid(),
        "[JSON] File has been overwritten/deleted before writing");
    auto it = m_jsonVals.find(filename);
    if (it == m_jsonVals.end())
    {
        return it;
    }

    (*it->second)["platform_byte_widths"] = platformSpecifics();

    auto writeSingleFile = [this, &it](std::string const &writeThisFile) {
        auto [fh, _, fh_with_precision] =
            getFilehandle(File(writeThisFile), Access::CREATE);
        (void)_;

        switch (m_fileFormat)
        {
        case FileFormat::Json:
            *fh_with_precision << *it->second << std::endl;
            break;
        case FileFormat::Toml:
            *fh_with_precision << openPMD::json::format_toml(
                                      openPMD::json::jsonToToml(*it->second))
                               << std::endl;
            break;
        }

        VERIFY(fh->good(), "[JSON] Failed writing data to disk.")
    };

    auto serialImplementation = [&filename, &writeSingleFile]() {
        writeSingleFile(*filename);
    };

#if openPMD_HAVE_MPI
    auto num_digits = [](unsigned n) -> unsigned {
        constexpr auto max = std::numeric_limits<unsigned>::max();
        unsigned base_10 = 1;
        unsigned res = 1;
        while (base_10 < max)
        {
            base_10 *= 10;
            if (n / base_10 == 0)
            {
                return res;
            }
            ++res;
        }
        return res;
    };

    auto parallelImplementation =
        [this, &filename, &writeSingleFile, &num_digits](MPI_Comm comm) {
            auto path = fullPath(*filename);
            auto dirpath = path + ".parallel";
            if (!auxiliary::create_directories(dirpath))
            {
                throw std::runtime_error(
                    "Failed creating directory '" + dirpath +
                    "' for parallel JSON output");
            }
            int rank = 0, size = 0;
            MPI_Comm_rank(comm, &rank);
            MPI_Comm_size(comm, &size);
            std::stringstream subfilePath;
            // writeSingleFile will prepend the base dir
            subfilePath << *filename << ".parallel/mpi_rank_"
                        << std::setw(num_digits(size - 1)) << std::setfill('0')
                        << rank << [&]() {
                               switch (m_fileFormat)
                               {
                               case FileFormat::Json:
                                   return ".json";
                               case FileFormat::Toml:
                                   return ".toml";
                               }
                               throw std::runtime_error("Unreachable!");
                           }();
            writeSingleFile(subfilePath.str());
            if (rank == 0)
            {
                constexpr char const *readme_msg = R"(
This folder has been created by a parallel instance of the JSON backend in
openPMD. There is one JSON file for each parallel writer MPI rank.
The parallel JSON backend performs no metadata or data aggregation at all.

This functionality is intended mainly for debugging and prototyping workflows.
There is no support in the openPMD-api for reading this folder as a single
dataset. For reading purposes, either pick a single .json file and read that, or
merge the .json files somehow (no tooling provided for this (yet)).
)";
                std::fstream readme_file;
                readme_file.open(
                    dirpath + "/README.txt",
                    std::ios_base::out | std::ios_base::trunc);
                readme_file << readme_msg + 1;
                readme_file.close();
                if (!readme_file.good() &&
                    !filename.fileState->printedReadmeWarningAlready)
                {
                    std::cerr
                        << "[Warning] Something went wrong in trying to create "
                           "README file at '"
                        << dirpath
                        << "/README.txt'. Will ignore and continue. The README "
                           "message would have been:\n----------\n"
                        << readme_msg + 1 << "----------" << std::endl;
                    filename.fileState->printedReadmeWarningAlready = true;
                }
            }
        };

    std::shared_ptr<nlohmann::json> res;
    if (m_communicator.has_value())
    {
        parallelImplementation(m_communicator.value());
    }
    else
    {
        serialImplementation();
    }

#else
    serialImplementation();
#endif
    if (unsetDirty)
    {
        m_dirty.erase(filename);
    }
    return it;
}

std::shared_ptr<JSONFilePosition> JSONIOHandlerImpl::setAndGetFilePosition(
    Writable *writable, std::string const &extend)
{
    std::string path;
    if (writable->abstractFilePosition)
    {
        // do NOT reuse the old pointer, we want to change the file position
        // only for the writable!
        path = filepositionOf(writable) + "/" + extend;
    }
    else if (writable->parent)
    {
        path = filepositionOf(writable->parent) + "/" + extend;
    }
    else
    { // we are root
        path = extend;
        if (!auxiliary::starts_with(path, "/"))
        {
            path = "/" + path;
        }
    }
    auto res = std::make_shared<JSONFilePosition>(json::json_pointer(path));

    writable->abstractFilePosition = res;

    return res;
}

std::shared_ptr<JSONFilePosition>
JSONIOHandlerImpl::setAndGetFilePosition(Writable *writable, bool write)
{
    std::shared_ptr<AbstractFilePosition> res;

    if (writable->abstractFilePosition)
    {
        res = writable->abstractFilePosition;
    }
    else if (writable->parent)
    {
        res = writable->parent->abstractFilePosition;
    }
    else
    { // we are root
        res = std::make_shared<JSONFilePosition>();
    }
    if (write)
    {
        writable->abstractFilePosition = res;
    }
    return std::dynamic_pointer_cast<JSONFilePosition>(res);
}

File JSONIOHandlerImpl::refreshFileFromParent(Writable *writable)
{
    if (writable->parent)
    {
        auto file = m_files.find(writable->parent)->second;
        associateWithFile(writable, file);
        return file;
    }
    else
    {
        return m_files.find(writable)->second;
    }
}

void JSONIOHandlerImpl::associateWithFile(Writable *writable, File file)
{
    // make sure to overwrite
    m_files[writable] = std::move(file);
}

bool JSONIOHandlerImpl::isDataset(nlohmann::json const &j)
{
    if (!j.is_object())
    {
        return false;
    }
    auto i = j.find("data");
    return i != j.end() && i.value().is_array();
}

bool JSONIOHandlerImpl::isGroup(nlohmann::json::const_iterator const &it)
{
    auto &j = it.value();
    if (it.key() == "attributes" || it.key() == "platform_byte_widths" ||
        !j.is_object())
    {
        return false;
    }
    auto i = j.find("data");
    return i == j.end() || !i.value().is_array();
}

template <typename Param>
void JSONIOHandlerImpl::verifyDataset(
    Param const &parameters, nlohmann::json &j)
{
    VERIFY_ALWAYS(
        isDataset(j),
        "[JSON] Specified dataset does not exist or is not a dataset.");

    try
    {
        auto datasetExtent = getExtent(j);
        VERIFY_ALWAYS(
            datasetExtent.size() == parameters.extent.size(),
            "[JSON] Read/Write request does not fit the dataset's dimension");
        for (unsigned int dimension = 0; dimension < parameters.extent.size();
             dimension++)
        {
            VERIFY_ALWAYS(
                parameters.offset[dimension] + parameters.extent[dimension] <=
                    datasetExtent[dimension],
                "[JSON] Read/Write request exceeds the dataset's size");
        }
        Datatype dt = stringToDatatype(j["datatype"].get<std::string>());
        VERIFY_ALWAYS(
            dt == parameters.dtype,
            "[JSON] Read/Write request does not fit the dataset's type");
    }
    catch (json::basic_json::type_error &)
    {
        throw std::runtime_error(
            "[JSON] The given path does not contain a valid dataset.");
    }
}

nlohmann::json JSONIOHandlerImpl::platformSpecifics()
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
        Datatype::CFLOAT,
        Datatype::CDOUBLE,
        Datatype::CLONG_DOUBLE,
        Datatype::BOOL};
    for (auto it = std::begin(datatypes); it != std::end(datatypes); it++)
    {
        res[datatypeToString(*it)] = toBytes(*it);
    }
    return res;
}

template <typename T>
void JSONIOHandlerImpl::DatasetWriter::call(
    nlohmann::json &json, const Parameter<Operation::WRITE_DATASET> &parameters)
{
    CppToJSON<T> ctj;
    syncMultidimensionalJson(
        json["data"],
        parameters.offset,
        parameters.extent,
        getMultiplicators(parameters.extent),
        [&ctj](nlohmann::json &j, T const &data) { j = ctj(data); },
        static_cast<T const *>(parameters.data.get()));
}

template <typename T>
void JSONIOHandlerImpl::DatasetReader::call(
    nlohmann::json &json, Parameter<Operation::READ_DATASET> &parameters)
{
    JsonToCpp<T> jtc;
    syncMultidimensionalJson(
        json,
        parameters.offset,
        parameters.extent,
        getMultiplicators(parameters.extent),
        [&jtc](nlohmann::json &j, T &data) { data = jtc(j); },
        static_cast<T *>(parameters.data.get()));
}

template <typename T>
void JSONIOHandlerImpl::AttributeWriter::call(
    nlohmann::json &value, Attribute::resource const &resource)
{
    CppToJSON<T> ctj;
    value = ctj(std::get<T>(resource));
}

template <typename T>
void JSONIOHandlerImpl::AttributeReader::call(
    nlohmann::json const &json, Parameter<Operation::READ_ATT> &parameters)
{
    JsonToCpp<T> jtc;
    *parameters.resource = jtc(json);
}

template <typename T>
nlohmann::json JSONIOHandlerImpl::CppToJSON<T>::operator()(const T &val)
{
    return nlohmann::json(val);
}

template <typename T>
nlohmann::json JSONIOHandlerImpl::CppToJSON<std::vector<T> >::operator()(
    const std::vector<T> &v)
{
    nlohmann::json j;
    CppToJSON<T> ctj;
    for (auto const &a : v)
    {
        j.emplace_back(ctj(a));
    }
    return j;
}

template <typename T, int n>
nlohmann::json JSONIOHandlerImpl::CppToJSON<std::array<T, n> >::operator()(
    const std::array<T, n> &v)
{
    nlohmann::json j;
    CppToJSON<T> ctj;
    for (auto const &a : v)
    {
        j.emplace_back(ctj(a));
    }
    return j;
}

template <typename T, typename Dummy>
T JSONIOHandlerImpl::JsonToCpp<T, Dummy>::operator()(nlohmann::json const &json)
{
    return json.get<T>();
}

template <typename T>
std::vector<T> JSONIOHandlerImpl::JsonToCpp<std::vector<T> >::operator()(
    nlohmann::json const &json)
{
    std::vector<T> v;
    JsonToCpp<T> jtp;
    for (auto const &j : json)
    {
        v.emplace_back(jtp(j));
    }
    return v;
}

template <typename T, int n>
std::array<T, n> JSONIOHandlerImpl::JsonToCpp<std::array<T, n> >::operator()(
    nlohmann::json const &json)
{
    std::array<T, n> a;
    JsonToCpp<T> jtp;
    size_t i = 0;
    for (auto const &j : json)
    {
        a[i] = jtp(j);
        i++;
    }
    return a;
}

template <typename T>
T JSONIOHandlerImpl::JsonToCpp<
    T,
    typename std::enable_if<std::is_floating_point<T>::value>::type>::
operator()(nlohmann::json const &j)
{
    try
    {
        return j.get<T>();
    }
    catch (...)
    {
        return std::numeric_limits<T>::quiet_NaN();
    }
}

} // namespace openPMD
