#include <iterator>

namespace openPMD::auxiliary
{

template <class Set>
class drain_view
{
    Set &set_;

    class iterator
    {
        Set *m_set;
        typename Set::node_type m_node;

    public:
        using value_type = typename Set::value_type;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;

        explicit iterator(Set *set) : m_set(set)
        {
            if (m_set && !m_set->empty())
                m_node = m_set->extract(m_set->begin());
        }

        auto operator*() const -> value_type &
        {
            return m_node.value();
        }

        auto operator->() const -> value_type *
        {
            return &m_node.value();
        }

        auto operator++() -> iterator &
        {
            m_node = {};

            if (!m_set->empty())
                m_node = m_set->extract(m_set->begin());

            return *this;
        }

        auto operator==(iterator const &other) const -> bool
        {
            return this->m_node.empty() == other.m_node.empty();
        }

        auto operator!=(iterator const &other) const -> bool
        {
            return !operator==(other);
        }
    };

public:
    explicit drain_view(Set &set) : set_(set)
    {}

    auto begin() -> iterator
    {
        return iterator(&set_);
    }

    auto end() -> iterator
    {
        return iterator{nullptr};
    }
};

template <class Set>
drain_view<Set> drain(Set &set)
{
    return drain_view<Set>(set);
}
} // namespace openPMD::auxiliary
