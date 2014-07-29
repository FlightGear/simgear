///@file
/// Really simple markdown parser.
//
// Copyright (C) 2014  Thomas Geymayer <tomgey@gmail.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#ifndef SIMPLE_MARKDOWN_HXX_
#define SIMPLE_MARKDOWN_HXX_

#include <string>

namespace simgear
{
  /**
   * Really simple mardown parser. Currently just paragraphs, new lines and
   * one level of unordered lists are supported.
   *
   * @see http://en.wikipedia.org/wiki/Markdown
   */
  class SimpleMarkdown
  {
    public:
      static std::string parse(const std::string& src);
  };
} // namespace simgear

#endif /* SIMPLE_MARKDOWN_HXX_ */
