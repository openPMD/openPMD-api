#ifndef OPENPMD_CHUNKINFO_H
#define OPENPMD_CHUNKINFO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct openPMD_ChunkInfo
    {
        uint64_t *offset;
        uint64_t *extent;
        size_t size;
    } openPMD_ChunkInfo;

    void openPMD_ChunkInfo_construct(openPMD_ChunkInfo *chunkInfo);
    void openPMD_ChunkInfo_destruct(openPMD_ChunkInfo *chunkInfo);

    typedef struct openPMD_WrittenChunkInfo
    {
        openPMD_ChunkInfo chunkInfo;
        unsigned int sourceID;
    } openPMD_WrittenChunkInfo;

    void openPMD_WrittenChunkInfo_construct(
        openPMD_WrittenChunkInfo *writtenChunkInfo);
    void openPMD_WrittenChunkInfo_destruct(
        openPMD_WrittenChunkInfo *writtenChunkInfo);

    typedef struct openPMD_ChunkTable
    {
        openPMD_WrittenChunkInfo *writtenChunkInfo;
        size_t size;
    } openPMD_ChunkTable;

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_CHUNKINFO_H
