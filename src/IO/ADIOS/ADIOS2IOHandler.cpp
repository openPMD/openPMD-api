/* Copyright 2017-2021 Franz Poeschel, Fabian Koller and Axel Huebl
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

#include "openPMD/IO/ADIOS/ADIOS2IOHandler.hpp"

#include "openPMD/Datatype.hpp"
#include "openPMD/IO/ADIOS/ADIOS2FilePosition.hpp"
#include "openPMD/IO/ADIOS/ADIOS2IOHandler.hpp"
#include "openPMD/auxiliary/Environment.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/auxiliary/StringManip.hpp"

#include <algorithm>
#include <cctype> // std::tolower
#include <iostream>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <type_traits>

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

#if openPMD_HAVE_ADIOS2

#if openPMD_HAVE_MPI

ADIOS2IOHandlerImpl::ADIOS2IOHandlerImpl(
    AbstractIOHandler *handler,
    MPI_Comm communicator,
    nlohmann::json cfg,
    std::string engineType)
    : AbstractIOHandlerImplCommon(handler)
    , m_ADIOS{communicator}
    , m_engineType(std::move(engineType))
{
    init(std::move(cfg));
}

#endif // openPMD_HAVE_MPI

ADIOS2IOHandlerImpl::ADIOS2IOHandlerImpl(
    AbstractIOHandler *handler, nlohmann::json cfg, std::string engineType)
    : AbstractIOHandlerImplCommon(handler)
    , m_ADIOS{}
    , m_engineType(std::move(engineType))
{
    init(std::move(cfg));
}

ADIOS2IOHandlerImpl::~ADIOS2IOHandlerImpl()
{
    /*
     * m_fileData is an unordered_map indexed by pointer addresses
     * to the fileState member of InvalidatableFile.
     * This means that destruction order is nondeterministic.
     * Let's determinize it (necessary if computing in parallel).
     */
    using file_t = std::unique_ptr<detail::BufferedActions>;
    std::vector<file_t> sorted;
    sorted.reserve(m_fileData.size());
    for (auto &pair : m_fileData)
    {
        sorted.push_back(std::move(pair.second));
    }
    m_fileData.clear();
    std::sort(
        sorted.begin(), sorted.end(), [](auto const &left, auto const &right) {
            return left->m_file <= right->m_file;
        });
    // run the destructors
    for (auto &file : sorted)
    {
        // std::unique_ptr interface
        file.reset();
    }
}

void ADIOS2IOHandlerImpl::init(nlohmann::json cfg)
{
    if (cfg.contains("adios2"))
    {
        m_config = std::move(cfg["adios2"]);

        if (m_config.json().contains("schema"))
        {
            m_schema = m_config["schema"].json().get<ADIOS2Schema::schema_t>();
        }

        if (m_config.json().contains("use_span_based_put"))
        {
            m_useSpanBasedPutByDefault =
                m_config["use_span_based_put"].json().get<bool>() ? UseSpan::Yes
                                                                  : UseSpan::No;
        }

        auto engineConfig = config(ADIOS2Defaults::str_engine);
        if (!engineConfig.json().is_null())
        {
            auto engineTypeConfig =
                config(ADIOS2Defaults::str_type, engineConfig).json();
            if (!engineTypeConfig.is_null())
            {
                // convert to string
                m_engineType = engineTypeConfig;
                std::transform(
                    m_engineType.begin(),
                    m_engineType.end(),
                    m_engineType.begin(),
                    [](unsigned char c) { return std::tolower(c); });
            }
        }
        auto operators = getOperators();
        if (operators)
        {
            defaultOperators = std::move(operators.get());
        }
    }
    // environment-variable based configuration
    m_schema = auxiliary::getEnvNum("OPENPMD2_ADIOS2_SCHEMA", m_schema);
}

auxiliary::Option<std::vector<ADIOS2IOHandlerImpl::ParameterizedOperator> >
ADIOS2IOHandlerImpl::getOperators(auxiliary::TracingJSON cfg)
{
    using ret_t = auxiliary::Option<std::vector<ParameterizedOperator> >;
    std::vector<ParameterizedOperator> res;
    if (!cfg.json().contains("dataset"))
    {
        return ret_t();
    }
    auto datasetConfig = cfg["dataset"];
    if (!datasetConfig.json().contains("operators"))
    {
        return ret_t();
    }
    auto _operators = datasetConfig["operators"];
    nlohmann::json const &operators = _operators.json();
    for (auto operatorIterator = operators.begin();
         operatorIterator != operators.end();
         ++operatorIterator)
    {
        nlohmann::json const &op = operatorIterator.value();
        std::string const &type = op["type"];
        adios2::Params adiosParams;
        if (op.contains("parameters"))
        {
            nlohmann::json const &params = op["parameters"];
            for (auto paramIterator = params.begin();
                 paramIterator != params.end();
                 ++paramIterator)
            {
                adiosParams[paramIterator.key()] =
                    paramIterator.value().get<std::string>();
            }
        }
        auxiliary::Option<adios2::Operator> adiosOperator =
            getCompressionOperator(type);
        if (adiosOperator)
        {
            res.emplace_back(ParameterizedOperator{
                adiosOperator.get(), std::move(adiosParams)});
        }
    }
    _operators.declareFullyRead();
    return auxiliary::makeOption(std::move(res));
}

auxiliary::Option<std::vector<ADIOS2IOHandlerImpl::ParameterizedOperator> >
ADIOS2IOHandlerImpl::getOperators()
{
    return getOperators(m_config);
}

std::string ADIOS2IOHandlerImpl::fileSuffix() const
{
    // SST engine adds its suffix unconditionally
    // so we don't add it
    static std::map<std::string, std::string> endings{
        {"sst", ""},
        {"staging", ""},
        {"bp4", ".bp"},
        {"bp3", ".bp"},
        {"file", ".bp"},
        {"hdf5", ".h5"},
        {"nullcore", ".nullcore"},
        {"ssc", ".ssc"}};
    auto it = endings.find(m_engineType);
    if (it != endings.end())
    {
        return it->second;
    }
    else
    {
        return ".adios2";
    }
}

std::future<void>
ADIOS2IOHandlerImpl::flush(internal::FlushParams const &flushParams)
{
    auto res = AbstractIOHandlerImpl::flush();
    for (auto &p : m_fileData)
    {
        if (m_dirty.find(p.first) != m_dirty.end())
        {
            p.second->flush(
                flushParams.flushLevel, /* writeAttributes = */ false);
        }
        else
        {
            p.second->drop();
        }
    }
    return res;
}

void ADIOS2IOHandlerImpl::createFile(
    Writable *writable, Parameter<Operation::CREATE_FILE> const &parameters)
{
    VERIFY_ALWAYS(
        m_handler->m_backendAccess != Access::READ_ONLY,
        "[ADIOS2] Creating a file in read-only mode is not possible.");

    if (!writable->written)
    {
        std::string name = parameters.name;
        std::string suffix(fileSuffix());
        if (!auxiliary::ends_with(name, suffix))
        {
            name += suffix;
        }

        auto res_pair = getPossiblyExisting(name);
        InvalidatableFile shared_name = InvalidatableFile(name);
        VERIFY_ALWAYS(
            !(m_handler->m_backendAccess == Access::READ_WRITE &&
              (!std::get<PE_NewlyCreated>(res_pair) ||
               auxiliary::file_exists(
                   fullPath(std::get<PE_InvalidatableFile>(res_pair))))),
            "[ADIOS2] Can only overwrite existing file in CREATE mode.");

        if (!std::get<PE_NewlyCreated>(res_pair))
        {
            auto file = std::get<PE_InvalidatableFile>(res_pair);
            m_dirty.erase(file);
            dropFileData(file);
            file.invalidate();
        }

        std::string const dir(m_handler->directory);
        if (!auxiliary::directory_exists(dir))
        {
            auto success = auxiliary::create_directories(dir);
            VERIFY(success, "[ADIOS2] Could not create directory.");
        }

        m_iterationEncoding = parameters.encoding;
        associateWithFile(writable, shared_name);
        this->m_dirty.emplace(shared_name);
        getFileData(shared_name, IfFileNotOpen::OpenImplicitly).m_mode =
            adios2::Mode::Write; // WORKAROUND
        // ADIOS2 does not yet implement ReadWrite Mode

        writable->written = true;
        writable->abstractFilePosition = std::make_shared<ADIOS2FilePosition>();
        // enforce opening the file
        // lazy opening is deathly in parallel situations
        getFileData(shared_name, IfFileNotOpen::OpenImplicitly);
    }
}

void ADIOS2IOHandlerImpl::createPath(
    Writable *writable, const Parameter<Operation::CREATE_PATH> &parameters)
{
    std::string path;
    refreshFileFromParent(writable, /* preferParentFile = */ true);

    /* Sanitize path */
    if (!auxiliary::starts_with(parameters.path, '/'))
    {
        path = filePositionToString(setAndGetFilePosition(writable)) + "/" +
            auxiliary::removeSlashes(parameters.path);
    }
    else
    {
        path = "/" + auxiliary::removeSlashes(parameters.path);
    }

    /* ADIOS has no concept for explicitly creating paths.
     * They are implicitly created with the paths of variables/attributes. */

    writable->written = true;
    writable->abstractFilePosition = std::make_shared<ADIOS2FilePosition>(
        path, ADIOS2FilePosition::GD::GROUP);
}

void ADIOS2IOHandlerImpl::createDataset(
    Writable *writable, const Parameter<Operation::CREATE_DATASET> &parameters)
{
    if (m_handler->m_backendAccess == Access::READ_ONLY)
    {
        throw std::runtime_error(
            "[ADIOS2] Creating a dataset in a file opened as read "
            "only is not possible.");
    }
    if (!writable->written)
    {
        /* Sanitize name */
        std::string name = auxiliary::removeSlashes(parameters.name);

        auto const file =
            refreshFileFromParent(writable, /* preferParentFile = */ false);
        auto filePos = setAndGetFilePosition(writable, name);
        filePos->gd = ADIOS2FilePosition::GD::DATASET;
        auto const varName = nameOfVariable(writable);

        std::vector<ParameterizedOperator> operators;
        nlohmann::json options = nlohmann::json::parse(parameters.options);
        if (options.contains("adios2"))
        {
            auxiliary::TracingJSON datasetConfig(options["adios2"]);
            auto datasetOperators = getOperators(datasetConfig);

            operators = datasetOperators ? std::move(datasetOperators.get())
                                         : defaultOperators;

            auto shadow = datasetConfig.invertShadow();
            if (shadow.size() > 0)
            {
                std::cerr << "Warning: parts of the JSON configuration for "
                             "ADIOS2 dataset '"
                          << varName << "' remain unused:\n"
                          << shadow << std::endl;
            }
        }
        else
        {
            operators = defaultOperators;
        }

        if (!parameters.compression.empty())
        {
            auxiliary::Option<adios2::Operator> adiosOperator =
                getCompressionOperator(parameters.compression);
            if (adiosOperator)
            {
                operators.push_back(ParameterizedOperator{
                    adiosOperator.get(), adios2::Params()});
            }
        }

        // cast from openPMD::Extent to adios2::Dims
        adios2::Dims const shape(
            parameters.extent.begin(), parameters.extent.end());

        auto &fileData = getFileData(file, IfFileNotOpen::ThrowError);
        switchAdios2VariableType(
            parameters.dtype,
            detail::VariableDefiner(),
            fileData.m_IO,
            varName,
            operators,
            shape);
        fileData.invalidateVariablesMap();
        writable->written = true;
        m_dirty.emplace(file);
    }
}

