#pragma once
/*
 * md5.h:  Header file for Colin Plumb's MD5 implementation.
 *         Modified by Ian Jackson so as not to use Colin Plumb's
 *         'usuals.h'.
 *
 *         This file is in the public domain.
 */

#include <array>
#include <cstddef>
#include <cstdint>

#define MD5_DIGEST_LENGTH 16

void md5(const uint8_t* buf, size_t len, std::array<uint8_t, 16>& digest);

struct md5Context
{
    std::array<uint32_t, 4> buf;
    uint64_t bytes;
    std::array<uint32_t, 16> in;
};

void md5Init(md5Context* ctx);
void md5Update(md5Context* ctx, uint8_t const* buf, size_t len);
void md5Final(md5Context* ctx, std::array<uint8_t, 16>& digest);
void md5Transform(std::array<uint32_t, 4>& buf, const std::array<uint32_t, 16>& in);
