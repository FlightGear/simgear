// -*- coding: utf-8 -*-
//
// argparse_test.cxx --- Automated tests for argparse.cxx / argparse.hxx
//
// Copyright (C) 2017  Florent Rougon
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA.

#include <simgear_config.h>

#include <iostream>             // std::cout
#include <vector>
#include <cstdlib>              // EXIT_SUCCESS

#include <simgear/misc/test_macros.hxx>
#include "argparse.hxx"

using std::string;
using std::vector;
using std::cout;
using std::cerr;
using std::endl;

void test_mixOfShortAndLongOptions()
{
  cout << "Testing a mix of short and long options, plus non-option arguments"
       << endl;

  using namespace simgear::argparse;
  ArgumentParser parser;

  parser.addOption("test option", OptionArgType::NO_ARGUMENT, "-t");
  parser.addOption("long opt w/o arg", OptionArgType::NO_ARGUMENT,
                   "", "--long-option-without-arg");
  parser.addOption("other test option", OptionArgType::NO_ARGUMENT, "-O");
  parser.addOption("yet another test option",
                   OptionArgType::OPTIONAL_ARGUMENT, "-y", "--yes-we-can");
  parser.addOption("and again", OptionArgType::MANDATORY_ARGUMENT, "-a",
                   "--all-you-need-is-love");
  parser.addOption("long option with opt arg",
                   OptionArgType::OPTIONAL_ARGUMENT, "", // no short alias
                   "--long-option-with-opt-arg");

  // Using an std::vector to avoid the need to count the elements ourselves
  const vector<const char*> v({
      "FoobarProg", "-Oy", "-aarg for -a", "-OtOty", "arg for -y",
      "-tyOther arg for -y", "--long-option-without-arg", "-a", "Arg for -a",
      "--long-option-with-opt-arg", "--long-option-with-opt-arg", "value 1",
      "--long-option-with-opt-arg=value 2", "-t",
      "--all-you-need-is-love", "oh this is true", "-ypouet",
      "--all-you-need-is-love=right, I'll shut up ;-)", "non option",
      " other non option  ", "--", "<-- came too late, treated as an arg"});
  // v.size() corresponds to argc, &v[0] corresponds to argv.
  const auto res = parser.parseArgs(v.size(), &v[0]);
  const auto& opts = res.first;
  const auto& otherArgs = res.second;

  SG_CHECK_EQUAL(opts.size(), 19);     // number of passed options
  SG_CHECK_EQUAL(otherArgs.size(), 4); // number of non-option arguments

  // Check all passed options and their values
  SG_CHECK_EQUAL(opts[0].passedAs(), "-O");
  SG_CHECK_EQUAL(opts[0].value(), "");
  SG_CHECK_EQUAL(opts[0].hasValue(), false);

  SG_CHECK_EQUAL(opts[0].id(), "other test option");
  SG_CHECK_EQUAL_NOSTREAM(opts[0].optionDesc()->argumentType(),
                          OptionArgType::NO_ARGUMENT);
  SG_CHECK_EQUAL_NOSTREAM(opts[0].optionDesc()->shortAliases(),
                          vector<char>(1, 'O'));
  SG_CHECK_EQUAL_NOSTREAM(opts[0].optionDesc()->longAliases(), vector<string>());

  SG_CHECK_EQUAL(opts[1].passedAs(), "-y");
  SG_CHECK_EQUAL(opts[1].value(), "");
  SG_CHECK_EQUAL(opts[1].hasValue(), false);

  SG_CHECK_EQUAL(opts[1].id(), "yet another test option");
  SG_CHECK_EQUAL_NOSTREAM(opts[1].optionDesc()->argumentType(),
                          OptionArgType::OPTIONAL_ARGUMENT);
  SG_CHECK_EQUAL_NOSTREAM(opts[1].optionDesc()->shortAliases(),
                          vector<char>(1, 'y'));
  SG_CHECK_EQUAL_NOSTREAM(opts[1].optionDesc()->longAliases(),
                          vector<string>(1, "yes-we-can"));

  SG_CHECK_EQUAL(opts[2].passedAs(), "-a");
  SG_CHECK_EQUAL(opts[2].value(), "arg for -a");
  SG_CHECK_EQUAL(opts[2].hasValue(), true);
  SG_CHECK_EQUAL(opts[2].id(), "and again");
  SG_CHECK_EQUAL_NOSTREAM(opts[2].optionDesc()->argumentType(),
                          OptionArgType::MANDATORY_ARGUMENT);
  SG_CHECK_EQUAL_NOSTREAM(opts[2].optionDesc()->shortAliases(),
                          vector<char>(1, 'a'));
  SG_CHECK_EQUAL_NOSTREAM(opts[2].optionDesc()->longAliases(),
                          vector<string>(1, "all-you-need-is-love"));

  SG_CHECK_EQUAL(opts[3].passedAs(), "-O");
  SG_CHECK_EQUAL(opts[3].value(), "");
  SG_CHECK_EQUAL(opts[3].hasValue(), false);
  SG_CHECK_EQUAL(opts[3].id(), "other test option");
  SG_CHECK_EQUAL_NOSTREAM(opts[3].optionDesc()->argumentType(),
                          OptionArgType::NO_ARGUMENT);
  SG_CHECK_EQUAL_NOSTREAM(opts[3].optionDesc()->shortAliases(),
                          vector<char>(1, 'O'));
  SG_CHECK_EQUAL_NOSTREAM(opts[3].optionDesc()->longAliases(), vector<string>());

  SG_CHECK_EQUAL(opts[4].passedAs(), "-t");
  SG_CHECK_EQUAL(opts[4].value(), "");
  SG_CHECK_EQUAL(opts[4].hasValue(), false);
  SG_CHECK_EQUAL(opts[4].id(), "test option");
  SG_CHECK_EQUAL_NOSTREAM(opts[4].optionDesc()->argumentType(),
                          OptionArgType::NO_ARGUMENT);
  SG_CHECK_EQUAL_NOSTREAM(opts[4].optionDesc()->shortAliases(),
                          vector<char>(1, 't'));
  SG_CHECK_EQUAL_NOSTREAM(opts[4].optionDesc()->longAliases(), vector<string>());

  SG_CHECK_EQUAL(opts[5].passedAs(), "-O");
  SG_CHECK_EQUAL(opts[5].value(), "");
  SG_CHECK_EQUAL(opts[5].hasValue(), false);
  SG_CHECK_EQUAL(opts[5].id(), "other test option");

  SG_CHECK_EQUAL(opts[6].passedAs(), "-t");
  SG_CHECK_EQUAL(opts[6].value(), "");
  SG_CHECK_EQUAL(opts[6].hasValue(), false);
  SG_CHECK_EQUAL(opts[6].id(), "test option");

  SG_CHECK_EQUAL(opts[7].passedAs(), "-y");
  SG_CHECK_EQUAL(opts[7].value(), "arg for -y");
  SG_CHECK_EQUAL(opts[7].hasValue(), true);
  SG_CHECK_EQUAL(opts[7].id(), "yet another test option");

  SG_CHECK_EQUAL(opts[8].passedAs(), "-t");
  SG_CHECK_EQUAL(opts[8].value(), "");
  SG_CHECK_EQUAL(opts[8].hasValue(), false);
  SG_CHECK_EQUAL(opts[8].id(), "test option");

  SG_CHECK_EQUAL(opts[9].passedAs(), "-y");
  SG_CHECK_EQUAL(opts[9].value(), "Other arg for -y");
  SG_CHECK_EQUAL(opts[9].hasValue(), true);
  SG_CHECK_EQUAL(opts[9].id(), "yet another test option");

  SG_CHECK_EQUAL(opts[10].passedAs(), "--long-option-without-arg");
  SG_CHECK_EQUAL(opts[10].value(), "");
  SG_CHECK_EQUAL(opts[10].hasValue(), false);
  SG_CHECK_EQUAL(opts[10].id(), "long opt w/o arg");

  SG_CHECK_EQUAL(opts[11].passedAs(), "-a");
  SG_CHECK_EQUAL(opts[11].value(), "Arg for -a");
  SG_CHECK_EQUAL(opts[11].hasValue(), true);
  SG_CHECK_EQUAL(opts[11].id(), "and again");

  SG_CHECK_EQUAL(opts[12].passedAs(), "--long-option-with-opt-arg");
  SG_CHECK_EQUAL(opts[12].value(), "");
  SG_CHECK_EQUAL(opts[12].hasValue(), false);
  SG_CHECK_EQUAL(opts[12].id(), "long option with opt arg");

  SG_CHECK_EQUAL(opts[13].passedAs(), "--long-option-with-opt-arg");
  SG_CHECK_EQUAL(opts[13].value(), "value 1");
  SG_CHECK_EQUAL(opts[13].hasValue(), true);
  SG_CHECK_EQUAL(opts[13].id(), "long option with opt arg");

  SG_CHECK_EQUAL(opts[14].passedAs(), "--long-option-with-opt-arg");
  SG_CHECK_EQUAL(opts[14].value(), "value 2");
  SG_CHECK_EQUAL(opts[14].hasValue(), true);
  SG_CHECK_EQUAL(opts[14].id(), "long option with opt arg");

  SG_CHECK_EQUAL(opts[15].passedAs(), "-t");
  SG_CHECK_EQUAL(opts[15].value(), "");
  SG_CHECK_EQUAL(opts[15].hasValue(), false);
  SG_CHECK_EQUAL(opts[15].id(), "test option");

  SG_CHECK_EQUAL(opts[16].passedAs(), "--all-you-need-is-love");
  SG_CHECK_EQUAL(opts[16].value(), "oh this is true");
  SG_CHECK_EQUAL(opts[16].hasValue(), true);
  SG_CHECK_EQUAL(opts[16].id(), "and again");

  SG_CHECK_EQUAL(opts[17].passedAs(), "-y");
  SG_CHECK_EQUAL(opts[17].value(), "pouet");
  SG_CHECK_EQUAL(opts[17].hasValue(), true);
  SG_CHECK_EQUAL(opts[17].id(), "yet another test option");

  SG_CHECK_EQUAL(opts[18].passedAs(), "--all-you-need-is-love");
  SG_CHECK_EQUAL(opts[18].value(), "right, I'll shut up ;-)");
  SG_CHECK_EQUAL(opts[18].hasValue(), true);
  SG_CHECK_EQUAL(opts[18].id(), "and again");

  // Check all non-option arguments that were passed to parser.parseArgs()
  SG_CHECK_EQUAL_NOSTREAM(
    otherArgs,
    vector<string>({"non option", " other non option  ", "--",
                    "<-- came too late, treated as an arg"}));
}

