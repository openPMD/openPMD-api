@testset "Container{$contT}" begin
    T = contT
    K = contK
    @test Container{T,K} isa Type
    @test isabstracttype(Container{T,K})

    @test cont isa CxxRef{<:Container{T,K}}

    @test isempty(cont) isa Bool
    @test (length(cont) == 0) == isempty(cont)

    empty!(cont)
    @test isempty(cont)
    @test length(cont) == 0

    newval = get!(cont, "bubble")
    @test length(cont) == 1
    @test cont["bubble"] == newval

    cont["bobble"] = newval
    @test length(cont) == 2

    @test count(cont, "bubble") == 1
    @test "bubble" ∈ cont

    delete!(cont, "bobble")
    @test length(cont) == 1

    @test keys(cont) == ["bubble"]

    # @cxxdereference Base.values(cont::Container) = values1(cont)
    # @cxxdereference Base.collect(cont::Container) = collect1(cont)
end