namespace detail
{
    struct DatasetExtender
    {
        template <typename T, typename... Args>
        void operator()(
            adios2::IO &IO, std::string const &variable, Extent const &newShape)
        {
            auto var = IO.InquireVariable<T>(variable);
            if (!var)
            {
                throw std::runtime_error(
                    "[ADIOS2] Unable to retrieve variable for resizing: '" +
                    variable + "'.");
            }
            adios2::Dims dims;
            dims.reserve(newShape.size());
            for (auto ext : newShape)
            {
                dims.push_back(ext);
            }
            var.SetShape(dims);
        }

        std::string errorMsg = "ADIOS2: extendDataset()";
    };
} // namespace detail

void ADIOS2IOHandlerImpl::extendDataset(
    Writable *writable, const Parameter<Operation::EXTEND_DATASET> &parameters)
{
    VERIFY_ALWAYS(
        m_handler->m_backendAccess != Access::READ_ONLY,
        "[ADIOS2] Cannot extend datasets in read-only mode.");
    setAndGetFilePosition(writable);
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    std::string name = nameOfVariable(writable);
    auto &filedata = getFileData(file, IfFileNotOpen::ThrowError);
    static detail::DatasetExtender de;
    Datatype dt = detail::fromADIOS2Type(filedata.m_IO.VariableType(name));
    switchAdios2VariableType(dt, de, filedata.m_IO, name, parameters.extent);
}

void ADIOS2IOHandlerImpl::openFile(
    Writable *writable, const Parameter<Operation::OPEN_FILE> &parameters)
{
    if (!auxiliary::directory_exists(m_handler->directory))
    {
        throw no_such_file_error(
            "[ADIOS2] Supplied directory is not valid: " +
            m_handler->directory);
    }

    std::string name = parameters.name;
    std::string suffix(fileSuffix());
    if (!auxiliary::ends_with(name, suffix))
    {
        name += suffix;
    }

    auto file = std::get<PE_InvalidatableFile>(getPossiblyExisting(name));

    associateWithFile(writable, file);

    writable->written = true;
    writable->abstractFilePosition = std::make_shared<ADIOS2FilePosition>();

    m_iterationEncoding = parameters.encoding;
    // enforce opening the file
    // lazy opening is deathly in parallel situations
    getFileData(file, IfFileNotOpen::OpenImplicitly);
}

void ADIOS2IOHandlerImpl::closeFile(
    Writable *writable, Parameter<Operation::CLOSE_FILE> const &)
{
    auto fileIterator = m_files.find(writable);
    if (fileIterator != m_files.end())
    {
        // do not invalidate the file
        // it still exists, it is just not open
        auto it = m_fileData.find(fileIterator->second);
        if (it != m_fileData.end())
        {
            /*
             * No need to finalize unconditionally, destructor will take care
             * of it.
             */
            it->second->flush(
                FlushLevel::UserFlush,
                [](detail::BufferedActions &ba, adios2::Engine &) {
                    ba.finalize();
                },
                /* writeAttributes = */ true,
                /* flushUnconditionally = */ false);
            m_fileData.erase(it);
        }
    }
}

void ADIOS2IOHandlerImpl::openPath(
    Writable *writable, const Parameter<Operation::OPEN_PATH> &parameters)
{
    /* Sanitize path */
    refreshFileFromParent(writable, /* preferParentFile = */ true);
    std::string prefix =
        filePositionToString(setAndGetFilePosition(writable->parent));
    std::string suffix = auxiliary::removeSlashes(parameters.path);
    std::string infix =
        suffix.empty() || auxiliary::ends_with(prefix, '/') ? "" : "/";

    /* ADIOS has no concept for explicitly creating paths.
     * They are implicitly created with the paths of variables/attributes. */

    writable->abstractFilePosition = std::make_shared<ADIOS2FilePosition>(
        prefix + infix + suffix, ADIOS2FilePosition::GD::GROUP);
    writable->written = true;
}

void ADIOS2IOHandlerImpl::openDataset(
    Writable *writable, Parameter<Operation::OPEN_DATASET> &parameters)
{
    auto name = auxiliary::removeSlashes(parameters.name);
    writable->abstractFilePosition.reset();
    auto pos = setAndGetFilePosition(writable, name);
    pos->gd = ADIOS2FilePosition::GD::DATASET;
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    auto varName = nameOfVariable(writable);
    *parameters.dtype =
        detail::fromADIOS2Type(getFileData(file, IfFileNotOpen::ThrowError)
                                   .m_IO.VariableType(varName));
    switchAdios2VariableType(
        *parameters.dtype,
        detail::DatasetOpener(this),
        file,
        varName,
        parameters);
    writable->written = true;
}

void ADIOS2IOHandlerImpl::deleteFile(
    Writable *, const Parameter<Operation::DELETE_FILE> &)
{
    throw std::runtime_error("[ADIOS2] Backend does not support deletion.");
}

void ADIOS2IOHandlerImpl::deletePath(
    Writable *, const Parameter<Operation::DELETE_PATH> &)
{
    throw std::runtime_error("[ADIOS2] Backend does not support deletion.");
}

void ADIOS2IOHandlerImpl::deleteDataset(
    Writable *, const Parameter<Operation::DELETE_DATASET> &)
{
    // call filedata.invalidateVariablesMap
    throw std::runtime_error("[ADIOS2] Backend does not support deletion.");
}

void ADIOS2IOHandlerImpl::deleteAttribute(
    Writable *, const Parameter<Operation::DELETE_ATT> &)
{
    // call filedata.invalidateAttributesMap
    throw std::runtime_error("[ADIOS2] Backend does not support deletion.");
}

void ADIOS2IOHandlerImpl::writeDataset(
    Writable *writable, const Parameter<Operation::WRITE_DATASET> &parameters)
{
    VERIFY_ALWAYS(
        m_handler->m_backendAccess != Access::READ_ONLY,
        "[ADIOS2] Cannot write data in read-only mode.");
    setAndGetFilePosition(writable);
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    detail::BufferedActions &ba = getFileData(file, IfFileNotOpen::ThrowError);
    detail::BufferedPut bp;
    bp.name = nameOfVariable(writable);
    bp.param = parameters;
    ba.enqueue(std::move(bp));
    m_dirty.emplace(std::move(file));
    writable->written = true; // TODO erst nach dem Schreiben?
}

void ADIOS2IOHandlerImpl::writeAttribute(
    Writable *writable, const Parameter<Operation::WRITE_ATT> &parameters)
{
    switch (attributeLayout())
    {
    case AttributeLayout::ByAdiosAttributes:
        switchType(
            parameters.dtype,
            detail::OldAttributeWriter(),
            this,
            writable,
            parameters);
        break;
    case AttributeLayout::ByAdiosVariables: {
        VERIFY_ALWAYS(
            m_handler->m_backendAccess != Access::READ_ONLY,
            "[ADIOS2] Cannot write attribute in read-only mode.");
        auto pos = setAndGetFilePosition(writable);
        auto file =
            refreshFileFromParent(writable, /* preferParentFile = */ false);
        auto fullName = nameOfAttribute(writable, parameters.name);
        auto prefix = filePositionToString(pos);

        auto &filedata = getFileData(file, IfFileNotOpen::ThrowError);
        filedata.invalidateAttributesMap();
        m_dirty.emplace(std::move(file));

        // this intentionally overwrites previous writes
        auto &bufferedWrite = filedata.m_attributeWrites[fullName];
        bufferedWrite.name = fullName;
        bufferedWrite.dtype = parameters.dtype;
        bufferedWrite.resource = parameters.resource;
        break;
    }
    default:
        throw std::runtime_error("Unreachable!");
    }
}

void ADIOS2IOHandlerImpl::readDataset(
    Writable *writable, Parameter<Operation::READ_DATASET> &parameters)
{
    setAndGetFilePosition(writable);
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    detail::BufferedActions &ba = getFileData(file, IfFileNotOpen::ThrowError);
    detail::BufferedGet bg;
    bg.name = nameOfVariable(writable);
    bg.param = parameters;
    ba.enqueue(std::move(bg));
    m_dirty.emplace(std::move(file));
}

namespace detail
{
    struct GetSpan
    {
        template <typename T, typename... Args>
        void operator()(
            ADIOS2IOHandlerImpl *impl,
            Parameter<Operation::GET_BUFFER_VIEW> &params,
            detail::BufferedActions &ba,
            std::string const &varName)
        {
            auto &IO = ba.m_IO;
            auto &engine = ba.getEngine();
            adios2::Variable<T> variable = impl->verifyDataset<T>(
                params.offset, params.extent, IO, varName);
            adios2::Dims offset(params.offset.begin(), params.offset.end());
            adios2::Dims extent(params.extent.begin(), params.extent.end());
            variable.SetSelection({std::move(offset), std::move(extent)});
            typename adios2::Variable<T>::Span span = engine.Put(variable);
            params.out->backendManagedBuffer = true;
            /*
             * SIC!
             * Do not emplace span.data() yet.
             * Only call span.data() as soon as the user needs the pointer
             * (will always be propagated to the backend with parameters.update
             *  = true).
             * This avoids repeated resizing of ADIOS2 internal buffers if
             * calling multiple spans.
             */
            // params.out->ptr = span.data();
            unsigned nextIndex;
            if (ba.m_updateSpans.empty())
            {
                nextIndex = 0;
            }
            else
            {
                nextIndex = ba.m_updateSpans.rbegin()->first + 1;
            }
            params.out->viewIndex = nextIndex;
            std::unique_ptr<I_UpdateSpan> updateSpan{
                new UpdateSpan<T>{std::move(span)}};
            ba.m_updateSpans.emplace_hint(
                ba.m_updateSpans.end(), nextIndex, std::move(updateSpan));
        }

        std::string errorMsg = "ADIOS2: getBufferView()";
    };

    struct HasOperators
    {
        template <typename T>
        bool operator()(std::string const &name, adios2::IO &IO) const
        {
            adios2::Variable<T> variable = IO.InquireVariable<T>(name);
            if (!variable)
            {
                return false;
            }
            return !variable.Operations().empty();
        }

        std::string errorMsg = "ADIOS2: getBufferView()";
    };
} // namespace detail

