/*
 * This software is available under 2 licenses -- choose whichever you prefer.
 *
 * ALTERNATIVE A - Modified BSD license
 *
 * Copyright (C) 2008-2023 by Erik Hofman.
 * Copyright (C) 2009-2023 by Adalin B.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *        this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY ADALIN B.V. ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL ADALIN B.V. OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUTOF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of Adalin B.V.
 *
 * -----------------------------------------------------------------------------
 * ALTERNATIVE B - Public Domain (www.unlicense.org)
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or distribute
 * this software, either in source code form or as a compiled binary, for any
 * purpose, commercial or non-commercial, and by any means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors of
 * this software dedicate any and all copyright interest in the software to
 * the public domain. We make this dedication for the benefit of the public at
 * large and to the detriment of our heirs and successors. We intend this
 * dedication to be an overt act of relinquishment in perpetuity of all
 * present and future rights to this software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __XML_NODECACHE
#define __XML_NODECACHE 1

#ifdef __cplusplus
extern "C" {
#endif

#include <xml.h> 

typedef struct _xml_node cacheId;

/**
 * Initialize a new cacheId structure.
 *
 * @return cacheId which is used for further processing
 */
const cacheId *cacheInit();

/**
 * Initialize a new level in the XML-tree.
 *
 * This is required to be able to assign new XML sub-nodes.
 *
 * @param cid Cache-id
 */
void cacheInitLevel(const cacheId *cid);

/**
 * Free a Cache-id.
 *
 * All allocations of the XML-tree will be freed.
 *
 * @param xid Cache-id to be freed.
 */
void cacheFree(const cacheId *cid);

/**
 * Allocate a new XML-node in the XML-tree.
 *
 * @param cid Cache-id
 * @return a pointer to the the newly created Cache-id
 */
const cacheId *cacheNodeNew(const cacheId *cid);

/**
 * Return the Cache-id which is associated with the XML-id.
 *
 * @param xid XML-id
 * @return a pointer to the the Cache-id
 */
const cacheId *cacheNodeGet(const xmlId *xid);

/**
 * Set all data for the Cache-id.
 *
 * @param cid Cache-id
 * @param name a pointer to the name-string
 * @param namelen the length of the name-string
 * @param data a pointer to the node data section
 * @param datalen the length of the node data section
 */
void cacheDataSet(const cacheId *cid, const char *name, int namelen, const char *data, int datalen);

/**
 * Allocate a new XML-node in the XML-tree and set all data for the Cache-id.
 *
 * This function combined cacheNodeNew and cacheDataSet.
 *
 * @param cid Cache-id
 * @param name a pointer to the name-string
 * @param namelen the length of the name-string
 * @param data a pointer to the node data section
 * @param datalen the length of the node data section
 */
void cacheNodeAdd(const cacheId *cid, const char *name, int namelen, const char *data, int datalen);

/**
 * Get the data from a cached node.
 *
 * When finished *cid will return a pointer to the request node XML-id,
 * *start points to the data section with the requested name
 * *len will be set to the length of the requested section,
 * *name will point to the actual name of the node (useful in case the name was
 * a wildcard character), *nlen will return the length of the actual name and
 * *nodenum will return the current occurence number of the requested section.
 *
 * @param cid Cache-id
 * @param start starting pointer for this section
 * @param len length to the end of the buffer
 * @param *name name of the node to look for
 * @param rlen length of the name of the node to look for
 * @param nodenum which occurence of the node name to look for
 * @return a pointer right after the section or NULL in case of an error
 */
const char* __zeroxml_get_node_from_cache(const cacheId **cid, const char **start, int *len, const char **name, int *rlen , int *nodenum);

#ifdef __cplusplus
}
#endif

#endif /* __XML_NODECACHE */

