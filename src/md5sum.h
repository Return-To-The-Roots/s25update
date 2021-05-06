// Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
// Copyright (c) 2005 - 2015 FloSoft (webmaster at flo-soft.de)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdio>
#include <string>

int md5file(FILE* fp, std::string& digest);