void ADIOS2IOHandlerImpl::getBufferView(
    Writable *writable, Parameter<Operation::GET_BUFFER_VIEW> &parameters)
{
    // @todo check access mode
    if (m_engineType != "bp4")
    {
        parameters.out->backendManagedBuffer = false;
        return;
    }
    setAndGetFilePosition(writable);
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    detail::BufferedActions &ba = getFileData(file, IfFileNotOpen::ThrowError);

    std::string name = nameOfVariable(writable);
    switch (m_useSpanBasedPutByDefault)
    {
    case UseSpan::No:
        parameters.out->backendManagedBuffer = false;
        return;
    case UseSpan::Auto: {
        detail::HasOperators hasOperators;
        if (switchAdios2VariableType(
                parameters.dtype, hasOperators, name, ba.m_IO))
        {
            parameters.out->backendManagedBuffer = false;
            return;
        }
        break;
    }
    case UseSpan::Yes:
        break;
    }

    if (parameters.update)
    {
        detail::I_UpdateSpan &updater =
            *ba.m_updateSpans.at(parameters.out->viewIndex);
        parameters.out->ptr = updater.update();
        parameters.out->backendManagedBuffer = true;
    }
    else
    {
        static detail::GetSpan gs;
        switchAdios2VariableType(
            parameters.dtype, gs, this, parameters, ba, name);
    }
}

namespace detail
{
    template <typename T>
    UpdateSpan<T>::UpdateSpan(adios2::detail::Span<T> span_in)
        : span(std::move(span_in))
    {}

    template <typename T>
    void *UpdateSpan<T>::update()
    {
        return span.data();
    }
} // namespace detail

void ADIOS2IOHandlerImpl::readAttribute(
    Writable *writable, Parameter<Operation::READ_ATT> &parameters)
{
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    auto pos = setAndGetFilePosition(writable);
    detail::BufferedActions &ba = getFileData(file, IfFileNotOpen::ThrowError);
    switch (attributeLayout())
    {
        using AL = AttributeLayout;
    case AL::ByAdiosAttributes: {
        detail::OldBufferedAttributeRead bar;
        bar.name = nameOfAttribute(writable, parameters.name);
        bar.param = parameters;
        ba.enqueue(std::move(bar));
        break;
    }
    case AL::ByAdiosVariables: {
        detail::BufferedAttributeRead bar;
        bar.name = nameOfAttribute(writable, parameters.name);
        bar.param = parameters;
        ba.m_attributeReads.push_back(std::move(bar));
        break;
    }
    default:
        throw std::runtime_error("Unreachable!");
    }
    m_dirty.emplace(std::move(file));
}

void ADIOS2IOHandlerImpl::listPaths(
    Writable *writable, Parameter<Operation::LIST_PATHS> &parameters)
{
    VERIFY_ALWAYS(
        writable->written,
        "[ADIOS2] Internal error: Writable not marked written during path "
        "listing");
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    auto pos = setAndGetFilePosition(writable);
    std::string myName = filePositionToString(pos);
    if (!auxiliary::ends_with(myName, '/'))
    {
        myName = myName + '/';
    }

    /*
     * since ADIOS does not have a concept of paths, restore them
     * from variables and attributes.
     */
    auto &fileData = getFileData(file, IfFileNotOpen::ThrowError);
    fileData.requireActiveStep();

    std::unordered_set<std::string> subdirs;
    /*
     * When reading an attribute, we cannot distinguish
     * whether its containing "folder" is a group or a
     * dataset. If we stumble upon a dataset at the current
     * level (which can be distinguished via variables),
     * we put in in the list 'delete_me' to remove them
     * again later on.
     * Note that the method 'listDatasets' does not have
     * this problem since datasets can be restored solely
     * from variables – attributes don't even need to be
     * inspected.
     */
    std::vector<std::string> delete_me;

    switch (attributeLayout())
    {
        using AL = AttributeLayout;
    case AL::ByAdiosVariables: {
        std::vector<std::string> vars =
            fileData.availableVariablesPrefixed(myName);
        for (auto var : vars)
        {
            // since current Writable is a group and no dataset,
            // var == "__data__" is not possible
            if (auxiliary::ends_with(var, "/__data__"))
            {
                // here be datasets
                var = auxiliary::replace_last(var, "/__data__", "");
                auto firstSlash = var.find_first_of('/');
                if (firstSlash != std::string::npos)
                {
                    var = var.substr(0, firstSlash);
                    subdirs.emplace(std::move(var));
                }
                else
                { // var is a dataset at the current level
                    delete_me.push_back(std::move(var));
                }
            }
            else
            {
                // here be attributes
                auto firstSlash = var.find_first_of('/');
                if (firstSlash != std::string::npos)
                {
                    var = var.substr(0, firstSlash);
                    subdirs.emplace(std::move(var));
                }
            }
        }
        break;
    }
    case AL::ByAdiosAttributes: {
        std::vector<std::string> vars =
            fileData.availableVariablesPrefixed(myName);
        for (auto var : vars)
        {
            auto firstSlash = var.find_first_of('/');
            if (firstSlash != std::string::npos)
            {
                var = var.substr(0, firstSlash);
                subdirs.emplace(std::move(var));
            }
            else
            { // var is a dataset at the current level
                delete_me.push_back(std::move(var));
            }
        }
        std::vector<std::string> attributes =
            fileData.availableAttributesPrefixed(myName);
        for (auto attr : attributes)
        {
            auto firstSlash = attr.find_first_of('/');
            if (firstSlash != std::string::npos)
            {
                attr = attr.substr(0, firstSlash);
                subdirs.emplace(std::move(attr));
            }
        }
        break;
    }
    }

    for (auto &d : delete_me)
    {
        subdirs.erase(d);
    }
    for (auto &path : subdirs)
    {
        parameters.paths->emplace_back(std::move(path));
    }
}

void ADIOS2IOHandlerImpl::listDatasets(
    Writable *writable, Parameter<Operation::LIST_DATASETS> &parameters)
{
    VERIFY_ALWAYS(
        writable->written,
        "[ADIOS2] Internal error: Writable not marked written during path "
        "listing");
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    auto pos = setAndGetFilePosition(writable);
    // adios2::Engine & engine = getEngine( file );
    std::string myName = filePositionToString(pos);
    if (!auxiliary::ends_with(myName, '/'))
    {
        myName = myName + '/';
    }

    /*
     * since ADIOS does not have a concept of paths, restore them
     * from variables and attributes.
     */

    auto &fileData = getFileData(file, IfFileNotOpen::ThrowError);
    fileData.requireActiveStep();

    std::unordered_set<std::string> subdirs;
    for (auto var : fileData.availableVariablesPrefixed(myName))
    {
        if (attributeLayout() == AttributeLayout::ByAdiosVariables)
        {
            // since current Writable is a group and no dataset,
            // var == "__data__" is not possible
            if (!auxiliary::ends_with(var, "/__data__"))
            {
                continue;
            }
            // variable is now definitely a dataset, let's strip the suffix
            var = auxiliary::replace_last(var, "/__data__", "");
        }
        // if string still contains a slash, variable is a dataset below the
        // current group
        // we only want datasets contained directly within the current group
        // let's ensure that
        auto firstSlash = var.find_first_of('/');
        if (firstSlash == std::string::npos)
        {
            subdirs.emplace(std::move(var));
        }
    }
    for (auto &dataset : subdirs)
    {
        parameters.datasets->emplace_back(std::move(dataset));
    }
}

void ADIOS2IOHandlerImpl::listAttributes(
    Writable *writable, Parameter<Operation::LIST_ATTS> &parameters)
{
    VERIFY_ALWAYS(
        writable->written,
        "[ADIOS2] Internal error: Writable not marked "
        "written during attribute writing");
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    auto pos = setAndGetFilePosition(writable);
    auto attributePrefix = filePositionToString(pos);
    if (attributePrefix == "/")
    {
        attributePrefix = "";
    }
    auto &ba = getFileData(file, IfFileNotOpen::ThrowError);
    ba.requireActiveStep(); // make sure that the attributes are present

    std::vector<std::string> attrs;
    switch (attributeLayout())
    {
        using AL = AttributeLayout;
    case AL::ByAdiosAttributes:
        attrs = ba.availableAttributesPrefixed(attributePrefix);
        break;
    case AL::ByAdiosVariables:
        attrs = ba.availableVariablesPrefixed(attributePrefix);
        break;
    }
    for (auto &rawAttr : attrs)
    {
        if (attributeLayout() == AttributeLayout::ByAdiosVariables &&
            (auxiliary::ends_with(rawAttr, "/__data__") ||
             rawAttr == "__data__"))
        {
            continue;
        }
        auto attr = auxiliary::removeSlashes(rawAttr);
        if (attr.find_last_of('/') == std::string::npos)
        {
            parameters.attributes->push_back(std::move(attr));
        }
    }
}

void ADIOS2IOHandlerImpl::advance(
    Writable *writable, Parameter<Operation::ADVANCE> &parameters)
{
    auto file = m_files[writable];
    auto &ba = getFileData(file, IfFileNotOpen::ThrowError);
    *parameters.status = ba.advance(parameters.mode);
}

void ADIOS2IOHandlerImpl::closePath(
    Writable *writable, Parameter<Operation::CLOSE_PATH> const &)
{
    VERIFY_ALWAYS(
        writable->written,
        "[ADIOS2] Cannot close a path that has not been written yet.");
    if (m_handler->m_backendAccess == Access::READ_ONLY)
    {
        // nothing to do
        return;
    }
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    auto &fileData = getFileData(file, IfFileNotOpen::ThrowError);
    if (!fileData.optimizeAttributesStreaming)
    {
        return;
    }
    auto position = setAndGetFilePosition(writable);
    auto const positionString = filePositionToString(position);
    VERIFY(
        !auxiliary::ends_with(positionString, '/'),
        "[ADIOS2] Position string has unexpected format. This is a bug "
        "in the openPMD API.");

    for (auto const &attr :
         fileData.availableAttributesPrefixed(positionString))
    {
        fileData.m_IO.RemoveAttribute(positionString + '/' + attr);
    }
}

void ADIOS2IOHandlerImpl::availableChunks(
    Writable *writable, Parameter<Operation::AVAILABLE_CHUNKS> &parameters)
{
    setAndGetFilePosition(writable);
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    detail::BufferedActions &ba = getFileData(file, IfFileNotOpen::ThrowError);
    std::string varName = nameOfVariable(writable);
    auto engine = ba.getEngine(); // make sure that data are present
    auto datatype = detail::fromADIOS2Type(ba.m_IO.VariableType(varName));
    static detail::RetrieveBlocksInfo rbi;
    switchAdios2VariableType(
        datatype, rbi, parameters, ba.m_IO, engine, varName);
}

