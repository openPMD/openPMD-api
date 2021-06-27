# Iteration

@doc """
    mutable struct Iteration <: Attributable
        ...
    end
""" Iteration
export Iteration

@doc """
    time(iter::Iteration)::CxxDouble
""" Base.time
@cxxdereference Base.time(iter) = time1(iter)

@doc """
    set_time!(iter::Iteration, time::CxxDouble)
""" set_time!
export set_time!

@doc """
    dt(iter::Iteration)::CxxDouble
""" dt
export dt

@doc """
    set_dt!(iter::Iteration, dt::CxxDouble)
""" set_dt!
export set_dt!

@doc """
    time_unit_SI(iter::Iteration)::CxxDouble
""" time_unit_SI
export time_unit_SI

@doc """
    set_time_unit_SI!(iter::Iteration, time_unit_SI::CxxDouble)
""" set_time_unit_SI!
export set_time_unit_SI!

@doc """
    close(iter::Iteration; flush::Bool=true)
""" Base.close
@cxxdereference Base.close(iter::Iteration; flush::Bool=true) = close1(iter, flush)

@doc """
    closed(iter::Iteration)::Bool
""" closed
export closed

@doc """
    closed_by_writer(iter::Iteration)::Bool
""" closed_by_writer
export closed_by_writer

@doc """
    meshes(iter::Iteration)::Container{Mesh}
""" meshes
export meshes
