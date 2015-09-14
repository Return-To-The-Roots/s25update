/*
 * md5sum.c - Generate/check MD5 Message Digests
 *
 * Compile and link with md5.c.  If you don't have getopt() in your library
 * also include getopt.c.  For MSDOS you can also link with the wildcard
 * initialization function (wildargs.obj for Turbo C and setargv.obj for MSC)
 * so that you can use wildcards on the commandline.
 *
 * Make sure that you compile with -DHIGHFIRST if you are on a big-endian
 * system. Here are some examples of correct MD5 sums:
 *
 * % echo "The meeting last week was swell." | md5sum
 * 050f3905211cddf36107ffc361c23e3d
 *
 * % echo 'There is $1500 in the blue box.' | md5sum
 * 05f8cfc03f4e58cbee731aa4a14b3f03
 *
 * If you get anything else than this, then you've done something wrong. ;)
 *
 * Written March 1993 by Branko Lankester
 * Modified June 1993 by Colin Plumb for altered md5.c.
 */
#include <cstdio>

#include <sstream>
#include <iomanip>
#include <string>

#include "md5.h"
#include "md5sum.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *
 */
int md5file(FILE* fp, std::string& digest)
{
    unsigned char d[16];
    unsigned char buf[1024];
    md5Context ctx;

    md5Init(&ctx);
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
        md5Update(&ctx, buf, n);
    md5Final(&ctx, d);

    std::stringstream s;
    for (int i = 0; i < 16; ++i)
        s << std::hex << std::setiosflags(std::ios::fixed) << std::setfill('0') << std::setw(2) << static_cast<int>(d[i]);

    digest = s.str();

    if (ferror(fp))
        return -1;
    return 0;
}

