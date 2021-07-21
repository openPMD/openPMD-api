// ChunkInfo

#include "defs.hpp"

// Define supertype relationships
namespace jlcxx {
template <> struct SuperType<WrittenChunkInfo> { typedef ChunkInfo type; };
} // namespace jlcxx

void define_julia_ChunkInfo(jlcxx::Module &mod) {
  auto chunkInfo = mod.add_type<ChunkInfo>("CXX_ChunkInfo");
  chunkInfo.constructor<>();
  chunkInfo.constructor<Offset, Extent>();
  chunkInfo.method("cxx_offset",
                   [](const ChunkInfo &chunkInfo) { return chunkInfo.offset; });
  chunkInfo.method("cxx_extent",
                   [](const ChunkInfo &chunkInfo) { return chunkInfo.extent; });

  auto writtenChunkInfo = mod.add_type<WrittenChunkInfo>(
      "CXX_WrittenChunkInfo", jlcxx::julia_base_type<ChunkInfo>());
  writtenChunkInfo.constructor<>();
  writtenChunkInfo.constructor<Offset, Extent>();
}
