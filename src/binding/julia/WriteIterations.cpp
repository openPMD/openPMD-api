// WriteIterations

#include "defs.hpp"

#include <cstdint>

void define_julia_WriteIterations(jlcxx::Module &mod)
{
    using key_type = Series::IterationIndex_t;
    // using mapped_type = typename iterations_t::mapped_type;

    auto type = mod.add_type<WriteIterations>("WriteIterations");
    type.method(
        "getindex1!", [](WriteIterations &w, const key_type &k) -> Iteration & {
            return w[k];
        });
}
