#include "defs.hpp"

#include "Container.hpp"

#include <cstdint>

////////////////////////////////////////////////////////////////////////////////

JLCXX_MODULE define_julia_module(jlcxx::Module &mod) {
  add_array_type<double, 7>(mod, "array_double_7");
  add_pair_type<std::string, bool>(mod, "pair_string_bool");

  define_julia_shared_ptr(mod);

  // The order of these calls matters. Julia types need to be defined before
  // they are used.

  // Stand-alone classes
  define_julia_Access(mod);
  define_julia_ChunkInfo(mod);
  define_julia_Datatype(mod);
  define_julia_Format(mod);
  define_julia_UnitDimension(mod);
  // All classes below need at least Datatype

  define_julia_Attribute(mod);
  define_julia_Attributable(mod);
  define_julia_Dataset(mod);

  define_julia_BaseRecordComponent(mod); // needs: Attributable
  define_julia_RecordComponent(mod);     // needs: BaseRecordComponent
  define_julia_MeshRecordComponent(mod); // needs: RecordComponent

  define_julia_Container<MeshRecordComponent>(
      mod); // needs: Attributable, MeshRecordComponent

  define_julia_Mesh(mod); // needs: Container<MeshRecordComponent>

  define_julia_Container<Mesh>(mod); // needs: Attributable

  define_julia_Iteration(mod); // needs: Attributable, Container<Mesh>

  define_julia_Container<Iteration, std::uint64_t>(mod); // needs: Attributable

  define_julia_WriteIterations(mod); // needs: Iteration

  // The main class
  define_julia_Series(mod);

  // Handle metadata
  define_julia_version(mod);
}
