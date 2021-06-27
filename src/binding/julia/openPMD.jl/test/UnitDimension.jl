@testset "UnitDimension" begin
    @test UnitDimension isa Type
    @test L isa UnitDimension
    @test M isa UnitDimension
    @test T isa UnitDimension
    @test I isa UnitDimension
    @test θ isa UnitDimension
    @test N isa UnitDimension
    @test J isa UnitDimension

    @test L == L
    @test L ≠ M
    @test hash(L) isa UInt
    @test hash(L) ≠ hash(convert(UInt, L))
end
