/* Copyright 2024-2025 Franz Poeschel
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

#include "openPMD/auxiliary/UniquePtr.hpp"

#include "openPMD/DatatypeMacros.hpp"

namespace openPMD
{

namespace auxiliary
{
    template <typename T>
    CustomDelete<T>::CustomDelete()
        : deleter_type{[]([[maybe_unused]] T_decayed *ptr) {
            if constexpr (std::is_void_v<T_decayed>)
            {
                std::cerr << "[Warning] Cannot standard-delete a void-type "
                             "pointer. Please specify a custom destructor. "
                             "Will let the memory leak."
                          << std::endl;
            }
            else
            {
                std::default_delete<T>{}(ptr);
            }
        }}
    {}

    template <typename T>
    CustomDelete<T>::CustomDelete(deleter_type func)
        : deleter_type(std::move(func))
    {}

#define OPENPMD_INSTANTIATE(type) template class CustomDelete<type>;
// need this for clang-tidy
#define OPENPMD_ARRAY(type) type[]
#define OPENPMD_INSTANTIATE_WITH_AND_WITHOUT_EXTENT(type)                      \
    OPENPMD_INSTANTIATE(type) OPENPMD_INSTANTIATE(OPENPMD_ARRAY(type))

    OPENPMD_FOREACH_DATASET_DATATYPE(
        OPENPMD_INSTANTIATE_WITH_AND_WITHOUT_EXTENT)
    OPENPMD_INSTANTIATE(void)
#undef OPENPMD_INSTANTIATE
#undef OPENPMD_INSTANTIATE_WITH_AND_WITHOUT_EXTENT

} // namespace auxiliary

template <typename T>
#ifdef __HIPCC__ // ROCm 6.2.4 issue, see #1797
__host__
#endif
UniquePtrWithLambda<T>::UniquePtrWithLambda() = default;

template <typename T>
UniquePtrWithLambda<T>::UniquePtrWithLambda(UniquePtrWithLambda &&) noexcept =
    default;
template <typename T>
UniquePtrWithLambda<T> &
UniquePtrWithLambda<T>::operator=(UniquePtrWithLambda &&) noexcept = default;

template <typename T>
template <typename bare_unique_ptr, typename SFINAE>
UniquePtrWithLambda<T>::UniquePtrWithLambda(bare_unique_ptr stdPtr)
    : BasePtr{stdPtr.release()}
{}

template <typename T>
UniquePtrWithLambda<T>::UniquePtrWithLambda(T_decayed *ptr) : BasePtr{ptr}
{}

template <typename T>
UniquePtrWithLambda<T>::UniquePtrWithLambda(
    T_decayed *ptr, std::function<void(T_decayed *)> deleter)
    : BasePtr{ptr, std::move(deleter)}
{}

#define OPENPMD_INSTANTIATE(type)                                              \
    template class UniquePtrWithLambda<type>;                                  \
    template UniquePtrWithLambda<type>::UniquePtrWithLambda(                   \
        std::unique_ptr<type>);

#define OPENPMD_INSTANTIATE_WITH_AND_WITHOUT_EXTENT(type)                      \
    OPENPMD_INSTANTIATE(type) OPENPMD_INSTANTIATE(OPENPMD_ARRAY(type))

OPENPMD_FOREACH_DATASET_DATATYPE(OPENPMD_INSTANTIATE_WITH_AND_WITHOUT_EXTENT)
// Instantiate this directly, do not instantiate the
// `std::unique_ptr<void>`-based constructor.
template class UniquePtrWithLambda<void>;
#undef OPENPMD_INSTANTIATE
#undef OPENPMD_INSTANTIATE_WITH_AND_WITHOUT_EXTENT
#undef OPENPMD_ARRAY

} // namespace openPMD
