@testset "version" begin
    @test get_version() isa AbstractString
    @test get_standard() isa AbstractString
    @test get_standard_minimum() isa AbstractString
    vars = get_variants()
    @test vars isa Dict{String,Bool}
    @test "json" ∈ keys(vars)
    @test vars["json"]
    exts = get_file_extensions()
    @test exts isa AbstractVector{<:AbstractString}
    @test "json" ∈ exts
end
