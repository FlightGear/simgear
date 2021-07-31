#include "Reporting.hxx"

namespace simgear
{

thread_local bool perThread_reportBadAlloc = true;

ReportBadAllocGuard::ReportBadAllocGuard()
{
    perThread_reportBadAlloc = false;
}

 
ReportBadAllocGuard::~ReportBadAllocGuard()
{
    perThread_reportBadAlloc = true;
}

bool ReportBadAllocGuard::isSet()
{
    return perThread_reportBadAlloc;
}

}