# Format

@doc """
    @enum Format begin
        HDF5
        ADIOS1
        ADIOS2
        ADIOS2_SST
        ADIOS2_SSC
        JSON
        DUMMY
    end
""" Format
export Format, HDF5, ADIOS1, ADIOS2, ADIOS2_SST, ADIOS2_SSC, JSON, DUMMY

Base.hash(fmt::Format, u::UInt) = hash(hash(convert(UInt, fmt), u), UInt(0x2d9a9364))

"""
    determine_format(filename::AbstractString)::Format
"""
function determine_format end
export determine_format

"""
    suffix(format::Format)::AbstractString
"""
function suffix end
export suffix
