/*
 * Copyright (C) 2006-2007 Tim Moore timoore@redhat.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#ifndef SGREADERWRITERBTG_HXX
#define SGREADERWRITERBTG_HXX 1
#include <osgDB/Registry>

class SGReaderWriterBTG : public osgDB::ReaderWriter {
public:
    SGReaderWriterBTG();
    virtual ~SGReaderWriterBTG();

    virtual const char* className() const;
 
    virtual bool acceptsExtension(const std::string& /*extension*/) const;
    virtual ReadResult readNode(const std::string& fileName,
                                const osgDB::Options* options)
        const;
};

#endif
    
