// -*- coding: utf-8 -*-
//
// argparse.hxx --- Simple, generic parser for command-line arguments
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

#ifndef _SIMGEAR_ARGPARSE_HXX_
#define _SIMGEAR_ARGPARSE_HXX_

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <utility>              // std::pair

#include <simgear/structure/exception.hxx>

// Usage example:
//
//   using simgear::argparse::OptionArgType;
//
//   simgear::argparse::ArgumentParser parser;
//   parser.addOption("root option", OptionArgType::MANDATORY_ARGUMENT,
//                    "", "--root");
//   parser.addOption("test option", OptionArgType::NO_ARGUMENT, "-t", "--test");
//
//   const auto res = parser.parseArgs(argc, argv);
//
//   for (const auto& opt: res.first) {
//     std::cerr << "Got option '" << opt.id() << "' as '" << opt.passedAs() <<
//       "'" << ((opt.hasValue()) ? " with value '" + opt.value() + "'" : "") <<
//       "\n";
//   }
//
//   for (const auto& arg: res.second) {
//     std::cerr << "Got non-option argument '" << arg << "'\n";
//   }

namespace simgear
{

namespace argparse
{

// Custom exception classes
class Error : public sg_exception
{
public:
  explicit Error(const std::string& message,
                 const std::string& origin = std::string());
  explicit Error(const char* message, const char* origin = nullptr);
};

class InvalidUserInput : public Error
{
  using Error::Error;           // inherit all constructors
};


enum class OptionArgType {
  NO_ARGUMENT = 0,
  OPTIONAL_ARGUMENT,
  MANDATORY_ARGUMENT
};

// All strings inside this class are encoded in UTF-8.
class OptionDesc
{
public:
  explicit OptionDesc(const std::string& optionId,
                      std::vector<char> shortAliases,
                      std::vector<std::string> longAliases,
                      OptionArgType argumentType);

  // Simple getters for the private members
  const std::string& id() const;
  const std::vector<char>& shortAliases() const;
  const std::vector<std::string>& longAliases() const;
  OptionArgType argumentType() const;

private:
  // Option identifier, invisible to the end user. Used to easily refer to the
  // option despite the various forms it may take (short and/or long aliases).
  std::string _id;
  // Each element of _shortAliases must be an ASCII character. For instance,
  // 'o' for an option called '-o'.
  std::vector<char> _shortAliases;
  // Each element of _longAliases should be the name of a long option, with
  // the two leading dashes removed. For instance, 'generate-foobar' for an
  // option named '--generate-foobar'.
  std::vector<std::string> _longAliases;
  OptionArgType _argumentType;
};

// All strings inside this class are encoded in UTF-8.
class OptionValue
{
public:
  explicit OptionValue(std::shared_ptr<const OptionDesc> optionDesc,
                       const std::string& passedAs,
                       const std::string& value = std::string(),
                       bool hasValue = false);

  // Simple getters/accessors for the private members
  std::shared_ptr<const OptionDesc> optionDesc() const;
  std::string passedAs() const;
  std::string value() const;
  bool hasValue() const;

  // The corresponding setters
  void setOptionDesc(std::shared_ptr<const OptionDesc>);
  void setPassedAs(const std::string&);
  void setValue(const std::string&);
  void setHasValue(bool);

  // For convenience: get the option ID from the result of optionDesc()
  const std::string id() const;

private:
  // Pointer to the option descriptor.
  std::shared_ptr<const OptionDesc> _optionDesc;
  // Exact option passed (e.g., -f or --foobar).
  std::string _passedAs;
  // Value given for the option, if any (otherwise, the empty string).
  std::string _value;
  // Tells whether the option has been given a value. This is of course mainly
  // useful for options taking an *optional* argument. The value in question
  // can be the empty string, if given on a separate command line argument
  // from the option.
  bool _hasValue;
};


// Main class for command line processing. Every string coming out of it is
// encoded in UTF-8.
class ArgumentParser
{
public:
  // Register an option, with zero or more short aliases (e.g., -a, -u, - F)
  // and zero or more long aliases (e.g., --foobar, --barnum, --bleh). The
  // option may take no argument, or one optional argument, or one mandatory
  // argument. The 'optionId' is used to refer to the option in a clear and
  // simple way, even in the presence of several short or long aliases. It is
  // thus visible to the programmer using this API, but not to users of the
  // command line interface being implemented.
  //
  // Note: this method and all its overloads take options in the form "-o" or
  //       "--foobar" (as std::string instances). While it would be possible
  //       to only require a char for each short option and to take long
  //       option declarations without the two leading dashes, the API chosen
  //       here should lead to more readable and searchable user code.
  //
  // shortAliases: each element should consist of two characters: an ASCII
  //               hyphen (-) followed by an ASCII character.
  // longAliases:  each element should be a string in UTF-8 encoding, starting
  //               with two ASCII/UTF-8 hyphens (U+002D).
  //
  // This API could be extended to automatically generate --help output from
  // strings passed to addOption().
  void addOption(const std::string& optionId,
                 OptionArgType argumentType,
                 std::vector<std::string> shortAliases,
                 std::vector<std::string> longAliases);
  // Convenience overload that should be enough for most cases. To register
  // only a short option or only a long option, simply pass the empty string
  // for the corresponding parameter.
  void addOption(const std::string& optionId,
                 OptionArgType argumentType,
                 std::string shortOpt = std::string(),
                 std::string longOpt = std::string());