void test_whenOptionValueIsASingleHyphen()
{
  cout << "Testing cases where a single hyphen is used as an option value" <<
    endl;

  using namespace simgear::argparse;
  ArgumentParser parser;

  parser.addOption("option -T", OptionArgType::NO_ARGUMENT, "-T", "--test");
  parser.addOption("option -o", OptionArgType::OPTIONAL_ARGUMENT,
                   "-o", "--with-opt-arg");
  parser.addOption("option -m", OptionArgType::MANDATORY_ARGUMENT, "-m",
                   "--with-mandatory-arg");

  const vector<const char*> v({
      "FoobarProg", "-To", "-", "-o-", "-oT", "-o", "-T", "-o", "-",
        "--with-opt-arg=-", "--with-opt-arg", "-", "--with-opt-arg",
        "-m-", "--with-mandatory-arg=-", "--with-mandatory-arg", "-", "-m", "-",
        "non option 1", "non option 2", "non option 3"});
  const auto res = parser.parseArgs(v.size(), &v[0]);
  const auto& opts = res.first;
  const auto& otherArgs = res.second;

  SG_CHECK_EQUAL(opts.size(), 14);     // number of passed options
  SG_CHECK_EQUAL(otherArgs.size(), 3); // number of non-option arguments

  SG_CHECK_EQUAL(opts[0].passedAs(), "-T");
  SG_CHECK_EQUAL(opts[0].value(), "");
  SG_CHECK_EQUAL(opts[0].hasValue(), false);
  SG_CHECK_EQUAL(opts[0].id(), "option -T");

  SG_CHECK_EQUAL(opts[1].passedAs(), "-o");
  SG_CHECK_EQUAL(opts[1].value(), "-");
  SG_CHECK_EQUAL(opts[1].hasValue(), true);
  SG_CHECK_EQUAL(opts[1].id(), "option -o");

  SG_CHECK_EQUAL(opts[2].passedAs(), "-o");
  SG_CHECK_EQUAL(opts[2].value(), "-");
  SG_CHECK_EQUAL(opts[2].hasValue(), true);
  SG_CHECK_EQUAL(opts[2].id(), "option -o");

  SG_CHECK_EQUAL(opts[3].passedAs(), "-o");
  SG_CHECK_EQUAL(opts[3].value(), "T");
  SG_CHECK_EQUAL(opts[3].hasValue(), true);
  SG_CHECK_EQUAL(opts[3].id(), "option -o");

  SG_CHECK_EQUAL(opts[4].passedAs(), "-o");
  SG_CHECK_EQUAL(opts[4].value(), "");
  SG_CHECK_EQUAL(opts[4].hasValue(), false);
  SG_CHECK_EQUAL(opts[4].id(), "option -o");

  SG_CHECK_EQUAL(opts[5].passedAs(), "-T");
  SG_CHECK_EQUAL(opts[5].value(), "");
  SG_CHECK_EQUAL(opts[5].hasValue(), false);
  SG_CHECK_EQUAL(opts[5].id(), "option -T");

  SG_CHECK_EQUAL(opts[6].passedAs(), "-o");
  SG_CHECK_EQUAL(opts[6].value(), "-");
  SG_CHECK_EQUAL(opts[6].hasValue(), true);
  SG_CHECK_EQUAL(opts[6].id(), "option -o");

  SG_CHECK_EQUAL(opts[7].passedAs(), "--with-opt-arg");
  SG_CHECK_EQUAL(opts[7].value(), "-");
  SG_CHECK_EQUAL(opts[7].hasValue(), true);
  SG_CHECK_EQUAL(opts[7].id(), "option -o");

  SG_CHECK_EQUAL(opts[8].passedAs(), "--with-opt-arg");
  SG_CHECK_EQUAL(opts[8].value(), "-");
  SG_CHECK_EQUAL(opts[8].hasValue(), true);
  SG_CHECK_EQUAL(opts[8].id(), "option -o");

  SG_CHECK_EQUAL(opts[9].passedAs(), "--with-opt-arg");
  SG_CHECK_EQUAL(opts[9].value(), "");
  SG_CHECK_EQUAL(opts[9].hasValue(), false);
  SG_CHECK_EQUAL(opts[9].id(), "option -o");

  SG_CHECK_EQUAL(opts[10].passedAs(), "-m");
  SG_CHECK_EQUAL(opts[10].value(), "-");
  SG_CHECK_EQUAL(opts[10].hasValue(), true);
  SG_CHECK_EQUAL(opts[10].id(), "option -m");

  SG_CHECK_EQUAL(opts[11].passedAs(), "--with-mandatory-arg");
  SG_CHECK_EQUAL(opts[11].value(), "-");
  SG_CHECK_EQUAL(opts[11].hasValue(), true);
  SG_CHECK_EQUAL(opts[11].id(), "option -m");

  SG_CHECK_EQUAL(opts[12].passedAs(), "--with-mandatory-arg");
  SG_CHECK_EQUAL(opts[12].value(), "-");
  SG_CHECK_EQUAL(opts[12].hasValue(), true);
  SG_CHECK_EQUAL(opts[12].id(), "option -m");

  SG_CHECK_EQUAL(opts[13].passedAs(), "-m");
  SG_CHECK_EQUAL(opts[13].value(), "-");
  SG_CHECK_EQUAL(opts[13].hasValue(), true);
  SG_CHECK_EQUAL(opts[13].id(), "option -m");

  SG_CHECK_EQUAL_NOSTREAM(
    otherArgs,
    vector<string>({"non option 1", "non option 2", "non option 3"}));
}

