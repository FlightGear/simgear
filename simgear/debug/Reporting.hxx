// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

namespace simgear
{

class ReportBadAllocGuard final
{
public:
    ReportBadAllocGuard();
    ~ReportBadAllocGuard(); // non-virtual is intentional

    static bool isSet();
};


}
