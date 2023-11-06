#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <type_traits>

namespace openPMD
{

namespace auxiliary
{
    /**
     * @brief Custom deleter type based on std::function.
     *
     * No need to interact with this class directly, used implicitly
     * by UniquePtrWithLambda.
     *
     * Has some special treatment for array types and falls back
     * to std::default_delete by default.
     *
     * @tparam T The to-be-deleted type, possibly an array.
     */
    template <typename T>
    class CustomDelete : public std::function<void(std::remove_extent_t<T> *)>
    {
    private:
        using T_decayed = std::remove_extent_t<T>;

    public:
        using deleter_type = std::function<void(T_decayed *)>;

        /*
         * Default constructor: Use std::default_delete<T>.
         * This ensures correct destruction of arrays by using delete[].
         */
        CustomDelete()
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

        CustomDelete(deleter_type func) : deleter_type(std::move(func))
        {}
    };
} // namespace auxiliary

/**
 * @brief Unique Pointer class that uses a dynamic destructor type.
 *
 * Unlike std::shared_ptr, std::unique_ptr has a second type parameter for the
 * destructor, in order to have as little runtime overhead as possible over
 * raw pointers.
 * This unique pointer class behaves like a std::unique_ptr with a std::function
 * based deleter type, making it possible to have one single unique_ptr-like
 * class that still enables user to specify custom destruction behavior, e.g.
 * for GPU buffers.
 *
 * If not specifying a custom deleter explicitly, this class emulates the
 * behavior of a std::unique_ptr with std::default_delete.
 * This also means that array types are supported as expected.
 *
 * @tparam T The pointer type, as in std::unique_ptr.
 */
template <typename T>
class UniquePtrWithLambda
    : public std::unique_ptr<
          T,
          /* Deleter = */ auxiliary::CustomDelete<T>>
{
private:
    using BasePtr = std::unique_ptr<T, auxiliary::CustomDelete<T>>;

public:
    using T_decayed = std::remove_extent_t<T>;

    UniquePtrWithLambda() = default;

    UniquePtrWithLambda(UniquePtrWithLambda &&) = default;
    UniquePtrWithLambda &operator=(UniquePtrWithLambda &&) = default;

    UniquePtrWithLambda(UniquePtrWithLambda const &) = delete;
    UniquePtrWithLambda &operator=(UniquePtrWithLambda const &) = delete;

    /**
     * Conversion constructor from std::unique_ptr<T> with default deleter.
     */
    UniquePtrWithLambda(std::unique_ptr<T>);

    /**
     * Conversion constructor from std::unique_ptr<T> with custom deleter.
     *
     * @tparam Del Custom deleter type.
     */
    template <typename Del>
    UniquePtrWithLambda(std::unique_ptr<T, Del>);

    /**
     * Construct from raw pointer with default deleter.
     */
    UniquePtrWithLambda(T_decayed *);

    /**
     * Construct from raw pointer with custom deleter.
     */
    UniquePtrWithLambda(T_decayed *, std::function<void(T_decayed *)>);

    /**
     * Like std::static_pointer_cast.
     * The dynamic destructor type makes this possible to implement in this
     * case.
     *
     * @tparam U Convert to unique pointer of this type.
     */
    template <typename U>
    UniquePtrWithLambda<U> static_cast_() &&;
};

template <typename T>
UniquePtrWithLambda<T>::UniquePtrWithLambda(std::unique_ptr<T> stdPtr)
    : BasePtr{stdPtr.release()}
{}

template <typename T>
template <typename Del>
UniquePtrWithLambda<T>::UniquePtrWithLambda(std::unique_ptr<T, Del> ptr)
    : BasePtr{ptr.release(), auxiliary::CustomDelete<T>{[&]() {
                  if constexpr (std::is_copy_constructible_v<Del>)
                  {
                      return [deleter = std::move(ptr.get_deleter())](
                                 T_decayed *del_ptr) { deleter(del_ptr); };
                  }
                  else
                  {
                      /*
                       * The constructor of std::function requires a copyable
                       * lambda. Since Del is not a copyable type, we cannot
                       * capture it directly, but need to put it into a
                       * shared_ptr to make it copyable.
                       */
                      return [deleter = std::make_shared<Del>(
                                  std::move(ptr.get_deleter()))](
                                 T_decayed *del_ptr) { (*deleter)(del_ptr); };
                  }
              }()}}
{}

template <typename T>
UniquePtrWithLambda<T>::UniquePtrWithLambda(T_decayed *ptr) : BasePtr{ptr}
{}

template <typename T>
UniquePtrWithLambda<T>::UniquePtrWithLambda(
    T_decayed *ptr, std::function<void(T_decayed *)> deleter)
    : BasePtr{ptr, std::move(deleter)}
{}

template <typename T>
template <typename U>
UniquePtrWithLambda<U> UniquePtrWithLambda<T>::static_cast_() &&
{
    using other_type = std::remove_extent_t<U>;
    return UniquePtrWithLambda<U>{
        static_cast<other_type *>(this->release()),
        [deleter = std::move(this->get_deleter())](other_type *ptr) {
            deleter(static_cast<T_decayed *>(ptr));
        }};
}
} // namespace openPMD