  // Parse arguments from an argc/argv pair of variables. 'argc' should be the
  // number of elements in 'argv', the first of which is ignored for the sake
  // of options and arguments extraction (since it normally holds the program
  // name).
  //
  // Note: this “number of elements” doesn't count the usual---and completely
  //       unneeded here---final null pointer.
  //
  // Short options may be grouped in the usual way. For instance, if '-x',
  // '-z' and '-f' are three short options, the first two taking no argument
  // and '-f' taking one mandatory argument, then both '-xzf bar' and
  // '-xzfbar' are equivalent to '-x -z -f bar' as well as to '-x -z -fbar'
  // ('bar' being the value taken by option '-f').
  //
  // Long options are handled in the usual way too:
  //
  //    '--foobar'        for an option taking no argument
  //
  //    '--foobar=value'  for an option taking an optional or mandatory
  // or '--foobar value'  argument (two separate command line arguments in the
  //                      second case)
  //
  // Long option names may contain spaces, though this is extremely uncommon
  // and inconvenient for users. Any option argument (be it for a long or a
  // short option) may contain spaces, as expected.
  //
  // As usual too, the special '--' argument consisting of two ASCII/UTF-8
  // hyphens, can be used to cause all subsequent arguments to be treated as
  // non-option arguments, regardless of whether they start with a hyphen or
  // not. In the absence of this special argument, the first argument that is
  // not the value of an option and is either a single hyphen, or doesn't
  // start with a hyphen, marks the end of options. This and all subsequent
  // arguments are read as non-option arguments.
  //
  // Return a pair containing:
  //  - the list of supplied options (with their respective values, when
  //    applicable);
  //  - the list of non-option arguments that were given after the options.
  //
  // Both of these lists (vectors) may be empty and preserve the order used in
  // 'argv'.
  std::pair< std::vector<OptionValue>, std::vector<std::string> >
  parseArgs(int argc, const char *const *argv) const;

private:
  // Convert from the encoding used for argv (command line arguments) to
  // UTF-8.
  //
  // This method is currently not very satisfactory (cf. comments in the
  // implementation). The Windows code path is untested; the non-Windows code
  // path assumes command line arguments are encoded in UTF-8 (in other words,
  // it's a no-op).
  static std::string cmdEncToUtf8(const std::string& stringInCmdLineEncoding);

  // Remove leading dashes and do sanity checks. 'longAliases' is modified
  // in-place. 'shortAliases' is not, because we build an std::vector<char>
  // from an std::vector<std::string>.
  static std::vector<char> removeHyphens(
    const std::vector<std::string>& shortAliases,
    std::vector<std::string>& longAliases);

  // Read a long option and its value, if any (in total: one or two command
  // line arguments).
  //
  // Return the number of arguments consumed by this process after
  // 'currentArg' (i.e., 0 or 1 depending on whether the last option in
  // 'currentArg' has been given a value).
  //
  // 'currentArg' comes from argv[nextArgIdx-1], after decoding by
  // cmdEncToUtf8(). Thus, argv[nextArgIdx] is the command-line argument
  // coming after 'currentArg'.
  int readLongOption(
    int argc, const char *const *argv, const std::string& currentArg,
    int nextArgIdx, std::vector<OptionValue>& optsWithValues) const;
  // Read all short options in a command line argument, plus the option value
  // of the last one of these, if any (even if the option value is in the next
  // command line argument).
  //
  // See readLongOption() for the return value and meaning of parameters.
  int readShortOptions(
    int argc, const char *const *argv, const std::string& currentArg,
    int nextArgIdx, std::vector<OptionValue>& optsWithValues) const;

  // Keys are short option names without the leading dash
  std::unordered_map< char,
                      std::shared_ptr<const OptionDesc> > _shortOptionMap;
  // Keys are long option names without the two leading dashes
  std::unordered_map< std::string,
                      std::shared_ptr<const OptionDesc> > _longOptionMap;
};

} // of namespace argparse

} // of namespace simgear

#endif  // _SIMGEAR_ARGPARSE_HXX_
