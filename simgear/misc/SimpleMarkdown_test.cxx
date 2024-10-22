// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Unit tests for SimpleMarkdown
 */

#define BOOST_TEST_MODULE misc
#include <BoostTestTargetConfig.h>

#include "SimpleMarkdown.hxx"

BOOST_AUTO_TEST_CASE( basic_markdown )
{
  std::string const src(
    "  \n"
    "\n"
    "blub\n"
    "* \tlist item\n"
    "*  another\t item\n"
    "\n"
    "test blubt\n"
    " aha  \n"
    "asd\n"
    "  * 2nd list, another   item\n"
    "  * 2nd list, one more..."
  );
  std::string const out(
    "blub\n"
    "\xE2\x80\xA2 list item\n"
    "\xE2\x80\xA2 another item\n"
    "\n"
    "test blubt aha\n"
    "asd\n"
    "\xE2\x80\xA2 2nd list, another item\n"
    "\xE2\x80\xA2 2nd list, one more..."
  );

  BOOST_CHECK_EQUAL(simgear::SimpleMarkdown::parse(src), out);
}