void test_frontierBetweenOptionsAndNonOptions()
{
  cout << "Testing around the frontier between options and non-options" << endl;

  using namespace simgear::argparse;
  ArgumentParser parser;

  parser.addOption("option -T", OptionArgType::NO_ARGUMENT, "-T");
  parser.addOption("long opt w/o arg", OptionArgType::NO_ARGUMENT,
                   "", "--long-option-without-arg");
  parser.addOption("option -a", OptionArgType::MANDATORY_ARGUMENT, "-a",
                   "--this-is-option-a");

  // Test 1: both options and non-options; '--' used as a normal non-option
  //         argument (i.e., after other non-option arguments).
  const vector<const char*> v1({
      "FoobarProg", "--long-option-without-arg", "-aval", "non option 1",
      "non option 2", "--", "non option 3"});
  // v1.size() corresponds to argc, &v1[0] corresponds to argv.
  const auto res1 = parser.parseArgs(v1.size(), &v1[0]);
  const auto& opts1 = res1.first;
  const auto& otherArgs1 = res1.second;

  SG_CHECK_EQUAL(opts1.size(), 2);      // number of passed options
  SG_CHECK_EQUAL(otherArgs1.size(), 4); // number of non-option arguments

  SG_CHECK_EQUAL_NOSTREAM(
    otherArgs1,
    vector<string>({"non option 1", "non option 2", "--", "non option 3"}));

  // Test 2: some options but no non-options arguments
  const vector<const char*> v2({
      "FoobarProg", "--long-option-without-arg", "-aval"});
  const auto res2 = parser.parseArgs(v2.size(), &v2[0]);
  const auto& opts2 = res2.first;
  const auto& otherArgs2 = res2.second;

  SG_CHECK_EQUAL(opts2.size(), 2);
  SG_VERIFY(otherArgs2.empty());

  SG_CHECK_EQUAL_NOSTREAM(otherArgs2, vector<string>());

  // Test 3: same as test 2, but with useless end-of-options delimiter
  const vector<const char*> v3({
      "FoobarProg", "--long-option-without-arg", "-aval", "--"});
  const auto res3 = parser.parseArgs(v3.size(), &v3[0]);
  const auto& opts3 = res3.first;
  const auto& otherArgs3 = res3.second;

  SG_CHECK_EQUAL(opts3.size(), 2);
  SG_VERIFY(otherArgs3.empty());

  SG_CHECK_EQUAL_NOSTREAM(otherArgs3, vector<string>());

  // Test 4: only non-option arguments
  const vector<const char*> v4({
      "FoobarProg", "non option 1",
      "non option 2", "--", "non option 3"});
  const auto res4 = parser.parseArgs(v4.size(), &v4[0]);
  const auto& opts4 = res4.first;
  const auto& otherArgs4 = res4.second;

  SG_VERIFY(opts4.empty());
  SG_CHECK_EQUAL(otherArgs4.size(), 4);

  SG_CHECK_EQUAL_NOSTREAM(
    otherArgs4,
    vector<string>({"non option 1", "non option 2", "--", "non option 3"}));

  // Test 5: only non-options arguments, but starting with --
  const vector<const char*> v5({
      "FoobarProg", "--", "non option 1",
      "non option 2", "--", "non option 3"});
  const auto res5 = parser.parseArgs(v5.size(), &v5[0]);
  const auto& opts5 = res5.first;
  const auto& otherArgs5 = res5.second;

  SG_VERIFY(opts5.empty());
  SG_CHECK_EQUAL(otherArgs5.size(), 4);

  SG_CHECK_EQUAL_NOSTREAM(
    otherArgs5,
    vector<string>({"non option 1", "non option 2", "--", "non option 3"}));

  // Test 6: use the '--' delimiter before what would otherwise be considered
  //         an option
  const vector<const char*> v6({
      "FoobarProg", "--long-option-without-arg", "-aval", "--", "-T",
      "non option 1", "non option 2", "--", "non option 3"});
  const auto res6 = parser.parseArgs(v6.size(), &v6[0]);
  const auto& opts6 = res6.first;
  const auto& otherArgs6 = res6.second;

  SG_CHECK_EQUAL(opts6.size(), 2);
  SG_CHECK_EQUAL(otherArgs6.size(), 5);

  SG_CHECK_EQUAL_NOSTREAM(
    otherArgs6,
    vector<string>({"-T", "non option 1", "non option 2", "--",
                    "non option 3"}));

  // Test 7: use the '--' delimiter before an argument that doesn't look like
  //         an option
  const vector<const char*> v7({
      "FoobarProg", "--long-option-without-arg", "-aval", "--",
        "doesn't look like an option", "non option 1", "non option 2", "--",
        "non option 3"});
  const auto res7 = parser.parseArgs(v7.size(), &v7[0]);
  const auto& opts7 = res7.first;
  const auto& otherArgs7 = res7.second;

  SG_CHECK_EQUAL(opts7.size(), 2);
  SG_CHECK_EQUAL(otherArgs7.size(), 5);

  SG_CHECK_EQUAL_NOSTREAM(
    otherArgs7,
    vector<string>({"doesn't look like an option", "non option 1",
                    "non option 2", "--", "non option 3"}));

  // Test 8: the argument marking the end of options is the empty string
  const vector<const char*> v8({
      "FoobarProg", "--long-option-without-arg", "-aval",
      "", "non option 1", "non option 2", "-", "non option 3"});
  const auto res8 = parser.parseArgs(v8.size(), &v8[0]);
  const auto& opts8 = res8.first;
  const auto& otherArgs8 = res8.second;

  SG_CHECK_EQUAL(opts8.size(), 2);
  SG_CHECK_EQUAL(otherArgs8.size(), 5);

  SG_CHECK_EQUAL_NOSTREAM(
    otherArgs8,
    vector<string>({"", "non option 1", "non option 2", "-", "non option 3"}));

  // Test 9: the argument marking the end of options is a single hyphen
  const vector<const char*> v9({
      "FoobarProg", "--long-option-without-arg", "-aval",
        "-", "non option 1", "non option 2", "-", "non option 3"});
  const auto res9 = parser.parseArgs(v9.size(), &v9[0]);
  const auto& opts9 = res9.first;
  const auto& otherArgs9 = res9.second;

  SG_CHECK_EQUAL(opts9.size(), 2);
  SG_CHECK_EQUAL(otherArgs9.size(), 5);

  SG_CHECK_EQUAL_NOSTREAM(
    otherArgs9,
    vector<string>({"-", "non option 1", "non option 2", "-", "non option 3"}));

  // Test 10: no other argument than the program name in argv
  const vector<const char*> v10({"FoobarProg"});
  const auto res10 = parser.parseArgs(v10.size(), &v10[0]);
  const auto& opts10 = res10.first;
  const auto& otherArgs10 = res10.second;

  SG_VERIFY(opts10.empty());
  SG_VERIFY(otherArgs10.empty());
}

