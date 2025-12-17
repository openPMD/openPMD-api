/* Copyright 2017-2025 Fabian Koller, Franz Poeschel, Axel Huebl, Junmin Gu
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

#include "openPMD/Datatype.hpp"

#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace openPMD
{
using Extent = std::vector<std::uint64_t>;
using Offset = std::vector<std::uint64_t>;

class Dataset
{
    friend class RecordComponent;

public:
    enum : std::uint64_t
    {
        /**
         * Setting one dimension of the extent as JOINED_DIMENSION means that
         * the extent along that dimension will be defined by the sum of all
         * parallel processes' contributions.
         * Only one dimension can be joined. For store operations, the offset
         * should be an empty array and the extent should give the actual
         * extent of the chunk (i.e. the number of joined elements along the
         * joined dimension, equal to the global extent in all other
         * dimensions). For more details, refer to
         * docs/source/usage/workflow.rst.
         */
        JOINED_DIMENSION = std::numeric_limits<std::uint64_t>::max(),
        /**
         * In some use cases, the extent needs not be specified.
         * For these, specify Extent{UNDEFINED_EXTENT}.
         * Use cases:
         *
         * 1. Some backends (i.e. JSON and TOML in template mode) support the
         *    creation of datasets with undefined datatype and extent.
         *    The extent should be given as {UNDEFINED_EXTENT} for that.
         * 2. With openPMD 2.0, the shape of constant components may be omitted
         *    in writing if it is defined somewhere else as part
         *    of the same Mesh / Species.
         *    (https://github.com/openPMD/openPMD-standard/pull/289)
         *    When reading such datasets, the openPMD-api will try to fill in
         *    the missing extents, so the extent for constistently-defined
         *    datasets should ideally not be reported by the read-side API
         *    as undefined.
         */
        UNDEFINED_EXTENT = std::numeric_limits<std::uint64_t>::max() - 1
    };

    Dataset(Datatype, Extent, std::string options = "{}");

    /**
     * @brief Constructor that sets the datatype to undefined.
     *
     * Helpful for:
     *
     * 1. Resizing datasets, since datatypes need not be given twice.
     * 2. Initializing datasets as undefined, as used by template mode in the
     *    JSON/TOML backend. In this case, the default (undefined) specification
     *    for the Extent may be used.
     *
     */
    Dataset(Extent = {UNDEFINED_EXTENT});

    Dataset &extend(Extent newExtent);

    Extent extent;
    Datatype dtype;
    uint8_t rank;
    std::string options = "{}"; //!< backend-dependent JSON configuration

    bool empty() const;

    std::optional<size_t> joinedDimension() const;
    static std::optional<size_t> joinedDimension(Extent const &);

    bool undefinedExtent() const;
    static bool undefinedExtent(Extent const &);
};
} // namespace openPMD
