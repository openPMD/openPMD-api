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

bool Snapshots::empty() const
{
    return get().empty();
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
} // namespace openPMD
