// Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "md5sum.h"
#include "s25util/md5.hpp"
#include <array>
#include <cstdint>

int md5file(FILE* fp, std::string& digest)
{
    if(!fp)
        return -1;
    std::array<uint8_t, 1024> buf;
    s25util::md5 md5("");

    size_t n;
    while((n = fread(buf.data(), 1, buf.size(), fp)) > 0)
        md5.process(buf.data(), n, true);

    digest = md5.toString();

    if(ferror(fp))
        return -1;
    return 0;
}
