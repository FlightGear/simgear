// Stream based logging mechanism.
//
// Written by Bernie Bright, 1998
//
// Copyright (C) 1998  Bernie Bright - bbright@c031.aone.net.au
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#include "logstream.hxx"

logstream *global_logstream = NULL;

bool            logbuf::logging_enabled = true;
sgDebugClass    logbuf::logClass        = SG_NONE;
sgDebugPriority logbuf::logPriority     = SG_INFO;
streambuf*      logbuf::sbuf            = NULL;

logbuf::logbuf()
{
//     if ( sbuf == NULL )
// 	sbuf = cerr.rdbuf();
}

logbuf::~logbuf()
{
    if ( sbuf )
	    sync();
}

void
logbuf::set_sb( streambuf* sb )
{
    if ( sbuf )
	    sync();

    sbuf = sb;
}

void
logbuf::set_log_level( sgDebugClass c, sgDebugPriority p )
{
    logClass = c;
    logPriority = p;
}

void
logbuf::set_log_classes (sgDebugClass c)
{
    logClass = c;
}

sgDebugClass
logbuf::get_log_classes ()
{
    return logClass;
}

void
logbuf::set_log_priority (sgDebugPriority p)
{
    logPriority = p;
}

sgDebugPriority
logbuf::get_log_priority ()
{
    return logPriority;
}

void
logstream::setLogLevels( sgDebugClass c, sgDebugPriority p )
{
    logbuf::set_log_level( c, p );
}

