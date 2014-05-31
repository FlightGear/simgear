// Testing canvas layouting system
//
// Copyright (C) 2014  Thomas Geymayer <tomgey@gmail.com>
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#define BOOST_TEST_MODULE canvas_layout
#include <BoostTestTargetConfig.h>

#include "BoxLayout.hxx"
#include "NasalWidget.hxx"

#include <simgear/debug/logstream.hxx>
#include <cstdlib>

//------------------------------------------------------------------------------
struct SetLogLevelFixture
{
  SetLogLevelFixture()
  {
    sglog().set_log_priority(SG_DEBUG);
  }
};
BOOST_GLOBAL_FIXTURE(SetLogLevelFixture);

//------------------------------------------------------------------------------
namespace sc = simgear::canvas;

class TestWidget:
  public sc::LayoutItem
{
  public:
    TestWidget( const SGVec2i& min_size,
                const SGVec2i& size_hint,
                const SGVec2i& max_size )
    {
      _size_hint = size_hint;
      _min_size = min_size;
      _max_size = max_size;
    }

    TestWidget(const TestWidget& rhs)
    {
      _size_hint = rhs._size_hint;
      _min_size = rhs._min_size;
      _max_size = rhs._max_size;
    }

    void setMinSize(const SGVec2i& size) { _min_size = size; }
    void setMaxSize(const SGVec2i& size) { _max_size = size; }
    void setSizeHint(const SGVec2i& size) { _size_hint = size; }

    virtual void setGeometry(const SGRecti& geom) { _geom = geom; }
    virtual SGRecti geometry() const { return _geom; }

  protected:

    SGRecti _geom;

    virtual SGVec2i sizeHintImpl() const { return _size_hint; }
    virtual SGVec2i minimumSizeImpl() const { return _min_size; }
    virtual SGVec2i maximumSizeImpl() const { return _max_size; }
};

