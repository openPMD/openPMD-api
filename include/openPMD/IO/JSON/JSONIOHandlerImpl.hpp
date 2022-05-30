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

#pragma once

#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/AbstractIOHandlerImpl.hpp"
#include "openPMD/IO/Access.hpp"
#include "openPMD/IO/JSON/JSONFilePosition.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/config.hpp"

#include <nlohmann/json.hpp>

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
// Wrapper around a shared pointer to:
// * a filename
// * and a boolean indicating whether the file still exists
// The wrapper adds no extra information, but some commodity functions.
// Invariant for JSONIOHandlerImpl:
// For any valid filename, there is at any time at most one
// such shared pointer (wrapper) in the HandlerImpl's data structures
// (counting by pointer equality)
// This means, that a file can be invalidated (i.e. deleted or overwritten)
// by simply searching for one instance of the file e.g. in m_files and
// invalidating this instance
// A new instance may hence only be created after making sure that there are
// no valid instances in the data structures.
struct File
{
    explicit File(std::string s) : fileState{std::make_shared<FileState>(s)}
    {}

    File() = default;

    struct FileState
    {
        explicit FileState(std::string s) : name{std::move(s)}
        {}

        std::string name;
        bool valid = true;
    };

    std::shared_ptr<FileState> fileState;

    void invalidate()
    {
        fileState->valid = false;
    }

    bool valid() const
    {
        return fileState->valid;
    }

    File &operator=(std::string s)
    {
        if (fileState)
        {
            fileState->name = s;
        }
        else
        {
            fileState = std::make_shared<FileState>(s);
        }
        return *this;
    }

    bool operator==(File const &f) const
    {
        return this->fileState == f.fileState;
    }

    std::string &operator*() const
    {
        return fileState->name;
    }

    std::string *operator->() const
    {
        return &fileState->name;
    }

    explicit operator bool() const
    {
        return fileState.operator bool();
    }
};
} // namespace openPMD

namespace std
{
template <>
struct hash<openPMD::File>
{
    typedef openPMD::File argument_type;
    typedef std::size_t result_type;

    result_type operator()(argument_type const &s) const noexcept
    {
        return std::hash<shared_ptr<openPMD::File::FileState>>{}(s.fileState);
    }
};

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
} // namespace std

namespace openPMD
{
class JSONIOHandlerImpl : public AbstractIOHandlerImpl
{
    using json = nlohmann::json;

public:
    explicit JSONIOHandlerImpl(AbstractIOHandler *);

    ~JSONIOHandlerImpl() override;

    void
    createFile(Writable *, Parameter<Operation::CREATE_FILE> const &) override;

    void
    createPath(Writable *, Parameter<Operation::CREATE_PATH> const &) override;

    void createDataset(
        Writable *, Parameter<Operation::CREATE_DATASET> const &) override;

    void extendDataset(
        Writable *, Parameter<Operation::EXTEND_DATASET> const &) override;

    void availableChunks(
        Writable *, Parameter<Operation::AVAILABLE_CHUNKS> &) override;

    void openFile(Writable *, Parameter<Operation::OPEN_FILE> const &) override;

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

    void writeDataset(
        Writable *, Parameter<Operation::WRITE_DATASET> const &) override;

    void writeAttribute(
        Writable *, Parameter<Operation::WRITE_ATT> const &) override;

    void readDataset(Writable *, Parameter<Operation::READ_DATASET> &) override;

    void readAttribute(Writable *, Parameter<Operation::READ_ATT> &) override;

    void listPaths(Writable *, Parameter<Operation::LIST_PATHS> &) override;

    void
    listDatasets(Writable *, Parameter<Operation::LIST_DATASETS> &) override;

    void listAttributes(Writable *, Parameter<Operation::LIST_ATTS> &) override;

    std::future<void> flush();

private:
    using FILEHANDLE = std::fstream;

    // map each Writable to its associated file
    // contains only the filename, without the OS path
    std::unordered_map<Writable *, File> m_files;

    std::unordered_map<File, std::shared_ptr<nlohmann::json>> m_jsonVals;

    // files that have logically, but not physically been written to
    std::unordered_set<File> m_dirty;

