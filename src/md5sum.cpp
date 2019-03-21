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
#include "md5sum.h"
#include "md5.h"
#include <array>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <string>

int md5file(FILE* fp, std::string& digest)
{
    if(!fp)
        return -1;
    std::array<unsigned char, 16> d;
    std::array<unsigned char, 1024> buf;
    md5Context ctx;

    md5Init(&ctx);
    size_t n;
    while((n = fread(buf.data(), 1, sizeof(buf), fp)) > 0)
        md5Update(&ctx, buf.data(), n);
    md5Final(&ctx, d);

    std::stringstream s;
    for(unsigned char i : d)
        s << std::hex << std::setiosflags(std::ios::fixed) << std::setfill('0') << std::setw(2) << static_cast<int>(i);

    digest = s.str();

    if(ferror(fp))
        return -1;
    return 0;
}
