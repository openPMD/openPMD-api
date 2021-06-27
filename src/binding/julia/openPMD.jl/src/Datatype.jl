# Datatype

@doc """
    @enum Datatype begin
        CHAR => CxxChar
        UCHAR => CxxUChar
        SHORT => CxxShort
        INT => CxxInt
        LONG => CxxLong,
        LONGLONG => CxxLongLong
        USHORT => CxxUShort
        UINT => CxxUInt
        ULONG => CxxULong,
        ULONGLONG => CxxULongLong
        FLOAT => CxxFloat
        DOUBLE => CxxDouble,
        CFLOAT => Complex{CxxFloat}
        CDOUBLE => Complex{CxxDouble}
        STRING => String,
        VEC_CHAR => Vector{CxxChar}
        VEC_UCHAR => Vector{CxxUChar}
        VEC_SHORT => Vector{CxxShort},
        VEC_INT => Vector{CxxInt}
        VEC_LONG => Vector{CxxLong}
        VEC_LONGLONG => Vector{CxxLongLong},
        VEC_USHORT => Vector{CxxUShort}
        VEC_UINT => Vector{CxxUInt}
        VEC_ULONG => Vector{CxxULong},
        VEC_ULONGLONG => Vector{CxxULongLong}
        VEC_FLOAT => Vector{CxxFloat},
        VEC_DOUBLE => Vector{CxxDouble}
        VEC_CFLOAT => Vector{Complex{CxxFloat}},
        VEC_CDOUBLE => Vector{Complex{CxxDouble}}
        VEC_STRING => Vector{String}
        BOOL => CxxBool,
        ARR_DBL_7 => SVector{7,CxxDouble})
    end
""" Datatype
export Datatype, CHAR, UCHAR, SHORT, INT, LONG, LONGLONG, USHORT, UINT, ULONG, ULONGLONG, FLOAT, DOUBLE, CFLOAT, CDOUBLE, STRING,
       VEC_CHAR, VEC_UCHAR, VEC_SHORT, VEC_INT, VEC_LONG, VEC_LONGLONG, VEC_USHORT, VEC_UINT, VEC_ULONG, VEC_ULONGLONG, VEC_FLOAT,
       VEC_DOUBLE, VEC_CFLOAT, VEC_CDOUBLE, VEC_STRING, ARR_DBL_7, BOOL

# CxxWrap promises these types, but doesn't define them
const CxxDouble = Cdouble
const CxxFloat = Cfloat
const CxxInt = Cint
const CxxShort = Cshort
const CxxUInt = Cuint
const CxxUShort = Cushort
export CxxDouble, CxxFloat, CxxInt, CxxShort, CxxUInt, CxxUShort

Base.hash(b::CxxBool, u::UInt) = hash(hash(Bool(b), u), UInt(0x4c87662d))

# We cannot use this definition for equality. It leads to `LONG` ==
# `LONGLONG` on some systems.

# Base.:(==)(d::Datatype, e::Datatype) = is_same1(d, e)
# function Base.hash(d::Datatype, h::UInt)
#     isvec = is_vector1(d)
#     isint, issig = is_integer1(d)
#     isfp = is_floating_point1(d)
#     iscfp = is_complex_floating_point1(d)
#     bits = to_bits1(d)
#     return hash(0x6b224312, hash(isvec, hash(isint, hash(issig, hash(isfp, hash(iscfp, hash(bits, h)))))))
# end

Base.hash(d::Datatype, u::UInt) = hash(hash(convert(UInt, d), u), UInt(0x6b224312))

# Convert between Julia types and OpenPMD type ids

const type_symbols = Dict{Datatype,Symbol}(CHAR => :CHAR, UCHAR => :UCHAR, SHORT => :SHORT, INT => :INT, LONG => :LONG,
                                           LONGLONG => :LONGLONG, USHORT => :USHORT, UINT => :UINT, ULONG => :ULONG,
                                           ULONGLONG => :ULONGLONG, FLOAT => :FLOAT, DOUBLE => :DOUBLE, CFLOAT => :CFLOAT,
                                           CDOUBLE => :CDOUBLE, STRING => :STRING, VEC_CHAR => :VEC_CHAR, VEC_UCHAR => :VEC_UCHAR,
                                           VEC_SHORT => :VEC_SHORT, VEC_INT => :VEC_INT, VEC_LONG => :VEC_LONG,
                                           VEC_LONGLONG => :VEC_LONGLONG, VEC_USHORT => :VEC_USHORT, VEC_UINT => :VEC_UINT,
                                           VEC_ULONG => :VEC_ULONG, VEC_ULONGLONG => :VEC_ULONGLONG, VEC_FLOAT => :VEC_FLOAT,
                                           VEC_DOUBLE => :VEC_DOUBLE, VEC_CFLOAT => :VEC_CFLOAT, VEC_CDOUBLE => :VEC_CDOUBLE,
                                           VEC_STRING => :VEC_STRING, ARR_DBL_7 => :ARR_DBL_7, BOOL => :BOOL)
