# MeshRecordComponent

@doc """
    mutable struct MeshRecordComponent <: RecordComponent
        ...
    end
""" MeshRecordComponent
export MeshRecordComponent

@doc """
    position(comp::MeshRecordComponent)::AbstractVector{CxxDouble}
""" Base.position
@cxxdereference Base.position(comp::MeshRecordComponent) = position1(comp)

@doc """
    set_position!(comp::MeshRecordComponent, newpos::Union{NTuple{D,CxxDouble}, AbstractVector{CxxDouble}})
""" set_position!
export set_position!
@cxxdereference function set_position!(comp::MeshRecordComponent, newpos::AbstractVector{CxxDouble})
    return set_position1!(comp, wrap_vector(newpos))
end
@cxxdereference function set_position!(comp::MeshRecordComponent, newpos::NTuple{D,CxxDouble} where {D})
    return set_position!(comp, CxxDouble[newpos...])
end

@doc """
    make_constant(comp::MeshRecordComponent, value::OpenMPType)
""" make_constant
export make_constant
for (otype, jtype) in julia_types
    @eval begin
        @cxxdereference function make_constant(comp::MeshRecordComponent, value::$jtype)
            return $(Symbol("make_constant1_", type_symbols[otype]))(comp, value)
        end
    end
end
