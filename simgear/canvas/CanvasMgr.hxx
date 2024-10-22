// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Canvas with 2D rendering API
 */

#ifndef SG_CANVAS_MGR_H_
#define SG_CANVAS_MGR_H_

#include "canvas_fwd.hxx"
#include <simgear/props/PropertyBasedMgr.hxx>

namespace simgear
{
namespace canvas
{

class CanvasMgr : public PropertyBasedMgr
{
public:
    /**
     * @param node    Root node of branch used to control canvasses
     */
    CanvasMgr(SGPropertyNode_ptr node);

    virtual ~CanvasMgr() = default;

    /**
     * Create a new canvas
     *
     * @param name    Name of the new canvas
     */
    CanvasPtr createCanvas(const std::string& name = "");

    /**
     * Get ::Canvas by index
     *
     * @param index Index of texture node in /canvas/by-index/
     */
    CanvasPtr getCanvas(size_t index) const;

    /**
     * Get ::Canvas by name
     *
     * @param name Value of child node "name" in
     *             /canvas/by-index/texture[i]/name
     */
    CanvasPtr getCanvas(const std::string& name) const;

protected:
    void elementCreated(PropertyBasedElementPtr element) override;
};

} // namespace canvas
} // namespace simgear

#endif /* SG_CANVAS_MGR_H_ */
