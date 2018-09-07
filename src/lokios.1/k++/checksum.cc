#include "checksum.h"

void
kernel::md5summer::process_chunk(const void* p)
{
    // Process a single 512-bit chunk of data.
    auto* M    = (const uint32_t*)p;
    uint32_t A = a0;
    uint32_t B = b0;
    uint32_t C = c0;
    uint32_t D = d0;

    for (size_t i=0; i<64; ++i)
    {
        uint32_t F;
        uint32_t g;
        if (i < 16)
        {
            F = (B & C) | (~B & D);
            g = i;
        }
        else if (i < 32)
        {
            F = (D & B) | (~D & C);
            g = (5*i + 1) % 16;
        }
        else if (i < 48)
        {
            F = (B ^ C ^ D);
            g = (3*i + 5) % 16;
        }
        else
        {
            F = C ^ (B | ~D);
            g = (7*i) % 16;
        }
        F += A + K[i] + M[g];
        A  = D;
        D  = C;
        C  = B;
        B += rotl(F, s[i]);
    }

    a0 += A;
    b0 += B;
    c0 += C;
    d0 += D;
}

void
kernel::md5summer::append(const void* _p, size_t len)
{
    total_len += len;

    auto* p = (uint8_t*)_p;
    if (chunk_len)
    {
        uint16_t chunk_rem = sizeof(chunk) - chunk_len;
        uint32_t copy_len  = MIN(chunk_rem,len);
        memcpy(chunk + chunk_len,p,copy_len);
        chunk_len += copy_len;
        if (chunk_len != sizeof(chunk))
            return;

        process_chunk(chunk);
        len      -= copy_len;
        p        += copy_len;
        chunk_len = 0;
    }

    while (len >= 64)
    {
        process_chunk(p);
        p   += 64;
        len -= 64;
    }

    memcpy(chunk,p,len);
    chunk_len = len;
}

void
kernel::md5summer::finish()
{
    chunk[chunk_len++] = 0x80;
    if (chunk_len == sizeof(chunk))
    {
        process_chunk(chunk);
        chunk_len = 0;
    }

    while (chunk_len != 56)
    {
        chunk[chunk_len++] = 0x00;
        if (chunk_len == sizeof(chunk))
        {
            process_chunk(chunk);
            chunk_len = 0;
        }
    }
    uint64_t total_len_bits = total_len*8;
    memcpy(chunk + 56,&total_len_bits,sizeof(total_len_bits));
    process_chunk(chunk);
}
