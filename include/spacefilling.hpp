#ifndef SPACEFILLING_H
#define SPACEFILLING_H

#include <cstdint>

// http://and-what-happened.blogspot.com/2011/08/fast-2d-and-3d-hilbert-curves-and.html
namespace SpaceFilling
{
    uint32_t MortonToHilbert3D(const uint32_t morton, const uint32_t bits);

    uint32_t HilbertToMorton3D(const uint32_t hilbert, const uint32_t bits);

    uint32_t Morton_3D_Encode_5bit(uint32_t index1, uint32_t index2, uint32_t index3);

    void Morton_3D_Decode_5bit(const uint32_t morton,
                               uint32_t &index1, uint32_t &index2, uint32_t &index3);

    uint32_t Morton_3D_Encode_10bit(uint32_t index1, uint32_t index2, uint32_t index3);

    void Morton_3D_Decode_10bit(const uint32_t morton,
                                uint32_t &index1, uint32_t &index2, uint32_t &index3);

    void initLUT();
};

#endif
