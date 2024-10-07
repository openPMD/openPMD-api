#include <openPMD/binding/c/ChunkInfo.h>

#include <openPMD/ChunkInfo.hpp>

#include <cstdlib>

void openPMD_ChunkInfo_construct(openPMD_ChunkInfo *chunkInfo)
{
    chunkInfo->offset = nullptr;
    chunkInfo->extent = nullptr;
    chunkInfo->size = 0;
}

void openPMD_ChunkInfo_destruct(openPMD_ChunkInfo *chunkInfo)
{
    free(chunkInfo->offset);
    free(chunkInfo->extent);
    chunkInfo->offset = nullptr;
    chunkInfo->extent = nullptr;
}

void openPMD_WrittenChunkInfo_construct(
    openPMD_WrittenChunkInfo *writtenChunkInfo)
{
    openPMD_ChunkInfo_construct(&writtenChunkInfo->chunkInfo);
    writtenChunkInfo->sourceID = 0;
}

void openPMD_WrittenChunkInfo_destruct(
    openPMD_WrittenChunkInfo *writtenChunkInfo)
{
    openPMD_ChunkInfo_destruct(&writtenChunkInfo->chunkInfo);
}
