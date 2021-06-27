@testset "Series" begin
    @test Series isa Type

    @test series isa Series

    ver = openPMD_version(series)
    set_openPMD_version!(series, ver)
    @test ver == openPMD_version(series)

    ext = openPMD_extension(series)
    set_openPMD_extension!(series, ext)
    @test ext == openPMD_extension(series)

    bpath = base_path(series)
    # Not allowed in openPMD ≤ 1.1.0
    # set_base_path!(series, bpath)
    @test bpath == base_path(series)

    # Error if accessed when not present
    if contains_attribute(series, "meshesPath")
        mpath = meshes_path(series)
        set_meshes_path!(series, mpath)
        @test mpath == meshes_path(series)
    end

    if contains_attribute(series, "particlesPath")
        ppath = particles_path(series)
        set_particles_path!(series, ppath)
        @test ppath == particles_path(series)
    end

    auth = author(series)
    set_author!(series, auth)
    @test auth == author(series)

    sw = software(series)
    sv = software_version(series)
    set_software!(series, sw, sv)
    @test sw == software(series)
    @test sv == software_version(series)

    dat = date(series)
    set_date!(series, dat)
    @test dat == date(series)

    if contains_attribute(series, "softwareDependencies")
        deps = software_dependencies(series)
        set_software_dependencies!(series, deps)
        @test deps == software_dependencies(series)
    end

    if contains_attribute(series, "machine")
        mach = machine(series)
        set_machine!(series, mach)
        @test mach == machine(series)
    end

    iterfmt = iteration_format(series)
    # Cannot be set after a file has been written
    # set_iteration_format!(series, iterfmt)
    @test iterfmt == iteration_format(series)

    if contains_attribute(series, "name")
        name = name(series)
        set_name!(series, name)
        @test name == name(series)
    end

    bend = backend(series)
    @test bend == "JSON"

    # flush(series)

    @test isvalid(series)

    @test write_iterations(series) isa WriteIterations
end