const julia_types = Dict{Datatype,Type}(CHAR => CxxChar, UCHAR => CxxUChar, SHORT => CxxShort, INT => CxxInt, LONG => CxxLong,
                                        LONGLONG => CxxLongLong, USHORT => CxxUShort, UINT => CxxUInt, ULONG => CxxULong,
                                        ULONGLONG => CxxULongLong, FLOAT => CxxFloat, DOUBLE => CxxDouble,
                                        CFLOAT => Complex{CxxFloat}, CDOUBLE => Complex{CxxDouble}, STRING => String,
                                        VEC_CHAR => Vector{CxxChar}, VEC_UCHAR => Vector{CxxUChar}, VEC_SHORT => Vector{CxxShort},
                                        VEC_INT => Vector{CxxInt}, VEC_LONG => Vector{CxxLong}, VEC_LONGLONG => Vector{CxxLongLong},
                                        VEC_USHORT => Vector{CxxUShort}, VEC_UINT => Vector{CxxUInt}, VEC_ULONG => Vector{CxxULong},
                                        VEC_ULONGLONG => Vector{CxxULongLong}, VEC_FLOAT => Vector{CxxFloat},
                                        VEC_DOUBLE => Vector{CxxDouble}, VEC_CFLOAT => Vector{Complex{CxxFloat}},
                                        VEC_CDOUBLE => Vector{Complex{CxxDouble}}, VEC_STRING => Vector{String},
                                        ARR_DBL_7 => SVector{7,CxxDouble}, BOOL => CxxBool)
function julia_type(d::Datatype)
    T = get(julia_types, d, nothing)
    T ≡ nothing && error("unknown Datatype $d")
    return T
end

openpmd_type(::Type{CxxChar}) = CHAR
openpmd_type(::Type{CxxUChar}) = UCHAR
openpmd_type(::Type{CxxShort}) = SHORT
openpmd_type(::Type{CxxInt}) = INT
openpmd_type(::Type{CxxLong}) = LONG
openpmd_type(::Type{CxxLongLong}) = LONGLONG
openpmd_type(::Type{CxxUShort}) = USHORT
openpmd_type(::Type{CxxUInt}) = UINT
openpmd_type(::Type{CxxULong}) = ULONG
openpmd_type(::Type{CxxULongLong}) = ULONGLONG
openpmd_type(::Type{CxxFloat}) = FLOAT
openpmd_type(::Type{CxxDouble}) = DOUBLE
openpmd_type(::Type{Complex{CxxFloat}}) = CFLOAT
openpmd_type(::Type{Complex{CxxDouble}}) = CDOUBLE
openpmd_type(::Type{String}) = STRING
openpmd_type(::Type{Vector{CxxChar}}) = VEC_CHAR
openpmd_type(::Type{Vector{CxxUChar}}) = VEC_UCHAR
openpmd_type(::Type{Vector{CxxShort}}) = VEC_SHORT
openpmd_type(::Type{Vector{CxxInt}}) = VEC_INT
openpmd_type(::Type{Vector{CxxLong}}) = VEC_LONG
openpmd_type(::Type{Vector{CxxLongLong}}) = VEC_LONGLONG
openpmd_type(::Type{Vector{CxxUShort}}) = VEC_USHORT
openpmd_type(::Type{Vector{CxxUInt}}) = VEC_UINT
openpmd_type(::Type{Vector{CxxULong}}) = VEC_ULONG
openpmd_type(::Type{Vector{CxxULongLong}}) = VEC_ULONGLONG
openpmd_type(::Type{Vector{CxxFloat}}) = VEC_FLOAT
openpmd_type(::Type{Vector{CxxDouble}}) = VEC_DOUBLE
openpmd_type(::Type{Vector{Complex{CxxFloat}}}) = VEC_CFLOAT
openpmd_type(::Type{Vector{Complex{CxxDouble}}}) = VEC_CDOUBLE
openpmd_type(::Type{Vector{String}}) = VEC_STRING
openpmd_type(::Type{SVector{7,CxxDouble}}) = ARR_DBL_7
openpmd_type(::Type{CxxBool}) = BOOL

