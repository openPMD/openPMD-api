@testset "WriteIterations" begin
    @test WriteIterations isa Type

    @test iters isa WriteIterations

    # We cannot test getting a new iteration; doing so closes the current iteration
    # newiter = get!(iters, 12)
    # @test newiter isa Iteration
    iter′ = get!(iters, 0)
    @test iter′ isa CxxRef{Iteration}
end
