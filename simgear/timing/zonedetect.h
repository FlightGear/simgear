// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2018 Bertold Van den Bergh <vandenbergh@bertold.org>

/**
 * @file
 * @brief Detect timezones and locations based on latitude and longitude
 */

#include <stddef.h>
#include <stdint.h>

#ifndef INCL_ZONEDETECT_H_
#define INCL_ZONEDETECT_H_

#if !defined(ZD_EXPORT)
#if defined(_MSC_VER)
#define ZD_EXPORT __declspec(dllimport)
#else
#define ZD_EXPORT
#endif
#endif

typedef enum {
    ZD_LOOKUP_IGNORE = -3,
    ZD_LOOKUP_END = -2,
    ZD_LOOKUP_PARSE_ERROR = -1,
    ZD_LOOKUP_NOT_IN_ZONE = 0,
    ZD_LOOKUP_IN_ZONE = 1,
    ZD_LOOKUP_IN_EXCLUDED_ZONE = 2,
    ZD_LOOKUP_ON_BORDER_VERTEX = 3,
    ZD_LOOKUP_ON_BORDER_SEGMENT = 4
} ZDLookupResult;

typedef struct {
    ZDLookupResult lookupResult;

    uint32_t polygonId;
    uint32_t metaId;
    uint8_t numFields;
    char **fieldNames;
    char **data;
} ZoneDetectResult;

struct ZoneDetectOpaque;
typedef struct ZoneDetectOpaque ZoneDetect;

#ifdef __cplusplus
extern "C" {
#endif

ZD_EXPORT ZoneDetect *ZDOpenDatabase(const char *path);
ZD_EXPORT ZoneDetect *ZDOpenDatabaseFromMemory(void* buffer, size_t length);
ZD_EXPORT void        ZDCloseDatabase(ZoneDetect *library);

ZD_EXPORT ZoneDetectResult *ZDLookup(const ZoneDetect *library, float lat, float lon, float *safezone);
ZD_EXPORT void              ZDFreeResults(ZoneDetectResult *results);

ZD_EXPORT const char *ZDGetNotice(const ZoneDetect *library);
ZD_EXPORT uint8_t     ZDGetTableType(const ZoneDetect *library);
ZD_EXPORT const char *ZDLookupResultToString(ZDLookupResult result);

ZD_EXPORT int         ZDSetErrorHandler(void (*handler)(int, int));
ZD_EXPORT const char *ZDGetErrorString(int errZD);

ZD_EXPORT float* ZDPolygonToList(const ZoneDetect *library, uint32_t polygonId, size_t* length);

ZD_EXPORT char* ZDHelperSimpleLookupString(const ZoneDetect* library, float lat, float lon);
ZD_EXPORT void ZDHelperSimpleLookupStringFree(char* str);

#ifdef __cplusplus
}
#endif

#endif // INCL_ZONEDETECT_H_
