@testset "Iteration" begin
    @test Iteration isa Type

    @test iter isa CxxRef{Iteration}

    t = 0.11
    set_time!(iter, t)
    @test time(iter) === t

    dt_ = 0.01
    set_dt!(iter, dt_)
    @test dt(iter) === dt_

    tu = 1.1
    set_time_unit_SI!(iter, tu)
    @test time_unit_SI(iter) === tu

    @test !closed(iter)
    @test !closed_by_writer(iter)

    @test meshes(iter) isa CxxRef{Container{Mesh,StdString}}
end
