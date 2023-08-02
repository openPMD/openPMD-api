module openPMD
using CxxWrap
@wrapmodule("build/libopenPMD.jl")
__init__() = @initcxx
end

Base.getindex(cont::openPMD.CXX_Container, index::Integer) = openPMD.cxx_getindex(cont, index)

series = openPMD.CXX_Series("sample.json", openPMD.ACCESS_CREATE)
openPMD.cxx_iterations(series)[100]
openPMD.cxx_flush(series, "{}")
