#pragma once

#include "debug_types.h"

/*
Support for additional logging control based on source filename, line number
and function name.

If environmental variable SG_LOG_DELTAS is defined, we use it to
increment/decrement logging levels based on source filename prefixes, function
name prefixes and source line numbers.

SG_LOG_DELTAS is a comma or space-separated list of individual items, each of
which specifies a source filename prefix, a function name prefix, a line number
and a log level delta.

Items are not added together, and the order in which items are specified does
not matter. Instead, matching items with longer filename prefixes override
matching items with short filename prefixes. If filename prefixes are the same
length, we compare based on the length of the items' function prefixes.

The syntax for each item is one of:

    <filename-prefix>=<delta>
    <filename-prefix>:<function-prefix>=<delta>
    <filename-prefix>:<function-prefix>:<line>=<delta>
    <filename-prefix>:<line>=<delta>

<delta> is required. The other fields can be empty, in which case they match
anything. So '=1' will increment the logging level for all calls to SG_LOG().

For example:
    SG_LOG_DELTAS="flightgear/=-1,flightgear/src/Scenery/SceneryPager.cxx=+2,simgear/scene/=1"

This will increment diagnostics level for all code in
flightgear/src/Scenery/SceneryPager.cxx by 2, and increment diagnostics
level in simgear/scene/ by 1. All code in flightgear/ (apart from code in
flightgear/src/Scenery/SceneryPager.cxx) will have its diagnostic level
decremented by 1.
*/

/* Returns new priority based on delta for file:line:function. */
sgDebugPriority logDeltaAdd(sgDebugPriority priority,
        const char* file, int line, const char* function,
        bool freeFilename);

/* Resets deltas. <items> should be a string in same format as $SG_LOG_DELTAS
as described above. */
void logDeltaSet(const char* items);
