/* Copyright 2019 Axel Huebl
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


namespace openPMD
{
    /** Data flush mode.
     *
     * Control if RecordComponent::load_chunk and RecordComponent::store_chunk
     * operations are flushed immediately or delayed until Series::flush
     *
     * DIRECT-ly populating passed data buffers is considered a safe and
     * convienient default for all small-scale I/O operations. For large-scale
     * IO or load/store of many small chunks, consider switching to DEFER
     * mode and ensure passed data buffers are valid and unmodified until
     * Series::flush is called.
     */
    enum class FlushType //! @todo we should call this AccessMode and FlushMode ...
    {
        DIRECT,  //!< immediately load/store data to passed data buffers (default)
        DEFER,   //!< load/store data to passed data buffers on Series::flush
    }; // FlushType
} // namespace openPMD