"""
    openPMD_datatypes::AbstractVector{Datatype}
"""
openPMD_datatypes
export openPMD_datatypes

"""
    OpenPMDType = Union{...}
"""
const OpenPMDType = Union{CxxChar,CxxUChar,CxxShort,CxxInt,CxxLong,CxxLongLong,CxxUShort,CxxUInt,CxxULong,CxxULongLong,CxxFloat,
                          CxxDouble,Complex{CxxFloat},Complex{CxxDouble},String,Vector{CxxChar},Vector{CxxUChar},Vector{CxxShort},
                          Vector{CxxInt},Vector{CxxLong},Vector{CxxLongLong},Vector{CxxUShort},Vector{CxxUInt},Vector{CxxULong},
                          Vector{CxxULongLong},Vector{CxxFloat},Vector{CxxDouble},Vector{Complex{CxxFloat}},
                          Vector{Complex{CxxDouble}},Vector{String},SVector{7,CxxDouble},CxxBool}
export OpenPMDType

"""
    determine_datatype(::Type)::Datatype
"""
determine_datatype(T::Type) = openpmd_type(T)
export determine_datatype

"""
    to_bytes(::Type)::Int
"""
to_bytes(T::Type) = Int(to_bytes1(openpmd_type(T)))
export to_bytes

"""
    to_bits(::Type)::Int
"""
to_bits(T::Type) = Int(to_bits1(openpmd_type(T)))
export to_bits

"""
    is_vector(::Type)::Bool
"""
is_vector(T::Type) = is_vector1(openpmd_type(T))
export is_vector

"""
    is_floating_point(::Type)::Bool
"""
is_floating_point(T::Type) = is_floating_point1(openpmd_type(T))
export is_floating_point

"""
    is_complex_floating_point(::Type)::Bool
"""
is_complex_floating_point(T::Type) = is_complex_floating_point1(openpmd_type(T))
export is_complex_floating_point

"""
    is_integer(::Type)::Tuple{Bool,Bool}

Whether the type is an integer (first tuple element), and if so,
whether it is signed (second tuple element).
"""
is_integer(T::Type) = Tuple{Bool,Bool}(is_integer1(openpmd_type(T)))
export is_integer

"""
    is_same(::Type, ::Type)::Bool
"""
is_same(T1::Type, T2::Type) = is_same1(openpmd_type(T1), openpmd_type(T2))
export is_same

# """
#     is_same_floating_point(::Type, ::Type)::Bool
# """
# is_same_floating_point(T1::Type, T2::Type) = is_same_floating_point(openpmd_type(T1), openpmd_type(T2))
# export is_same_floating_point
# 
# """
#     is_same_complex_floating_point(::Type, ::Type)::Bool
# """
# is_same_complex_floating_point(T!::Type, T2::Type) = is_same_complex_floating_point(openpmd_type(T1), openpmd_type(T2))
# export is_same_complex_floating_point
# 
# """
#     is_same_integer(::Type, ::Type)::Bool
# """
# is_same_integer(T1::Type, T2::Type) = is_same_integer(openpmd_type(T1), openpmd_type(T2))
# export is_same_integer

"""
    basic_datatype(::Type)::Type
"""
basic_datatype(T::Type) = julia_type(basic_datatype1(openpmd_type(T)))
export basic_datatype

"""
    to_vector_type(::Type)::Type
"""
to_vector_type(T::Type) = julia_type(to_vector_type1(openpmd_type(T)))
export to_vector_type

"""
    datatype_to_string(::Type)::AbstractString
"""
datatype_to_string(T::Type) = datatype_to_string1(openpmd_type(T))
export datatype_to_string

"""
    string_to_datatype(str::AbstractString)::Type
"""
string_to_datatype(str::AbstractString) = julia_type(string_to_datatype1(str))
export string_to_datatype

"""
    warn_wrong_dtype(key::AbstractString, ::Type{Store}, ::Type{Request}) where {Store,Request}
"""
function warn_wrong_dtype1(key::AbstractString, ::Type{Store}, ::Type{Request}) where {Store,Request}
    return warn_wrong_dtype(key, openpmd_type(Store), openpmd_type(Request))
end
export warn_wrong_dtype
