// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022 James Turner

/**
 * @file
 * @brief Grid layout for Canvas, closely modelled on the equivalent layouts in Gtk/Qt
 */

#pragma once

#include "Layout.hxx"

namespace simgear::canvas {

/**
   * Align LayoutItems in a grid (which fills a rectangular area)
   * Each column / row has consistent sizing for its items
   */
class GridLayout : public Layout
{
public:
    GridLayout();
    virtual ~GridLayout();

    void setDimensions(const SGVec2i& dim);
    size_t numRows() const;
    size_t numColumns() const;

    void addItem(const LayoutItemRef& item, int column, int row, int colSpan = 1, int rowSpan = 1);

    void addItem(const LayoutItemRef& item) override;

    size_t count() const override;
    LayoutItemRef itemAt(size_t index) override;
    LayoutItemRef takeAt(size_t index) override;
    void clear() override;

    void setSpacing(int spacing) override;
    int spacing() const override;

    void invalidate() override;

    bool hasHeightForWidth() const override;

    void setCanvas(const CanvasWeakPtr& canvas) override;

    void setRowStretch(size_t index, int stretch);
    void setColumnStretch(size_t index, int stretch);

protected:
    int _padding = 5;

    struct ItemData {
        LayoutItemRef layout_item;
        SGVec2i size; //!< layout size
        bool visible : 1,
            has_align : 1, //!< Has alignment factor set (!= AlignFill)
            has_hfw : 1,   //!< height for width
            done : 1;      //!< layout done

        /** Clear values (reset to default/empty state) */
        void reset();

        int hfw(int w) const;
        int mhfw(int w) const;

        bool containsCell(const SGVec2i& cell) const;
    };

    struct RowColumnData {
        int stretch = 0;
        int minSize = 0;
        int hintSize = 0;
        int maxSize = 0;
        int calcStretch = 0;
        int calcSize = 0;
        int calcStart = 0;
        bool hasVisible = false;
        ///<padding preceding this row/col. Zero for first row/col, or if there are no visible items
        ///this ensures spacing items or hidden items don't cause double padding
        int padding = 0;

        void resetSizeData();
    };

    using Axis = std::vector<RowColumnData>;
    mutable Axis _rows, _columns;

    using LayoutItems = std::vector<ItemData>;

    using CellTable = std::vector<int>;

    mutable LayoutItems _layout_items;
    SGVec2i _dimensions;
    mutable CellTable _cells; // NxM lookup into _layoutItems


    void updateCells();
    void updateSizeHints() const;
    SGVec2i sizeHintImpl() const override;
    SGVec2i minimumSizeImpl() const override;
    SGVec2i maximumSizeImpl() const override;


    void doLayout(const SGRecti& geom) override;

    void visibilityChanged(bool visible) override;

    LayoutItems::iterator firstInRow(int row);

    LayoutItems::iterator itemInCell(const SGVec2i& cell);

    SGVec2i innerFindUnusedLocation(const SGVec2i& curLoc);
};

typedef SGSharedPtr<GridLayout> GridLayoutRef;

} // namespace simgear::canvas
