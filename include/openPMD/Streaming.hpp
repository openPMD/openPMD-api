/* Copyright 2025 Franz Poeschel
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
/**
 * In step-based mode (i.e. when using the Streaming API),
 * indicate whether there are further steps to read or not
 * (i.e. the stream is not over or it is).
 *
 */
enum class AdvanceStatus : unsigned char
{
    OK, ///< stream goes on
    OVER, ///< stream is over
    RANDOMACCESS ///< there is no stream, it will never be over
};

/**
 * In step-based mode (i.e. when using the Streaming API),
 * stepping/advancing through the Series is performed in terms
 * of interleaving begin- and end-step calls.
 * Distinguish both kinds by using this enum.
 *
 */
enum class AdvanceMode : unsigned char
{
    BEGINSTEP,
    ENDSTEP
};

/**
 * Used in step-based mode (i.e. when using the Streaming API)
 * to determine whether a step is currently active or not.
 *
 */
enum class StepStatus : unsigned char
{
    DuringStep, /* step is currently active */
    OutOfStep, /* steps used, but currently no step active */
    NoStep /* no step is currently active */
};
} // namespace openPMD
