/*
 * md5.h:  Header file for Colin Plumb's MD5 implementation.
 *         Modified by Ian Jackson so as not to use Colin Plumb's
 *         'usuals.h'.
 *
 *         This file is in the public domain.
 */

#ifndef MD5_H
#define MD5_H

#include <cstddef>
#include <stdint.h>

#define MD5_DIGEST_LENGTH 16

void md5(const uint8_t* buf, size_t len, uint8_t* digest);

struct md5Context
{
    uint32_t buf[4];
    uint64_t bytes;
    uint32_t in[16];
};

void md5Init(struct md5Context* context);
void md5Update(struct md5Context* context, uint8_t const* buf, size_t len);
void md5Final(struct md5Context* context, unsigned char digest[16]);
void md5Transform(uint32_t buf[4], const uint32_t in[16]);

#endif
