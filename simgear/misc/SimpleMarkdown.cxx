// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief  Really simple markdown parser
 */

#include "SimpleMarkdown.hxx"

namespace simgear
{

  // White space
  static bool isSpace(const char c)
  {
    return c == ' ' || c == '\t';
  }
  
  // Unordered list item
  static bool isULItem(const char c)
  {
    return c == '*' || c == '+' || c == '-';
  }

  //----------------------------------------------------------------------------
  std::string SimpleMarkdown::parse(const std::string& src)
  {
    std::string out;

    int num_space = 0,
        num_newline = 0;
    bool line_empty = true;

    for( std::string::const_iterator it = src.begin();
                                     it != src.end();
                                   ++it )
    {
      if( isSpace(*it) )
      {
        ++num_space;
      }
      else if( *it == '\n' )
      {
        // Two or more whitespace at end of line -> line break
        if( !line_empty && num_space >= 2 )
        {
          out.push_back('\n');
          line_empty = true;
        }

        ++num_newline;
        num_space = 0;
      }
      else
      {
        // Remove all new lines before first printable character
        if( out.empty() )
          num_newline = 0;

        // Two or more new lines (aka. at least one empty line) -> new paragraph
        if( num_newline >= 2 )
        {
          out.append("\n\n");

          line_empty = true;
          num_newline = 0;
        }

        // Replace unordered list item markup at begin of line with bullet
        // (TODO multilevel lists, indent multiple lines, etc.)
        if( (line_empty || num_newline) && isULItem(*it) )
        {
          if( num_newline < 2 )
            out.push_back('\n');
          out.append("\xE2\x80\xA2");
        }
        else
        {
          // Collapse multiple whitespace
          if( !line_empty && (num_space || num_newline) )
            out.push_back(' ');
          out.push_back(*it);
        }

        line_empty = false;
        num_space = 0;
        num_newline = 0;
      }
    }
    return out;
  }

} // namespace simgear
