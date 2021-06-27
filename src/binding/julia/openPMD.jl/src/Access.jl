# Access

@doc """
    @enum Access begin
        READ_ONLY
        READ_WRITE
        CREATE
    end
""" Access
export Access, READ_ONLY, READ_WRITE, CREATE

Base.hash(acc::Access, u::UInt) = hash(hash(convert(UInt, acc), u), UInt(0x0bec3b3e))