typedef SGSharedPtr<TestWidget> TestWidgetRef;

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( horizontal_layout )
{
  sc::BoxLayout box_layout(sc::BoxLayout::BottomToTop);
  box_layout.setSpacing(5);

  BOOST_CHECK_EQUAL(box_layout.direction(), sc::BoxLayout::BottomToTop);
  BOOST_CHECK_EQUAL(box_layout.spacing(), 5);

  box_layout.setDirection(sc::BoxLayout::LeftToRight);
  box_layout.setSpacing(9);

  BOOST_CHECK_EQUAL(box_layout.direction(), sc::BoxLayout::LeftToRight);
  BOOST_CHECK_EQUAL(box_layout.spacing(), 9);

  TestWidgetRef fixed_size_widget( new TestWidget( SGVec2i(16, 16),
                                                   SGVec2i(16, 16),
                                                   SGVec2i(16, 16) ) );
  box_layout.addItem(fixed_size_widget);

  BOOST_CHECK_EQUAL(box_layout.minimumSize(), SGVec2i(16, 16));
  BOOST_CHECK_EQUAL(box_layout.sizeHint(), SGVec2i(16, 16));
  BOOST_CHECK_EQUAL(box_layout.maximumSize(), SGVec2i(16, 16));

  TestWidgetRef limited_resize_widget( new TestWidget( SGVec2i(16, 16),
                                                       SGVec2i(32, 32),
                                                       SGVec2i(256, 64) ) );
  box_layout.addItem(limited_resize_widget);

  // Combined sizes of both widget plus the padding between them
  BOOST_CHECK_EQUAL(box_layout.minimumSize(), SGVec2i(41, 16));
  BOOST_CHECK_EQUAL(box_layout.sizeHint(), SGVec2i(57, 32));
  BOOST_CHECK_EQUAL(box_layout.maximumSize(), SGVec2i(281, 64));

  // Test with different spacing/padding
  box_layout.setSpacing(5);

  BOOST_CHECK_EQUAL(box_layout.minimumSize(), SGVec2i(37, 16));
  BOOST_CHECK_EQUAL(box_layout.sizeHint(), SGVec2i(53, 32));
  BOOST_CHECK_EQUAL(box_layout.maximumSize(), SGVec2i(277, 64));

  box_layout.setGeometry(SGRecti(0, 0, 128, 32));

  // Fixed size for first widget and remaining space goes to second widget
  BOOST_CHECK_EQUAL(fixed_size_widget->geometry(), SGRecti(0, 8, 16, 16));
  BOOST_CHECK_EQUAL(limited_resize_widget->geometry(), SGRecti(21, 0, 107, 32));

  TestWidgetRef stretch_widget( new TestWidget( SGVec2i(16, 16),
                                                SGVec2i(32, 32),
                                                SGVec2i(128, 32) ) );
  box_layout.addItem(stretch_widget, 1);
  box_layout.update();

  BOOST_CHECK_EQUAL(box_layout.minimumSize(), SGVec2i(58, 16));
  BOOST_CHECK_EQUAL(box_layout.sizeHint(), SGVec2i(90, 32));
  BOOST_CHECK_EQUAL(box_layout.maximumSize(), SGVec2i(410, 64));

  // Due to the stretch factor only the last widget gets additional space. All
  // other widgets get the preferred size.
  BOOST_CHECK_EQUAL(fixed_size_widget->geometry(), SGRecti(0, 8, 16, 16));
  BOOST_CHECK_EQUAL(limited_resize_widget->geometry(), SGRecti(21, 0, 32, 32));
  BOOST_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(58, 0, 70, 32));

  // Test stretch factor
  TestWidgetRef fast_stretch( new TestWidget(*stretch_widget) );
  sc::BoxLayout box_layout_stretch(sc::BoxLayout::LeftToRight);

  box_layout_stretch.addItem(stretch_widget, 1);
  box_layout_stretch.addItem(fast_stretch, 2);

  box_layout_stretch.setGeometry(SGRecti(0,0,128,32));

  BOOST_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(0, 0, 41, 32));
  BOOST_CHECK_EQUAL(fast_stretch->geometry(), SGRecti(46, 0, 82, 32));

  box_layout_stretch.setGeometry(SGRecti(0,0,256,32));

  BOOST_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(0, 0, 123, 32));
  BOOST_CHECK_EQUAL(fast_stretch->geometry(), SGRecti(128, 0, 128, 32));

  // Test superflous space to padding
  box_layout_stretch.setGeometry(SGRecti(0,0,512,32));

  BOOST_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(83, 0, 128, 32));
  BOOST_CHECK_EQUAL(fast_stretch->geometry(), SGRecti(300, 0, 128, 32));

  // Test more space then preferred, but less than maximum
  {
    sc::HBoxLayout hbox;
    TestWidgetRef w1( new TestWidget( SGVec2i(16,   16),
                                      SGVec2i(32,   32),
                                      SGVec2i(9999, 32) ) ),
                  w2( new TestWidget(*w1) );

    hbox.addItem(w1);
    hbox.addItem(w2);

    hbox.setGeometry( SGRecti(0, 0, 256, 32) );

    BOOST_CHECK_EQUAL(w1->geometry(), SGRecti(0,   0, 126, 32));
    BOOST_CHECK_EQUAL(w2->geometry(), SGRecti(131, 0, 125, 32));

    hbox.setStretch(0, 1);
    hbox.setStretch(1, 1);

    BOOST_CHECK_EQUAL(hbox.stretch(0), 1);
    BOOST_CHECK_EQUAL(hbox.stretch(1), 1);

    hbox.update();

    BOOST_CHECK_EQUAL(w1->geometry(), SGRecti(0,   0, 125, 32));
    BOOST_CHECK_EQUAL(w2->geometry(), SGRecti(130, 0, 126, 32));
  }
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( vertical_layout)
{
  sc::BoxLayout vbox(sc::BoxLayout::TopToBottom);
  vbox.setSpacing(7);

  TestWidgetRef fixed_size_widget( new TestWidget( SGVec2i(16, 16),
                                                   SGVec2i(16, 16),
                                                   SGVec2i(16, 16) ) );
  TestWidgetRef limited_resize_widget( new TestWidget( SGVec2i(16, 16),
                                                       SGVec2i(32, 32),
                                                       SGVec2i(256, 64) ) );

  vbox.addItem(fixed_size_widget);
  vbox.addItem(limited_resize_widget);

  BOOST_CHECK_EQUAL(vbox.minimumSize(), SGVec2i(16, 39));
  BOOST_CHECK_EQUAL(vbox.sizeHint(), SGVec2i(32, 55));
  BOOST_CHECK_EQUAL(vbox.maximumSize(), SGVec2i(256, 87));

  vbox.setGeometry(SGRecti(10, 20, 16, 55));

  BOOST_CHECK_EQUAL(fixed_size_widget->geometry(), SGRecti(10, 20, 16, 16));
  BOOST_CHECK_EQUAL(limited_resize_widget->geometry(), SGRecti(10, 43, 16, 32));

  vbox.setDirection(sc::BoxLayout::BottomToTop);
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( nasal_layout )
{
  naContext c = naNewContext();
  naRef me = naNewHash(c);

  sc::LayoutItemRef nasal_item( new sc::NasalWidget(me) );

  naFreeContext(c);
}
