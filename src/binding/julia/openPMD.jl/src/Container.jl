# Container

@doc """
    abstract type Container{T,K} <: Attributable end
""" Container
export Container

Base.eltype(::Type{Container{T,K}}) where {T,K} = T
Base.keytype(::Type{Container{T,K}}) where {T,K} = K

@doc """
    isempty(cont::Container)
""" Base.isempty
@cxxdereference Base.isempty(cont::Container) = empty1(cont)

@doc """
    length(cont::Container)
""" Base.length
@cxxdereference Base.length(cont::Container) = Int(length1(cont))

@doc """
    empty!(cont::Container)
""" Base.empty!
@cxxdereference Base.empty!(cont::Container) = (empty1!(cont); cont)

@doc """
    getindex(cont::Container, key)
    cont[key]
""" Base.getindex
@cxxdereference Base.getindex(cont::Container, key) = getindex1(cont, key)

@doc """
    get!(cont::Container, key)
""" Base.get!
@cxxdereference Base.get!(cont::Container, key) = get1!(cont, key)

@doc """
    setindex!(cont::Container, value, key)
    cont[key] = value
""" Base.setindex!
@cxxdereference Base.setindex!(cont::Container, value, key) = setindex1!(cont, value, key)

@doc """
    count(cont::Container)
""" Base.count
@cxxdereference Base.count(cont::Container, key) = Int(count1(cont, key))

@doc """
    in(key, cont::Container)
    key in cont
""" Base.in
@cxxdereference Base.in(key, cont::Container) = contains1(cont, key)

@doc """
    delete!(cont::Container, key)
""" Base.delete!
@cxxdereference Base.delete!(cont::Container, key) = (delete1!(cont, key); cont)

@doc """
    keys(cont::Container)::AbstractVector
""" Base.keys
@cxxdereference Base.keys(cont::Container) = keys1(cont)

# @cxxdereference Base.values(cont::Container) = values1(cont)
# @cxxdereference Base.collect(cont::Container) = collect1(cont)