    // HELPER FUNCTIONS

    // will use the IOHandler to retrieve the correct directory
    // shared pointer to circumvent the fact that c++ pre 17 does
    // not enforce (only allow) copy elision in return statements
    std::shared_ptr<FILEHANDLE> getFilehandle(
        File,
        Access access); //, Access
                        // m_frontendAccess=this->m_handler->m_frontendAccess);

    // full operating system path of the given file
    std::string fullPath(File);

    std::string fullPath(std::string const &);

    // from a path specification /a/b/c, remove the last
    // "folder" (i.e. modify the string to equal /a/b)
    static void parentDir(std::string &);

    // Fileposition is assumed to have already been set,
    // get it in string form
    static std::string filepositionOf(Writable *w);

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

    static nlohmann::json initializeNDArray(Extent const &extent);

    static Extent getExtent(nlohmann::json &j);

    // remove single '/' in the beginning and end of a string
    static std::string removeSlashes(std::string);

    template <typename KeyT>
    static bool hasKey(nlohmann::json &, KeyT &&key);

    // make sure that the given path exists in proper form in
    // the passed json value
    static void ensurePath(nlohmann::json *json, std::string path);

    // In order not to insert the same file name into the data structures
    // with a new pointer (e.g. when reopening), search for a possibly
    // existing old pointer. Construct a new pointer only upon failure.
    // The bool is true iff the pointer has been newly-created.
    // The iterator is an iterator for m_files
    std::tuple<File, std::unordered_map<Writable *, File>::iterator, bool>
    getPossiblyExisting(std::string file);

    // get the json value representing the whole file, possibly reading
    // from disk
    std::shared_ptr<nlohmann::json> obtainJsonContents(File);

    // get the json value at the writable's fileposition
    nlohmann::json &obtainJsonContents(Writable *writable);

    // write to disk the json contents associated with the file
    // remove from m_dirty if unsetDirty == true
    void putJsonContents(File, bool unsetDirty = true);

    // figure out the file position of the writable
    // (preferring the parent's file position) and extend it
    // by extend. return the modified file position.
    std::shared_ptr<JSONFilePosition>
    setAndGetFilePosition(Writable *, std::string extend);

    // figure out the file position of the writable
    // (preferring the parent's file position)
    // only modify the writable's fileposition when specified
    std::shared_ptr<JSONFilePosition>
    setAndGetFilePosition(Writable *, bool write = true);

    // get the writable's containing file
    // if the parent is associated with another file,
    // associate the writable with that file and return it
    File refreshFileFromParent(Writable *writable);

    void associateWithFile(Writable *writable, File);

    // need to check the name too in order to exclude "attributes" key
    static bool isGroup(nlohmann::json::const_iterator it);

    static bool isDataset(nlohmann::json const &j);

    // check whether the json reference contains a valid dataset
    template <typename Param>
    void verifyDataset(Param const &parameters, nlohmann::json &);

    static nlohmann::json platformSpecifics();

    struct DatasetWriter
    {
        template <typename T>
        void operator()(
            nlohmann::json &json,
            const Parameter<Operation::WRITE_DATASET> &parameters);

        std::string errorMsg = "JSON: writeDataset";
    };

    struct DatasetReader
    {
        template <typename T>
        void operator()(
            nlohmann::json &json,
            Parameter<Operation::READ_DATASET> &parameters);

        std::string errorMsg = "JSON: readDataset";
    };

    struct AttributeWriter
    {
        template <typename T>
        void operator()(nlohmann::json &, Attribute::resource const &);

        std::string errorMsg = "JSON: writeAttribute";
    };

    struct AttributeReader
    {
        template <typename T>
        void operator()(nlohmann::json &, Parameter<Operation::READ_ATT> &);

        std::string errorMsg = "JSON: writeAttribute";
    };

    template <typename T>
    struct CppToJSON
    {
        nlohmann::json operator()(T const &);
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
    struct JsonToCpp<
        T,
        typename std::enable_if<std::is_floating_point<T>::value>::type>
    {
        T operator()(nlohmann::json const &);
    };
};

} // namespace openPMD
