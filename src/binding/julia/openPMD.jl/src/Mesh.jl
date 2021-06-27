# Mesh

@doc """
    @enum Geometry begin
        cartesian
        theta_mode
        cylindrical
        spherical
        other
    end
""" Geometry
export Geometry, cartesian, theta_mode, cylindrical, spherical, other

@doc """
    @enum DataOrder begin
        C
        F
    end
""" DataOrder
export DataOrder, C, F

@doc """
    mutable struct Mesh <: Container{MeshRecordComponent}
        ...
    end
""" Mesh
export Mesh

@doc """
    unit_dimension(mesh::Mesh)::SVector{7,Double}
""" unit_dimension
export unit_dimension

@doc """
    isscalar(mesh::Mesh)::Bool
""" isscalar
export isscalar

@doc """
    geometry(mesh::Mesh)::Geometry
""" geometry
export geometry

@doc """
    set_geometry!(mesh::Mesh, geom::Geometry)
""" set_geometry!
export set_geometry!

@doc """
    geometry_parameters(mesh::Mesh)::AbstractString
""" geometry_parameters
export geometry_parameters

@doc """
    set_geometry_parameters!(mesh::Mesh, params::AbstractString)
""" set_geometry_parameters!
export set_geometry_parameters!

@doc """
    data_order(mesh::Mesh)
""" data_order
export data_order

@doc """
    set_data_order!(mesh::Mesh, order::DataOrder)
""" set_data_order!
export set_data_order!

@doc """
    axis_labels(mesh::Mesh)::AbstractVector{<:AbstractString}
""" axis_labels
export axis_labels

@doc """
    set_axis_labels!(mesh::Mesh, labels::AbstractVector{<:AbstractString})
""" set_axis_labels!
export set_axis_labels!
@cxxdereference function set_axis_labels!(mesh::Mesh, labels::AbstractVector{<:AbstractString})
    return set_axis_labels1!(mesh, wrap_vector(labels))
end

@doc """
    grid_spacing(mesh::Mesh)::AbstractVector{CxxDouble}
""" grid_spacing
export grid_spacing

@doc """
    set_grid_spacing!(mesh::Mesh, spacing::AbstractVector{CxxDouble})
""" set_grid_spacing!
export set_grid_spacing!
@cxxdereference set_grid_spacing!(mesh::Mesh, spacing::AbstractVector{CxxDouble}) = set_grid_spacing1!(mesh, wrap_vector(spacing))

@doc """
    grid_global_offset(mesh::Mesh)::AbstractVector{CxxDouble}
""" grid_global_offset
export grid_global_offset

@doc """
    set_grid_global_offset!(mesh::Mesh, offset::AbstractVector{CxxDouble})
""" set_grid_global_offset!
export set_grid_global_offset!
@cxxdereference function set_grid_global_offset!(mesh::Mesh, offset::AbstractVector{CxxDouble})
    return set_grid_global_offset1!(mesh, wrap_vector(offset))
end

@doc """
    grid_unit_SI(mesh::Mesh)::CxxDouble
""" grid_unit_SI
export grid_unit_SI

@doc """
    set_grid_unit_SI!(mesh::Mesh, unit::CxxDouble)
""" set_grid_unit_SI!
export set_grid_unit_SI!

@doc """
    set_unit_dimension!(mesh::Mesh, unit_dim::Dict{UnitDimension,<:AbstractFloat})
""" set_unit_dimension!
export set_unit_dimension!
@cxxdereference function set_unit_dimension!(mesh::Mesh, unit_dim::Dict{UnitDimension,<:AbstractFloat})
    return set_unit_dimension1!(mesh,
                                SVector{7,Float64}(get(unit_dim, L, 0.0), get(unit_dim, M, 0.0), get(unit_dim, T, 0.0),
                                                   get(unit_dim, I, 0.0), get(unit_dim, θ, 0.0), get(unit_dim, N, 0.0),
                                                   get(unit_dim, J, 0.0)))
end

@doc """
    time_offset(mesh::Mesh)::CxxDouble
""" time_offset
export time_offset

@doc """
    set_time_offset!(mesh::Mesh, unit::CxxDouble)
""" set_time_offset!
export set_time_offset!

"""
    const SCALAR::AbstractString
"""
const SCALAR = SCALAR1()
