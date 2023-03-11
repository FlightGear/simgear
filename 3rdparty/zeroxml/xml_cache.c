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
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
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

#include <simgear_zeroxml_config.h>

#include <sys/types.h>
#include <assert.h>

#include "xml.h"
#include "api.h"

/* number of pointers to allocate for every block increase */
# define NODE_BLOCKSIZE		16

struct _xml_node
{
    /* Cache node information */
    const struct _xml_node *parent; /* parent node */
    const cacheId **node; /* list of child nodes */
    short int max_nodes; /* maximum number of child nodes */
    short int no_nodes; /* available number of nodes */

    /* XML node information */
    short int name_len;	/* lenght of the name of the XML node */
    const char *name;	/* name of the XML node */
    int data_len;	/* lenght of the  data section of the XML node */
    const char *data;	/* data section of the XML node */
};

const cacheId*
cacheInit(const struct _root_id *rid)
{
    return CACHED_NODES(rid) ? calloc(1, sizeof(struct _xml_node))
                             : NULL;
}

void
cacheInitLevel(const cacheId *nc)
{
    struct _xml_node *cache = (struct _xml_node *)nc;
    if (cache)
    {
        assert(cache->node == 0);

        cache->node = calloc(NODE_BLOCKSIZE, sizeof(struct _xml_node *));
        cache->max_nodes = NODE_BLOCKSIZE;
    }
}

void
cacheFree(const cacheId *nc)
{
    struct _xml_node *cache = (struct _xml_node *)nc;
    if (cache)
    {
        if (cache->no_nodes)
        {
            const struct _xml_node **node = cache->node;
            int i = 0;

            while(i < cache->no_nodes) {
                cacheFree((cacheId*)node[i++]);
            }
        }
        free(cache->node);
        free(cache);
    }
}

const cacheId*
cacheNodeGet(const xmlId *id)
{
    const struct _root_id *rid = (const struct _root_id *)id;
    const cacheId *cache = NULL;

    assert(rid != 0);

    if (rid->root == rid) {
        cache = rid->node;
    }
    else
    {
        struct _xml_id *xid = (struct _xml_id *)id;
        cache = xid->node;
    }

    return cache;
}

const cacheId*
cacheNodeNew(const cacheId *nc)
{
    struct _xml_node *cache = (struct _xml_node *)nc;
    struct _xml_node *rv = NULL;

    if (cache)
    {
        int i = cache->no_nodes;
        if (i == cache->max_nodes)
        {
            int size, max_nodes;
            void *p;

            max_nodes = cache->max_nodes + NODE_BLOCKSIZE;
            size = max_nodes * sizeof(struct _xml_node*);
            if ((p = realloc(cache->node, size)) == NULL) {
                return rv;
            }

            cache->node = p;
            cache->max_nodes = max_nodes;
        }

        if ((rv = calloc(1, sizeof(struct _xml_node))) != NULL)
        {
            rv->parent = cache;
            cache->no_nodes++;
        }
        cache->node[i] = rv;
    }

    return rv;
}

void
cacheDataSet(const cacheId *nc, const char *name, int namelen, const char *data, int datalen)
{
    struct _xml_node *cache = (struct _xml_node *)nc;
    if (cache)
    {
        assert(name != 0);
        assert(namelen != 0);
        assert(data != 0);

        cache->name = name;
        cache->name_len = namelen;
        cache->data = data;
        cache->data_len = datalen;
    }
}

void
cacheNodeAdd(const cacheId *n, const char *name, int namelen, const char *data, int datalen)
{
    const cacheId *nc = cacheNodeNew(n);
    cacheDataSet(nc, name, strlen(name), data, datalen);
}

/*
 * Recursively walk the node tree to get te section with the '*element' name.
 *
 * When finished *buf will point to the start of the data section of the node,
 * *len will be set to the length of the requested data section, *element will
 * point to the actual name of the node (useful in case the name was a wildcard
 * character), *nlen will return the length of the actual name and *nodenum will
 * return the current occurence number of the requested section.
 *
 * In case of an error *element will point to the location of the error within
 * the buffer, *len will contain the error code and *nodenum the line in the
 * source code where the error happens.
 *
 * @param nc node from the node-cache
 * @param *buf starting pointer for this section
 * @param *len length to the end of the buffer
 * @param *element name of the node to look for
 * @param *elementlen length of the name of the node to look for
 * @param *nodenum which occurence of the node name to look for
 * @return a pointer to the section with the requested name or NULL in case
           of an error
 */
const char*
__zeroxml_get_node_from_cache(const cacheId **nc, const char **buf, int *len,
                      const char **element, int *elementlen, int *nodenum)
{
    struct _xml_node *cache;
    const char *name = *element;
    const char *rv = NULL;
    int found;
    int num;

    assert(buf != 0);
    assert(*buf != 0);
    assert(len != 0);
    assert(element != 0);
    assert(elementlen != 0);
    assert(nodenum != 0);

    cache = (struct _xml_node*)*nc;
    assert(cache != 0);

    num = *nodenum;
    if (cache->no_nodes == 0) /* leaf node */
    {
        rv = *buf = cache->data;
        *len = cache->data_len;
        *element = cache->name;
        *elementlen = cache->name_len;
        found = 0;
    }
    else if (num < cache->no_nodes)
    {
        if (*name == '*') /* everything goes */
        {
            const struct _xml_node *node = cache->node[(num > 0) ? num : 0];
            *nc = (cacheId*)node;
            rv = *buf = node->data;
            *len = node->data_len;
            *element = node->name;
            *elementlen = node->name_len;
            found = cache->no_nodes;
        }
        else
        {
            int namelen = *elementlen;
            int i;

            found = 0;
            for (i=0; i<cache->no_nodes; i++)
            {
                 const struct _xml_node *node = cache->node[i];

                 assert(node);

                 if ((node->name_len == namelen) &&
                     (!strncasecmp(node->name, name, namelen)))
                 {
                      if (found == num || num == -1)
                      {
                           *nc = (cacheId*)node;
                           rv = *buf = node->data;
                           *len = node->data_len;
                           *element = node->name;
                           *elementlen = node->name_len;
                           if (num != -1) {
                               break;
                           }
                      }
                      found++;
                 }
            }
        }
    }

    if (!rv)
    {
       *element = *buf;
       *elementlen = XML_NO_ERROR;
       *len = XML_NODE_NOT_FOUND;
       return rv;
    }
    *nodenum = found;

    return rv;
}

