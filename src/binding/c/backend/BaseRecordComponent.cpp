#include <openPMD/binding/c/backend/BaseRecordComponent.h>

#include <openPMD/backend/BaseRecordComponent.hpp>

#include <cassert>
#include <cstdlib>

const openPMD_Attributable *openPMD_BaseRecordComponent_getConstAttributable(
    const openPMD_BaseRecordComponent *component)
{
    const auto cxx_component = (const openPMD::BaseRecordComponent *)component;
    const auto cxx_attributable = (const openPMD::Attributable *)cxx_component;
    const auto attributable = (const openPMD_Attributable *)cxx_attributable;
    return attributable;
}

openPMD_Attributable *openPMD_BaseRecordComponent_getAttributable(
    openPMD_BaseRecordComponent *component)
{
    const auto cxx_component = (openPMD::BaseRecordComponent *)component;
    const auto cxx_attributable = (openPMD::Attributable *)cxx_component;
    const auto attributable = (openPMD_Attributable *)cxx_attributable;
    return attributable;
}

void openPMD_BaseRecordComponent_resetDatatype(
    openPMD_BaseRecordComponent *component, openPMD_Datatype datatype)
{
    const auto cxx_component = (openPMD::BaseRecordComponent *)component;
    cxx_component->resetDatatype(openPMD::Datatype(datatype));
}

openPMD_Datatype openPMD_BaseRecordComponent_getDatatype(
    const openPMD_BaseRecordComponent *component)
{
    const auto cxx_component = (const openPMD::BaseRecordComponent *)component;
    return openPMD_Datatype(cxx_component->getDatatype());
}

bool openPMD_BaseRecordComponent_constant(
    const openPMD_BaseRecordComponent *component)
{
    const auto cxx_component = (const openPMD::BaseRecordComponent *)component;
    return cxx_component->constant();
}

openPMD_ChunkTable openPMD_BaseRecordComponent_availableChunks(
    openPMD_BaseRecordComponent *component)
{
    const auto cxx_component = (openPMD::BaseRecordComponent *)component;
    const auto cxx_chunkTable = cxx_component->availableChunks();
    openPMD_ChunkTable chunkTable;
    chunkTable.size = cxx_chunkTable.size();
    chunkTable.writtenChunkInfo = (openPMD_WrittenChunkInfo *)malloc(
        chunkTable.size * sizeof *chunkTable.writtenChunkInfo);
    for (size_t n = 0; n < chunkTable.size; ++n)
    {
        const auto &cxx_writtenChunkInfo = cxx_chunkTable[n];
        openPMD_ChunkInfo chunkInfo;
        chunkInfo.size = cxx_chunkTable[n].offset.size();
        assert(cxx_chunkTable[n].extent.size() == chunkInfo.size);
        chunkInfo.offset =
            (uint64_t *)malloc(chunkInfo.size * sizeof *chunkInfo.offset);
        chunkInfo.extent =
            (uint64_t *)malloc(chunkInfo.size * sizeof *chunkInfo.extent);
        for (size_t d = 0; d < chunkInfo.size; ++d)
        {
            chunkInfo.offset[d] = cxx_writtenChunkInfo.offset[d];
            chunkInfo.extent[d] = cxx_writtenChunkInfo.extent[d];
        }
        chunkTable.writtenChunkInfo[n].chunkInfo = chunkInfo;
        chunkTable.writtenChunkInfo[n].sourceID = cxx_writtenChunkInfo.sourceID;
    }
    return chunkTable;
}
