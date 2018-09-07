#ifndef __KERNEL_CHECKSUM_H
#define __KERNEL_CHECKSUM_H

#include "kmath.h"
#include "string_stream.h"
#include "kern/kassert.h"
#include "hdr/endian.h"
#include <string.h>

namespace kernel
{
    template<typename T>
    T checksum(const void* _base, size_t len)
    {
        kassert((len % sizeof(T)) == 0);
        const T* p = (const T*)_base;
        T sum = 0;
        while (len--)
            sum += *p++;
        return sum;
    }

    struct md5summer
    {
        uint32_t K[64] = {0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
                          0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
                          0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
                          0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
                          0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
                          0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
                          0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
                          0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
                          0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
                          0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
                          0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
                          0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
                          0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
                          0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
                          0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
                          0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
                          };
        const uint32_t s[64] = {
                7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
                4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
                };
        union
        {
            struct
            {
                uint32_t    a0 = 0x67452301;
                uint32_t    b0 = 0xefcdab89;
                uint32_t    c0 = 0x98badcfe;
                uint32_t    d0 = 0x10325476;
            };
            uint8_t digest[16];
        };

        uint64_t    total_len = 0;
        uint16_t    chunk_len = 0;
        char        chunk[64];

        inline uint32_t rotl(uint32_t v, uint8_t n)
        {
            return (v << n) | (v >> (32-n));
        }

        void process_chunk(const void* p);
        void append(const void* _p, size_t len);
        void finish();

        md5summer(const void* p, size_t len)
        {
            append(p,len);
        }

        template<size_t N>
        md5summer(const char (&p)[N])
        {
            append(p,N-1);
        }
    };

    static inline void md5sum(const void* p, size_t len, uint8_t (&digest)[16])
    {
        md5summer s(p,len);
        s.finish();
        memcpy(digest,s.digest,sizeof(digest));
    }
}

#endif /* __KERNEL_CHECKSUM_H */
