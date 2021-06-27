# Attributable

@doc """
    abstract type Attributable end
""" Attributable
export Attributable

# We cannot use `setindex!` and `getindex` for `Attributable` types
# because the concrete types might implement their own `setindex!` and
# `getindex`, which would make attributes inaccessible.
#
# @doc """
#     setindex!(attr::Attributeable, key::AbstractString, value::OpenPMDType)
#     attr[key] = value
# """ Base.setindex!
# for (otype, jtype) in julia_types
#     @eval begin
#         Base.setindex!(attr::Attributable, value::$jtype, key::AbstractString) = set_attribute1!(attr, key, value)
#     end
# end
# 
# @doc """
#     getindex(attr::Attributable, key::AbstractString)::OpenPMDType
#     attr[key]::Attribute
# """ Base.getindex
# Base.getindex(attr::Attributable, key::AbstractString) = get_attribute1(attr, key)[]
# 
# @doc """
#     delete!(attr:Attributable, key::AbstractString)
# """ Base.delete!
# Base.delete!(attr::Attributable, key::AbstractString) = (delete_attribute1(attr, key); attr)

@doc """
    set_attribute!(attr::Attributeable, key::AbstractString, value::OpenPMDType)
""" set_attribute!
export set_attribute!
for (otype, jtype) in julia_types
    @eval begin
        @cxxdereference function set_attribute!(attr::Attributable, key::AbstractString, value::$jtype)
            $(Symbol("set_attribute1_", type_symbols[otype], "!"))(attr, key, wrap_vector(value))
            return attr
        end
    end
end

@doc """
    get_attribute(attr::Attributable, key::AbstractString)::OpenPMDType
""" get_attribute
export get_attribute
@cxxdereference get_attribute(attr::Attributable, key::AbstractString) = get_attribute1(attr, key)[]

@doc """
    delete_attribute!(attr:Attributable, key::AbstractString)
""" delete_attribute!
export delete_attribute!

@doc """
    attributes(attr:Attributable)::AbstractVector{<:AbstractString}
""" attributes
export attributes

@doc """
    num_attributes(attr:Attributable)::Int
""" num_attributes
export num_attributes
@cxxdereference num_attributes(attr::Attributable) = Int(num_attributes1(attr))

@doc """
    contains_attribute(attr:Attributable, key::AbstractString)::Bool
""" contains_attribute
export contains_attribute

@doc """
    comment(attr::Attributable)::AbstractString
""" comment
export comment

@doc """
    set_comment!(attr::Attributable, comment::AbstractString)::Attributable
""" set_comment!
export set_comment!

@doc """
    series_flush(attr::Attributable)
""" series_flush
export series_flush
