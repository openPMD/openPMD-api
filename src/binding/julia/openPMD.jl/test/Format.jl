@testset "Format" begin
    @test Format isa Type
    @test HDF5 isa Format
    @test ADIOS1 isa Format
    @test ADIOS2 isa Format
    @test ADIOS2_SST isa Format
    @test ADIOS2_SSC isa Format
    @test JSON isa Format
    @test DUMMY isa Format

    @test HDF5 == HDF5
    @test HDF5 ≠ ADIOS1
    @test hash(HDF5) isa UInt
    @test hash(HDF5) ≠ hash(convert(UInt, HDF5))

    for format in [HDF5, ADIOS1, ADIOS2, ADIOS2_SST, ADIOS2_SSC, JSON]
        suf = suffix(format)
        @test suf isa AbstractString
        @test !isempty(suf)
        # Not all formats can be recognized by suffix
        want_format = format == ADIOS1 ? ADIOS2 : format
        @test determine_format("hello.$suf") == want_format
    end
end
