#pragma once


#include <cstdlib>
#include <vector>


using Extent = std::vector< std::size_t >;
using Offset = std::vector< std::size_t >;

class Patch
{
public:
    Patch();

    Extent globalWindowSize() const;
    Patch& setGlobalWindowSize(Extent const&);

    Offset localWindowOffset() const;
    Patch& setLocalWindowOffset(Offset const&);

    Extent localWindowSize() const;
    Patch& setLocalWindowSize(Extent const&);

private:
    Extent m_globalWindowSize;
    Offset m_localWindowOffset;
    Extent m_localWindowSize;
};  //Patch