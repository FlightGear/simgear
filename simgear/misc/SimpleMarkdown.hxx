// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief  Really simple markdown parser
 */

#ifndef SIMPLE_MARKDOWN_HXX_
#define SIMPLE_MARKDOWN_HXX_

#include <string>

namespace simgear
{
  /**
   * Really simple markdown parser. Currently just paragraphs, new lines and
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