adios2::Mode ADIOS2IOHandlerImpl::adios2AccessMode(std::string const &fullPath)
{
    switch (m_handler->m_backendAccess)
    {
    case Access::CREATE:
        return adios2::Mode::Write;
    case Access::READ_ONLY:
        return adios2::Mode::Read;
    case Access::READ_WRITE:
        if (auxiliary::directory_exists(fullPath) ||
            auxiliary::file_exists(fullPath))
        {
            std::cerr << "ADIOS2 does currently not yet implement ReadWrite "
                         "(Append) mode. "
                      << "Replacing with Read mode." << std::endl;
            return adios2::Mode::Read;
        }
        else
        {
            std::cerr << "ADIOS2 does currently not yet implement ReadWrite "
                         "(Append) mode. "
                      << "Replacing with Write mode." << std::endl;
            return adios2::Mode::Write;
        }
    default:
        return adios2::Mode::Undefined;
    }
}

auxiliary::TracingJSON ADIOS2IOHandlerImpl::nullvalue = nlohmann::json();

std::string ADIOS2IOHandlerImpl::filePositionToString(
    std::shared_ptr<ADIOS2FilePosition> filepos)
{
    return filepos->location;
}

std::shared_ptr<ADIOS2FilePosition> ADIOS2IOHandlerImpl::extendFilePosition(
    std::shared_ptr<ADIOS2FilePosition> const &oldPos, std::string s)
{
    auto path = filePositionToString(oldPos);
    if (!auxiliary::ends_with(path, '/') && !auxiliary::starts_with(s, '/'))
    {
        path = path + "/";
    }
    else if (auxiliary::ends_with(path, '/') && auxiliary::starts_with(s, '/'))
    {
        path = auxiliary::replace_last(path, "/", "");
    }
    return std::make_shared<ADIOS2FilePosition>(
        path + std::move(s), oldPos->gd);
}

auxiliary::Option<adios2::Operator>
ADIOS2IOHandlerImpl::getCompressionOperator(std::string const &compression)
{
    adios2::Operator res;
    auto it = m_operators.find(compression);
    if (it == m_operators.end())
    {
        try
        {
            res = m_ADIOS.DefineOperator(compression, compression);
        }
        catch (std::invalid_argument const &e)
        {
            std::cerr << "Warning: ADIOS2 backend does not support compression "
                         "method "
                      << compression << ". Continuing without compression."
                      << "\nOriginal error: " << e.what() << std::endl;
            return auxiliary::Option<adios2::Operator>();
        }
        catch (std::string const &s)
        {
            std::cerr << "Warning: ADIOS2 backend does not support compression "
                         "method "
                      << compression << ". Continuing without compression."
                      << "\nOriginal error: " << s << std::endl;
            return auxiliary::Option<adios2::Operator>();
        }
        m_operators.emplace(compression, res);
    }
    else
    {
        res = it->second;
    }
    return auxiliary::makeOption(adios2::Operator(res));
}

std::string ADIOS2IOHandlerImpl::nameOfVariable(Writable *writable)
{
    auto filepos = setAndGetFilePosition(writable);
    auto res = filePositionToString(filepos);
    if (attributeLayout() == AttributeLayout::ByAdiosAttributes)
    {
        return res;
    }
    switch (filepos->gd)
    {
    case ADIOS2FilePosition::GD::GROUP:
        return res;
    case ADIOS2FilePosition::GD::DATASET:
        if (auxiliary::ends_with(res, '/'))
        {
            return res + "__data__";
        }
        else
        {
            // By convention, this path should always be taken
            // But let's be safe
            return res + "/__data__";
        }
    default:
        throw std::runtime_error("[ADIOS2IOHandlerImpl] Unreachable!");
    }
}

std::string
ADIOS2IOHandlerImpl::nameOfAttribute(Writable *writable, std::string attribute)
{
    auto pos = setAndGetFilePosition(writable);
    return filePositionToString(
        extendFilePosition(pos, auxiliary::removeSlashes(attribute)));
}

ADIOS2FilePosition::GD ADIOS2IOHandlerImpl::groupOrDataset(Writable *writable)
{
    return setAndGetFilePosition(writable)->gd;
}

detail::BufferedActions &
ADIOS2IOHandlerImpl::getFileData(InvalidatableFile file, IfFileNotOpen flag)
{
    VERIFY_ALWAYS(
        file.valid(),
        "[ADIOS2] Cannot retrieve file data for a file that has "
        "been overwritten or deleted.")
    auto it = m_fileData.find(file);
    if (it == m_fileData.end())
    {
        switch (flag)
        {
        case IfFileNotOpen::OpenImplicitly: {

            auto res = m_fileData.emplace(
                std::move(file),
                std::make_unique<detail::BufferedActions>(*this, file));
            return *res.first->second;
        }
        case IfFileNotOpen::ThrowError:
            throw std::runtime_error(
                "[ADIOS2] Requested file has not been opened yet: " +
                (file.fileState ? file.fileState->name : "Unknown file name"));
        }
    }
    else
    {
        return *it->second;
    }
}

void ADIOS2IOHandlerImpl::dropFileData(InvalidatableFile file)
{
    auto it = m_fileData.find(file);
    if (it != m_fileData.end())
    {
        it->second->drop();
        m_fileData.erase(it);
    }
}

template <typename T>
adios2::Variable<T> ADIOS2IOHandlerImpl::verifyDataset(
    Offset const &offset,
    Extent const &extent,
    adios2::IO &IO,
    std::string const &varName)
{
    {
        auto requiredType = adios2::GetType<T>();
        auto actualType = IO.VariableType(varName);
        std::stringstream errorMessage;
        errorMessage
            << "[ADIOS2] Trying to access a dataset with wrong type (trying to "
               "access dataset with type "
            << determineDatatype<T>() << ", but has type "
            << detail::fromADIOS2Type(actualType, false) << ")";
        VERIFY_ALWAYS(requiredType == actualType, errorMessage.str());
    }
    adios2::Variable<T> var = IO.InquireVariable<T>(varName);
    VERIFY_ALWAYS(
        var.operator bool(),
        "[ADIOS2] Internal error: Failed opening ADIOS2 variable.")
    // TODO leave this check to ADIOS?
    adios2::Dims shape = var.Shape();
    auto actualDim = shape.size();
    {
        auto requiredDim = extent.size();
        VERIFY_ALWAYS(
            requiredDim == actualDim,
            "[ADIOS2] Trying to access a dataset with wrong dimensionality "
            "(trying to access dataset with dimensionality " +
                std::to_string(requiredDim) + ", but has dimensionality " +
                std::to_string(actualDim) + ")")
    }
    for (unsigned int i = 0; i < actualDim; i++)
    {
        VERIFY_ALWAYS(
            offset[i] + extent[i] <= shape[i],
            "[ADIOS2] Dataset access out of bounds.")
    }

    var.SetSelection(
        {adios2::Dims(offset.begin(), offset.end()),
         adios2::Dims(extent.begin(), extent.end())});
    return var;
}

namespace detail
{
    DatasetReader::DatasetReader(openPMD::ADIOS2IOHandlerImpl *impl)
        : m_impl{impl}
    {}

    template <typename T>
    void DatasetReader::operator()(
        detail::BufferedGet &bp,
        adios2::IO &IO,
        adios2::Engine &engine,
        std::string const &fileName)
    {
        adios2::Variable<T> var = m_impl->verifyDataset<T>(
            bp.param.offset, bp.param.extent, IO, bp.name);
        if (!var)
        {
            throw std::runtime_error(
                "[ADIOS2] Failed retrieving ADIOS2 Variable with name '" +
                bp.name + "' from file " + fileName + ".");
        }
        auto ptr = std::static_pointer_cast<T>(bp.param.data).get();
        engine.Get(var, ptr);
    }

    template <typename T>
    Datatype OldAttributeReader::operator()(
        adios2::IO &IO,
        std::string name,
        std::shared_ptr<Attribute::resource> resource)
    {
        /*
         * If we store an attribute of boolean type, we store an additional
         * attribute prefixed with '__is_boolean__' to indicate this information
         * that would otherwise be lost. Check whether this has been done.
         */
        using rep = AttributeTypes<bool>::rep;
        if
#if __cplusplus >= 201703L
            constexpr
#endif
            (std::is_same<T, rep>::value)
        {
            std::string metaAttr =
                ADIOS2Defaults::str_isBooleanOldLayout + name;
            /*
             * In verbose mode, attributeInfo will yield a warning if not
             * finding the requested attribute. Since we expect the attribute
             * not to be present in many cases (i.e. when it is actually not
             * a boolean), let's tell attributeInfo to be quiet.
             */
            auto type = attributeInfo(
                IO,
                ADIOS2Defaults::str_isBooleanOldLayout + name,
                /* verbose = */ false);
            if (type == determineDatatype<rep>())
            {
                auto attr = IO.InquireAttribute<rep>(metaAttr);
                if (attr.Data().size() == 1 && attr.Data()[0] == 1)
                {
                    AttributeTypes<bool>::oldReadAttribute(IO, name, resource);
                    return determineDatatype<bool>();
                }
            }
        }
        AttributeTypes<T>::oldReadAttribute(IO, name, resource);
        return determineDatatype<T>();
    }

    template <int n, typename... Params>
    Datatype OldAttributeReader::operator()(Params &&...)
    {
        throw std::runtime_error(
            "[ADIOS2] Internal error: Unknown datatype while "
            "trying to read an attribute.");
    }

    template <typename T>
    Datatype AttributeReader::operator()(
        adios2::IO &IO,
        detail::PreloadAdiosAttributes const &preloadedAttributes,
        std::string name,
        std::shared_ptr<Attribute::resource> resource)
    {
        /*
         * If we store an attribute of boolean type, we store an additional
         * attribute prefixed with '__is_boolean__' to indicate this information
         * that would otherwise be lost. Check whether this has been done.
         */
        using rep = AttributeTypes<bool>::rep;
        if
#if __cplusplus >= 201703L
            constexpr
#endif
            (std::is_same<T, rep>::value)
        {
            std::string metaAttr =
                ADIOS2Defaults::str_isBooleanNewLayout + name;
            /*
             * In verbose mode, attributeInfo will yield a warning if not
             * finding the requested attribute. Since we expect the attribute
             * not to be present in many cases (i.e. when it is actually not
             * a boolean), let's tell attributeInfo to be quiet.
             */
            auto type = attributeInfo(
                IO,
                ADIOS2Defaults::str_isBooleanNewLayout + name,
                /* verbose = */ false);
            if (type == determineDatatype<rep>())
            {
                auto attr = IO.InquireAttribute<rep>(metaAttr);
                if (attr.Data().size() == 1 && attr.Data()[0] == 1)
                {
                    AttributeTypes<bool>::readAttribute(
                        preloadedAttributes, name, resource);
                    return determineDatatype<bool>();
                }
            }
        }
        AttributeTypes<T>::readAttribute(preloadedAttributes, name, resource);
        return determineDatatype<T>();
    }

    template <int n, typename... Params>
    Datatype AttributeReader::operator()(Params &&...)
    {
        throw std::runtime_error(
            "[ADIOS2] Internal error: Unknown datatype while "
            "trying to read an attribute.");
    }

