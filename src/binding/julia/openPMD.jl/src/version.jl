# version

@doc """
    get_version()::AbstractString
""" get_version
export get_version

@doc """
    get_standard()::AbstractString
""" get_standard
export get_standard

@doc """
    get_standard_minimum()::AbstractString
""" get_standard_minimum
export get_standard_minimum

@doc """
    get_variants()::Dict{String,Bool}
""" get_variants
export get_variants
function get_variants()
    variants1 = get_variants1()
    variants = Dict{String,Bool}()
    for var1 in variants1
        variants[first(var1)] = second(var1)
    end
    return variants
end

@doc """
    get_file_extensions()::AbstractVector{<:AbstractString}
""" get_file_extensions
export get_file_extensions
