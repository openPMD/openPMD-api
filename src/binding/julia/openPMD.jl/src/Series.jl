# Series

@doc """
    mutable struct Series <: Attributable
        ...
    end
    Series()
    Series(filepath::AbstractString, access::Access, comm::MPI_Comm, options::AbstractString="{}")
    Series(filepath::AbstractString, access::Access, options::AbstractString="{}")
""" Series
export Series

@doc """
    openPMD_version(series::Series)::AbstractString
""" openPMD_version
export openPMD_version

@doc """
    set_openPMD_version!(series::Series, version::AbstractString)
""" set_openPMD_version!
export set_openPMD_version!

@doc """
    openPMD_extension(series::Series)::AbstractString
""" openPMD_extension
export openPMD_extension

@doc """
    set_openPMD_extension!(series::Series, extension::AbstractString)
""" set_openPMD_extension!
export set_openPMD_extension!

@doc """
    base_path(series::Series)::AbstractString
""" base_path
export base_path

@doc """
    set_base_path!(series::Series, path::AbstractString)
""" set_base_path!
export set_base_path!

@doc """
    meshes_path(series::Series)::AbstractString
""" meshes_path
export meshes_path

@doc """
    set_meshes_path!(series::Series, path::AbstractString)
""" set_meshes_path!
export set_meshes_path!

@doc """
    particles_path(series::Series)::AbstractString
""" particles_path
export particles_path

@doc """
    set_particles_path!(series::Series, path::AbstractString)
""" set_particles_path!
export set_particles_path!

@doc """
    author(series::Series)::AbstractString
""" author
export author

@doc """
    set_author!(series::Series, author::AbstractString)
""" set_author!
export set_author!

@doc """
    software(series::Series)::AbstractString
""" software
export software

@doc """
    set_software!(series::Series, software::AbstractString, version::AbstractString="unspecified")
""" set_software!
export set_software!

@doc """
    software_version(series::Series)::AbstractString
""" software_version
export software_version

@doc """
    date(series::Series)::AbstractString
""" date
export date

@doc """
    set_date!(series::Series, date::AbstractString)
""" set_date!
export set_date!

@doc """
    software_dependencies(series::Series)::AbstractString
""" software_dependencies
export software_dependencies

@doc """
    set_software_dependencies!(series::Series, dependencies::AbstractString)
""" set_software_dependencies!
export set_software_dependencies!

@doc """
    machine(series::Series)::AbstractString
""" machine
export machine

@doc """
    set_machine!(series::Series, machine::AbstractString)
""" set_machine!
export set_machine!

# TODO: type.method("iteration_encoding", &SeriesImpl::iterationEncoding);
# TODO: type.method("set_iteration_encoding!", &SeriesImpl::setIterationEncoding);

@doc """
    iteration_format(series::Series)::AbstractString
""" iteration_format
export iteration_format

@doc """
    set_iteration_format!(series::Series, format::AbstractString)
""" set_iteration_format!
export set_iteration_format!

@doc """
    name(series::Series)::AbstractString
""" name
export name

@doc """
    set_name!(series::Series, name::AbstractString)
""" set_name!
export set_name!

@doc """
    backend(series::Series)::AbstractString
""" backend
export backend

@doc """
    flush(series::Series)::AbstractString
""" Base.flush
@cxxdereference Base.flush(series::SeriesImpl) = flush1(series)

@doc """
    isvalid(series::Series)
""" Base.isvalid
@cxxdereference Base.isvalid(series::Series) = isvalid1(series)

@doc """
    write_iterations(series::Series)
""" write_iterations
export write_iterations
