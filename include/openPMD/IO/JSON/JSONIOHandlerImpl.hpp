/* Copyright 2017-2025 Franz Poeschel, Axel Huebl, Luca Fedeli
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

#pragma once

#include "openPMD/IO/ADIOS/ADIOS2File.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/AbstractIOHandlerImpl.hpp"
#include "openPMD/IO/AbstractIOHandlerImplCommon.hpp"
#include "openPMD/IO/Access.hpp"
#include "openPMD/IO/InvalidatableFile.hpp"
#include "openPMD/IO/JSON/JSONFilePosition.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/auxiliary/JSON_internal.hpp"
#include "openPMD/backend/Variant_internal.hpp"
#include "openPMD/config.hpp"

#include <istream>
#include <nlohmann/json.hpp>
#if openPMD_HAVE_MPI
#include <mpi.h>
#endif

#include <complex>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace openPMD
{

// std::complex handling
template <class T>
void to_json(nlohmann::json &j, const std::complex<T> &p)
{
    j = nlohmann::json{p.real(), p.imag()};
}

template <class T>
void from_json(const nlohmann::json &j, std::complex<T> &p)
{
    p.real(j.at(0));
    p.imag(j.at(1));
}
} // namespace openPMD

namespace openPMD
{

class JSONIOHandlerImpl
    : public AbstractIOHandlerImplCommon<JSONIOHandlerImpl, JSONFilePosition>
{
    using json = nlohmann::json;
    template <typename, typename>
    friend class AbstractIOHandlerImplCommon;

public:
    struct BackendSpecificFileState
    {
        nlohmann::json data;
        bool printedReadmeWarningAlready = false;

        BackendSpecificFileState(nlohmann::json);

        operator nlohmann::json &();
        operator nlohmann::json const &() const;
    };

    using FilePositionType = JSONFilePosition;

    enum class FileFormat
    {
        Json,
        Toml
    };

    explicit JSONIOHandlerImpl(
        AbstractIOHandler *, FileFormat, std::string originalExtension);

#if openPMD_HAVE_MPI
    JSONIOHandlerImpl(
        AbstractIOHandler *,
        MPI_Comm,
        FileFormat,
        std::string originalExtension);
#endif

    void init(openPMD::json::TracingJSON config);

    ~JSONIOHandlerImpl() override;

    void
    createFile(Writable *, Parameter<Operation::CREATE_FILE> const &) override;

    void checkFile(Writable *, Parameter<Operation::CHECK_FILE> &) override;

    void
    createPath(Writable *, Parameter<Operation::CREATE_PATH> const &) override;

    void createDataset(
        Writable *, Parameter<Operation::CREATE_DATASET> const &) override;

    void extendDataset(
        Writable *, Parameter<Operation::EXTEND_DATASET> const &) override;

    void availableChunks(
        Writable *, Parameter<Operation::AVAILABLE_CHUNKS> &) override;

    void openFile(Writable *, Parameter<Operation::OPEN_FILE> &) override;

    void
    closeFile(Writable *, Parameter<Operation::CLOSE_FILE> const &) override;

    void openPath(Writable *, Parameter<Operation::OPEN_PATH> const &) override;

    void openDataset(Writable *, Parameter<Operation::OPEN_DATASET> &) override;

    void
    deleteFile(Writable *, Parameter<Operation::DELETE_FILE> const &) override;

    void
    deletePath(Writable *, Parameter<Operation::DELETE_PATH> const &) override;

    void deleteDataset(
        Writable *, Parameter<Operation::DELETE_DATASET> const &) override;

    void deleteAttribute(
        Writable *, Parameter<Operation::DELETE_ATT> const &) override;

    void
    writeDataset(Writable *, Parameter<Operation::WRITE_DATASET> &) override;

    void writeAttribute(
        Writable *, Parameter<Operation::WRITE_ATT> const &) override;

    void readDataset(Writable *, Parameter<Operation::READ_DATASET> &) override;

    void readAttribute(Writable *, Parameter<Operation::READ_ATT> &) override;

    void listPaths(Writable *, Parameter<Operation::LIST_PATHS> &) override;

    void
    listDatasets(Writable *, Parameter<Operation::LIST_DATASETS> &) override;

    void listAttributes(Writable *, Parameter<Operation::LIST_ATTS> &) override;

    void
    deregister(Writable *, Parameter<Operation::DEREGISTER> const &) override;

    void touch(Writable *, Parameter<Operation::TOUCH> const &) override;

    std::future<void> flush(internal::ParsedFlushParams &params);

private:
#if openPMD_HAVE_MPI
    std::optional<MPI_Comm> m_communicator;
#endif

    using FILEHANDLE = std::fstream;

    /*
     * Is set by constructor.
     */
    FileFormat m_fileFormat{};

    /*
     * Under which key do we find the backend configuration?
     * -> "json" for the JSON backend, "toml" for the TOML backend.
     */
    std::string backendConfigKey() const;

    /*
     * First return value: The location of the JSON value (either "json" or
     * "toml") Second return value: The value that was maybe found at this place
     */
    std::pair<std::string, std::optional<openPMD::json::TracingJSON>>
    getBackendConfig(openPMD::json::TracingJSON &) const;

    std::string m_originalExtension;

    /*
     * Was the config value explicitly user-chosen, or are we still working with
     * defaults?
     */
    enum class SpecificationVia
    {
        DefaultValue,
        Manually
    };

    /////////////////////
    // Dataset IO mode //
    /////////////////////

    enum class DatasetMode
    {
        Dataset,
        Template
    };

    // IOMode m_mode{};
    // SpecificationVia m_IOModeSpecificationVia =
    // SpecificationVia::DefaultValue; bool m_printedSkippedWriteWarningAlready
    // = false;

    struct DatasetMode_s
    {
        // Initialized in init()
        DatasetMode m_mode{};
        SpecificationVia m_specificationVia = SpecificationVia::DefaultValue;
        bool m_skipWarnings = false;

        template <typename A, typename B, typename C>
        operator std::tuple<A, B, C>()
        {
            return std::tuple<A, B, C>{
                m_mode, m_specificationVia, m_skipWarnings};
        }
    };
    DatasetMode_s m_datasetMode;
    DatasetMode_s retrieveDatasetMode(openPMD::json::TracingJSON &config) const;

    ///////////////////////
    // Attribute IO mode //
    ///////////////////////

    enum class AttributeMode
    {
        Short,
        Long
    };

    struct AttributeMode_s
    {
        // Will be modified in init() based on the openPMD version and the
        // active file format (JSON/TOML)
        AttributeMode m_mode{};
        SpecificationVia m_specificationVia = SpecificationVia::DefaultValue;
    };
    AttributeMode_s m_attributeMode;

    AttributeMode_s
    retrieveAttributeMode(openPMD::json::TracingJSON &config) const;

    // HELPER FUNCTIONS

    // will use the IOHandler to retrieve the correct directory.
    // first tuple element will be the underlying opened file handle.
    // if Access is read mode, then the second tuple element will be the istream
    // casted to precision std::numeric_limits<double>::digits10 + 1, else null.
    // if Access is write mode, then the second tuple element will be the
    // ostream casted to precision std::numeric_limits<double>::digits10 + 1,
    // else null. first tuple element needs to be a pointer, since the casted
    // streams are references only.
    std::tuple<std::unique_ptr<FILEHANDLE>, std::istream *, std::ostream *>
    getFilehandle(internal::FileState const &, Access access);

    // full operating system path of the given file
    std::string fullPath(internal::FileState const &);

    std::string fullPath(std::string const &);

    // from a path specification /a/b/c, remove the last
    // "folder" (i.e. modify the string to equal /a/b)
    static void parentDir(std::string &);

    // Fileposition is assumed to have already been set,
    // get it in string form
    std::string filePositionToString(JSONFilePosition const &);

    std::shared_ptr<JSONFilePosition>
    extendFilePosition(JSONFilePosition const &, std::string);

    // Execute visitor on each pair of positions in the json value
    // and the flattened multidimensional array.
    // Used for writing from the data to JSON and for reading back into
    // the array from JSON
    template <typename T, typename Visitor>
    static void syncMultidimensionalJson(
        nlohmann::json &j,
        Offset const &offset,
        Extent const &extent,
        Extent const &multiplicator,
        Visitor visitor,
        T *data,
        size_t currentdim = 0);

    // multiplicators: an array [m_0,...,m_n] s.t.
    // data[i_0]...[i_n] = data[m_0*i_0+...+m_n*i_n]
    // (m_n = 1)
    // essentially: m_i = \prod_{j=0}^{i-1} extent_j
    static Extent getMultiplicators(Extent const &extent);

    static std::pair<Extent, DatasetMode> getExtent(nlohmann::json &j);

    // remove single '/' in the beginning and end of a string
    static std::string removeSlashes(std::string);

    template <typename KeyT>
    static bool hasKey(nlohmann::json const &, KeyT &&key);

    // make sure that the given path exists in proper form in
    // the passed json value
    static void ensurePath(nlohmann::json *json, std::string const &path);

    // get the json value representing the whole file, possibly reading
    // from disk
    nlohmann::json &obtainJsonContents(internal::FileState &);

    // get the json value at the writable's fileposition
    nlohmann::json &obtainJsonContents(Writable *writable);

    // write to disk the json contents associated with the file
    // remove from m_dirty if unsetDirty == true
    void putJsonContents(internal::FileState &);

    // need to check the name too in order to exclude "attributes" key
    static bool isGroup(nlohmann::json::const_iterator const &it);

    static bool isDataset(nlohmann::json const &j);

    // check whether the json reference contains a valid dataset
    template <typename Param>
    DatasetMode verifyDataset(Param const &parameters, nlohmann::json &);

    static nlohmann::json platformSpecifics();

    struct DatasetWriter
    {
        template <typename T>
        static void call(
            nlohmann::json &json,
            const Parameter<Operation::WRITE_DATASET> &parameters);

        static constexpr char const *errorMsg = "JSON: writeDataset";
    };

    struct DatasetReader
    {
        template <typename T>
        static void call(
            nlohmann::json &json,
            Parameter<Operation::READ_DATASET> &parameters);

        static constexpr char const *errorMsg = "JSON: readDataset";
    };

    struct AttributeWriter
    {
        template <typename T>
        static void call(nlohmann::json &, attribute_types const &);

        static constexpr char const *errorMsg = "JSON: writeAttribute";
    };

    struct AttributeReader
    {
        template <typename T>
        static void
        call(nlohmann::json const &, Parameter<Operation::READ_ATT> &);

        static constexpr char const *errorMsg = "JSON: writeAttribute";
    };

    template <typename T>
    struct CppToJSON
    {
        nlohmann::json operator()(T const &);
    };

    template <typename T>
    struct CppToJSON<std::complex<T>>
    {
        nlohmann::json operator()(std::complex<T> const &);
    };

    template <typename T>
    struct CppToJSON<std::vector<T>>
    {
        nlohmann::json operator()(std::vector<T> const &);
    };

    template <typename T, int n>
    struct CppToJSON<std::array<T, n>>
    {
        nlohmann::json operator()(std::array<T, n> const &);
    };

    template <typename T, typename Enable = T>
    struct JsonToCpp
    {
        T operator()(nlohmann::json const &);
    };

    template <typename T>
    struct JsonToCpp<std::complex<T>>
    {
        std::complex<T> operator()(nlohmann::json const &);
    };

    template <typename T>
    struct JsonToCpp<std::vector<T>>
    {
        std::vector<T> operator()(nlohmann::json const &);
    };

    template <typename T, int n>
    struct JsonToCpp<std::array<T, n>>
    {
        std::array<T, n> operator()(nlohmann::json const &);
    };

    template <typename T>
    struct JsonToCpp<T, typename std::enable_if_t<std::is_floating_point_v<T>>>
    {
        T operator()(nlohmann::json const &);
    };
};

} // namespace openPMD
