@testset "RecordComponent" begin
    @test RecordComponent isa Type
    @test isabstracttype(RecordComponent)

    data = Int[10i + j for i in 1:2, j in 1:3]
    @test size(data) == ext
    @test eltype(data) == dsetT
    store_chunk(comp, data, (0, 0), size(data))
end
