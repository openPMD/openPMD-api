@testset "Datatype" begin
    @test Datatype isa Type

    otypes = [CHAR, UCHAR, SHORT, INT, LONG, LONGLONG, USHORT, UINT, ULONG, ULONGLONG, FLOAT, DOUBLE, CFLOAT, CDOUBLE, STRING,
              VEC_CHAR, VEC_UCHAR, VEC_SHORT, VEC_INT, VEC_LONG, VEC_LONGLONG, VEC_USHORT, VEC_UINT, VEC_ULONG, VEC_ULONGLONG,
              VEC_FLOAT, VEC_DOUBLE, VEC_CFLOAT, VEC_CDOUBLE, VEC_STRING, BOOL, ARR_DBL_7]
    otype_equiv = Dict(otype => otype for otype in otypes)
    if sizeof(CxxLong) == sizeof(CxxLongLong)
        otype_equiv[LONG] = LONGLONG
        otype_equiv[ULONG] = ULONGLONG
        otype_equiv[VEC_LONG] = VEC_LONGLONG
        otype_equiv[VEC_ULONG] = VEC_ULONGLONG
    end
    for otype in otypes
        @test otype isa Datatype
        jtype = openPMD.julia_type(otype)
        @test jtype <: OpenPMDType
        otype′ = determine_datatype(jtype)
        @test otype_equiv[otype′] ≡ otype_equiv[otype]
    end

    jtypes = [CxxChar, CxxUChar, CxxShort, CxxInt, CxxLong, CxxLongLong, CxxUShort, CxxUInt, CxxULong, CxxULongLong, CxxFloat,
              CxxDouble, Complex{CxxFloat}, Complex{CxxDouble}, String, Vector{CxxChar}, Vector{CxxUChar}, Vector{CxxShort},
              Vector{CxxInt}, Vector{CxxLong}, Vector{CxxLongLong}, Vector{CxxUShort}, Vector{CxxUInt}, Vector{CxxULong},
              Vector{CxxULongLong}, Vector{CxxFloat}, Vector{CxxDouble}, Vector{Complex{CxxFloat}}, Vector{Complex{CxxDouble}},
              Vector{String}, SVector{7,CxxDouble}, CxxBool]
    jtype_equiv = Dict(jt => jt for jt in jtypes)
    if sizeof(CxxLong) == sizeof(CxxLongLong)
        jtype_equiv[CxxLong] = CxxLongLong
        jtype_equiv[CxxULong] = CxxULongLong
        jtype_equiv[Vector{CxxLong}] = Vector{CxxLongLong}
        jtype_equiv[Vector{CxxULong}] = Vector{CxxULongLong}
    end
    for jtype in jtypes
        @test jtype <: OpenPMDType
        otype = determine_datatype(jtype)
        @test otype isa Datatype
        jtype′ = openPMD.julia_type(otype)
        @test jtype′ ≡ jtype
    end

    @test CHAR == CHAR
    @test CHAR ≠ UCHAR
    @test hash(CHAR) isa UInt
    @test hash(CHAR) ≠ hash(convert(UInt8, CHAR))

    for jtype in jtypes
        if jtype <: Vector
            # pass
        elseif jtype <: String
            # pass
        else
            @test to_bytes(jtype) == sizeof(eltype(jtype))
            @test to_bits(jtype) == 8 * sizeof(eltype(jtype))
        end
        @test is_vector(jtype) == (jtype <: Vector)
        ejtype = jtype <: Vector ? eltype(jtype) : jtype
        @test is_floating_point(jtype) == (ejtype <: AbstractFloat)
        @test is_complex_floating_point(jtype) == (ejtype <: Complex)
        @test is_integer(jtype)[1] == (ejtype <: Integer && ejtype ∉ (CxxBool, CxxChar, CxxUChar))
        if is_integer(jtype)[1]
            @test is_integer(jtype)[2] == (ejtype <: Signed)
        end
        for jtype′ in jtypes
            @test is_same(jtype′, jtype) == (jtype_equiv[jtype′] === jtype_equiv[jtype])
        end
        if jtype <: AbstractVector
            @test basic_datatype(jtype) === eltype(jtype)
            # @test to_vector_type(jtype) === jtype
        else
            @test basic_datatype(jtype) === jtype
            if jtype === CxxBool
                # skip; `CxxBool` is not accepted
                # @test to_vector_type(jtype) === Vector{jtype}
            else
                @test to_vector_type(jtype) === Vector{jtype}
            end
        end
        str = datatype_to_string(jtype)
        @test str isa AbstractString
        @test string_to_datatype(str) === jtype
    end
end
