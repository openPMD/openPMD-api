#include "openPMD/IO/JSON/JSONFilePosition.hpp"

#include <utility>

namespace openPMD
{
JSONFilePosition::JSONFilePosition(json::json_pointer ptr) : id(std::move(ptr))
{}
} // namespace openPMD
