# Dataset

"""
    const Extent = Union{NTuple{D,Int},SVector{D,Int}}
"""
const Extent{D} = Union{NTuple{D,Int},SVector{D,Int}}
export Extent

"""
    const Offset = Union{NTuple{D,Int},SVector{D,Int}}
"""
const Offset{D} = Union{NTuple{D,Int},SVector{D,Int}}
export Offset

@doc """
    mutable struct Dataset
        ....
    end
    Dataset(::Type{<:OpenPMDType}, extent::Extent, options::AbstractString="{}")
    Dataset(extent::Extent)
""" Dataset
export Dataset

@doc """
    extend!(dset::Dataset, newextent::Extent)
""" extend!
export extend!

@doc """
    set_chunk_size!(dset::Dataset, chunk_size::Extent)
""" set_chunk_size!
export set_chunk_size!

@doc """
    set_compression!(dset::Dataset, compression::AbstractString, level::Int)
""" set_compression!
export set_compression!

@doc """
    set_custom_transform!(dset::Dataset, transform::AbstractString)
""" set_custom_transform!
export set_custom_transform!

"""
    size(dset::Dataset)
"""
@cxxdereference Base.size(dset::Dataset) = Tuple(extent1(dset))

"""
    eltype(dset::Dataset)
"""
@cxxdereference Base.eltype(dset::Dataset) = julia_type(dtype1(dset))

"""
    ndims(dset::Dataset)
"""
@cxxdereference Base.ndims(dset::Dataset) = Int(rank1(dset))

"""
    chunk_size(dset::Dataset)
"""
@cxxdereference chunk_size(dset::Dataset) = Tuple(chunk_size1(dset))
export chunk_size

@doc """
    compression(dset::Dataset)::AbstractString
""" compression
export compression

@doc """
    transform(dset::Dataset)::AbstractString
""" transform
export transform

@doc """
    options(dset::Dataset)::AbstractString
""" options
export options
