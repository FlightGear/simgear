/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <osgDB/ReaderWriter>

namespace simgear {

class ReaderWriterAC3D : public osgDB::ReaderWriter {
public:
    ReaderWriterAC3D();
    virtual ~ReaderWriterAC3D();

    const char* className() const override { return "AC3D Database Reader"; }

    ReadResult readObject(const std::string& location, const Options* options) const override;
    ReadResult readNode(const std::string& location, const Options* options) const override;

    ReadResult readObject(std::istream& fin, const Options* options) const override;
    ReadResult readNode(std::istream& fin, const Options* options) const override;
};

} // namespace simgear