    template <typename T>
    void OldAttributeWriter::operator()(
        ADIOS2IOHandlerImpl *impl,
        Writable *writable,
        const Parameter<Operation::WRITE_ATT> &parameters)
    {
        VERIFY_ALWAYS(
            impl->m_handler->m_backendAccess != Access::READ_ONLY,
            "[ADIOS2] Cannot write attribute in read-only mode.");
        auto pos = impl->setAndGetFilePosition(writable);
        auto file = impl->refreshFileFromParent(
            writable, /* preferParentFile = */ false);
        auto fullName = impl->nameOfAttribute(writable, parameters.name);
        auto prefix = impl->filePositionToString(pos);

        auto &filedata = impl->getFileData(
            file, ADIOS2IOHandlerImpl::IfFileNotOpen::ThrowError);
        filedata.invalidateAttributesMap();
        adios2::IO IO = filedata.m_IO;
        impl->m_dirty.emplace(std::move(file));

        std::string t = IO.AttributeType(fullName);
        if (!t.empty()) // an attribute is present <=> it has a type
        {
            // don't overwrite attributes if they are equivalent
            // overwriting is only legal within the same step

            auto attributeModifiable = [&filedata, &fullName]() {
                auto it = filedata.uncommittedAttributes.find(fullName);
                return it != filedata.uncommittedAttributes.end();
            };
            if (AttributeTypes<T>::attributeUnchanged(
                    IO, fullName, variantSrc::get<T>(parameters.resource)))
            {
                return;
            }
            else if (attributeModifiable())
            {
                IO.RemoveAttribute(fullName);
            }
            else
            {
                std::cerr << "[Warning][ADIOS2] Cannot modify attribute from "
                             "previous step: "
                          << fullName << std::endl;
                return;
            }
        }
        else
        {
            filedata.uncommittedAttributes.emplace(fullName);
        }

        AttributeTypes<T>::oldCreateAttribute(
            IO, fullName, variantSrc::get<T>(parameters.resource));
    }

    template <int n, typename... Params>
    void OldAttributeWriter::operator()(Params &&...)
    {
        throw std::runtime_error(
            "[ADIOS2] Internal error: Unknown datatype while "
            "trying to write an attribute.");
    }

    template <typename T>
    void AttributeWriter::operator()(
        detail::BufferedAttributeWrite &params, BufferedActions &fileData)
    {
        AttributeTypes<T>::createAttribute(
            fileData.m_IO,
            fileData.requireActiveStep(),
            params,
            variantSrc::get<T>(params.resource));
    }

    template <int n, typename... Params>
    void AttributeWriter::operator()(Params &&...)
    {
        throw std::runtime_error(
            "[ADIOS2] Internal error: Unknown datatype while "
            "trying to write an attribute.");
    }

    DatasetOpener::DatasetOpener(ADIOS2IOHandlerImpl *impl) : m_impl{impl}
    {}

    template <typename T>
    void DatasetOpener::operator()(
        InvalidatableFile file,
        const std::string &varName,
        Parameter<Operation::OPEN_DATASET> &parameters)
    {
        auto &fileData = m_impl->getFileData(
            file, ADIOS2IOHandlerImpl::IfFileNotOpen::ThrowError);
        fileData.requireActiveStep();
        auto &IO = fileData.m_IO;
        adios2::Variable<T> var = IO.InquireVariable<T>(varName);
        if (!var)
        {
            throw std::runtime_error(
                "[ADIOS2] Failed retrieving ADIOS2 Variable with name '" +
                varName + "' from file " + *file + ".");
        }

        // cast from adios2::Dims to openPMD::Extent
        auto const shape = var.Shape();
        parameters.extent->clear();
        parameters.extent->reserve(shape.size());
        std::copy(
            shape.begin(), shape.end(), std::back_inserter(*parameters.extent));
    }

    WriteDataset::WriteDataset(ADIOS2IOHandlerImpl *handlerImpl)
        : m_handlerImpl{handlerImpl}
    {}

    template <typename T>
    void WriteDataset::operator()(
        detail::BufferedPut &bp, adios2::IO &IO, adios2::Engine &engine)
    {
        VERIFY_ALWAYS(
            m_handlerImpl->m_handler->m_backendAccess != Access::READ_ONLY,
            "[ADIOS2] Cannot write data in read-only mode.");

        auto ptr = std::static_pointer_cast<const T>(bp.param.data).get();

        adios2::Variable<T> var = m_handlerImpl->verifyDataset<T>(
            bp.param.offset, bp.param.extent, IO, bp.name);

        engine.Put(var, ptr);
    }

    template <int n, typename... Params>
    void WriteDataset::operator()(Params &&...)
    {
        throw std::runtime_error("[ADIOS2] WRITE_DATASET: Invalid datatype.");
    }

