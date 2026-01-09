/* Copyright 2022 Franz Poeschel
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

#include "openPMD/IO/AbstractIOHandler.hpp"

#include "openPMD/Error.hpp"
#include "openPMD/IO/AbstractIOHandler_internal.hpp"
#include "openPMD/IO/FlushParametersInternal.hpp"
#include "openPMD/auxiliary/JSONMatcher.hpp"

#include <utility>
#include <variant>

namespace openPMD
{
std::ostream &operator<<(std::ostream &os, FlushLevel l)
{
    switch (l)
    {
    case FlushLevel::UserFlush:
        os << "UserFlush";
        break;
    case FlushLevel::InternalFlush:
        os << "InternalFlush";
        break;
    case FlushLevel::SkeletonOnly:
        os << "SkeletonOnly";
        break;
    case FlushLevel::CreateOrOpenFiles:
        os << "CreateOrOpenFiles";
        break;
    }
    return os;
}
} // namespace openPMD

namespace openPMD::auxiliary
{
using pair_t = std::pair<OpenpmdStandard, char const *>;
constexpr pair_t STANDARD_VERSIONS[] = {
    pair_t{OpenpmdStandard::v_1_0_0, "1.0.0"},
    pair_t{OpenpmdStandard::v_1_0_1, "1.0.1"},
    pair_t{OpenpmdStandard::v_1_1_0, "1.1.0"},
    pair_t{OpenpmdStandard::v_2_0_0, "2.0.0"}};

auto parseStandard(const std::string &str) -> OpenpmdStandard
{
    for (auto const &[res, compare] : STANDARD_VERSIONS)
    {
        if (str == compare)
        {
            return res;
        }
    }
    throw error::IllegalInOpenPMDStandard(
        "Standard version is not supported: '" + str + "'.");
}

auto formatStandard(OpenpmdStandard std) -> char const *
{
    for (auto const &[compare, res] : STANDARD_VERSIONS)
    {
        if (std == compare)
        {
            return res;
        }
    }
    throw error::Internal(
        "[auxiliary::formatStandard] Match should be exhaustive.");
}
} // namespace openPMD::auxiliary

namespace openPMD::internal
{
GlobalParameters::GlobalParameters(Access at)
    : m_backendAccess(at), m_frontendAccess(at)
{}
GlobalParameters::GlobalParameters() = default;
} // namespace openPMD::internal

namespace openPMD
{
void AbstractIOHandler::setIterationEncoding(IterationEncoding encoding)
{
    /*
     * In file-based iteration encoding, the APPEND mode is handled entirely
     * by the frontend, the backend should just treat it as CREATE mode.
     * Similar for READ_LINEAR which should be treated as READ_RANDOM_ACCESS
     * in the backend.
     */
    if (encoding == IterationEncoding::fileBased)
    {
        switch (m_backendAccess)
        {

        case Access::READ_LINEAR:
            // do we really want to have those as const members..?
            *const_cast<Access *>(&m_backendAccess) =
                Access::READ_RANDOM_ACCESS;
            break;
        case Access::APPEND_LINEAR:
        case Access::APPEND_RANDOM_ACCESS:
            *const_cast<Access *>(&m_backendAccess) =
                Access::CREATE_RANDOM_ACCESS;
            break;
        case Access::READ_RANDOM_ACCESS:
        case Access::READ_WRITE:
        case Access::CREATE_RANDOM_ACCESS:
        case Access::CREATE_LINEAR:
            break;
        }
    }
    else
    {
        m_backendAccess = m_frontendAccess;
    }

    m_encoding = encoding;
}

std::future<void> AbstractIOHandler::flush(internal::FlushParams const &params)
{
    internal::ParsedFlushParams parsedParams{params};
    auto future = [this, &parsedParams]() {
        try
        {
            return this->flush(parsedParams);
        }
        catch (...)
        {
            m_lastFlushSuccessful = false;
            throw;
        }
    }();
    m_lastFlushSuccessful = true;
    json::warnGlobalUnusedOptions(parsedParams.backendConfig);
    return future;
}

std::future<void> AbstractIOHandler::flush(internal::ParsedFlushParams &params)
{
    // The flush counter indicates the number of times that m_work has been
    // emptied. Only increment it if m_work was full before operation and is
    // empty after operation.
    // Enqueuers can use this counter to check if the enqueued operation has
    // been flushed already.
    bool increase_flush_counter = !m_work.empty();
    auto res = this->flush_impl(params);
    if (!m_work.empty())
    {
        throw error::Internal("flush() did not clear all work!");
    }
    if (increase_flush_counter)
    {
        ++*m_flushCounter;
    }
    return res;
}

bool AbstractIOHandler::fullSupportForVariableBasedEncoding() const
{
    return false;
}

template <>
AbstractIOHandler::AbstractIOHandler(
    detail::InitFrom_Tag, internal::AbstractIOHandlerInitFrom &&initialize_from)
{
    using IF = internal::AbstractIOHandlerInitFrom;
    std::visit(
        auxiliary::overloaded{
            [this](IF::Left &&l) {
                static_cast<internal::GlobalParameters *>(this)->operator=(
                    std::move(l));
            },
            [this](IF::Right &&r) { this->operator=(std::move(*r)); }},
        std::move(initialize_from.as_parent()));
}

#if openPMD_HAVE_MPI
template <>
AbstractIOHandler::AbstractIOHandler(
    internal::AbstractIOHandlerInitFrom &&initialize_from,
    json::TracingJSON &&jsonConfig,
    MPI_Comm)
    : AbstractIOHandler(detail::InitFrom_Tag_v, std::move(initialize_from))
{
    jsonMatcher = std::make_unique<json::JsonMatcher>(std::move(jsonConfig));
}
#endif

template <>
AbstractIOHandler::AbstractIOHandler(
    internal::AbstractIOHandlerInitFrom &&initialize_from,
    json::TracingJSON &&jsonConfig)
    : AbstractIOHandler(detail::InitFrom_Tag_v, std::move(initialize_from))
{
    jsonMatcher = std::make_unique<json::JsonMatcher>(std::move(jsonConfig));
}

AbstractIOHandler::~AbstractIOHandler() = default;
// std::queue::queue(queue&&) is not noexcept
// NOLINTNEXTLINE(performance-noexcept-move-constructor)
AbstractIOHandler::AbstractIOHandler(AbstractIOHandler &&) = default;

AbstractIOHandler &
AbstractIOHandler::operator=(AbstractIOHandler &&) noexcept = default;
} // namespace openPMD

namespace openPMD::internal
{
auto AbstractIOHandlerInitFrom::asGlobalParameters() const
    -> GlobalParameters const &
{
    return std::visit(
        auxiliary::overloaded{
            [](Left const &params) -> GlobalParameters const & {
                // ?? why
                // NOLINTNEXTLINE(bugprone-return-const-ref-from-parameter)
                return params;
            },
            [](Right const &ioHandler) -> GlobalParameters const & {
                return *ioHandler;
            }},
        this->as_parent());
}

auto AbstractIOHandlerInitFrom::asGlobalParameters() -> GlobalParameters &
{
    return const_cast<GlobalParameters &>(
        static_cast<AbstractIOHandlerInitFrom const *>(this)
            ->asGlobalParameters());
}
} // namespace openPMD::internal