void test_optionsWithMultipleAliases()
{
  cout << "Testing options with multiple aliases" << endl;

  using namespace simgear::argparse;
  ArgumentParser parser;

  parser.addOption("option -o", OptionArgType::OPTIONAL_ARGUMENT,
                   vector<string>({"-o", "-O", "-0"}),
                   vector<string>({"--o-alias-1", "--o-alias-2"}));
  parser.addOption("option -a", OptionArgType::MANDATORY_ARGUMENT,
                   vector<string>({"-a", "-r"}),
                   vector<string>({"--a-alias-1", "--a-alias-2",
                                   "--a-alias-3"}));
  parser.addOption("option -N", OptionArgType::NO_ARGUMENT,
                   vector<string>({"-N", "-p"}),
                   vector<string>({"--N-alias-1", "--N-alias-2"}));

  const vector<const char*> v({
      "FoobarProg", "--o-alias-1", "-aarg for -a", "-pO", "arg for -O",
        "--a-alias-2=value 1", "--o-alias-2", "value 2", "-Novalue 3",
        "--N-alias-2", "--a-alias-3=value 4", "-0value 5", "--N-alias-1",
        "non option 1", "non option 2", "non option 3"});
  // v.size() corresponds to argc, &v[0] corresponds to argv.
  const auto res = parser.parseArgs(v.size(), &v[0]);
  const auto& opts = res.first;
  const auto& otherArgs = res.second;

  SG_CHECK_EQUAL(opts.size(), 12);      // number of passed options
  SG_CHECK_EQUAL(otherArgs.size(), 3);  // number of non-option arguments

  SG_CHECK_EQUAL(opts[0].passedAs(), "--o-alias-1");
  SG_CHECK_EQUAL(opts[0].value(), "");
  SG_CHECK_EQUAL(opts[0].hasValue(), false);
  SG_CHECK_EQUAL(opts[0].id(), "option -o");

  SG_CHECK_EQUAL(opts[1].passedAs(), "-a");
  SG_CHECK_EQUAL(opts[1].value(), "arg for -a");
  SG_CHECK_EQUAL(opts[1].hasValue(), true);
  SG_CHECK_EQUAL(opts[1].id(), "option -a");

  SG_CHECK_EQUAL(opts[2].passedAs(), "-p");
  SG_CHECK_EQUAL(opts[2].value(), "");
  SG_CHECK_EQUAL(opts[2].hasValue(), false);
  SG_CHECK_EQUAL(opts[2].id(), "option -N");

  SG_CHECK_EQUAL(opts[3].passedAs(), "-O");
  SG_CHECK_EQUAL(opts[3].value(), "arg for -O");
  SG_CHECK_EQUAL(opts[3].hasValue(), true);
  SG_CHECK_EQUAL(opts[3].id(), "option -o");

  SG_CHECK_EQUAL(opts[4].passedAs(), "--a-alias-2");
  SG_CHECK_EQUAL(opts[4].value(), "value 1");
  SG_CHECK_EQUAL(opts[4].hasValue(), true);
  SG_CHECK_EQUAL(opts[4].id(), "option -a");

  SG_CHECK_EQUAL(opts[5].passedAs(), "--o-alias-2");
  SG_CHECK_EQUAL(opts[5].value(), "value 2");
  SG_CHECK_EQUAL(opts[5].hasValue(), true);
  SG_CHECK_EQUAL(opts[5].id(), "option -o");

  SG_CHECK_EQUAL(opts[6].passedAs(), "-N");
  SG_CHECK_EQUAL(opts[6].value(), "");
  SG_CHECK_EQUAL(opts[6].hasValue(), false);
  SG_CHECK_EQUAL(opts[6].id(), "option -N");

  SG_CHECK_EQUAL(opts[7].passedAs(), "-o");
  SG_CHECK_EQUAL(opts[7].value(), "value 3");
  SG_CHECK_EQUAL(opts[7].hasValue(), true);
  SG_CHECK_EQUAL(opts[7].id(), "option -o");

  SG_CHECK_EQUAL(opts[8].passedAs(), "--N-alias-2");
  SG_CHECK_EQUAL(opts[8].value(), "");
  SG_CHECK_EQUAL(opts[8].hasValue(), false);
  SG_CHECK_EQUAL(opts[8].id(), "option -N");

  SG_CHECK_EQUAL(opts[9].passedAs(), "--a-alias-3");
  SG_CHECK_EQUAL(opts[9].value(), "value 4");
  SG_CHECK_EQUAL(opts[9].hasValue(), true);
  SG_CHECK_EQUAL(opts[9].id(), "option -a");

  SG_CHECK_EQUAL(opts[10].passedAs(), "-0");
  SG_CHECK_EQUAL(opts[10].value(), "value 5");
  SG_CHECK_EQUAL(opts[10].hasValue(), true);
  SG_CHECK_EQUAL(opts[10].id(), "option -o");

  SG_CHECK_EQUAL(opts[11].passedAs(), "--N-alias-1");
  SG_CHECK_EQUAL(opts[11].value(), "");
  SG_CHECK_EQUAL(opts[11].hasValue(), false);
  SG_CHECK_EQUAL(opts[11].id(), "option -N");

  SG_CHECK_EQUAL_NOSTREAM(
    otherArgs,
    vector<string>({"non option 1", "non option 2", "non option 3"}));
}

