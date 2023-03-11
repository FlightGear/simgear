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

#ifndef __XML_CONFIG
#define __XML_CONFIG 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <limits.h>

#ifdef HAVE_RMALLOC_H
# include <rmalloc.h>
#elif HAVE_UNISTD_H
# include <unistd.h>
#endif

#if defined _WIN32 || defined __CYGWIN__
# define XML_APIENTRY __cdecl
# define XML_API 
#else
# define XML_APIENTRY 
# define XML_API __attribute__((visibility("hidden")))
#endif

#if defined(TARGET_OS_MAC) && TARGET_OS_MAC
# pragma export on
#endif

#define XML_MAJOR_VERSION	2
#define XML_MINOR_VERSION	3
#define XML_MICRO_VERSION	3

#ifdef _MSC_VER
# include <float.h>
# pragma warning( disable : 4056 )
# define XML_FPINFINITE         (DBL_MAX+DBL_MAX)
# define XML_FPNONE             (XML_FPINFINITE-XML_FPINFINITE)
#else
# define XML_FPNONE             (0.0/0.0)
# define XML_FPINFINITE         (1.0/0.0)
#endif
#define XML_BOOL_NONE		INT_MIN
#define XML_NONE		LONG_MIN
#define XML_FALSE		0
#define XML_TRUE		1
#define XML_COMMENT		"!--"

enum
{
    XML_NO_ERROR = 0,
    XML_OUT_OF_MEMORY,
    XML_FILE_NOT_FOUND,
    XML_NODE_NOT_FOUND,
    XML_UNEXPECTED_EOF,
    XML_TRUNCATE_RESULT,
    XML_INVALID_COMMENT,
    XML_INVALID_INFO_BLOCK,
    XML_ELEMENT_NO_OPENING_TAG,
    XML_ELEMENT_NO_CLOSING_TAG,
    XML_ATTRIB_NO_OPENING_QUOTE,
    XML_ATTRIB_NO_CLOSING_QUOTE,
    XML_INVALID_MULTIBYTE_SEQUENCE,
    XML_MAX_ERROR
};

enum xmlFlags
{
    XML_INDEX_STARTS_AT_ONE  = 0x0001,
    XML_INDEX_STARTS_AT_ZERO = 0x0002,

    XML_RETURN_NONE_VALUE    = 0x0004,
    XML_RETURN_ZERO          = 0x0008,

    XML_CASE_INSENSITIVE     = 0x0010,
    XML_CASE_SENSITIVE       = 0x0020,

    XML_IGNORE_COMMENT       = 0x0040,
    XML_COMMENT_AS_NODE      = 0x0080,

    XML_NONVALIDATING        = 0x0100,
    XML_VALIDATING           = 0x0200,

    XML_SCAN_NODES           = 0x0400,
    XML_CACHE_NODES          = 0x0800,

    XML_US_ASCII             = 0x1000,
    XML_LOCALIZATION         = 0x2000,

    XML_DEFAULT_FLAGS        = -1
};

typedef struct _root_id xmlId;

/**
 * Open an XML file for processing.
 *
 * No data is being allocated for the file. All actions are in mmap-ed
 * file buffers.
 *
 * For the XML_COMMENT_AS_NODE/XML_IGNORE_COMMENT and
 * XML_SCAN_NODES/XML_CACHE_NODES modes it is required to set them when
 * creating the XML-id. Therefore the xmlOpenFlags function should be used
 * with the appropriate flags.
 *
 * @param fname path to the file 
 * @param flags one or more modes of operation for the interpreter
 * @return XML-id which is used for further processing
 */
XML_API xmlId* XML_APIENTRY xmlOpen(const char *fname);
XML_API xmlId* XML_APIENTRY xmlOpenFlags(const char *fname, enum xmlFlags flags);

/**
 * Process a section of XML code in a preallocated buffer.
 * The buffer may not be freed until xmlClose has been called.
 *
 * For the XML_COMMENT_AS_NODE/XML_IGNORE_COMMENT and
 * XML_SCAN_NODES/XML_CACHE_NODES modes it is required to set them when
 * creating the XML-id. Therefore the xmlInitBufferFlags function should be used
 * with the appropriate flags.
 *
 * @param buffer pointer to the buffer
 * @param size size of the buffer
 * @param flags one or more modes of operation for the interpreter
 * @return XML-id which is used for further processing
 */
