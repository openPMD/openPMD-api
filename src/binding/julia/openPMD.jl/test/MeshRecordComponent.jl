@testset "MeshRecordComponent" begin
    @test MeshRecordComponent isa Type

    comp::CxxRef{MeshRecordComponent}
    pos = [1.0, 2.0, 3.0]
    set_position!(comp, pos)
    @test position(comp) == pos
    make_constant(comp, 3)
    @test is_constant(comp)
    make_constant(comp, float(π))
    @test is_constant(comp)
    make_constant(comp, 1.0im)
    @test is_constant(comp)
end
