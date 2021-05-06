// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    ifdef _MSC_VER
#        include <crtdbg.h>
#        ifndef assert
#            define assert _ASSERT
#        endif
#    else
#        include <assert.h>
#    endif
#    ifdef _DEBUG
#        include <crtdbg.h>
#    endif // _WIN32 && _DEBUG
#else
#endif // !_WIN32