XML_API xmlId* XML_APIENTRY xmlInitBuffer(const char *buffer, int size);
XML_API xmlId* XML_APIENTRY xmlInitBufferFlags(const char *buffer, int size, enum xmlFlags flags);

/**
 * Close the XML file after which no further processing is possible.
 *
 * @param xid XML-id
 */
XML_API void XML_APIENTRY xmlClose(xmlId *xid);

/**
 * Set one or more modes of operation for the interpreter.
 *
 * @param flags a bit wise flag containing one or more mode options
 */
XML_API void XML_APIENTRY xmlSetFlags(const xmlId *xid, enum xmlFlags flags);


/**
 * Test whether the node path exists.
 *
 * A node path may be a solitary node name or a node path separated by the
 * slash character '/' (in which case the code walks the XML tree).
 *
 * Node names adhere to the XML convention for valid node names, may be the
 * asterisk character '*' to indicate that any name is acceptable or may
 * contain a question mark '?' to indicate that any character is acceptable
 * for that particular location.
 * Node names may also specify which occurrence of a particular name to look
 * for by specifying the number, starting at one, between straight brackets.
 * e.g. node[1] or "*[3]" to get the fourth node, regardless of the names.
 *
 * If node points to XML_COMMENT then the function will test whether the
 * current node is a comment node.
 *
 * @param xid XML-id
 * @param path path to the node containing the subsection
 * @return true if the XML-subsection-id exists, false otherwise.
 */
XML_API int XML_APIENTRY xmlNodeTest(const xmlId *xid, const char *path);

/**
 * Locate a subsection of the XML tree for further processing.
 * The memory allocated for the XML-subsection-id has to be freed by the
 * calling process using xmlFree.
 *
 * This method adds processing speed since the required nodes will only be
 * searched in the subsection.
 *
 * For a description of node paths see xmlNodeTest.
 *
 * @param xid XML-id
 * @param path path to the node containing the subsection
 * @return XML-subsection-id for further processing
 */
XML_API xmlId* XML_APIENTRY xmlNodeGet(const xmlId *xid, const char *path);

/**
 * Copy a subsection of the XML tree for further processing.
 * This is useful when it's required to process a section of the XML code
 * after the file has been closed. The drawback is the added memory
 * requirements.
 *
 * For a description of node paths see xmlNodeTest.
 *
 * The memory allocated for the XML-subsection-id has to be freed by the
 * calling process using xmlClose.
 *
 * @param xid XML-id
 * @param path path to the node containing the subsection
 * @return XML-subsection-id for further processing
 */
XML_API xmlId* XML_APIENTRY xmlNodeCopy(const xmlId *xid, const char *path);


/**
 * Return the name of this node.
 * The returned string has to be freed by the calling process using xmlFree.
 *
 * @param xid XML-id
 * @return a newly allocated string containing the node name
 */
XML_API char* XML_APIENTRY xmlNodeGetName(const xmlId *xid);

/**
 * Copy the name of this node in a pre-allocated buffer.
 *
 * @param xid XML-id
 * @param buffer the buffer to copy the string to
 * @param buflen length of the destination buffer
 * @return the length of the node name
 */
XML_API int XML_APIENTRY xmlNodeCopyName(const xmlId *xid, char *buffer, int buflen);

/**
 * Compare the name of a node to a string.
 * Comparing is done in a case insensitive way.
 *
 * @param xid XML-id
 * @param str the string to compare to
 * @return an integer less than, equal to, or greater than zero if the value
 * of the node is found, respectively, to be less than, to match, or be greater
 * than str
 */
XML_API int XML_APIENTRY xmlNodeCompareName(const xmlId *xid, const char *str);


/**
 * Return the name of the nth attribute.
 *
 * @param xid XML-id
 * @param n position of the attribute in the attribute list. starts at 0.
 * @return a newly allocated string containing the name of the attribute.
 */

XML_API char* XML_APIENTRY xmlAttributeGetName(const xmlId *xid, int n);

