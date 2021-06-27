@testset "Dataset" begin
    @test Extent isa Type
    @test Offset isa Type
    @test Dataset isa Type

    @test dset isa Dataset

    @test size(dset) == ext
    @test eltype(dset) == dsetT
    @test ndims(dset) == length(ext)

    # TODO: test chunk_size, compression, transform, options
end
