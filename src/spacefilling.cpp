#include "spacefilling.hpp"
#include "chunk.hpp"
#include "globals.hpp"

#include <cstdint>

// http://and-what-happened.blogspot.com/2011/08/fast-2d-and-3d-hilbert-curves-and.html
namespace SpaceFilling
{
    uint32_t MortonToHilbert3D(const uint32_t morton, const uint32_t bits)
    {
        uint32_t hilbert = morton;
        if (bits > 1)
        {
            uint32_t block = ((bits * 3) - 3);
            uint32_t hcode = ((hilbert >> block) & 7);
            uint32_t mcode, shift, signs;
            shift = signs = 0;
            while (block)
            {
                block -= 3;
                hcode <<= 2;
                mcode = ((0x20212021 >> hcode) & 3);
                shift = ((0x48 >> (7 - shift - mcode)) & 3);
                signs = ((signs | (signs << 3)) >> mcode);
                signs = ((signs ^ (0x53560300 >> hcode)) & 7);
                mcode = ((hilbert >> block) & 7);
                hcode = mcode;
                hcode = (((hcode | (hcode << 3)) >> shift) & 7);
                hcode ^= signs;
                hilbert ^= ((mcode ^ hcode) << block);
            }
        }
        hilbert ^= ((hilbert >> 1) & 0x92492492);
        hilbert ^= ((hilbert & 0x92492492) >> 1);
        return (hilbert);
    }

    uint32_t HilbertToMorton3D(const uint32_t hilbert, const uint32_t bits)
    {
        uint32_t morton = hilbert;
        morton ^= ((morton & 0x92492492) >> 1);
        morton ^= ((morton >> 1) & 0x92492492);
        if (bits > 1)
        {
            uint32_t block = ((bits * 3) - 3);
            uint32_t hcode = ((morton >> block) & 7);
            uint32_t mcode, shift, signs;
            shift = signs = 0;
            while (block)
            {
                block -= 3;
                hcode <<= 2;
                mcode = ((0x20212021 >> hcode) & 3);
                shift = ((0x48 >> (4 - shift + mcode)) & 3);
                signs = ((signs | (signs << 3)) >> mcode);
                signs = ((signs ^ (0x53560300 >> hcode)) & 7);
                hcode = ((morton >> block) & 7);
                mcode = hcode;
                mcode ^= signs;
                mcode = (((mcode | (mcode << 3)) >> shift) & 7);
                morton ^= ((hcode ^ mcode) << block);
            }
        }
        return (morton);
    }

    uint32_t Morton_3D_Encode_5bit(uint32_t index1, uint32_t index2, uint32_t index3)
    { // pack 3 5-bit indices into a 15-bit Morton code
        index1 &= 0x0000001f;
        index2 &= 0x0000001f;
        index3 &= 0x0000001f;
        index1 *= 0x01041041;
        index2 *= 0x01041041;
        index3 *= 0x01041041;
        index1 &= 0x10204081;
        index2 &= 0x10204081;
        index3 &= 0x10204081;
        index1 *= 0x00011111;
        index2 *= 0x00011111;
        index3 *= 0x00011111;
        index1 &= 0x12490000;
        index2 &= 0x12490000;
        index3 &= 0x12490000;
        return ((index1 >> 16) | (index2 >> 15) | (index3 >> 14));
    }

    void Morton_3D_Decode_5bit(const uint32_t morton,
                               uint32_t &index1, uint32_t &index2, uint32_t &index3)
    { // unpack 3 5-bit indices from a 15-bit Morton code
        uint32_t value1 = morton;
        uint32_t value2 = (value1 >> 1);
        uint32_t value3 = (value1 >> 2);
        value1 &= 0x00001249;
        value2 &= 0x00001249;
        value3 &= 0x00001249;
        value1 |= (value1 >> 2);
        value2 |= (value2 >> 2);
        value3 |= (value3 >> 2);
        value1 &= 0x000010c3;
        value2 &= 0x000010c3;
        value3 &= 0x000010c3;
        value1 |= (value1 >> 4);
        value2 |= (value2 >> 4);
        value3 |= (value3 >> 4);
        value1 &= 0x0000100f;
        value2 &= 0x0000100f;
        value3 &= 0x0000100f;
        value1 |= (value1 >> 8);
        value2 |= (value2 >> 8);
        value3 |= (value3 >> 8);
        value1 &= 0x0000001f;
        value2 &= 0x0000001f;
        value3 &= 0x0000001f;
        index1 = value1;
        index2 = value2;
        index3 = value3;
    }