/**
 * Copy the name of the nth attribute in a pre-allocated buffer.
 *
 * @param xid XML-id
 * @param buffer the buffer to copy the string to
 * @param buflen length of the destination buffer
 * @param n position of the attribute in the attribute list. starts at 0.
 * @return the length of the attribute name
 */

XML_API int XML_APIENTRY xmlAttributeCopyName(const xmlId *xid, char *buffer, int buflen, int n);

/**
 * Compare the name of the nth attribute to a string.
 * Comparing is done in a case insensitive way.
 *
 * @param xid XML-id
 * @param n position of the attribute in the attribute list. starts at 0.
 * @param str the string to compare to
 * @return an integer less than, equal to, or greater than zero if the value
 * of the node is found, respectively, to be less than, to match, or be greater
 * than str
 */
XML_API int XML_APIENTRY xmlAttributeCompareName(const xmlId *xid, int n, const char *str);


/**
 * Create a marker XML-id that starts out with the same settings as the
 * reference XML-id.
 * The returned XML-id has to be freed by the calling process using xmlFree.
 *
 * Marker id's are required when xmlNodeGetNum() and xmlNodeGetPos() are used
 * to walk a number of nodes. The xmlNodeGetPos function adjusts the contents
 * of the provided XML-id to keep track of it's position within the XML section.
 * The returned XML-id is limited to the boundaries of the requested XML tag.
 *
 * @param xid reference XML-id
 * @return a copy of the reference XML-id
 */
XML_API xmlId* XML_APIENTRY xmlMarkId(const xmlId *xid);


/**
 * Free an XML-id.
 *
 * @param p a pointer to the memory location to be freed.
 */
XML_API void XML_APIENTRY xmlFree(void *p);

/**
 * Get the number of nodes with the same name from a specified XML path.
 *
 * For a description of node paths see xmlNodeTest.
 *
 * @param xid XML-id
 * @param path path to the XML node
 * @return the number count of the nodename
 */
XML_API int XML_APIENTRY xmlNodeGetNum(const xmlId *xid, const char *path);
XML_API int XML_APIENTRY xmlNodeGetNumRaw(const xmlId *xid, const char *path);

/**
 * Get the number of attributes for this node.
 *
 * @param xid XML-id
 * @return the number count of the node
 */
XML_API int XML_APIENTRY xmlAttributeGetNum(const xmlId *xid);

/**
 * Get the nth occurrence of node in the parent node.
 * The return value should never be altered or freed by the caller.
 *
 * @param pid XML-id of the parent node of this node
 * @param xid XML-id
 * @param node name of the node to search for
 * @param num specify which occurrence to return. starts at 0.
 * @return XML-subsection-id for further processing or NULL if unsuccessful
 */
XML_API xmlId* XML_APIENTRY xmlNodeGetPos(const xmlId *pid, xmlId *xid, const char *node, int num);
XML_API xmlId* XML_APIENTRY xmlNodeGetPosRaw(const xmlId *pid, xmlId *xid, const char *node, int num);

/**
 * Copy the nth occurrence of node in the parent node.
 * The return value should be freed by the caller using xmlFree.
 *
 * @param pid XML-id of the parent node of this node
 * @param xid XML-id which will get unusbale after the call, use the returned
 *            id as the new XML-id
 * @param node name of the node to search for
 * @param num specify which occurrence to return. starts at 0.
 * @return XML-subsection-id for further processing or NULL if unsuccessful
 */
XML_API xmlId* XML_APIENTRY xmlNodeCopyPos(const xmlId *pid, xmlId *xid, const char *node, int num);

/**
 * Get a string of characters from the current node.
 * The returned string has to be freed by the calling process using xmlFree.
 *
 * xmlGetStringRaw returns the unformatted string including leading and trailing
 * spaces and includes comments and the ![CDATA[]] sequence in the result.
 *
 * xmlGetString does not include comments and the ![CDATA[]] sequence in the
 * result and omits leading and trailing spaces.
 *
 * @param xid XML-id
 * @return a newly allocated string containing the contents of the node
 */
XML_API char* XML_APIENTRY xmlGetString(const xmlId *xid);
XML_API char* XML_APIENTRY xmlGetStringRaw(const xmlId *xid);