    template <typename T>
    void VariableDefiner::operator()(
        adios2::IO &IO,
        std::string const &name,
        std::vector<ADIOS2IOHandlerImpl::ParameterizedOperator> const
            &compressions,
        adios2::Dims const &shape,
        adios2::Dims const &start,
        adios2::Dims const &count,
        bool const constantDims)
    {
        /*
         * Step/Variable-based iteration layout:
         * The variable may already be defined from a previous step,
         * so check if it's already here.
         */
        adios2::Variable<T> var = IO.InquireVariable<T>(name);
        if (!var)
        {
            var = IO.DefineVariable<T>(name, shape, start, count, constantDims);
        }
        else
        {
            var.SetShape(shape);
            if (count.size() > 0)
            {
                var.SetSelection({start, count});
            }
            // don't add compression operators multiple times
            return;
        }

        if (!var)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: Could not create Variable '" + name +
                "'.");
        }
        for (auto const &compression : compressions)
        {
            if (compression.op)
            {
                var.AddOperation(compression.op, compression.params);
            }
        }
    }

    template <typename T>
    void RetrieveBlocksInfo::operator()(
        Parameter<Operation::AVAILABLE_CHUNKS> &params,
        adios2::IO &IO,
        adios2::Engine &engine,
        std::string const &varName)
    {
        auto var = IO.InquireVariable<T>(varName);
        auto blocksInfo = engine.BlocksInfo<T>(var, engine.CurrentStep());
        auto &table = *params.chunks;
        table.reserve(blocksInfo.size());
        for (auto const &info : blocksInfo)
        {
            Offset offset;
            Extent extent;
            auto size = info.Start.size();
            offset.reserve(size);
            extent.reserve(size);
            for (unsigned i = 0; i < size; ++i)
            {
                offset.push_back(info.Start[i]);
                extent.push_back(info.Count[i]);
            }
            table.emplace_back(
                std::move(offset), std::move(extent), info.WriterID);
        }
    }

    template <int n, typename... Args>
    void RetrieveBlocksInfo::operator()(Args &&...)
    {
        // variable has not been found, so we don't fill in any blocks
    }

    template <typename T>
    void AttributeTypes<T>::oldCreateAttribute(
        adios2::IO &IO, std::string name, const T value)
    {
        auto attr = IO.DefineAttribute(name, value);
        if (!attr)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: Failed defining attribute '" + name +
                "'.");
        }
    }

    template <typename T>
    void AttributeTypes<T>::oldReadAttribute(
        adios2::IO &IO,
        std::string name,
        std::shared_ptr<Attribute::resource> resource)
    {
        auto attr = IO.InquireAttribute<T>(name);
        if (!attr)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: Failed reading attribute '" + name +
                "'.");
        }
        *resource = attr.Data()[0];
    }

    template <typename T>
    void AttributeTypes<T>::createAttribute(
        adios2::IO &IO,
        adios2::Engine &engine,
        detail::BufferedAttributeWrite &params,
        const T value)
    {
        auto attr = IO.InquireVariable<T>(params.name);
        // @todo check size
        if (!attr)
        {
            // std::cout << "DATATYPE OF " << name << ": "
            //     << IO.VariableType( name ) << std::endl;
            attr = IO.DefineVariable<T>(params.name);
        }
        if (!attr)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: Failed defining variable '" +
                params.name + "'.");
        }
        engine.Put(attr, value, adios2::Mode::Deferred);
    }

    template <typename T>
    void AttributeTypes<T>::readAttribute(
        detail::PreloadAdiosAttributes const &preloadedAttributes,
        std::string name,
        std::shared_ptr<Attribute::resource> resource)
    {
        detail::AttributeWithShape<T> attr =
            preloadedAttributes.getAttribute<T>(name);
        if (!(attr.shape.size() == 0 ||
              (attr.shape.size() == 1 && attr.shape[0] == 1)))
        {
            throw std::runtime_error(
                "[ADIOS2] Expecting scalar ADIOS variable, got " +
                std::to_string(attr.shape.size()) + "D: " + name);
        }
        *resource = *attr.data;
    }

    template <typename T>
    void AttributeTypes<std::vector<T> >::oldCreateAttribute(
        adios2::IO &IO, std::string name, const std::vector<T> &value)
    {
        auto attr = IO.DefineAttribute(name, value.data(), value.size());
        if (!attr)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: Failed defining attribute '" + name +
                "'.");
        }
    }

    template <typename T>
    void AttributeTypes<std::vector<T> >::oldReadAttribute(
        adios2::IO &IO,
        std::string name,
        std::shared_ptr<Attribute::resource> resource)
    {
        auto attr = IO.InquireAttribute<T>(name);
        if (!attr)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: Failed reading attribute '" + name +
                "'.");
        }
        *resource = attr.Data();
    }

    template <typename T>
    void AttributeTypes<std::vector<T> >::createAttribute(
        adios2::IO &IO,
        adios2::Engine &engine,
        detail::BufferedAttributeWrite &params,
        const std::vector<T> &value)
    {
        auto size = value.size();
        auto attr = IO.InquireVariable<T>(params.name);
        // @todo check size
        if (!attr)
        {
            attr = IO.DefineVariable<T>(params.name, {size}, {0}, {size});
        }
        if (!attr)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: Failed defining variable '" +
                params.name + "'.");
        }
        engine.Put(attr, value.data(), adios2::Mode::Deferred);
    }

    template <typename T>
    void AttributeTypes<std::vector<T> >::readAttribute(
        detail::PreloadAdiosAttributes const &preloadedAttributes,
        std::string name,
        std::shared_ptr<Attribute::resource> resource)
    {
        detail::AttributeWithShape<T> attr =
            preloadedAttributes.getAttribute<T>(name);
        if (attr.shape.size() != 1)
        {
            throw std::runtime_error("[ADIOS2] Expecting 1D ADIOS variable");
        }

        std::vector<T> res(attr.shape[0]);
        std::copy_n(attr.data, attr.shape[0], res.data());
        *resource = std::move(res);
    }

    void AttributeTypes<std::vector<std::string> >::oldCreateAttribute(
        adios2::IO &IO, std::string name, const std::vector<std::string> &value)
    {
        auto attr = IO.DefineAttribute(name, value.data(), value.size());
        if (!attr)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: Failed defining attribute '" + name +
                "'.");
        }
    }

    void AttributeTypes<std::vector<std::string> >::oldReadAttribute(
        adios2::IO &IO,
        std::string name,
        std::shared_ptr<Attribute::resource> resource)
    {
        auto attr = IO.InquireAttribute<std::string>(name);
        if (!attr)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: Failed reading attribute '" + name +
                "'.");
        }
        *resource = attr.Data();
    }

    void AttributeTypes<std::vector<std::string> >::createAttribute(
        adios2::IO &IO,
        adios2::Engine &engine,
        detail::BufferedAttributeWrite &params,
        const std::vector<std::string> &vec)
    {
        size_t width = 0;
        for (auto const &str : vec)
        {
            width = std::max(width, str.size());
        }
        ++width; // null delimiter
        size_t const height = vec.size();

        auto attr = IO.InquireVariable<char>(params.name);
        // @todo check size
        if (!attr)
        {
            attr = IO.DefineVariable<char>(
                params.name, {height, width}, {0, 0}, {height, width});
        }
        if (!attr)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: Failed defining variable '" +
                params.name + "'.");
        }

        // write this thing to the params, so we don't get a use after free
        // due to deferred writing
        params.bufferForVecString = std::vector<char>(width * height, 0);
        for (size_t i = 0; i < height; ++i)
        {
            size_t start = i * width;
            std::string const &str = vec[i];
            std::copy(
                str.begin(),
                str.end(),
                params.bufferForVecString.begin() + start);
        }

        engine.Put(
            attr, params.bufferForVecString.data(), adios2::Mode::Deferred);
    }

    void AttributeTypes<std::vector<std::string> >::readAttribute(
        detail::PreloadAdiosAttributes const &preloadedAttributes,
        std::string name,
        std::shared_ptr<Attribute::resource> resource)
    {
        /*
         * char_type parameter only for specifying the "template" type.
         */
        auto loadFromDatatype = [&preloadedAttributes, &name, &resource](
                                    auto char_type) {
            using char_t = decltype(char_type);
            detail::AttributeWithShape<char_t> attr =
                preloadedAttributes.getAttribute<char_t>(name);
            if (attr.shape.size() != 2)
            {
                throw std::runtime_error(
                    "[ADIOS2] Expecting 2D ADIOS variable");
            }
            char_t const *loadedData = attr.data;
            size_t height = attr.shape[0];
            size_t width = attr.shape[1];

            std::vector<std::string> res(height);
            if (std::is_signed<char>::value == std::is_signed<char_t>::value)
            {
                /*
                 * This branch is chosen if the signedness of the
                 * ADIOS variable corresponds with the signedness of the
                 * char type on the current platform.
                 * In this case, the C++ standard guarantees that the
                 * representations for char and (un)signed char are
                 * identical, reinterpret_cast-ing the loadedData to
                 * char in order to construct our strings will be fine.
                 */
                for (size_t i = 0; i < height; ++i)
                {
                    size_t start = i * width;
                    char const *start_ptr =
                        reinterpret_cast<char const *>(loadedData + start);
                    size_t j = 0;
                    while (j < width && start_ptr[j] != 0)
                    {
                        ++j;
                    }
                    std::string &str = res[i];
                    str.append(start_ptr, start_ptr + j);
                }
            }
            else
            {
                /*
                 * This branch is chosen if the signedness of the
                 * ADIOS variable is different from the signedness of the
                 * char type on the current platform.
                 * In this case, we play it safe, and explicitly convert
                 * the loadedData to char pointwise.
                 */
                std::vector<char> converted(width);
                for (size_t i = 0; i < height; ++i)
                {
                    size_t start = i * width;
                    auto const *start_ptr = loadedData + start;
                    size_t j = 0;
                    while (j < width && start_ptr[j] != 0)
                    {
                        converted[j] = start_ptr[j];
                        ++j;
                    }
                    std::string &str = res[i];
                    str.append(converted.data(), converted.data() + j);
                }
            }

            *resource = res;
        };
        /*
         * If writing char variables in ADIOS2, they might become either int8_t
         * or uint8_t on disk depending on the platform.
         * So allow reading from both types.
         */
        switch (preloadedAttributes.attributeType(name))
        {
        /*
         * Workaround for two bugs at once:
         * ADIOS2 does not have an explicit char type,
         * we don't have an explicit schar type.
         * Until this is fixed, we use CHAR to represent ADIOS signed char.
         */
        case Datatype::CHAR: {
            using schar_t = signed char;
            loadFromDatatype(schar_t{});
            break;
        }
        case Datatype::UCHAR: {
            using uchar_t = unsigned char;
            loadFromDatatype(uchar_t{});
            break;
        }
        default: {
            throw std::runtime_error(
                "[ADIOS2] Expecting 2D ADIOS variable of "
                "type signed or unsigned char.");
        }
        }
    }

    template <typename T, size_t n>
    void AttributeTypes<std::array<T, n> >::oldCreateAttribute(
        adios2::IO &IO, std::string name, const std::array<T, n> &value)
    {
        auto attr = IO.DefineAttribute(name, value.data(), n);
        if (!attr)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: Failed defining attribute '" + name +
                "'.");
        }
    }

    template <typename T, size_t n>
    void AttributeTypes<std::array<T, n> >::oldReadAttribute(
        adios2::IO &IO,
        std::string name,
        std::shared_ptr<Attribute::resource> resource)
    {
        auto attr = IO.InquireAttribute<T>(name);
        if (!attr)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: Failed reading attribute '" + name +
                "'.");
        }
        auto data = attr.Data();
        std::array<T, n> res;
        for (size_t i = 0; i < n; i++)
        {
            res[i] = data[i];
        }
        *resource = res;
    }

    template <typename T, size_t n>
    void AttributeTypes<std::array<T, n> >::createAttribute(
        adios2::IO &IO,
        adios2::Engine &engine,
        detail::BufferedAttributeWrite &params,
        const std::array<T, n> &value)
    {
        auto attr = IO.InquireVariable<T>(params.name);
        // @todo check size
        if (!attr)
        {
            attr = IO.DefineVariable<T>(params.name, {n}, {0}, {n});
        }
        if (!attr)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: Failed defining variable '" +
                params.name + "'.");
        }
        engine.Put(attr, value.data(), adios2::Mode::Deferred);
    }

    template <typename T, size_t n>
    void AttributeTypes<std::array<T, n> >::readAttribute(
        detail::PreloadAdiosAttributes const &preloadedAttributes,
        std::string name,
        std::shared_ptr<Attribute::resource> resource)
    {
        detail::AttributeWithShape<T> attr =
            preloadedAttributes.getAttribute<T>(name);
        if (attr.shape.size() != 1 || attr.shape[0] != n)
        {
            throw std::runtime_error(
                "[ADIOS2] Expecting 1D ADIOS variable of extent " +
                std::to_string(n));
        }

        std::array<T, n> res;
        std::copy_n(attr.data, n, res.data());
        *resource = std::move(res);
    }

    void AttributeTypes<bool>::oldCreateAttribute(
        adios2::IO &IO, std::string name, const bool value)
    {
        IO.DefineAttribute<bool_representation>(
            ADIOS2Defaults::str_isBooleanOldLayout + name, 1);
        AttributeTypes<bool_representation>::oldCreateAttribute(
            IO, name, toRep(value));
    }

    void AttributeTypes<bool>::oldReadAttribute(
        adios2::IO &IO,
        std::string name,
        std::shared_ptr<Attribute::resource> resource)
    {
        auto attr = IO.InquireAttribute<rep>(name);
        if (!attr)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: Failed reading attribute '" + name +
                "'.");
        }
        *resource = fromRep(attr.Data()[0]);
    }

    void AttributeTypes<bool>::createAttribute(
        adios2::IO &IO,
        adios2::Engine &engine,
        detail::BufferedAttributeWrite &params,
        const bool value)
    {
        IO.DefineAttribute<bool_representation>(
            ADIOS2Defaults::str_isBooleanNewLayout + params.name, 1);
        AttributeTypes<bool_representation>::createAttribute(
            IO, engine, params, toRep(value));
    }

    void AttributeTypes<bool>::readAttribute(
        detail::PreloadAdiosAttributes const &preloadedAttributes,
        std::string name,
        std::shared_ptr<Attribute::resource> resource)
    {
        detail::AttributeWithShape<rep> attr =
            preloadedAttributes.getAttribute<rep>(name);
        if (!(attr.shape.size() == 0 ||
              (attr.shape.size() == 1 && attr.shape[0] == 1)))
        {
            throw std::runtime_error(
                "[ADIOS2] Expecting scalar ADIOS variable, got " +
                std::to_string(attr.shape.size()) + "D: " + name);
        }

        *resource = fromRep(*attr.data);
    }

    void BufferedGet::run(BufferedActions &ba)
    {
        switchAdios2VariableType(
            param.dtype,
            ba.m_readDataset,
            *this,
            ba.m_IO,
            ba.getEngine(),
            ba.m_file);
    }

    void BufferedPut::run(BufferedActions &ba)
    {
        switchAdios2VariableType(
            param.dtype, ba.m_writeDataset, *this, ba.m_IO, ba.getEngine());
    }

    void OldBufferedAttributeRead::run(BufferedActions &ba)
    {
        auto type = attributeInfo(ba.m_IO, name, /* verbose = */ true);

        if (type == Datatype::UNDEFINED)
        {
            throw std::runtime_error(
                "[ADIOS2] Requested attribute (" + name +
                ") not found in backend.");
        }

        Datatype ret = switchType(
            type, detail::OldAttributeReader{}, ba.m_IO, name, param.resource);
        *param.dtype = ret;
    }

    void BufferedAttributeRead::run(BufferedActions &ba)
    {
        auto type = attributeInfo(
            ba.m_IO,
            name,
            /* verbose = */ true,
            VariableOrAttribute::Variable);

        if (type == Datatype::UNDEFINED)
        {
            throw std::runtime_error(
                "[ADIOS2] Requested attribute (" + name +
                ") not found in backend.");
        }

        Datatype ret = switchType(
            type,
            detail::AttributeReader{},
            ba.m_IO,
            ba.preloadAttributes,
            name,
            param.resource);
        *param.dtype = ret;
    }

    void BufferedAttributeWrite::run(BufferedActions &fileData)
    {
        switchType(dtype, detail::AttributeWriter(), *this, fileData);
    }

    BufferedActions::BufferedActions(
        ADIOS2IOHandlerImpl &impl, InvalidatableFile file)
        : m_file(impl.fullPath(std::move(file)))
        , m_IOName(std::to_string(impl.nameCounter++))
        , m_ADIOS(impl.m_ADIOS)
        , m_IO(impl.m_ADIOS.DeclareIO(m_IOName))
        , m_mode(impl.adios2AccessMode(m_file))
        , m_writeDataset(&impl)
        , m_readDataset(&impl)
        , m_attributeReader()
        , m_impl(&impl)
        , m_engineType(impl.m_engineType)
    {
        if (!m_IO)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: Failed declaring ADIOS2 IO object "
                "for file " +
                m_file);
        }
        else
        {
            configure_IO(impl);
        }
    }

    BufferedActions::~BufferedActions()
    {
        finalize();
    }

    void BufferedActions::finalize()
    {
        if (finalized)
        {
            return;
        }
        // if write accessing, ensure that the engine is opened
        // and that all attributes are written
        // (attributes are written upon closing a step or a file
        // which users might never do)
        bool needToWriteAttributes = !m_attributeWrites.empty();
        if ((needToWriteAttributes || !m_engine) &&
            m_mode != adios2::Mode::Read)
        {
            auto &engine = getEngine();
            if (needToWriteAttributes)
            {
                for (auto &pair : m_attributeWrites)
                {
                    pair.second.run(*this);
                }
                engine.PerformPuts();
            }
        }
        if (m_engine)
        {
            auto &engine = m_engine.get();
            // might have been closed previously
            if (engine)
            {
                if (streamStatus == StreamStatus::DuringStep)
                {
                    engine.EndStep();
                }
                engine.Close();
                m_ADIOS.RemoveIO(m_IOName);
            }
        }
        finalized = true;
    }

    void BufferedActions::configure_IO(ADIOS2IOHandlerImpl &impl)
    {
        (void)impl;
        static std::set<std::string> streamingEngines = {
            "sst", "insitumpi", "inline", "staging", "nullcore", "ssc"};
        static std::set<std::string> fileEngines = {
            "bp4", "bp3", "hdf5", "file"};

        // step/variable-based iteration encoding requires the new schema
        if (m_impl->m_iterationEncoding == IterationEncoding::variableBased)
        {
            m_impl->m_schema = ADIOS2Schema::schema_2021_02_09;
        }

        // set engine type
        bool isStreaming = false;
        {
            // allow overriding through environment variable
            m_engineType =
                auxiliary::getEnvString("OPENPMD_ADIOS2_ENGINE", m_engineType);
            std::transform(
                m_engineType.begin(),
                m_engineType.end(),
                m_engineType.begin(),
                [](unsigned char c) { return std::tolower(c); });
            impl.m_engineType = this->m_engineType;
            m_IO.SetEngine(m_engineType);
            auto it = streamingEngines.find(m_engineType);
            if (it != streamingEngines.end())
            {
                isStreaming = true;
                optimizeAttributesStreaming =
                    schema() == SupportedSchema::s_0000_00_00;
                streamStatus = StreamStatus::OutsideOfStep;
            }
            else
            {
                it = fileEngines.find(m_engineType);
                if (it != fileEngines.end())
                {
                    switch (m_mode)
                    {
                    case adios2::Mode::Read:
                        /*
                         * File engines, read mode:
                         * Use of steps is dictated by what is detected in the
                         * file being read.
                         */
                        streamStatus = StreamStatus::Undecided;
                        // @todo no?? should be default in both modes
                        delayOpeningTheFirstStep = true;
                        break;
                    case adios2::Mode::Write:
                        /*
                         * File engines, write mode:
                         * Default for old layout is no steps.
                         * Default for new layout is to use steps.
                         */
                        switch (schema())
                        {
                        case SupportedSchema::s_0000_00_00:
                            streamStatus = StreamStatus::NoStream;
                            break;
                        case SupportedSchema::s_2021_02_09:
                            streamStatus = StreamStatus::OutsideOfStep;
                            break;
                        }
                        break;
                    default:
                        throw std::runtime_error("Unreachable!");
                    }
                    optimizeAttributesStreaming = false;
                }
                else
                {
                    throw std::runtime_error(
                        "[ADIOS2IOHandler] Unknown engine type. Please choose "
                        "one out of "
                        "[sst, staging, bp4, bp3, hdf5, file, null]");
                    // not listing unsupported engines
                }
            }
        }

        // set engine parameters
        std::set<std::string> alreadyConfigured;
        auto engineConfig = impl.config(ADIOS2Defaults::str_engine);
        if (!engineConfig.json().is_null())
        {
            auto params = impl.config(ADIOS2Defaults::str_params, engineConfig);
            params.declareFullyRead();
            if (params.json().is_object())
            {
                for (auto it = params.json().begin(); it != params.json().end();
                     it++)
                {
                    m_IO.SetParameter(it.key(), it.value());
                    alreadyConfigured.emplace(it.key());
                }
            }
            auto _useAdiosSteps =
                impl.config(ADIOS2Defaults::str_usesteps, engineConfig);
            if (!_useAdiosSteps.json().is_null() &&
                m_mode != adios2::Mode::Read)
            {
                bool tmp = _useAdiosSteps.json();
                if (isStreaming && !bool(tmp))
                {
                    throw std::runtime_error(
                        "Cannot switch off steps for streaming engines.");
                }
                streamStatus = bool(tmp) ? StreamStatus::OutsideOfStep
                                         : StreamStatus::NoStream;
            }
        }

        auto shadow = impl.m_config.invertShadow();
        if (shadow.size() > 0)
        {
            std::cerr << "Warning: parts of the JSON configuration for ADIOS2 "
                         "remain unused:\n"
                      << shadow << std::endl;
        }
        auto notYetConfigured = [&alreadyConfigured](std::string const &param) {
            auto it = alreadyConfigured.find(param);
            return it == alreadyConfigured.end();
        };

        // read parameters from environment
        if (notYetConfigured("CollectiveMetadata"))
        {
            if (1 ==
                auxiliary::getEnvNum("OPENPMD_ADIOS2_HAVE_METADATA_FILE", 1))
            {
                m_IO.SetParameter("CollectiveMetadata", "On");
            }
            else
            {
                m_IO.SetParameter("CollectiveMetadata", "Off");
            }
        }
        if (notYetConfigured("Profile"))
        {
            if (1 == auxiliary::getEnvNum("OPENPMD_ADIOS2_HAVE_PROFILING", 1) &&
                notYetConfigured("Profile"))
            {
                m_IO.SetParameter("Profile", "On");
            }
            else
            {
                m_IO.SetParameter("Profile", "Off");
            }
        }
#if openPMD_HAVE_MPI
        {
            auto num_substreams =
                auxiliary::getEnvNum("OPENPMD_ADIOS2_NUM_SUBSTREAMS", 0);
            if (notYetConfigured("SubStreams") && 0 != num_substreams)
            {
                m_IO.SetParameter("SubStreams", std::to_string(num_substreams));
            }
        }
#endif
        if (notYetConfigured("StatsLevel"))
        {
            /*
             * Switch those off by default since they are expensive to compute
             * and to enable it, set the JSON option "StatsLevel" or the
             * environment variable "OPENPMD_ADIOS2_STATS_LEVEL" be positive.
             * The ADIOS2 default was "1" (on).
             */
            auto stats_level =
                auxiliary::getEnvNum("OPENPMD_ADIOS2_STATS_LEVEL", 0);
            m_IO.SetParameter("StatsLevel", std::to_string(stats_level));
        }
        if (m_engineType == "sst" && notYetConfigured("QueueLimit"))
        {
            /*
             * By default, the SST engine of ADIOS2 does not set a limit on its
             * internal queue length.
             * If the reading end is slower than the writing end, this will
             * lead to a congestion in the queue and hence an increasing
             * memory usage while the writing code goes forward.
             * We could set a default queue limit of 1, thus forcing the
             * two codes to proceed entirely in lock-step.
             * We prefer a default queue limit of 2, which is still lower than
             * the default infinity, but allows writer and reader to process
             * data asynchronously as long as neither code fails to keep up the
             * rhythm. The writer can produce the next iteration while the
             * reader still deals with the old one.
             * Thus, a limit of 2 is a good balance between 1 and infinity,
             * keeping pipeline parallelism a default without running the risk
             * of using unbound memory.
             */
            m_IO.SetParameter("QueueLimit", "2");
        }

        // We need to open the engine now already to inquire configuration
        // options stored in there
        getEngine();
    }

    adios2::Engine &BufferedActions::getEngine()
    {
        if (!m_engine)
        {
            switch (m_mode)
            {
            case adios2::Mode::Write: {
                // usesSteps attribute only written upon ::advance()
                // this makes sure that the attribute is only put in case
                // the streaming API was used.
                m_IO.DefineAttribute<ADIOS2Schema::schema_t>(
                    ADIOS2Defaults::str_adios2Schema, m_impl->m_schema);
                m_engine = auxiliary::makeOption(
                    adios2::Engine(m_IO.Open(m_file, m_mode)));
                break;
            }
            case adios2::Mode::Read: {
                m_engine = auxiliary::makeOption(
                    adios2::Engine(m_IO.Open(m_file, m_mode)));
                // decide attribute layout
                // in streaming mode, this needs to be done after opening
                // a step
                // in file-based mode, we do it before
                auto layoutVersion = [IO{m_IO}]() mutable {
                    auto attr = IO.InquireAttribute<ADIOS2Schema::schema_t>(
                        ADIOS2Defaults::str_adios2Schema);
                    if (!attr)
                    {
                        return ADIOS2Schema::schema_0000_00_00;
                    }
                    else
                    {
                        return attr.Data()[0];
                    }
                };
                // decide streaming mode
                switch (streamStatus)
                {
                case StreamStatus::Undecided: {
                    m_impl->m_schema = layoutVersion();
                    auto attr = m_IO.InquireAttribute<bool_representation>(
                        ADIOS2Defaults::str_usesstepsAttribute);
                    if (attr && attr.Data()[0] == 1)
                    {
                        if (delayOpeningTheFirstStep)
                        {
                            streamStatus = StreamStatus::Parsing;
                        }
                        else
                        {
                            if (m_engine.get().BeginStep() !=
                                adios2::StepStatus::OK)
                            {
                                throw std::runtime_error(
                                    "[ADIOS2] Unexpected step status when "
                                    "opening file/stream.");
                            }
                            streamStatus = StreamStatus::DuringStep;
                        }
                    }
                    else
                    {
                        streamStatus = StreamStatus::NoStream;
                    }
                    break;
                }
                case StreamStatus::OutsideOfStep:
                    if (m_engine.get().BeginStep() != adios2::StepStatus::OK)
                    {
                        throw std::runtime_error(
                            "[ADIOS2] Unexpected step status when "
                            "opening file/stream.");
                    }
                    m_impl->m_schema = layoutVersion();
                    streamStatus = StreamStatus::DuringStep;
                    break;
                default:
                    throw std::runtime_error("[ADIOS2] Control flow error!");
                }
                if (attributeLayout() == AttributeLayout::ByAdiosVariables)
                {
                    preloadAttributes.preloadAttributes(m_IO, m_engine.get());
                }
                break;
            }
            default:
                throw std::runtime_error("[ADIOS2] Invalid ADIOS access mode");
            }

            if (!m_engine.get())
            {
                throw std::runtime_error("[ADIOS2] Failed opening Engine.");
            }
        }
        return m_engine.get();
    }

    adios2::Engine &BufferedActions::requireActiveStep()
    {
        adios2::Engine &eng = getEngine();
        if (streamStatus == StreamStatus::OutsideOfStep)
        {
            m_lastStepStatus = eng.BeginStep();
            if (m_mode == adios2::Mode::Read &&
                attributeLayout() == AttributeLayout::ByAdiosVariables)
            {
                preloadAttributes.preloadAttributes(m_IO, m_engine.get());
            }
            streamStatus = StreamStatus::DuringStep;
        }
        return eng;
    }

    template <typename BA>
    void BufferedActions::enqueue(BA &&ba)
    {
        enqueue<BA>(std::forward<BA>(ba), m_buffer);
    }

    template <typename BA>
    void BufferedActions::enqueue(BA &&ba, decltype(m_buffer) &buffer)
    {
        using _BA = typename std::remove_reference<BA>::type;
        buffer.emplace_back(
            std::unique_ptr<BufferedAction>(new _BA(std::forward<BA>(ba))));
    }

    template <typename F>
    void BufferedActions::flush(
        FlushLevel level,
        F &&performPutGets,
        bool writeAttributes,
        bool flushUnconditionally)
    {
        if (streamStatus == StreamStatus::StreamOver)
        {
            if (flushUnconditionally)
            {
                throw std::runtime_error(
                    "[ADIOS2] Cannot access engine since stream is over.");
            }
            return;
        }
        auto &eng = getEngine();
        /*
         * Only open a new step if it is necessary.
         */
        if (streamStatus == StreamStatus::OutsideOfStep)
        {
            if (m_buffer.empty() &&
                (!writeAttributes || m_attributeWrites.empty()) &&
                m_attributeReads.empty())
            {
                if (flushUnconditionally)
                {
                    performPutGets(*this, eng);
                }
                return;
            }
            else
            {
                requireActiveStep();
            }
        }
        for (auto &ba : m_buffer)
        {
            ba->run(*this);
        }

        if (writeAttributes)
        {
            for (auto &pair : m_attributeWrites)
            {
                pair.second.run(*this);
            }
        }

        if (this->m_mode == adios2::Mode::Read)
        {
            level = FlushLevel::UserFlush;
        }

        switch (level)
        {
        case FlushLevel::UserFlush:
            performPutGets(*this, eng);
            m_updateSpans.clear();
            m_buffer.clear();
            m_alreadyEnqueued.clear();
            if (writeAttributes)
            {
                m_attributeWrites.clear();
            }

            for (BufferedAttributeRead &task : m_attributeReads)
            {
                task.run(*this);
            }
            m_attributeReads.clear();
            break;

        case FlushLevel::InternalFlush:
        case FlushLevel::SkeletonOnly:
        case FlushLevel::CreateOrOpenFiles:
            /*
             * Tasks have been given to ADIOS2, but we don't flush them
             * yet. So, move everything to m_alreadyEnqueued to avoid
             * use-after-free.
             */
            for (auto &task : m_buffer)
            {
                m_alreadyEnqueued.emplace_back(std::move(task));
            }
            if (writeAttributes)
            {
                for (auto &task : m_attributeWrites)
                {
                    m_alreadyEnqueued.emplace_back(
                        std::unique_ptr<BufferedAction>{
                            new BufferedAttributeWrite{
                                std::move(task.second)}});
                }
                m_attributeWrites.clear();
            }
            m_buffer.clear();
            break;
        }
    }

    void BufferedActions::flush(FlushLevel level, bool writeAttributes)
    {
        flush(
            level,
            [](BufferedActions &ba, adios2::Engine &eng) {
                switch (ba.m_mode)
                {
                case adios2::Mode::Write:
                    eng.PerformPuts();
                    break;
                case adios2::Mode::Read:
                    eng.PerformGets();
                    break;
                case adios2::Mode::Append:
                    // TODO order?
                    eng.PerformGets();
                    eng.PerformPuts();
                    break;
                default:
                    break;
                }
            },
            writeAttributes,
            /* flushUnconditionally = */ false);
    }

    AdvanceStatus BufferedActions::advance(AdvanceMode mode)
    {
        if (streamStatus == StreamStatus::Undecided)
        {
            // stream status gets decided on upon opening an engine
            getEngine();
        }
        // sic! no else
        if (streamStatus == StreamStatus::NoStream)
        {
            m_IO.DefineAttribute<bool_representation>(
                ADIOS2Defaults::str_usesstepsAttribute, 0);
            flush(FlushLevel::UserFlush, /* writeAttributes = */ false);
            return AdvanceStatus::OK;
        }

        m_IO.DefineAttribute<bool_representation>(
            ADIOS2Defaults::str_usesstepsAttribute, 1);
        switch (mode)
        {
        case AdvanceMode::ENDSTEP: {
            /*
             * Advance mode write:
             * Close the current step, defer opening the new step
             * until one is actually needed:
             * (1) The engine is accessed in BufferedActions::flush
             * (2) A new step is opened before the currently active step
             *     has seen an access. See the following lines: open the
             *     step just to skip it again.
             */
            if (streamStatus == StreamStatus::OutsideOfStep)
            {
                if (getEngine().BeginStep() != adios2::StepStatus::OK)
                {
                    throw std::runtime_error(
                        "[ADIOS2] Trying to close a step that cannot be "
                        "opened.");
                }
            }
            flush(
                FlushLevel::UserFlush,
                [](BufferedActions &, adios2::Engine &eng) { eng.EndStep(); },
                /* writeAttributes = */ true,
                /* flushUnconditionally = */ true);
            uncommittedAttributes.clear();
            m_updateSpans.clear();
            streamStatus = StreamStatus::OutsideOfStep;
            return AdvanceStatus::OK;
        }
        case AdvanceMode::BEGINSTEP: {
            adios2::StepStatus adiosStatus = m_lastStepStatus;

            // Step might have been opened implicitly already
            // by requireActiveStep()
            // In that case, streamStatus is DuringStep and Adios
            // return status is stored in m_lastStepStatus
            if (streamStatus != StreamStatus::DuringStep)
            {
                flush(
                    FlushLevel::UserFlush,
                    [&adiosStatus](BufferedActions &, adios2::Engine &engine) {
                        adiosStatus = engine.BeginStep();
                    },
                    /* writeAttributes = */ false,
                    /* flushUnconditionally = */ true);
                if (adiosStatus == adios2::StepStatus::OK &&
                    m_mode == adios2::Mode::Read &&
                    attributeLayout() == AttributeLayout::ByAdiosVariables)
                {
                    preloadAttributes.preloadAttributes(m_IO, m_engine.get());
                }
            }
            AdvanceStatus res = AdvanceStatus::OK;
            switch (adiosStatus)
            {
            case adios2::StepStatus::EndOfStream:
                streamStatus = StreamStatus::StreamOver;
                res = AdvanceStatus::OVER;
                break;
            case adios2::StepStatus::OK:
                streamStatus = StreamStatus::DuringStep;
                res = AdvanceStatus::OK;
                break;
            case adios2::StepStatus::NotReady:
            case adios2::StepStatus::OtherError:
                throw std::runtime_error("[ADIOS2] Unexpected step status.");
            }
            invalidateAttributesMap();
            invalidateVariablesMap();
            return res;
        }
        }
        throw std::runtime_error(
            "Internal error: Advance mode should be explicitly"
            " chosen by the front-end.");
    }

    void BufferedActions::drop()
    {
        m_buffer.clear();
    }

    static std::vector<std::string> availableAttributesOrVariablesPrefixed(
        std::string const &prefix,
        BufferedActions::AttributeMap_t const &(
            BufferedActions::*getBasicMap)(),
        BufferedActions &ba)
    {
        std::string var =
            auxiliary::ends_with(prefix, '/') ? prefix : prefix + '/';
        BufferedActions::AttributeMap_t const &attributes = (ba.*getBasicMap)();
        std::vector<std::string> ret;
        for (auto it = attributes.lower_bound(prefix); it != attributes.end();
             ++it)
        {
            if (auxiliary::starts_with(it->first, var))
            {
                ret.emplace_back(auxiliary::replace_first(it->first, var, ""));
            }
            else
            {
                break;
            }
        }
        return ret;
    }

    std::vector<std::string>
    BufferedActions::availableAttributesPrefixed(std::string const &prefix)
    {
        return availableAttributesOrVariablesPrefixed(
            prefix, &BufferedActions::availableAttributes, *this);
    }

    std::vector<std::string>
    BufferedActions::availableVariablesPrefixed(std::string const &prefix)
    {
        return availableAttributesOrVariablesPrefixed(
            prefix, &BufferedActions::availableVariables, *this);
    }

    void BufferedActions::invalidateAttributesMap()
    {
        m_availableAttributes = auxiliary::Option<AttributeMap_t>();
    }

    BufferedActions::AttributeMap_t const &
    BufferedActions::availableAttributes()
    {
        if (m_availableAttributes)
        {
            return m_availableAttributes.get();
        }
        else
        {
            m_availableAttributes =
                auxiliary::makeOption(m_IO.AvailableAttributes());
            return m_availableAttributes.get();
        }
    }

    void BufferedActions::invalidateVariablesMap()
    {
        m_availableVariables = auxiliary::Option<AttributeMap_t>();
    }

    BufferedActions::AttributeMap_t const &BufferedActions::availableVariables()
    {
        if (m_availableVariables)
        {
            return m_availableVariables.get();
        }
        else
        {
            m_availableVariables =
                auxiliary::makeOption(m_IO.AvailableVariables());
            return m_availableVariables.get();
        }
    }

} // namespace detail

