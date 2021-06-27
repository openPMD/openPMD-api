# RecordComponent

@doc """
    @enum Allocation begin
        USER
        API
        AUTO
    end
""" Allocation
export Allocation, USER, API, AUTO

@doc """
    abstract type RecordComponent <: BaseRecordComponent end
""" RecordComponent
export RecordComponent

@doc """
    set_unit_SI!(comp::RecordComponent, unit::CxxDouble)
""" set_unit_SI!
export set_unit_SI!

@doc """
    reset_dataset!(comp::RecordComponent, dset::Dataset)
""" reset_dataset!
export reset_dataset!

@doc """
    ndims(comp::RecordComponent)
""" Base.ndims
@cxxdereference Base.ndims(comp::RecordComponent) = Int(get_dimensionality1(comp))

@doc """
    size(comp::RecordComponent)
""" Base.size
@cxxdereference Base.size(comp::RecordComponent) = Tuple(get_extent1(comp))

@doc """
    make_constant(comp::RecordComponent, value::OpenMPType)
""" make_constant
export make_constant
for (otype, jtype) in julia_types
    @eval begin
        @cxxdereference function make_constant(comp::RecordComponent, value::$jtype)
            return $(Symbol("make_constant1_", type_symbols[otype]))(comp, value)
        end
    end
end

@doc """
    make_empty(T::Type, comp::RecordComponent, ndims::Int)
""" make_empty
export make_empty
@cxxdereference make_empty(T::Type, comp::RecordComponent, ndims::Int) = make_empty1(comp, openpmd_type(T), ndims)

@doc """
    isempty(comp::RecordComponent)
""" Base.isempty
@cxxdereference Base.isempty(comp::RecordComponent) = empty1(comp)

# TODO: load_chunk

@doc """
    store_chunk(comp::RecordComponent, data::AbstractArray, offset::Offset, extent::Extent)
""" store_chunk
export store_chunk
# Object lifetime management for store_chunk: Ideally, we would ensure
# that the Julia object stays alive until openPMD has written the
# dataset. To simplify things, we currently create a temporary buffer
# that is managed by a C++ std::shared_ptr. Note that `wrap_vector`
# copies the data, which also needs to be avoided.
for (otype, jtype) in julia_types
    @eval begin
        @cxxdereference function store_chunk(comp::RecordComponent, data::AbstractArray{$jtype}, offset::Offset, extent::Extent)
            # @assert length(offset) == length(extent) == ndims(comp)
            @assert all(extent .>= 0)
            @assert length(data) == prod(extent)
            return $(Symbol("store_chunk_copy1_", type_symbols[otype]))(comp, wrap_vector(reshape(data, :)), wrap_offset(offset),
                                                                        wrap_extent(extent))
        end
    end
end