/**
 * Get a string of characters from the current node.
 * This function has the advantage of not allocating its own return buffer,
 * keeping the memory management to an absolute minimum but the disadvantage
 * is that it's unreliable in multithread environments.
 *
 * @param xid XML-id
 * @param buffer the buffer to copy the string to
 * @param buflen length of the destination buffer
 * @return the length of the string
 */
XML_API int XML_APIENTRY xmlCopyString(const xmlId *xid, char *buffer, int buflen);

/**
 * Compare the value of this node to a reference string.
 * Comparing is done in a case insensitive way.
 *
 * @param xid XML-id
 * @param str the string to compare to
 * @return an integer less than, equal to, or greater than zero if the value
 * of the node is found, respectively, to be less than, to match, or be greater
 * than str
 */
XML_API int XML_APIENTRY xmlCompareString(const xmlId *xid, const char *str);

/**
 * Get a string of characters from a specified XML path.
 * The returned string has to be freed by the calling process using xmlFree.
 *
 * For a description of node paths see xmlNodeTest.
 *
 * @param xid XML-id
 * @param path path to the XML node
 * @return a newly allocated string containing the contents of the node
 */
XML_API char* XML_APIENTRY xmlNodeGetString(const xmlId *xid, const char *path);

/**
 * Get a string of characters from a specified XML path.
 *
 * This function has the advantage of not allocating its own return buffer,
 * keeping the memory management to an absolute minimum but the disadvantage
 * is that it's unreliable in multithread environments.
 *
 * For a description of node paths see xmlNodeTest.
 *
 * @param xid XML-id
 * @param path path to the XML node
 * @param buffer the buffer to copy the string to
 * @param buflen length of the destination buffer
 * @return the length of the string
 */
XML_API int XML_APIENTRY xmlNodeCopyString(const xmlId *xid, const char *path, char *buffer, int buflen);

/**
 * Compare the value of a node to the value of a node at a specified XML path.
 * Comparing is done in a case insensitive way.
 *
 * For a description of node paths see xmlNodeTest.
 *
 * @param xid XML-id
 * @param path path to the XML node to compare to
 * @param str the string to compare to
 * @return an integer less than, equal to, or greater than zero if the value
 * of the node is found, respectively, to be less than, to match, or be greater
 * than str
 */
XML_API int XML_APIENTRY xmlNodeCompareString(const xmlId *xid, const char *path, const char *str);

/**
 * Get a string of characters from a named attribute.
 * The returned string has to be freed by the calling process using xmlFree.
 *
 * @param xid XML-id
 * @param name name of the attribute to acquire
 * @return the contents of the node converted to an integer value
 */
XML_API char* XML_APIENTRY xmlAttributeGetString(const xmlId *xid, const char *name);

/**
 * Get a string of characters from a named attribute.
 * This function has the advantage of not allocating its own return buffer,
 * keeping the memory management to an absolute minimum but the disadvantage
 * is that it's unreliable in multithread environments.
 *
 * @param xid XML-id
 * @param name name of the attribute to acquire.
 * @param buffer the buffer to copy the string to
 * @param buflen length of the destination buffer
 * @return the length of the string
 */
XML_API int XML_APIENTRY xmlAttributeCopyString(const xmlId *xid, const char *name, char *buffer, int buflen);

/**
 * Compare the value of an attribute to a reference string.
 * Comparing is done in a case insensitive way.
 *
 * @param xid XML-id
 * @param name name of the attribute to acquire.
 * @param str the string to compare to
 * @return an integer less than, equal to, or greater than zero if the value
 * of the node is found, respectively, to be less than, to match, or be greater
 * than str
 */
XML_API int XML_APIENTRY xmlAttributeCompareString(const xmlId *xid, const char *name, const char *str);


/**
 * Get the boolean value from the current node/
 *
 * @param xid XML-id
 * @return the contents of the node converted to an boolean value
 */
XML_API int XML_APIENTRY xmlGetBool(const xmlId *xid);

/**
 * Get an boolean value from a specified XML path.
 *
 * For a description of node paths see xmlNodeTest.
 *
 * Boolean values for true are (without quotes) 'on', 'yes', 'true' or
 * any non-zero number. Boolean values for false is anything else.
 *
 * @param xid XML-id
 * @param path path to the XML node
 * @return the contents of the node converted to an boolean value
 */
