@testset "Attributable" begin
    @test Attributable isa Type
    @test isabstracttype(Attributable)

    obj = series
    @test obj isa Attributable
    set_attribute!(obj, "hello", 42)
    set_attribute!(obj, "world", [float(π)])
    @test get_attribute(obj, "hello") === 42
    @test get_attribute(obj, "world") == [float(π)]
    delete_attribute!(obj, "hello")
    @test !contains_attribute(obj, "hello")
    @test contains_attribute(obj, "world")
    @test "world" ∈ attributes(obj)
    @test num_attributes(obj) ≥ 1

    set_comment!(obj, "abc αβγ")
    @test comment(obj) == "abc αβγ"

    # Don't flush or close anything until the end of the test
    # series_flush(obj)
end
