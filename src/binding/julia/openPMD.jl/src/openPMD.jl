module openPMD

using CxxWrap
using StaticArrays

@wrapmodule "/Users/eschnett/src/openPMD-api/build/lib/libopenPMD_jl.dylib"

__init__() = @initcxx

################################################################################

# TODO:
#
# - do we need to worry about `jlcxx::StrictlyTypedNumber`?
#
# - do we really need all the `@cxxdereference` macros? yes we do.
#   maybe we need it only for explicitly declared functions and not
#   for wrapped functions?
#
# - remove all the `Bool(...)` wrappers; they are not needed
#
# - put enum values into a namespace
#
# - do we really need different names to call templated functions? probably not, at least not for constructors.

################################################################################

# Convert Julia vectors to `StdVector`, leave all other types alone
wrap_vector(xs::AbstractVector) = StdVector(xs)
wrap_vector(x) = x

################################################################################

# The order in which these files are included matters. The order here
# should mirror the order in the file `openPMD.cpp` that implements
# the respective types, constants, and functions.

include("Access.jl")
include("Datatype.jl")
include("Format.jl")
include("UnitDimension.jl")

include("Attribute.jl")
include("Attributable.jl")
include("Dataset.jl")

include("Container.jl")

include("BaseRecordComponent.jl")
include("RecordComponent.jl")
include("MeshRecordComponent.jl")

include("Mesh.jl")

include("Iteration.jl")

include("WriteIterations.jl")

include("Series.jl")

include("version.jl")

end
