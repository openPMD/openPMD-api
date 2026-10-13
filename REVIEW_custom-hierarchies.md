# Review: custom hierarchies API

Scope: the `custom-hierarchies` feature branch (new `CustomHierarchy`,
`CustomDataset`, `ConvertibleContainer`, optional-attribute accessors, and the
`HierarchyVisitor`/`visitHierarchy` rework). This document records bugs found
during review and testing, the fixes applied, and remaining API gaps and design
concerns.

## Bugs found and fixed

### 1. `HierarchyVisitor` visited custom-managed objects twice

`Attributable::visitHierarchy()` calls `visitHierarchyImpl()` and then recurses
into `GroupMetaData::m_children_managed_as_custom_hierarchy`.
`CustomHierarchy::visitHierarchyImpl()` dispatched to `visitHierarchyContainer`,
which traverses the *whole frontend container*. That container is a superset of
the managed map (it also aliases objects belonging to the regular openPMD
hierarchy), so self-created custom objects were visited at least twice, and the
`CustomHierarchy` group nodes' own `v(self)` was emitted only there (so a naive
"no-op" would instead drop them).

Reproduced before the fix (typed `Iteration` root already double-visits):

```
A: it.visitHierarchy (typed Iteration root)
  1x /data/0/mygroup, 2x /data/0/mygroup/sub
```

Fixed in `src/CustomHierarchy.cpp`: the override now emits `v(*this)` and
traverses only those frontend children that are *not* in the managed map,
leaving managed children to the base class. Result for all roots is exactly one
visit per object.

### 2. `access_policy_iterator` violated the post-increment contract

`operator++(int)`/`operator--(int)` returned `self_t&` to the iterator *after*
the step, so `it++` / `it--` did not yield the previous position. This is a
public iterator (via `Container::begin()`), so any user code or `<algorithm>`
relying on postfix copy semantics misbehaved. Fixed in
`include/openPMD/backend/Container.hpp` to return a copy of the old position.

### 3. `asDataset()` silently discarded children of an unwritten group

`ConvertibleContainer::asDataset()` guarded the *written* group case (requiring
a constant dataset: `value`+`shape`+`empty`), but the branch for objects that
had not been written yet called `initDataset()` unconditionally. Casting a
non-empty, unwritten group to a dataset replaced the group metadata (dropping
the contained objects) and left the object in an inconsistent state, corrupting
subsequent flushes. Fixed in `src/CustomHierarchy.cpp` by rejecting a group that
has contained objects.

### 4. `erase()` did not remove custom-hierarchy objects from backend storage

`Container::erase()` (and `clear()`) removed entries from the frontend container
and the backend children map, but not from
`GroupMetaData::m_children_managed_as_custom_hierarchy`. Objects created through
a `CustomHierarchy` were therefore re-created and re-written on the next flush
after being erased:

```
before erase m_children: drop keep ...        | managed: drop keep
after erase  m_children: keep ...             | managed: drop keep   <- stale
reopen: size=4, contains drop=1               <- resurrected
```

Fixed in `include/openPMD/backend/ContainerImpl.tpp` for both `erase` overloads
and `clear_unchecked()`.

## Documentation / internal-error fixes

- `Attributable::getAttributeOptional` was documented as throwing
  `no_such_attribute_error`, but returns an empty `std::optional` by design.
- The `Container` static assertion message still named `Writable` while the
  checked trait had been renamed to test for `Attributable`.
- `customHierarchyFlush` threw `std::runtime_error("Unreachable!")`; replaced
  with `error::Internal` and a descriptive message.

## API gaps and design concerns

- **Return-by-value lifetime footgun.** `Attributable::customHierarchies()`
  returns a `CustomHierarchy` by value, and `operator[]` returns a reference into
  it. `auto &g = it.customHierarchies()["g"];` therefore binds to a
  soon-destroyed temporary and is a use-after-free; only chained use within a
  single full expression (`it.customHierarchies()["g"].setAttribute(...)`) or
  keeping the parent alive (`CustomHierarchy ch = ...; ch["g"]`) is safe. This is
  easy to hit and undocumented. Consider returning children by value or
  documenting the required lifetime.
- **Internal flush entry point is public.**
  `Attributable::customHierarchyFlush(internal::FlushParams const &, bool)` sits
  in the public section but takes an `internal::` type and an internal
  `managed_as_custom_object` flag. It cannot be meaningfully called by users and
  should be protected/private.
- **Debug output in the public API.** `CustomHierarchy::printRecursively()` and
  the newly added `Variant::print()` write to `std::cout`. These are
  diagnostics; they should not be part of the public surface (or should take an
  `std::ostream &`).
- **Detached default construction.** `CustomHierarchy` and `CustomDataset` are
  publicly default-constructible and produce objects without an `IOHandler`,
  parent, or associated backend object; operations on them are meaningless. A
  protected constructor with the existing `NoInit` tag would avoid the footgun.
- **Read caching is per view.** `CustomHierarchyData::has_been_read` lives in the
  per-view data, and every `customHierarchies()` call creates a fresh view.
  Hence each call re-reads one level from the backend; the cache is ineffective
  across calls. Repeated `iteration.customHierarchies()[...]` in loops
  (as used in the tests) performs repeated I/O. The flag would be better stored
  with the backend (shared) data.
- **`subgroups()` on a dataset returns empty silently** rather than reporting
  that the object is a dataset.
- **No Python bindings.** `customHierarchies()`, `CustomHierarchy`,
  `CustomDataset`, `getAttributeOptional`, and
  `meshesPathOptional`/`particlesPathOptional` are C++-only; the Python API
  exposes none of them.
- **`MonadicOperations.hpp` is not part of `openPMD.hpp`**, so the
  `optional_and_then` helper it provides is effectively internal despite living
  in a public-looking `auxiliary/` header.
- **`isDerivedFromAttributable<CustomHierarchy>` is special-cased to `true`**
  (`Container.hpp`), disabling the container element type check for the one new
  type most likely to need it, because `CustomHierarchy` is incomplete at that
  point.

## Behavior changes to confirm

- `Series::readFileBased` now catches `std::exception` around `std::stoull` and
  silently skips iteration entries whose names are not numbers (previously an
  error). This can hide malformed input; confirm it is intended.

## Test coverage added

- `custom_hierarchy` (test/Files_Core/custom_hierarchy.cpp):
  - `HierarchyVisitor` visits every object exactly once, for typed and
    custom-hierarchy roots, across all backends/encodings.
  - Casting a non-empty group to a dataset throws.
  - `erase()` of a custom object survives a flush/reopen round trip.
- `container_access_test` (test/AuxiliaryTest.cpp): post-increment/decrement
  yield the previous position.
- `optional_attribute_test` (test/CoreTest.cpp): `getAttributeOptional`,
  `meshesPathOptional`, `particlesPathOptional` for present and absent
  attributes, and that `getAttribute` still throws.

## Verification

- Full `ctest` suite (Serial Core/Auxiliary/SerialIO/JSON, MPI, examples/CLI):
  all passing.
- The `custom_hierarchy` test runs the new assertions for all backends and all
  three iteration encodings.
