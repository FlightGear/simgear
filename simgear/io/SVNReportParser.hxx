// SVNReportParser -- parser for SVN report XML data
//
// Copyright (C) 2012  James Turner <zakalawe@mac.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


#ifndef SG_IO_SVNREPORTPARSER_HXX
#define SG_IO_SVNREPORTPARSER_HXX

#include <string>
#include <memory> // for auto_ptr

#include "SVNRepository.hxx"

class SGPath;

namespace simgear
{
  
class SVNRepository;

class SVNReportParser
{
public:
  SVNReportParser(SVNRepository* repo);
  ~SVNReportParser();
  
  // incremental XML parsing
  SVNRepository::ResultCode parseXML(const char* data, int size);
  
  SVNRepository::ResultCode finishParse();
        
  static std::string etagFromRevision(unsigned int revision);
          
  class SVNReportParserPrivate;
private:
    SVNRepository::ResultCode innerParseXML(const char* data, int size);
        
  std::auto_ptr<SVNReportParserPrivate> _d;
};

} // of namespace simgear

#endif // of SG_IO_SVNREPORTPARSER_HXX
