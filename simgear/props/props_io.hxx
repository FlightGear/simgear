/**
 * \file props_io.hxx
 * Interface definition for property list io.
 * Started Fall 2000 by David Megginson, david@megginson.com
 * This code is released into the Public Domain.
 *
 * See props.html for documentation [replace with URL when available].
 *
 * $Id$
 */

#ifndef __PROPS_IO_HXX
#define __PROPS_IO_HXX

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>

#include <stdio.h>

#include STL_STRING
#include <vector>
#include <map>
#include STL_IOSTREAM

SG_USING_STD(string);
SG_USING_STD(vector);
SG_USING_STD(map);
SG_USING_STD(istream);
SG_USING_STD(ostream);

/**
 * Read properties from an XML input stream.
 */
void readProperties (istream &input, SGPropertyNode * start_node,
		     const string &base = "");


/**
 * Read properties from an XML file.
 */
void readProperties (const string &file, SGPropertyNode * start_node);


/**
 * Read properties from an in-memory buffer.
 */
void readProperties (const char *buf, const int size,
                     SGPropertyNode * start_node);


/**
 * Write properties to an XML output stream.
 */
void writeProperties (ostream &output, const SGPropertyNode * start_node,
		      bool write_all = false);


/**
 * Write properties to an XML file.
 */
void writeProperties (const string &file, const SGPropertyNode * start_node,
		      bool write_all = false);


/**
 * Copy properties from one node to another.
 */
bool copyProperties (const SGPropertyNode *in, SGPropertyNode *out);


#endif // __PROPS_IO_HXX

// end of props_io.hxx
