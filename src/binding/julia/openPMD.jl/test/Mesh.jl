@testset "Mesh" begin
    @test Geometry isa Type
    @test cartesian isa Geometry
    @test theta_mode isa Geometry
    @test cylindrical isa Geometry
    @test spherical isa Geometry
    @test other isa Geometry

    @test DataOrder isa Type
    @test C isa DataOrder
    @test F isa DataOrder

    @test Mesh isa Type
end
