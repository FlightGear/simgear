// -*- coding: utf-8 -*-
//
// argparse.cxx --- Simple, generic parser for command-line arguments
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

#include <string>
#include <vector>
#include <memory>
#include <utility>              // std::pair, std::move()
#include <cstddef>              // std::size_t
#include <cstring>              // std::strcmp()
#include <cassert>

#include <simgear/misc/strutils.hxx>
#include <simgear/structure/exception.hxx>
#include "argparse.hxx"

using std::string;
using std::shared_ptr;


namespace simgear
{

namespace argparse
{

// ***************************************************************************
// *                    Base class for custom exceptions                     *
// ***************************************************************************
Error::Error(const string& message, const std::string& origin)
  : sg_exception("Argument parser error: " + message, origin)
{ }

Error::Error(const char* message, const char* origin)
  : Error(string(message), string(origin))
{ }

// ***************************************************************************
// *                            OptionDesc class                             *
// ***************************************************************************
OptionDesc::OptionDesc(
  const string& optionId, std::vector<char> shortAliases,
  std::vector<string> longAliases, OptionArgType argumentType)
  : _id(optionId),
    _shortAliases(shortAliases),
    _longAliases(longAliases),
    _argumentType(argumentType)
{ }

const std::string& OptionDesc::id() const
{ return _id; }

const std::vector<char>& OptionDesc::shortAliases() const
{ return _shortAliases; }

const std::vector<std::string>& OptionDesc::longAliases() const
{ return _longAliases; }

OptionArgType OptionDesc::argumentType() const
{ return _argumentType; }


// ***************************************************************************
// *                            OptionValue class                            *
// ***************************************************************************
OptionValue::OptionValue(shared_ptr<const OptionDesc> optionDesc,
                         const string& passedAs, const string& value,
                         bool hasValue)
  : _optionDesc(std::move(optionDesc)),
    _passedAs(passedAs),
    _value(value),
    _hasValue(hasValue)
{ }

shared_ptr<const OptionDesc> OptionValue::optionDesc() const
{ return _optionDesc; }         // return a copy of the shared_ptr

void OptionValue::setOptionDesc(shared_ptr<const OptionDesc> descPtr)
{ _optionDesc = std::move(descPtr); }

string OptionValue::passedAs() const
{ return _passedAs; }

void OptionValue::setPassedAs(const string& passedAs)
{ _passedAs = passedAs; }

string OptionValue::value() const
{ return _value; }

void OptionValue::setValue(const string& value)
{ _value = value; }

bool OptionValue::hasValue() const
{ return _hasValue; }

void OptionValue::setHasValue(bool hasValue)
{ _hasValue = hasValue; }

const string OptionValue::id() const
{
  const auto desc = optionDesc();
  return (desc) ? desc->id() : string();
}


// ***************************************************************************
// *                          ArgumentParser class                           *
// ***************************************************************************

// Static utility method.
std::vector<char>
ArgumentParser::removeHyphens(const std::vector<string>& shortAliases,
                              std::vector<string>& longAliases)
{
  std::vector<char> shortAliasesCharVec;
  shortAliasesCharVec.reserve(shortAliases.size());

  for (const string& opt: shortAliases) {
    if (opt.size() != 2 || opt[0] != '-' || opt[1] == '-' || opt[1] > 127) {
      throw Error("unexpected form for a short option: '" + opt + "' (expecting "
                  "a string of size 2 whose first character is a hyphen and "
                  "second character an ASCII char that is not a hyphen)");
    }

    shortAliasesCharVec.emplace_back(opt[1]); // emplace the char after hyphen
  }

  for (string& longOpt: longAliases) {
    if (longOpt.size() < 3 ||
        !simgear::strutils::starts_with(longOpt, string("--"))) {
      throw Error("unexpected form for a long option: '" + longOpt + "' "
                  "(expecting a string of size 3 or more that starts with "
                  "two hyphens)");
    }

    longOpt.erase(0, 2);        // remove the two leading hyphens
  }

  return shortAliasesCharVec;
}

void
ArgumentParser::addOption(const string& optionId,
                          OptionArgType argType,
                          std::vector<string> shortAliases,
                          std::vector<string> longAliases)
{
  // Remove the leading dashes and do a sanity check for these arguments
  std::vector<char> shortAliasesCharVec = removeHyphens(shortAliases,
                                                        longAliases);

  const auto desc_p = std::make_shared<const OptionDesc>(
    optionId, std::move(shortAliasesCharVec), std::move(longAliases), argType);

  for (const char c: desc_p->shortAliases()) {
    if (!_shortOptionMap.emplace(c, desc_p).second) {
      throw Error(
        "trying to add option '-" + string(1, c) + "', however it is already "
        "in the short option map");
    }
  }

  for (const string& longOpt: desc_p->longAliases()) {
    if (!_longOptionMap.emplace(longOpt, desc_p).second) {
      throw Error(
        "trying to add option '--" + longOpt + "', however it is already in "
        "the long option map");
    }
  }
}

void
ArgumentParser::addOption(const string& optionId, OptionArgType argumentType,
                          string shortOpt, string longOpt)
{
  std::vector<string> shortOptList;
  std::vector<string> longOptList;

  if (!shortOpt.empty()) {
    shortOptList.push_back(std::move(shortOpt));
  }

  if (!longOpt.empty()) {
    longOptList.push_back(std::move(longOpt));
  }

  addOption(optionId, argumentType, std::move(shortOptList),
            std::move(longOptList));
}

std::pair< std::vector<OptionValue>, std::vector<string> >
ArgumentParser::parseArgs(int argc, const char *const *argv) const
{
  std::pair< std::vector<OptionValue>, std::vector<string> > res;
  std::vector<OptionValue>& optsWithValues = res.first;
  std::vector<string>& nonOptionArgs = res.second;
  bool inOptions = true;

  for (int i = 1; i < argc; i++) {
    // Decode from command line encoding
    const string currentArg = cmdEncToUtf8(argv[i]);

    if ((inOptions) && (currentArg == "--")) {
      // We found the end-of-options delimiter
      inOptions = false;
      continue;
    }

    if (inOptions) {
      if (currentArg.size() >= 2 && currentArg[0] == '-') {
        if (currentArg[1] == '-') {
          i += readLongOption(argc, argv, currentArg, i+1, optsWithValues);
        } else {
          i += readShortOptions(argc, argv, currentArg, i+1, optsWithValues);
        }
      } else {
        // The argument is neither an option, nor a cluster of short options.
        inOptions = false;
        nonOptionArgs.push_back(currentArg);
      }
    } else {
      nonOptionArgs.push_back(currentArg);
    }
  }

  return res;
}

// Static method
string ArgumentParser::cmdEncToUtf8(const string& s)
{
#if defined(SG_WINDOWS)
  // Untested code path. Comments and/or testing by Windows people welcome.
  return simgear::strutils::convertWindowsLocal8BitToUtf8(s);
#else
  // XXX This assumes UTF-8 encoding for command line arguments on non-Windows
  // platforms. Unfortunately, the current (April 2017) standard C++ API for
  // encoding conversions has big problems (cf.
  // <http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0618r0.html>).
  // Should be fixed when we have a good way to do such conversions.
  return s;
#endif
}

// Return the number of arguments used by the option value, if any (i.e., how
// much the caller should shift to resume arguments processing).
int ArgumentParser::readLongOption(int argc, const char *const *argv,
                                   const string& currentArg, int nextArgIdx,
                                   std::vector<OptionValue>& optsWithValues)
  const
{
  const string s = currentArg.substr(2); // skip the two initial dashes

  // UTF-8 guarantees that ASCII bytes (here, '=') cannot be part of the
  // encoding of a non-ASCII character.
  std::size_t optEnd = s.find('=');
  string opt = s.substr(0, optEnd);

  const auto mapElt = _longOptionMap.find(opt);
  if (mapElt != _longOptionMap.end()) {
    const shared_ptr<const OptionDesc>& optDesc = mapElt->second;
    OptionValue optVal(optDesc, string("--") + opt);

    switch (optDesc->argumentType()) {
    case OptionArgType::NO_ARGUMENT:
      optVal.setHasValue(false);
      optsWithValues.push_back(std::move(optVal));
      return 0;
    case OptionArgType::OPTIONAL_ARGUMENT: // pass through
    case OptionArgType::MANDATORY_ARGUMENT:
      if (optEnd != string::npos) {
        // The optional value is present in the same command line
        // argument as the option name (syntax '--option=value').
        optVal.setHasValue(true);
        optVal.setValue(s.substr(optEnd + 1));
        optsWithValues.push_back(std::move(optVal));
        return 0;
      } else if (nextArgIdx < argc &&
                 (argv[nextArgIdx][0] != '-' ||
                  !std::strcmp(argv[nextArgIdx], "-"))) {
        // The optional value is present as a separate command line argument
        // (syntax '--option value').
        optVal.setHasValue(true);
        optVal.setValue(cmdEncToUtf8(argv[nextArgIdx]));
        optsWithValues.push_back(std::move(optVal));
        return 1;
      } else if (optDesc->argumentType() ==
                 OptionArgType::OPTIONAL_ARGUMENT) {
        // No argument (value) can be found for the option
        optVal.setHasValue(false);
        optsWithValues.push_back(std::move(optVal));
        return 0;
      } else {
        assert(optDesc->argumentType() == OptionArgType::MANDATORY_ARGUMENT);
        throw InvalidUserInput("option '" + optVal.passedAs() + "' requires an "
                               "argument, but none was provided");
      }
    default:
      throw sg_error("This piece of code should be unreachable.");
    }
  } else {
    throw InvalidUserInput("invalid option: '--" + opt + "'");
  }
}

int ArgumentParser::readShortOptions(int argc, const char *const *argv,
                                     const string& currentArg, int nextArgIdx,
                                     std::vector<OptionValue>& optsWithValues)
  const
{
  shared_ptr<const OptionDesc> optDesc;
  const string s = currentArg.substr(1); // skip the initial dash
  std::size_t i = 0;                     // index inside s

  // Read all options taking no argument in 'currentArg'; stop at the first
  // taking an optional or mandatory argument.
  for (/* empty */; i < s.size(); i++) {
    const auto mapElt = _shortOptionMap.find(s[i]);
    if (mapElt != _shortOptionMap.end()) {
      optDesc = mapElt->second;

      if (optDesc->argumentType() == OptionArgType::NO_ARGUMENT) {
        optsWithValues.emplace_back(optDesc, string("-") + s[i], string(),
                                    false /* no value */);
      } else {
        break;
      }
    } else {
      throw InvalidUserInput(string("invalid option: '-") + s[i] + "'");
    }
  }

  if (i == s.size()) {
    // The command line argument in 'currentArg' was fully read and only
    // contains options that take no argument.
    return 0;
  }

  // We've already “eaten” all options taking no argument in 'currentArg'.
  assert(optDesc->argumentType() == OptionArgType::OPTIONAL_ARGUMENT ||
         optDesc->argumentType() == OptionArgType::MANDATORY_ARGUMENT);

  if (i + 1 < s.size()) {
    // The option has a value at the end of 'currentArg': s.substr(i+1)
    optsWithValues.emplace_back(optDesc, string("-") + s[i], s.substr(i+1),
                                true /* hasValue */);
    return 0;
  } else if (nextArgIdx < argc &&
             (argv[nextArgIdx][0] != '-' ||
              !std::strcmp(argv[nextArgIdx], "-"))) {
    assert(i + 1 == s.size());
    // The option is at the end of 'currentArg' and has a value:
    // argv[nextArgIdx].
    optsWithValues.emplace_back(optDesc, string("-") + s[i],
                                cmdEncToUtf8(argv[nextArgIdx]),
                                true /* hasValue */);
    return 1;
  } else if (optDesc->argumentType() ==
             OptionArgType::OPTIONAL_ARGUMENT) {
    // No argument (value) can be found for the option
    optsWithValues.emplace_back(optDesc, string("-") + s[i], string(),
                                false /* no value */);
    return 0;
  } else {
    assert(optDesc->argumentType() == OptionArgType::MANDATORY_ARGUMENT);
    throw InvalidUserInput(string("option '-") + s[i] + "' requires an "
                           "argument, but none was provided");
  }
}

} // of namespace argparse

} // of namespace simgear