#if openPMD_HAVE_MPI

ADIOS2IOHandler::ADIOS2IOHandler(
    std::string path,
    openPMD::Access at,
    MPI_Comm comm,
    nlohmann::json options,
    std::string engineType)
    : AbstractIOHandler(std::move(path), at, comm)
    , m_impl{this, comm, std::move(options), std::move(engineType)}
{}

#endif

ADIOS2IOHandler::ADIOS2IOHandler(
    std::string path, Access at, nlohmann::json options, std::string engineType)
    : AbstractIOHandler(std::move(path), at)
    , m_impl{this, std::move(options), std::move(engineType)}
{}

std::future<void>
ADIOS2IOHandler::flush(internal::FlushParams const &flushParams)
{
    return m_impl.flush(flushParams);
}

#else // openPMD_HAVE_ADIOS2

#if openPMD_HAVE_MPI
ADIOS2IOHandler::ADIOS2IOHandler(
    std::string path, Access at, MPI_Comm comm, nlohmann::json, std::string)
    : AbstractIOHandler(std::move(path), at, comm)
{}

#endif // openPMD_HAVE_MPI

ADIOS2IOHandler::ADIOS2IOHandler(
    std::string path, Access at, nlohmann::json, std::string)
    : AbstractIOHandler(std::move(path), at)
{}

std::future<void> ADIOS2IOHandler::flush(internal::FlushParams const &)
{
    return std::future<void>();
}

#endif

} // namespace openPMD
