# WriteIterations

@doc """
   mutable struct WriteIterations
        ...
   end
""" WriteIterations
export WriteIterations

@doc """
    get!(iters::WriteIterations, key)
""" Base.get!
@cxxdereference Base.get!(iters::WriteIterations, key) = get1!(iters, key)
