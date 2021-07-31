#pragma once

namespace simgear
{

class ReportBadAllocGuard
{
public:
    ReportBadAllocGuard();
    ~ReportBadAllocGuard();

    static bool isSet();
};


}
