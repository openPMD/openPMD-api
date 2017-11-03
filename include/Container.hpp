#pragma once


#include <string>
#include <unordered_map>

#include "Attributable.hpp"
#include "Writable.hpp"


template<
        typename T,
        typename T_key = std::string
>
class Container : public Attributable
{
    static_assert(std::is_base_of< Writable, T >::value, "Type of container element must be derived from Writable");
    using InternalContainer = std::unordered_map< T_key, T >;

    friend class Iteration;
    friend class Series;

public:
    using key_type = typename InternalContainer::key_type;
    using mapped_type = typename InternalContainer::mapped_type;
    using value_type = typename InternalContainer::value_type;
    using size_type = typename InternalContainer::size_type;
    using difference_type = typename InternalContainer::difference_type;
    using key_equal = typename InternalContainer::key_equal;
    using allocator_type = typename InternalContainer::allocator_type;
    using reference = typename InternalContainer::reference;
    using const_reference = typename InternalContainer::const_reference;
    using pointer = typename InternalContainer::pointer;
    using const_pointer = typename InternalContainer::const_pointer;
    using iterator = typename InternalContainer::iterator;
    using const_iterator = typename InternalContainer::const_iterator;

    virtual ~Container() { }

    iterator begin() noexcept { return m_container.begin(); }
    const_iterator begin() const noexcept { return m_container.begin(); }
    const_iterator cbegin() const noexcept { return m_container.cbegin(); }

    iterator end() noexcept { return m_container.end(); }
    const_iterator end() const noexcept { return m_container.end(); }
    const_iterator cend() const noexcept { return m_container.cend(); }

    bool empty() const noexcept { return m_container.empty(); }

    size_type size() const noexcept { return m_container.size(); }

    void clear()
    {
        if( AccessType::READ_ONLY != IOHandler->accessType )
            clear_unchecked();
        else
            throw std::runtime_error("Can not clear a container in a read-only Series.");
    }

    std::pair< iterator, bool > insert(value_type const& value) { return m_container.insert(value); }
    template< class P >
    std::pair< iterator, bool > insert(P&& value) { return m_container.insert(value); }
    iterator insert(const_iterator hint, value_type const& value) { return m_container.insert(hint, value); }
    template< class P >
    iterator insert(const_iterator hint, P&& value) { return m_container.insert(hint, value); }
    template< class InputIt >
    void insert(InputIt first, InputIt last) { m_container.insert(first, last); }
    void insert(std::initializer_list< value_type > ilist) { m_container.insert(ilist); }

    void swap(Container & other) { m_container.swap(other.m_container); }

    mapped_type& at(key_type const& key) { return m_container.at(key); }
    mapped_type const& at(key_type const& key) const { return m_container.at(key); }

    virtual mapped_type& operator[](key_type const& key)
    {
        auto it = m_container.find(key);
        if( it != m_container.end() )
            return it->second;
        else
        {
            T t = T();
            t.IOHandler = IOHandler;
            t.parent = this;
            return m_container.insert({key, std::move(t)}).first->second;
        }
    }
    virtual mapped_type& operator[](key_type&& key)
    {
        auto it = m_container.find(key);
        if( it != m_container.end() )
            return it->second;
        else
        {
            T t = T();
            t.IOHandler = IOHandler;
            t.parent = this;
            return m_container.insert({std::move(key), std::move(t)}).first->second;
        }
    }

    size_type count(key_type const& key) const { return m_container.count(key); }

    iterator find(key_type const& key) { return m_container.find(key); }
    const_iterator find(key_type const& key) const { return m_container.find(key); }

    virtual size_type erase(key_type const& key)
    {
        auto res = m_container.find(key);
        if( res != m_container.end() && res->second.written )
        {
            Parameter< Operation::DELETE_PATH > path_parameter;
            path_parameter.path = ".";
            IOHandler->enqueue(IOTask(&res->second, path_parameter));
            IOHandler->flush();
        }
        return m_container.erase(key);
    }

protected:
    InternalContainer m_container;

    void clear_unchecked()
    {
        if( written )
            throw std::runtime_error("Clearing a written container not (yet) implemented.");
        m_container.clear();
    }

private:
    void flush(std::string const path)
    {
        if( !written )
        {
            Parameter< Operation::CREATE_PATH > path_parameter;
            path_parameter.path = path;
            IOHandler->enqueue(IOTask(this, path_parameter));
            IOHandler->flush();
        }

        flushAttributes();
    }
};
