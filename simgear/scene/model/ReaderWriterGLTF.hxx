/*
 * SPDX-FileCopyrightText: Copyright (C) 2021 - 2024 Fernando García Liñán
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <osgDB/ReaderWriter>

namespace simgear {

class ReaderWriterGLTF : public osgDB::ReaderWriter {
public:
    ReaderWriterGLTF();
    virtual ~ReaderWriterGLTF();

    virtual const char* className() const;

    virtual ReadResult readNode(const std::string& location,
                                const osgDB::Options* options) const;
};

} // namespace simgear
