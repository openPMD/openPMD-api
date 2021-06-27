# BaseRecordComponent

@doc """
   abstract type BaseRecordComponent <: Attributable end
""" BaseRecordComponent
export BaseRecordComponent

@doc """
    unit_SI(comp::BaseRecordComponent)::CxxDouble
""" unit_SI
export unit_SI

@doc """
    reset_datatype!(comp::BaseRecordComponent, T::Type)
""" reset_datatype!
export reset_datatype!
@cxxdereference reset_datatype!(comp::BaseRecordComponent, T::Type) = reset_datatype1!(comp, openpmd_type(T))

@doc """
    get_datatype(comp::BaseRecordComponent)::Type
""" get_datatype
export get_datatype
@cxxdereference get_datatype(comp::BaseRecordComponent) = julia_type(get_datatype1(comp))

@doc """
    is_constant(comp::BaseRecordComponent)::Bool
""" is_constant
export is_constant
