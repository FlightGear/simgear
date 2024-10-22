// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022 James Turner

/**
 * @file
 * @brief Grid layout for Canvas, closely modelled on the equivalent layouts in Gtk/Qt
 */

#include <simgear_config.h>

#include "GridLayout.hxx"
#include "SpacerItem.hxx"
#include <simgear/canvas/Canvas.hxx>

namespace simgear {
namespace canvas {

// see https://code.qt.io/cgit/qt/qtbase.git/tree/src/widgets/kernel/qgridlayout.cpp?h=dev
// for similar code :)

static bool isValidLocation(const SGVec2i& loc)
{
    return loc.x() >= 0 && loc.y() >= 0;
}

//----------------------------------------------------------------------------
void GridLayout::ItemData::reset()
{
    layout_item = 0;
    size = {};
    visible = false;
    has_align = false;
    has_hfw = false;
    done = false;
}

void GridLayout::RowColumnData::resetSizeData()
{
    minSize = 0;
    hintSize = 0;
    maxSize = 0;
    calcStretch = stretch;
    calcSize = 0;
    calcStart = 0;
    padding = 0;
    hasVisible = false;
}

bool GridLayout::ItemData::containsCell(const SGVec2i& cell) const
{
    return (layout_item->gridLocation().x() <= cell.x()) &&
           (layout_item->gridLocation().y() <= cell.y()) &&
           (layout_item->gridEnd().x() >= cell.x()) &&
           (layout_item->gridEnd().y() >= cell.y());
}

//----------------------------------------------------------------------------
GridLayout::GridLayout()
{
    // FIXME : share default padding value with BoxLayout
}

//----------------------------------------------------------------------------
GridLayout::~GridLayout()
{
    _parent.reset(); // No need to invalidate parent again...
    clear();
}

//----------------------------------------------------------------------------

void GridLayout::addItem(const LayoutItemRef& item, int column, int row, int colSpan, int rowSpan)
{
    item->setGridLocation({column, row});
    item->setGridSpan({colSpan, rowSpan});
    addItem(item);
}

//----------------------------------------------------------------------------
void GridLayout::addItem(const LayoutItemRef& item)
{
    if (isValidLocation(item->gridLocation())) {
        // re-dimension as required
        const auto itemEnd = item->gridEnd();
        if (itemEnd.x() >= static_cast<int>(numColumns())) {
            // expand columns
            _dimensions.x() = itemEnd.x() + 1;
            _columns.resize(_dimensions.x());
        }

        if (itemEnd.y() >= static_cast<int>(numRows())) {
            // expand rows
            _dimensions.y() = itemEnd.y() + 1;
            _rows.resize(_dimensions.y());
        }
    } else {
        // find first empty grid slot and use
        auto loc = innerFindUnusedLocation(item->gridLocation());
    }

    ItemData d;
    d.reset();
    d.layout_item = item;

    if (SGWeakReferenced::count(this)) {
        item->setParent(this);
    } else {
        SG_LOG(SG_GUI,
               SG_DEV_WARN,
               "Adding item to expired or non-refcounted grid layout");
    }

    _layout_items.push_back(d);
    invalidate();
}

void GridLayout::invalidate()
{
    Layout::invalidate();
    _cells.clear();
}

SGVec2i GridLayout::innerFindUnusedLocation(const SGVec2i& curLoc)
{
    updateCells(); // build the cell-map on demand

    // TODO: this code does work for spanning items yet, only for single cell items.

    // special case: row was specified, but not column. This means only
    // search that row, and maybe extend our dimension if there's no free slots
    if (curLoc.y() >= 0) {
        // TODO: implement me :)
    }

    const auto stride = _dimensions.x();
    for (int j = 0; j < _dimensions.y(); ++j) {
        for (int i = 0; i < stride; ++i) {
            const auto ii = _cells.at(j * stride + i);
            if (ii < 0) {
                return {i, j};
            }
        }
    }

    // grid is full, add a new row on the bottom and return first column
    // of it as our unused location.
    _dimensions.y() += 1;
    _rows.resize(_dimensions.y());
    return {0, _dimensions.y() - 1};
}

void GridLayout::updateCells()
{
    const auto dim = _dimensions.x() * _dimensions.y();
    if (static_cast<int>(_cells.size()) == dim) {
        return;
    }

    _cells.resize(dim);

    const auto stride = _dimensions.x();
    std::fill(_cells.begin(), _cells.end(), -1);
    int itemIndex = 0;
    for (auto& item : _layout_items) {
        const auto topLeftCell = item.layout_item->gridLocation();
        const auto bottomRightCell = item.layout_item->gridEnd();
        for (int row = topLeftCell.y(); row <= bottomRightCell.y(); ++row) {
            for (int cell = topLeftCell.x(); cell <= bottomRightCell.x(); ++cell) {
                _cells[row * stride + cell] = itemIndex;
            }
        }
        ++itemIndex;
    }
}

GridLayout::LayoutItems::iterator GridLayout::itemInCell(const SGVec2i& cell)
{
    updateCells();

    const auto stride = _dimensions.x();
    const auto index = _cells.at(cell.y() * stride + cell.x());
    if (index < 0)
        return _layout_items.end();
    return _layout_items.begin() + index;
}


GridLayout::LayoutItems::iterator GridLayout::firstInRow(int row)
{
    updateCells();

    const auto stride = _dimensions.x();
    for (int col = 0; col < _dimensions.y(); ++col) {
        if (_cells.at(row * stride + col) >= 0) {
            return itemInCell({col, row});
        }
    }

    return _layout_items.end();
}

//----------------------------------------------------------------------------
// void BoxLayout::insertItem( int index,
//                             const LayoutItemRef& item,
//                             int stretch,
//                             uint8_t alignment )
// {
//   ItemData item_data = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//   item_data.layout_item = item;
//   item_data.stretch = std::max(0, stretch);

//   if( alignment != AlignFill )
//     item->setAlignment(alignment);

//   if( SGWeakReferenced::count(this) )
//     item->setParent(this);
//   else
//     SG_LOG( SG_GUI,
//             SG_WARN,
//             "Adding item to expired or non-refcounted layout" );

//   if( index < 0 )
//     _layout_items.push_back(item_data);
//   else
//     _layout_items.insert(_layout_items.begin() + index, item_data);

//   invalidate();
// }

//----------------------------------------------------------------------------
size_t GridLayout::count() const
{
    return _layout_items.size();
}

//----------------------------------------------------------------------------
LayoutItemRef GridLayout::itemAt(size_t index)
{
    if (index >= _layout_items.size())
        return {};

    return _layout_items[index].layout_item;
}

//----------------------------------------------------------------------------
LayoutItemRef GridLayout::takeAt(size_t index)
{
    if (index >= _layout_items.size())
        return {};

    auto it = _layout_items.begin() + index;
    LayoutItemRef item = it->layout_item;
    item->onRemove();
    item->setParent(LayoutItemWeakRef());
    _layout_items.erase(it);

    invalidate();

    return item;
}

//----------------------------------------------------------------------------
void GridLayout::clear()
{
    std::for_each(_layout_items.begin(), _layout_items.end(), [](ItemData item) {
        item.layout_item->onRemove();
        item.layout_item->setParent({});
    });
    _layout_items.clear();
    invalidate();
}

//----------------------------------------------------------------------------
void GridLayout::setSpacing(int spacing)
{
    if (spacing == _padding)
        return;

    _padding = spacing;
    invalidate();
}

//----------------------------------------------------------------------------
int GridLayout::spacing() const
{
    return _padding;
}

//----------------------------------------------------------------------------
void GridLayout::setCanvas(const CanvasWeakPtr& canvas)
{
    _canvas = canvas;

    for (size_t i = 0; i < _layout_items.size(); ++i)
        _layout_items[i].layout_item->setCanvas(canvas);
}

//----------------------------------------------------------------------------
void GridLayout::setDimensions(const SGVec2i& dim)
{
    _dimensions = {
        std::max(dim.x(), _dimensions.x()),
        std::max(dim.y(), _dimensions.y())};
    invalidate();
}

//----------------------------------------------------------------------------
size_t GridLayout::numRows() const
{
    return _dimensions.y();
}

//----------------------------------------------------------------------------
size_t GridLayout::numColumns() const
{
    return _dimensions.x();
}


//----------------------------------------------------------------------------
void GridLayout::setRowStretch(size_t index, int stretch)
{
    if (static_cast<int>(index) >= _dimensions.y()) {
        throw sg_range_exception("GridLayout::setRowStretch: invalid row");
    }

    if (stretch < 0) {
        throw sg_range_exception("GridLayout: negative stretch values are forbidden");
    }

    // becuase we lazily update the rows data, we'd have nowhere to store the
    // new stretch value, so actively resize it now.
    if (index >= _rows.size()) {
        _rows.resize(_dimensions.y());
    }

    _rows[index].stretch = stretch;
    invalidate();
}

//----------------------------------------------------------------------------

void GridLayout::setColumnStretch(size_t index, int stretch)
{
    if (static_cast<int>(index) >= _dimensions.x()) {
        throw sg_range_exception("GridLayout::setColumnStretch: invalid column");
    }

    if (stretch < 0) {
        throw sg_range_exception("GridLayout: negative stretch values are forbidden");
    }

    // becuase we lazily update the columns data, we'd have nowhere to store the
    // new stretch value, so actively resize it now.
    if (index >= _columns.size()) {
        _columns.resize(_dimensions.x());
    }

    _columns[index].stretch = stretch;
    invalidate();
}

//----------------------------------------------------------------------------
void GridLayout::updateSizeHints() const
{
    _columns.resize(_dimensions.x());
    _rows.resize(_dimensions.y());

    // pre-pass: reset row/column data, compute stretch totals
    int totalRowStretch = 0, totalColStretch = 0;
    for (auto& r : _rows) {
        r.resetSizeData();
        totalRowStretch += r.stretch;
    }

    for (auto& c : _columns) {
        c.resetSizeData();
        totalColStretch += c.stretch;
    }

    // if no row/column has any stretch set, use '1' for every row/column
    // this means we don't need to special case this in all the rest of the code
    if (totalColStretch == 0) {
        std::for_each(_columns.begin(), _columns.end(), [](RowColumnData& a) {
            a.calcStretch = 1;
        });
        totalColStretch = _columns.size();
    }

    if (totalRowStretch == 0) {
        std::for_each(_rows.begin(), _rows.end(), [](RowColumnData& a) {
            a.calcStretch = 1;
        });
        totalRowStretch = _rows.size();
    }

    // first pass: do span=1 items, where the child size values
    // can be mapped directly to the row/column
    for (auto& i : _layout_items) {
        if (!i.layout_item->isVisible()) {
            continue;
        }

        const bool isSpacer = dynamic_cast<SpacerItem*>(i.layout_item.get()) != nullptr;
        const auto minSize = i.layout_item->minimumSize();
        const auto hint = i.layout_item->sizeHint();
        const auto maxSize = i.layout_item->maximumSize();

        // TODO: check hfw status of item

        const auto span = i.layout_item->gridSpan();
        const auto loc = i.layout_item->gridLocation();
        if (span.x() == 1) {
            auto& cd = _columns[loc.x()];
            cd.minSize = std::max(cd.minSize, minSize.x());
            cd.hintSize = std::max(cd.hintSize, hint.x());
            cd.hasVisible |= !isSpacer;
            if (maxSize.x() < MAX_SIZE.x()) {
                cd.maxSize = std::max(cd.maxSize, maxSize.x());
            }
        }

        if (span.y() == 1) {
            auto& rd = _rows[loc.y()];
            rd.minSize = std::max(rd.minSize, minSize.y());
            rd.hintSize = std::max(rd.hintSize, hint.y());
            rd.hasVisible |= !isSpacer;
            if (maxSize.y() < MAX_SIZE.y()) {
                rd.maxSize = std::max(rd.maxSize, maxSize.y());
            }
        }
    } // of span=1 items

    // second pass: spanning directions of items: add remaining min/hint size
    // based on stretch factors. Doing this as a second pass means only add on
    // the extra hint amounts, which depending on span=1 items, might not be
    // very much at all
    //
    // when padding is specified for the grid, we need to remove the spanned
    // padding from our hint/min sizes, since this will always be added back
    // on to the geometry when laying out
    for (auto& i : _layout_items) {
        if (!i.layout_item->isVisible()) {
            continue;
        }

        const auto minSize = i.layout_item->minimumSize();
        const auto hint = i.layout_item->sizeHint();
        const auto span = i.layout_item->gridSpan();
        const auto loc = i.layout_item->gridLocation();

        if (span.x() > 1) {
            int spanStretch = 0;
            int spanMinSize = 0;
            int spanHint = 0;

            for (int c = loc.x(); c < loc.x() + span.x(); ++c) {
                spanStretch += _columns.at(c).calcStretch;
                spanMinSize += _columns.at(c).minSize;
                spanHint += _columns.at(c).hintSize;
            }

            // add spanned padding onto these totals as part of the 'space we
            // already get' and hence don't need assign as extra below
            const int spannedPadding = (span.x() - 1) * _padding;
            spanHint += spannedPadding;
            spanMinSize += spannedPadding;

            // no stretch defined, just divide equally. This is not ideal
            // but the user should specify stretch to get the result they
            // want
            if (spanStretch == 0) {
                spanStretch = span.x();
            }

            int extraMinSize = minSize.x() - spanMinSize;
            int extraSizeHint = hint.x() - spanHint;

            for (int c = loc.x(); c < loc.x() + span.x(); ++c) {
                auto& cd = _columns[c];
                if (extraMinSize > 0) {
                    cd.minSize += extraMinSize * cd.calcStretch / spanStretch;
                }

                if (extraSizeHint > 0) {
                    cd.hintSize += extraSizeHint * cd.calcStretch / spanStretch;
                }
            }
        }

        if (span.y() > 1) {
            int spanStretch = 0;
            int spanMinSize = 0;
            int spanHint = 0;

            for (int r = loc.y(); r < loc.y() + span.y(); ++r) {
                spanStretch += _rows.at(r).calcStretch;
                spanMinSize += _rows.at(r).minSize;
                spanHint += _rows.at(r).hintSize;
            }

            // add spanned padding onto these totals as part of the 'space we
            // already get' and hence don't need assign as extra below
            const int spannedPadding = (span.y() - 1) * _padding;
            spanHint += spannedPadding;
            spanMinSize += spannedPadding;

            if (spanStretch == 0) {
                spanStretch = span.y();
            }

            int extraMinSize = minSize.y() - spanMinSize;
            int extraSizeHint = hint.y() - spanHint;

            for (int r = loc.y(); r < loc.y() + span.y(); ++r) {
                auto& rd = _rows[r];
                if (extraMinSize > 0) {
                    rd.minSize += extraMinSize * rd.calcStretch / spanStretch;
                }

                if (extraSizeHint > 0) {
                    rd.hintSize += extraSizeHint * rd.calcStretch / spanStretch;
                }
            }
        }
    } // of second items iteratio

    _min_size = {0, 0};
    _max_size = MAX_SIZE;
    _size_hint = {0, 0};

    bool isFirst = true;
    for (auto& rd : _rows) {
        _min_size.y() += rd.minSize;
        // TODO: handle max-size correctly
        // _max_size.y() +=
        _size_hint.y() += rd.hintSize;
        if (isFirst) {
            isFirst = false;
        } else if (rd.hasVisible) {
            rd.padding = _padding;
        }
    }

    isFirst = true;
    for (auto& cd : _columns) {
        _min_size.x() += cd.minSize;
        // TODO: handle max-size correctly
        // _max_size.x() +=
        _size_hint.x() += cd.hintSize;
        if (isFirst) {
            isFirst = false;
        } else if (cd.hasVisible) {
            cd.padding = _padding;
        }
    }

    _flags &= ~SIZE_INFO_DIRTY;
}

//----------------------------------------------------------------------------
SGVec2i GridLayout::sizeHintImpl() const
{
    updateSizeHints();
    return _size_hint;
}

//----------------------------------------------------------------------------
SGVec2i GridLayout::minimumSizeImpl() const
{
    updateSizeHints();
    return _min_size;
}

//----------------------------------------------------------------------------
SGVec2i GridLayout::maximumSizeImpl() const
{
    updateSizeHints();
    return _max_size;
}


//----------------------------------------------------------------------------
void GridLayout::doLayout(const SGRecti& geom)
{
    if (_flags & SIZE_INFO_DIRTY)
        updateSizeHints();

    int availWidth = geom.width();
    int availHeight = geom.height();
    // compute extra available space
    int rowStretchTotal = 0;
    int columnStretchTotal = 0;

    SGVec2i totalMinSize = {0, 0},
            totalPreferredSize = {0, 0};
    for (auto row = 0; row < static_cast<int>(_rows.size()); ++row) {
        totalMinSize.y() += _rows.at(row).minSize;
        totalPreferredSize.y() += _rows.at(row).hintSize;
        rowStretchTotal += _rows.at(row).calcStretch;
        availHeight -= _rows.at(row).padding;
    }

    for (auto col = 0; col < static_cast<int>(_columns.size()); ++col) {
        totalMinSize.x() += _columns.at(col).minSize;
        totalPreferredSize.x() += _columns.at(col).hintSize;
        columnStretchTotal += _columns.at(col).calcStretch;
        availWidth -= _columns.at(col).padding;
    }

    SGVec2i toDistribute = {0, 0};
    bool havePreferredWidth = false,
         havePreferredHeight = false;
    if (availWidth >= totalPreferredSize.x()) {
        havePreferredWidth = true;
        toDistribute.x() = availWidth - totalPreferredSize.x();
    } else if (availWidth >= totalMinSize.x()) {
        toDistribute.x() = availWidth - totalMinSize.x();
    } else {
        // available width is less than min, we will overflow
    }

    if (availHeight >= totalPreferredSize.y()) {
        havePreferredHeight = true;
        toDistribute.y() = availHeight - totalPreferredSize.y();
    } else if (geom.height() >= totalMinSize.y()) {
        toDistribute.y() = availHeight - totalMinSize.y();
    } else {
        // available height is less than min, we will overflow
    }

    // distribute according to stretch factors
    for (auto col = 0; col < static_cast<int>(_columns.size()); ++col) {
        auto& c = _columns[col];
        c.calcSize = havePreferredWidth ? c.hintSize : c.minSize;
        if (columnStretchTotal > 0)
            c.calcSize += (toDistribute.x() * c.calcStretch) / columnStretchTotal;

        // compute running total of size to give us the actual start coordinate
        if (col > 0) {
            c.calcStart = _columns.at(col - 1).calcStart + _columns.at(col - 1).calcSize + c.padding;
        }
    }

    // TODO: apply height-for-width to all items, to calculate real heights now

    // re-calculate row min/preferred now? Or is it not dependant?

    for (auto row = 0; row < static_cast<int>(_rows.size()); ++row) {
        auto& r = _rows[row];
        r.calcSize = havePreferredHeight ? r.hintSize : r.minSize;
        if (rowStretchTotal > 0)
            r.calcSize += (toDistribute.y() * r.calcStretch) / rowStretchTotal;

        if (row > 0) {
            r.calcStart = _rows.at(row - 1).calcStart + _rows.at(row - 1).calcSize + r.padding;
        }
    }

    // set layout-ed geometry on items
    for (auto& i : _layout_items) {
        const auto loc = i.layout_item->gridLocation();

        // from the end location, we can use the start+size to ensure all padding
        // etc, in between was covered, since we already summed those above.
        const auto end = i.layout_item->gridEnd();
        const auto& endRow = _rows.at(end.y());
        const auto& endCol = _columns.at(end.x());

        // note this is building the rect as a min,max pair, and not as min+(w,h)
        // as we normally do.
        const auto newGeom = SGRecti{
            SGVec2i{_columns.at(loc.x()).calcStart + geom.x(),
                    _rows.at(loc.y()).calcStart + geom.y()},
            SGVec2i{endCol.calcStart + endCol.calcSize + geom.x(),
                    endRow.calcStart + endRow.calcSize + geom.y()},
        };
        // set geometry : alignment is handled internally
        i.layout_item->setGeometry(newGeom);
    }
}

//----------------------------------------------------------------------------
void GridLayout::visibilityChanged(bool visible)
{
    for (size_t i = 0; i < _layout_items.size(); ++i)
        callSetVisibleInternal(_layout_items[i].layout_item.get(), visible);
}

bool GridLayout::hasHeightForWidth() const
{
    // FIXME
    return false;
}


} // namespace canvas
} // namespace simgear