XML_API int XML_APIENTRY xmlNodeGetBool(const xmlId *xid, const char *path);

/**
 * Get the boolean value from the named attribute.
 *
 * Boolean values for true are (without quotes) 'on', 'yes', 'true' or
 * any non-zero number. Boolean values for false is anything else.
 *
 * @param xid XML-id
 * @param name name of the attribute to acquire
 * @return the contents of the node converted to an boolean value
 */
XML_API int XML_APIENTRY xmlAttributeGetBool(const xmlId *xid, const char *name);

/**
 * Get the integer value from the current node/
 *
 * @param xid XML-id
 * @return the contents of the node converted to an integer value
 */
XML_API long int XML_APIENTRY xmlGetInt(const xmlId *xid);

/**
 * Get an integer value from a specified XML path.
 *
 * For a description of node paths see xmlNodeTest.
 *
 * @param xid XML-id
 * @param path path to the XML node
 * @return the contents of the node converted to an integer value
 */
XML_API long int XML_APIENTRY xmlNodeGetInt(const xmlId *xid, const char *path);

/**
 * Get the integer value from the named attribute.
 *
 * @param xid XML-id
 * @param name name of the attribute to acquire
 * @return the contents of the node converted to an integer value
 */
XML_API long int XML_APIENTRY xmlAttributeGetInt(const xmlId *xid, const char *name);


/**
 * Get the double value from the curent node/
 *
 * @param xid XML-id
 * @return the contents of the node converted to a double value
 */
XML_API double XML_APIENTRY xmlGetDouble(const xmlId *xid);

/**
 * Get a double value from a specified XML path/
 *
 * For a description of node paths see xmlNodeTest.
 *
 * @param xid XML-id
 * @param path path to the XML node
 * @return the contents of the node converted to a double value
 */
XML_API double XML_APIENTRY xmlNodeGetDouble(const xmlId *xid, const char *path);

/**
 * Get the double value from the named attribute.
 *
 * @param xid XML-id
 * @param name name of the attribute to acquire
 * @return the contents of the node converted to an integer value
 */
XML_API double XML_APIENTRY xmlAttributeGetDouble(const xmlId *xid, const char *name);


/**
 * Test whether the named attribute does exist.
 *
 * @param xid XML-id
 * @param name name of the attribute to acquire
 * @return the contents of the node converted to an integer value
 */
XML_API int XML_APIENTRY xmlAttributeExists(const xmlId *xid, const char *name);

/**
 * Get the error number of the last error and clear it.
 *
 * @param xid XML-id
 * @param clear clear the error state if non zero
 * @return the numer of the last error, 0 means no error detected.
 */
XML_API int XML_APIENTRY xmlErrorGetNo(const xmlId *xid, int clear);

/**
 * Get the line number of the last detected syntax error in the XML file.
 *
 * @param xid XML-id
 * @param clear clear the error state if non zero
 * @return the line number of the detected syntax error.
 */
XML_API int XML_APIENTRY xmlErrorGetLineNo(const xmlId *xid, int clear);

/**
 * Get the column number of the last detected syntax error in the XML file.
 *
 * @param xid XML-id
 * @param clear clear the error state if non zero
 * @return the line number of the detected syntax error.
 */
XML_API int XML_APIENTRY xmlErrorGetColumnNo(const xmlId *xid, int clear);

/**
 * Get a string that explains the last error.
 *
 * @param xid XML-id
 * @param clear clear the error state if non zero
 * @return a string that explains the last error.
 */
XML_API const char* XML_APIENTRY xmlErrorGetString(const xmlId *xid, int clear);

/**
 * Get the encoding as specified by the XML document.
 *
 * @param xid XML-id
 * @return a string containing the encoding as specified by the XML document
 */
XML_API const char* XML_APIENTRY xmlGetEncoding(const xmlId *xid);

#if defined(TARGET_OS_MAC) && TARGET_OS_MAC
# pragma export off
#endif

#ifdef __cplusplus
}
#endif

#endif /* __XML_CONFIG */

