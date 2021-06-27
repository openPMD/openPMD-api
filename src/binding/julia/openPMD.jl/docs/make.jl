# Generate documentation with this command:
# (cd docs && julia --color=yes make.jl)

push!(LOAD_PATH, "..")

using Documenter
using openPMD

makedocs(; sitename="openPMD", format=Documenter.HTML(), modules=[openPMD])

deploydocs(; repo="github.com/eschnett/openPMD.jl.git", devbranch="main", push_preview=true)
