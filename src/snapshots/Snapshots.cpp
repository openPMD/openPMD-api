#include "openPMD/snapshots/Snapshots.hpp"
namespace openPMD
{
Snapshots::Snapshots(std::shared_ptr<AbstractSnapshotsContainer> snapshots)
    : m_snapshots(std::move(snapshots))
{}
inline auto Snapshots::get() const -> AbstractSnapshotsContainer const &
{
    return *m_snapshots;
}
inline auto Snapshots::get() -> AbstractSnapshotsContainer &
{
    return *m_snapshots;
}
auto Snapshots::currentIteration() -> std::optional<value_type *>
{
    return get().currentIteration();
}
auto Snapshots::currentIteration() const -> std::optional<value_type const *>
{
    return get().currentIteration();
}
auto Snapshots::begin() -> iterator
{
    return get().begin();
}
auto Snapshots::end() -> iterator
{
    return get().end();
}
auto Snapshots::begin() const -> const_iterator
{
    return static_cast<AbstractSnapshotsContainer const &>(*m_snapshots)
        .begin();
}
auto Snapshots::end() const -> const_iterator
{
    return static_cast<AbstractSnapshotsContainer const &>(*m_snapshots).end();
}
auto Snapshots::rbegin() -> reverse_iterator
{
    return get().begin();
}
auto Snapshots::rend() -> reverse_iterator
{
    return get().end();
}
auto Snapshots::rbegin() const -> const_reverse_iterator
{
    return static_cast<AbstractSnapshotsContainer const &>(*m_snapshots)
        .begin();
}
auto Snapshots::rend() const -> const_reverse_iterator
{
    return get().end();
}

auto Snapshots::empty() const -> bool
{
    return get().empty();
}
auto Snapshots::size() const -> size_t
{
    return get().size();
}

auto Snapshots::at(key_type const &key) const -> mapped_type const &
{
    return get().at(key);
}
auto Snapshots::at(key_type const &key) -> mapped_type &
{
    return get().at(key);
}
auto Snapshots::operator[](key_type const &key) -> mapped_type &
{
    return get().operator[](key);
}

auto Snapshots::clear() -> void
{
    return get().clear();
}

auto Snapshots::find(key_type const &key) -> iterator
{
    return get().find(key);
}
auto Snapshots::find(key_type const &key) const -> const_iterator
{
    return get().find(key);
}

auto Snapshots::count(key_type const &key) const -> size_t
{
    return contains(key) ? 1 : 0;
}

auto Snapshots::contains(key_type const &key) const -> bool
{
    return get().contains(key);
}

} // namespace openPMD
