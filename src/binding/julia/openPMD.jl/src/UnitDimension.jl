# UnitDimension

@doc """
    @enum UnitDimension begin
        L    # length
        M    # mass
        T    # time
        I    # electric current
        θ    # thermodynamic temperature
        N    # amount of substance
        J    # luminous intensity
    end
""" UnitDimension
export UnitDimension, L, M, T, I, θ, N, J

Base.convert(::Type{I}, d::UnitDimension) where {I<:Integer} = convert(I, reinterpret(UInt8, d))

Base.hash(d::UnitDimension, u::UInt) = hash(hash(convert(UInt, d), u), UInt(0xe2ff8533))
