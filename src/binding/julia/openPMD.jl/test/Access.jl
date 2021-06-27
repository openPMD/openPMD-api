@testset "Access" begin
    @test Access isa Type
    @test READ_ONLY isa Access
    @test READ_WRITE isa Access
    @test CREATE isa Access

    @test READ_ONLY == READ_ONLY
    @test READ_ONLY ≠ READ_WRITE
    @test hash(READ_ONLY) isa UInt
    @test hash(READ_ONLY) ≠ hash(convert(UInt, READ_ONLY))
end
