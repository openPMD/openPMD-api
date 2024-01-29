#include "openPMD/snapshots/Snapshots.hpp"
namespace openPMD
{
Snapshots::Snapshots(std::shared_ptr<AbstractSnapshotsContainer> snapshots)
    : m_snapshots(std::move(snapshots))
{}
auto Snapshots::begin() -> iterator
{
    return m_snapshots->begin();
}
auto Snapshots::end() -> iterator
{
    return m_snapshots->end();
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
    return m_snapshots->begin();
}
auto Snapshots::rend() -> reverse_iterator
{
    return m_snapshots->end();
}
auto Snapshots::rbegin() const -> const_reverse_iterator
{
    return static_cast<AbstractSnapshotsContainer const &>(*m_snapshots)
        .begin();
}
auto Snapshots::rend() const -> const_reverse_iterator
{
    return static_cast<AbstractSnapshotsContainer const &>(*m_snapshots).end();
}
} // namespace openPMD
