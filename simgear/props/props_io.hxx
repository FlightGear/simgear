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

#include <string>
#include <iosfwd>
#include <functional>

/**
 * Read properties from an XML input stream.
 */
void readProperties (std::istream &input, SGPropertyNode * start_node,
		     const std::string &base = "", int default_mode = 0,
                     bool extended = false);


/**
 * Read properties from an XML file.
 */
void readProperties (const SGPath &file, SGPropertyNode * start_node,
                     int default_mode = 0, bool extended = false);


/**
 * Read properties from an in-memory buffer.
 */
void readProperties (const char *buf, const int size,
                     SGPropertyNode * start_node, int default_mode = 0,
                     bool extended = false);


/**
 * Write properties to an XML output stream.
 */
void writeProperties (std::ostream &output, const SGPropertyNode * start_node,
		      bool write_all = false,
		      SGPropertyNode::Attribute archive_flag = SGPropertyNode::ARCHIVE);

/**
 * Convenience manipulator for calling writeProperties(), e.g.:
 * std::cerr << "node is:\n" << writePropertiesStream(node, true) << "\n";
 */
struct writePropertiesInline
{
    writePropertiesInline(
            const SGPropertyNode * start_node,
		    bool write_all = false,
		    SGPropertyNode::Attribute archive_flag = SGPropertyNode::ARCHIVE
            );
    const SGPropertyNode * m_start_node;
    bool m_write_all;
    SGPropertyNode::Attribute m_archive_flag;
};

std::ostream& operator<< (std::ostream& out, const writePropertiesInline& wpm);

/**
 * Write properties to an XML file.
 */
void writeProperties (const SGPath &file,
                      const SGPropertyNode * start_node,
		      bool write_all = false,
		      SGPropertyNode::Attribute archive_flag = SGPropertyNode::ARCHIVE);


/**
 * Copy properties from one node to another.
 */
bool copyProperties (const SGPropertyNode *in, SGPropertyNode *out);


bool copyPropertiesWithAttribute(const SGPropertyNode *in, SGPropertyNode *out,
                                 SGPropertyNode::Attribute attr);

using PropertyPredicate = std::function<bool (const SGPropertyNode* in)>;

/**
 * Copy properties, if the predicate returns true for the in node.
 * If a parent node returns false, descendats will <em>not</em> be
 * checked
 */
bool copyPropertiesIf(const SGPropertyNode *in, SGPropertyNode *out,
                        PropertyPredicate predicate);

#endif // __PROPS_IO_HXX

// end of props_io.hxx
