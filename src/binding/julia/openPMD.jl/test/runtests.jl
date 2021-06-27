using Base.Filesystem
using CxxWrap
using StaticArrays
using Test
using openPMD

include("Access.jl")
include("Datatype.jl")
include("Format.jl")
include("UnitDimension.jl")

#TODO const dirname = Filesystem.mktempdir(; cleanup=true)
const dirname = Filesystem.mktempdir(; cleanup=false)
const filename = joinpath(dirname, "hello.json")

const series = Series(filename, CREATE)
set_name!(series, "hello")
set_author!(series, "Erik Schnetter <schnetter@gmail.com>")
const iters = write_iterations(series)
const iter = get!(iters, 0)
const mesh = get!(meshes(iter), "my_first_mesh")
const comp = get!(mesh, "my_first_record")
set_position!(comp, (0.0, 0.0))
const dsetT = Int
const ext = (2, 3)
const dset = Dataset(dsetT, ext)
reset_dataset!(comp, dset)

include("Attribute.jl")
include("Attributable.jl")
include("Dataset.jl")

include("BaseRecordComponent.jl")
include("RecordComponent.jl")
include("MeshRecordComponent.jl")

cont = mesh
contT = MeshRecordComponent
contK = StdString
include("Container.jl")
include("Mesh.jl")

include("Iteration.jl")

include("WriteIterations.jl")

include("Series.jl")

include("version.jl")

series_flush(series)

close(iter)