    uint32_t Morton_3D_Encode_10bit(uint32_t index1, uint32_t index2, uint32_t index3)
    { // pack 3 10-bit indices into a 30-bit Morton code
        index1 &= 0x000003ff;
        index2 &= 0x000003ff;
        index3 &= 0x000003ff;
        index1 |= (index1 << 16);
        index2 |= (index2 << 16);
        index3 |= (index3 << 16);
        index1 &= 0x030000ff;
        index2 &= 0x030000ff;
        index3 &= 0x030000ff;
        index1 |= (index1 << 8);
        index2 |= (index2 << 8);
        index3 |= (index3 << 8);
        index1 &= 0x0300f00f;
        index2 &= 0x0300f00f;
        index3 &= 0x0300f00f;
        index1 |= (index1 << 4);
        index2 |= (index2 << 4);
        index3 |= (index3 << 4);
        index1 &= 0x030c30c3;
        index2 &= 0x030c30c3;
        index3 &= 0x030c30c3;
        index1 |= (index1 << 2);
        index2 |= (index2 << 2);
        index3 |= (index3 << 2);
        index1 &= 0x09249249;
        index2 &= 0x09249249;
        index3 &= 0x09249249;
        return (index1 | (index2 << 1) | (index3 << 2));
    }

    void Morton_3D_Decode_10bit(const uint32_t morton,
                                uint32_t &index1, uint32_t &index2, uint32_t &index3)
    { // unpack 3 10-bit indices from a 30-bit Morton code
        uint32_t value1 = morton;
        uint32_t value2 = (value1 >> 1);
        uint32_t value3 = (value1 >> 2);
        value1 &= 0x09249249;
        value2 &= 0x09249249;
        value3 &= 0x09249249;
        value1 |= (value1 >> 2);
        value2 |= (value2 >> 2);
        value3 |= (value3 >> 2);
        value1 &= 0x030c30c3;
        value2 &= 0x030c30c3;
        value3 &= 0x030c30c3;
        value1 |= (value1 >> 4);
        value2 |= (value2 >> 4);
        value3 |= (value3 >> 4);
        value1 &= 0x0300f00f;
        value2 &= 0x0300f00f;
        value3 &= 0x0300f00f;
        value1 |= (value1 >> 8);
        value2 |= (value2 >> 8);
        value3 |= (value3 >> 8);
        value1 &= 0x030000ff;
        value2 &= 0x030000ff;
        value3 &= 0x030000ff;
        value1 |= (value1 >> 16);
        value2 |= (value2 >> 16);
        value3 |= (value3 >> 16);
        value1 &= 0x000003ff;
        value2 &= 0x000003ff;
        value3 &= 0x000003ff;
        index1 = value1;
        index2 = value2;
        index3 = value3;
    }

    void initLUT()
    {
        uint32_t morton, hilbert,j;
        for (uint32_t i = 0; i < CHUNK_SIZE; i++)
        {
            for (uint32_t y = 0; y < CHUNK_SIZE; y++)
            {
                for (uint32_t k = 0; k < CHUNK_SIZE; k++)
                {
                    j = CHUNK_SIZE - 1 - y;
                    morton = Morton_3D_Encode_10bit(i, j, k);
                    hilbert = MortonToHilbert3D(morton, 2); // 4 hilbert bits

                    MORTON_XYZ_ENCODE[i][j][k] = morton;
                    MORTON_XYZ_DECODE[morton][0] = i;
                    MORTON_XYZ_DECODE[morton][1] = j;
                    MORTON_XYZ_DECODE[morton][2] = k;

                    HILBERT_XYZ_ENCODE[i][j][k] = hilbert;
                    HILBERT_XYZ_DECODE[hilbert][0] = i;
                    HILBERT_XYZ_DECODE[hilbert][1] = j;
                    HILBERT_XYZ_DECODE[hilbert][2] = k;
                }
            }
        }
    }
};