// Auxiliary function used by test_invalidOptionOrArgumentMissing()
void aux_invalidOptionOrMissingArgument_checkRaiseExcecption(
  const simgear::argparse::ArgumentParser& parser,
  const vector<const char*>& v)
{
  bool gotException = false;

  try {
    parser.parseArgs(v.size(), &v[0]);
  } catch (const simgear::argparse::Error&) {
    gotException = true;
  }

  SG_VERIFY(gotException);
}

void test_invalidOptionOrMissingArgument()
{
  cout << "Testing passing invalid options and other syntax errors" << endl;

  using simgear::argparse::OptionArgType;

  simgear::argparse::ArgumentParser parser;
  parser.addOption("option -o", OptionArgType::OPTIONAL_ARGUMENT, "-o");
  parser.addOption("option -m", OptionArgType::MANDATORY_ARGUMENT,
                   "-m", "--mandatory-arg");
  parser.addOption("option -n", OptionArgType::NO_ARGUMENT, "-n", "--no-arg");

  const vector<vector<const char*> > listOfArgvs({
      {"FoobarProg", "-ovalue", "-n", "-X",
       "non option 1", "non option 2", "non option 3"},
      {"FoobarProg", "-ovalue", "-nXn",
          "non option 1", "non option 2", "non option 3"},
      {"FoobarProg", "-ovalue", "-n", "--non-existent-option",
       "non option 1", "non option 2", "non option 3"},
      {"FoobarProg", "-ovalue", "-n", "--non-existent-option=value",
       "non option 1", "non option 2", "non option 3"},
      {"FoobarProg", "-ovalue", "-n", "-m", "--",
       "non option 1", "non option 2", "non option 3"},
      {"FoobarProg", "-ovalue", "-n", "-X", "-m"},
      {"FoobarProg", "-ovalue", "-n", "--mandatory-arg", "--",
       "non option 1", "non option 2", "non option 3"},
      {"FoobarProg", "-ovalue", "-n", "--mandatory-arg"}
    });

  for (const auto& argv: listOfArgvs) {
    aux_invalidOptionOrMissingArgument_checkRaiseExcecption(parser, argv);
  }
}

int main(int argc, const char *const *argv)
{
  test_mixOfShortAndLongOptions();
  test_whenOptionValueIsASingleHyphen();
  test_frontierBetweenOptionsAndNonOptions();
  test_optionsWithMultipleAliases();
  test_invalidOptionOrMissingArgument();

  return EXIT_SUCCESS;
}
