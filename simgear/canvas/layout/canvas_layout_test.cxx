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
#include <simgear/nasal/cppbind/NasalContext.hxx>

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
                const SGVec2i& max_size = MAX_SIZE )
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

  protected:

    virtual SGVec2i sizeHintImpl() const { return _size_hint; }
    virtual SGVec2i minimumSizeImpl() const { return _min_size; }
    virtual SGVec2i maximumSizeImpl() const { return _max_size; }

    virtual void visibilityChanged(bool visible)
    {
      if( !visible )
        _geometry.set(0, 0, 0, 0);
    }
};

class TestWidgetHFW:
  public TestWidget
{
  public:
    TestWidgetHFW( const SGVec2i& min_size,
                   const SGVec2i& size_hint,
                   const SGVec2i& max_size = MAX_SIZE ):
      TestWidget(min_size, size_hint, max_size)
    {

    }

    virtual bool hasHeightForWidth() const
    {
      return true;
    }

    virtual int heightForWidthImpl(int w) const
    {
      return _size_hint.x() * _size_hint.y() / w;
    }

    virtual int minimumHeightForWidthImpl(int w) const
    {
      return _min_size.x() * _min_size.y() / w;
    }
};

typedef SGSharedPtr<TestWidget> TestWidgetRef;

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( horizontal_layout )
{
  sc::BoxLayoutRef box_layout(new sc::BoxLayout(sc::BoxLayout::BottomToTop));
  box_layout->setSpacing(5);

  BOOST_CHECK_EQUAL(box_layout->direction(), sc::BoxLayout::BottomToTop);
  BOOST_CHECK_EQUAL(box_layout->spacing(), 5);

  box_layout->setDirection(sc::BoxLayout::LeftToRight);
  box_layout->setSpacing(9);

  BOOST_CHECK_EQUAL(box_layout->direction(), sc::BoxLayout::LeftToRight);
  BOOST_CHECK_EQUAL(box_layout->spacing(), 9);

  TestWidgetRef fixed_size_widget( new TestWidget( SGVec2i(16, 16),
                                                   SGVec2i(16, 16),
                                                   SGVec2i(16, 16) ) );
  box_layout->addItem(fixed_size_widget);

  BOOST_CHECK_EQUAL(box_layout->minimumSize(), SGVec2i(16, 16));
  BOOST_CHECK_EQUAL(box_layout->sizeHint(), SGVec2i(16, 16));
  BOOST_CHECK_EQUAL(box_layout->maximumSize(), SGVec2i(16, 16));

  TestWidgetRef limited_resize_widget( new TestWidget( SGVec2i(16, 16),
                                                       SGVec2i(32, 32),
                                                       SGVec2i(256, 64) ) );
  box_layout->addItem(limited_resize_widget);

  // Combined sizes of both widget plus the padding between them
  BOOST_CHECK_EQUAL(box_layout->minimumSize(), SGVec2i(41, 16));
  BOOST_CHECK_EQUAL(box_layout->sizeHint(), SGVec2i(57, 32));
  BOOST_CHECK_EQUAL(box_layout->maximumSize(), SGVec2i(281, 64));

  // Test with different spacing/padding
  box_layout->setSpacing(5);

  BOOST_CHECK_EQUAL(box_layout->minimumSize(), SGVec2i(37, 16));
  BOOST_CHECK_EQUAL(box_layout->sizeHint(), SGVec2i(53, 32));
  BOOST_CHECK_EQUAL(box_layout->maximumSize(), SGVec2i(277, 64));

  box_layout->setGeometry(SGRecti(0, 0, 128, 32));

  // Fixed size for first widget and remaining space goes to second widget
  BOOST_CHECK_EQUAL(fixed_size_widget->geometry(), SGRecti(0, 8, 16, 16));
  BOOST_CHECK_EQUAL(limited_resize_widget->geometry(), SGRecti(21, 0, 107, 32));

  TestWidgetRef stretch_widget( new TestWidget( SGVec2i(16, 16),
                                                SGVec2i(32, 32),
                                                SGVec2i(128, 32) ) );
  box_layout->addItem(stretch_widget, 1);
  box_layout->update();

  BOOST_CHECK_EQUAL(box_layout->minimumSize(), SGVec2i(58, 16));
  BOOST_CHECK_EQUAL(box_layout->sizeHint(), SGVec2i(90, 32));
  BOOST_CHECK_EQUAL(box_layout->maximumSize(), SGVec2i(410, 64));

  // Due to the stretch factor only the last widget gets additional space. All
  // other widgets get the preferred size.
  BOOST_CHECK_EQUAL(fixed_size_widget->geometry(), SGRecti(0, 8, 16, 16));
  BOOST_CHECK_EQUAL(limited_resize_widget->geometry(), SGRecti(21, 0, 32, 32));
  BOOST_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(58, 0, 70, 32));

  // Test stretch factor
  TestWidgetRef fast_stretch( new TestWidget(*stretch_widget) );
  sc::BoxLayoutRef box_layout_stretch(
    new sc::BoxLayout(sc::BoxLayout::LeftToRight)
  );

  box_layout_stretch->addItem(stretch_widget, 1);
  box_layout_stretch->addItem(fast_stretch, 2);

  box_layout_stretch->setGeometry(SGRecti(0,0,128,32));

  BOOST_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(0, 0, 41, 32));
  BOOST_CHECK_EQUAL(fast_stretch->geometry(), SGRecti(46, 0, 82, 32));

  box_layout_stretch->setGeometry(SGRecti(0,0,256,32));

  BOOST_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(0, 0, 123, 32));
  BOOST_CHECK_EQUAL(fast_stretch->geometry(), SGRecti(128, 0, 128, 32));

  // Test superflous space to padding
  box_layout_stretch->setGeometry(SGRecti(0,0,512,32));

  BOOST_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(83, 0, 128, 32));
  BOOST_CHECK_EQUAL(fast_stretch->geometry(), SGRecti(300, 0, 128, 32));

  // ...and now with alignment
  //
  // All widgets without alignment get their maximum space and the remaining
  // space is equally distributed to the remaining items. All items with
  // alignment are set to their size hint and positioned according to their
  // alignment.

  // Left widget: size hint and positioned on the left
  // Right widget: maximum size and positioned on the right
  stretch_widget->setAlignment(sc::AlignLeft);
  box_layout_stretch->update();
  BOOST_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(0, 0, 32, 32));
  BOOST_CHECK_EQUAL(fast_stretch->geometry(), SGRecti(384, 0, 128, 32));

  // Left widget: align right
  stretch_widget->setAlignment(sc::AlignRight);
  box_layout_stretch->update();
  BOOST_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(347, 0, 32, 32));
  BOOST_CHECK_EQUAL(fast_stretch->geometry(), SGRecti(384, 0, 128, 32));

  // Left widget: size hint and positioned on the right
  // Right widget: size hint and positioned on the left of the right half
  fast_stretch->setAlignment(sc::AlignLeft);
  box_layout_stretch->update();
  BOOST_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(221, 0, 32, 32));
  BOOST_CHECK_EQUAL(fast_stretch->geometry(), SGRecti(258, 0, 32, 32));

  // Also check vertical alignment
  stretch_widget->setAlignment(sc::AlignLeft | sc::AlignTop);
  fast_stretch->setAlignment(sc::AlignLeft | sc::AlignBottom);
  box_layout_stretch->setGeometry(SGRecti(0,0,512,64));
  BOOST_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(0, 0, 32, 32));
  BOOST_CHECK_EQUAL(fast_stretch->geometry(), SGRecti(258, 32, 32, 32));
}

//------------------------------------------------------------------------------
// Test more space then preferred, but less than maximum
BOOST_AUTO_TEST_CASE( hbox_pref_to_max )
{
  sc::BoxLayoutRef hbox(new sc::HBoxLayout());
  TestWidgetRef w1( new TestWidget( SGVec2i(16,   16),
                                    SGVec2i(32,   32),
                                    SGVec2i(9999, 32) ) ),
                w2( new TestWidget(*w1) );

  hbox->addItem(w1);
  hbox->addItem(w2);

  hbox->setGeometry( SGRecti(0, 0, 256, 32) );

  BOOST_CHECK_EQUAL(w1->geometry(), SGRecti(0,   0, 126, 32));
  BOOST_CHECK_EQUAL(w2->geometry(), SGRecti(131, 0, 125, 32));

  hbox->setStretch(0, 1);
  hbox->setStretch(1, 1);

  BOOST_CHECK_EQUAL(hbox->stretch(0), 1);
  BOOST_CHECK_EQUAL(hbox->stretch(1), 1);

  hbox->update();

  BOOST_CHECK_EQUAL(w1->geometry(), SGRecti(0,   0, 125, 32));
  BOOST_CHECK_EQUAL(w2->geometry(), SGRecti(130, 0, 126, 32));

  BOOST_REQUIRE( hbox->setStretchFactor(w1, 2) );
  BOOST_REQUIRE( hbox->setStretchFactor(w2, 3) );
  BOOST_CHECK_EQUAL(hbox->stretch(0), 2);
  BOOST_CHECK_EQUAL(hbox->stretch(1), 3);

  hbox->removeItem(w1);

  BOOST_CHECK( !hbox->setStretchFactor(w1, 0) );
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( spacer_layouting )
{
  sc::HBoxLayout hbox;
  TestWidgetRef w1( new TestWidget( SGVec2i(16, 16),
                                    SGVec2i(32, 32),
                                    SGVec2i(9999, 9999) ) ),
                w2( new TestWidget(*w1) );

  hbox.addItem(w1);
  hbox.addItem(w2);
  hbox.addStretch(1);

  BOOST_CHECK_EQUAL(hbox.minimumSize(), SGVec2i(37, 16));
  BOOST_CHECK_EQUAL(hbox.sizeHint(), SGVec2i(69, 32));
  BOOST_CHECK_EQUAL(hbox.maximumSize(), sc::LayoutItem::MAX_SIZE);

  hbox.setGeometry(SGRecti(0, 0, 256, 40));

  BOOST_CHECK_EQUAL(w1->geometry(), SGRecti(0,  0, 32, 40));
  BOOST_CHECK_EQUAL(w2->geometry(), SGRecti(37, 0, 32, 40));

  // now center with increased spacing between both widgets
  hbox.insertStretch(0, 1);
  hbox.insertSpacing(2, 10);

  BOOST_CHECK_EQUAL(hbox.minimumSize(), SGVec2i(47, 16));
  BOOST_CHECK_EQUAL(hbox.sizeHint(), SGVec2i(79, 32));
  BOOST_CHECK_EQUAL(hbox.maximumSize(), sc::LayoutItem::MAX_SIZE);

  hbox.update();

  BOOST_CHECK_EQUAL(w1->geometry(), SGRecti(88,  0, 32, 40));
  BOOST_CHECK_EQUAL(w2->geometry(), SGRecti(135, 0, 32, 40));
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
BOOST_AUTO_TEST_CASE( boxlayout_insert_remove )
{
  sc::BoxLayoutRef hbox( new sc::HBoxLayout );

  BOOST_CHECK_EQUAL(hbox->count(), 0);
  BOOST_CHECK(!hbox->itemAt(0));
  BOOST_CHECK(!hbox->takeAt(0));

  TestWidgetRef w1( new TestWidget( SGVec2i(16,   16),
                                    SGVec2i(32,   32),
                                    SGVec2i(9999, 32) ) ),
                w2( new TestWidget(*w1) );

  hbox->addItem(w1);
  BOOST_CHECK_EQUAL(hbox->count(), 1);
  BOOST_CHECK_EQUAL(hbox->itemAt(0), w1);
  BOOST_CHECK_EQUAL(w1->getParent(), hbox);

  hbox->insertItem(0, w2);
  BOOST_CHECK_EQUAL(hbox->count(), 2);
  BOOST_CHECK_EQUAL(hbox->itemAt(0), w2);
  BOOST_CHECK_EQUAL(hbox->itemAt(1), w1);
  BOOST_CHECK_EQUAL(w2->getParent(), hbox);

  hbox->removeItem(w2);
  BOOST_CHECK_EQUAL(hbox->count(), 1);
  BOOST_CHECK_EQUAL(hbox->itemAt(0), w1);
  BOOST_CHECK( !w2->getParent() );

  hbox->addItem(w2);
  BOOST_CHECK_EQUAL(hbox->count(), 2);
  BOOST_CHECK_EQUAL(w2->getParent(), hbox);

  hbox->clear();
  BOOST_CHECK_EQUAL(hbox->count(), 0);
  BOOST_CHECK( !w1->getParent() );
  BOOST_CHECK( !w2->getParent() );
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( boxlayout_visibility )
{
  sc::BoxLayoutRef hbox( new sc::HBoxLayout );
  TestWidgetRef w1( new TestWidget( SGVec2i(16, 16),
                                    SGVec2i(32, 32) ) ),
                w2( new TestWidget(*w1) ),
                w3( new TestWidget(*w1) );

  hbox->addItem(w1);
  hbox->addItem(w2);
  hbox->addItem(w3);

  BOOST_REQUIRE_EQUAL(hbox->sizeHint().x(), 3 * 32 + 2 * hbox->spacing());

  hbox->setGeometry(SGRecti(0, 0, 69, 32));

  BOOST_CHECK_EQUAL(w1->geometry(), SGRecti(0,  0, 20, 32));
  BOOST_CHECK_EQUAL(w2->geometry(), SGRecti(25, 0, 20, 32));
  BOOST_CHECK_EQUAL(w3->geometry(), SGRecti(50, 0, 19, 32));

  w2->setVisible(false);

  BOOST_REQUIRE(hbox->isVisible());
  BOOST_REQUIRE(w1->isVisible());
  BOOST_REQUIRE(!w2->isVisible());
  BOOST_REQUIRE(w2->isExplicitlyHidden());
  BOOST_REQUIRE(w3->isVisible());

  BOOST_CHECK_EQUAL(hbox->sizeHint().x(), 2 * 32 + 1 * hbox->spacing());

  hbox->update();

  BOOST_CHECK_EQUAL(w1->geometry(), SGRecti(0,  0, 32, 32));
  BOOST_CHECK_EQUAL(w2->geometry(), SGRecti(0,  0,  0,  0));
  BOOST_CHECK_EQUAL(w3->geometry(), SGRecti(37, 0, 32, 32));

  hbox->setVisible(false);

  BOOST_REQUIRE(!hbox->isVisible());
  BOOST_REQUIRE(hbox->isExplicitlyHidden());
  BOOST_REQUIRE(!w1->isVisible());
  BOOST_REQUIRE(!w1->isExplicitlyHidden());
  BOOST_REQUIRE(!w2->isVisible());
  BOOST_REQUIRE(w2->isExplicitlyHidden());
  BOOST_REQUIRE(!w3->isVisible());
  BOOST_REQUIRE(!w3->isExplicitlyHidden());

  BOOST_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0, 0, 0));
  BOOST_CHECK_EQUAL(w2->geometry(), SGRecti(0, 0, 0, 0));
  BOOST_CHECK_EQUAL(w3->geometry(), SGRecti(0, 0, 0, 0));

  w2->setVisible(true);

  BOOST_REQUIRE(!w2->isVisible());
  BOOST_REQUIRE(!w2->isExplicitlyHidden());

  hbox->setVisible(true);

  BOOST_REQUIRE(hbox->isVisible());
  BOOST_REQUIRE(w1->isVisible());
  BOOST_REQUIRE(w2->isVisible());
  BOOST_REQUIRE(w3->isVisible());

  hbox->update();

  BOOST_CHECK_EQUAL(w1->geometry(), SGRecti(0,  0, 20, 32));
  BOOST_CHECK_EQUAL(w2->geometry(), SGRecti(25, 0, 20, 32));
  BOOST_CHECK_EQUAL(w3->geometry(), SGRecti(50, 0, 19, 32));
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( boxlayout_contents_margins )
{
  sc::Margins m;

  BOOST_REQUIRE(m.isNull());

  m = sc::Margins(5);

  BOOST_REQUIRE_EQUAL(m.l, 5);
  BOOST_REQUIRE_EQUAL(m.t, 5);
  BOOST_REQUIRE_EQUAL(m.r, 5);
  BOOST_REQUIRE_EQUAL(m.b, 5);

  m = sc::Margins(6, 7);

  BOOST_REQUIRE_EQUAL(m.l, 6);
  BOOST_REQUIRE_EQUAL(m.t, 7);
  BOOST_REQUIRE_EQUAL(m.r, 6);
  BOOST_REQUIRE_EQUAL(m.b, 7);

  BOOST_REQUIRE_EQUAL(m.horiz(), 12);
  BOOST_REQUIRE_EQUAL(m.vert(), 14);
  BOOST_REQUIRE(!m.isNull());

  m = sc::Margins(1, 2, 3, 4);

  BOOST_REQUIRE_EQUAL(m.l, 1);
  BOOST_REQUIRE_EQUAL(m.t, 2);
  BOOST_REQUIRE_EQUAL(m.r, 3);
  BOOST_REQUIRE_EQUAL(m.b, 4);

  BOOST_REQUIRE_EQUAL(m.horiz(), 4);
  BOOST_REQUIRE_EQUAL(m.vert(), 6);
  BOOST_REQUIRE_EQUAL(m.size(), SGVec2i(4, 6));

  sc::BoxLayoutRef hbox( new sc::HBoxLayout );

  hbox->setContentsMargins(5, 10, 15, 20);

  BOOST_CHECK_EQUAL(hbox->minimumSize(), SGVec2i(20, 30));
  BOOST_CHECK_EQUAL(hbox->sizeHint(),    SGVec2i(20, 30));
  BOOST_CHECK_EQUAL(hbox->maximumSize(), SGVec2i(20, 30));

  hbox->setGeometry(SGRecti(0, 0, 30, 40));

  BOOST_CHECK_EQUAL(hbox->geometry(), SGRecti(0, 0, 30, 40));
  BOOST_CHECK_EQUAL(hbox->contentsRect(), SGRecti(5, 10, 10, 10));

  TestWidgetRef w1( new TestWidget( SGVec2i(16, 16),
                                    SGVec2i(32, 32) ) ),
                w2( new TestWidget(*w1) ),
                w3( new TestWidget(*w1) );

  w1->setContentsMargin(5);
  w2->setContentsMargin(6);
  w3->setContentsMargin(7);

  BOOST_CHECK_EQUAL(w1->minimumSize(), SGVec2i(26, 26));
  BOOST_CHECK_EQUAL(w1->sizeHint(),    SGVec2i(42, 42));
  BOOST_CHECK_EQUAL(w1->maximumSize(), sc::LayoutItem::MAX_SIZE);

  BOOST_CHECK_EQUAL(w2->minimumSize(), SGVec2i(28, 28));
  BOOST_CHECK_EQUAL(w2->sizeHint(),    SGVec2i(44, 44));
  BOOST_CHECK_EQUAL(w2->maximumSize(), sc::LayoutItem::MAX_SIZE);

  BOOST_CHECK_EQUAL(w3->minimumSize(), SGVec2i(30, 30));
  BOOST_CHECK_EQUAL(w3->sizeHint(),    SGVec2i(46, 46));
  BOOST_CHECK_EQUAL(w3->maximumSize(), sc::LayoutItem::MAX_SIZE);

  hbox->addItem(w1);
  hbox->addItem(w2);
  hbox->addItem(w3);

  BOOST_CHECK_EQUAL(hbox->minimumSize(), SGVec2i(114, 60));
  BOOST_CHECK_EQUAL(hbox->sizeHint(),    SGVec2i(162, 76));
  BOOST_CHECK_EQUAL(hbox->maximumSize(), sc::LayoutItem::MAX_SIZE);

  hbox->setGeometry(SGRecti(0, 0, hbox->sizeHint().x(), hbox->sizeHint().y()));

  BOOST_CHECK_EQUAL(hbox->contentsRect(), SGRecti(5, 10, 142, 46));

  BOOST_CHECK_EQUAL(w1->geometry(), SGRecti(5,   10, 42, 46));
  BOOST_CHECK_EQUAL(w2->geometry(), SGRecti(52,  10, 44, 46));
  BOOST_CHECK_EQUAL(w3->geometry(), SGRecti(101, 10, 46, 46));

  BOOST_CHECK_EQUAL(w1->contentsRect(), SGRecti(10,  15, 32, 36));
  BOOST_CHECK_EQUAL(w2->contentsRect(), SGRecti(58,  16, 32, 34));
  BOOST_CHECK_EQUAL(w3->contentsRect(), SGRecti(108, 17, 32, 32));
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( boxlayout_hfw )
{
  TestWidgetRef w1( new TestWidgetHFW( SGVec2i(16,   16),
                                       SGVec2i(32,   32) ) ),
                w2( new TestWidgetHFW( SGVec2i(24,   24),
                                       SGVec2i(48,   48) ) );

  BOOST_CHECK_EQUAL(w1->heightForWidth(16), 64);
  BOOST_CHECK_EQUAL(w1->minimumHeightForWidth(16), 16);
  BOOST_CHECK_EQUAL(w2->heightForWidth(24), 96);
  BOOST_CHECK_EQUAL(w2->minimumHeightForWidth(24), 24);

  TestWidgetRef w_no_hfw( new TestWidget( SGVec2i(16,   16),
                                          SGVec2i(32,   32) ) );
  BOOST_CHECK(!w_no_hfw->hasHeightForWidth());
  BOOST_CHECK_EQUAL(w_no_hfw->heightForWidth(16), -1);
  BOOST_CHECK_EQUAL(w_no_hfw->minimumHeightForWidth(16), -1);

  // horizontal
  sc::HBoxLayout hbox;
  hbox.setSpacing(5);
  hbox.addItem(w1);
  hbox.addItem(w2);

  BOOST_CHECK_EQUAL(hbox.heightForWidth(45), w2->heightForWidth(24));
  BOOST_CHECK_EQUAL(hbox.heightForWidth(85), w2->heightForWidth(48));

  hbox.addItem(w_no_hfw);

  BOOST_CHECK_EQUAL(hbox.heightForWidth(66), 96);
  BOOST_CHECK_EQUAL(hbox.heightForWidth(122), 48);
  BOOST_CHECK_EQUAL(hbox.minimumHeightForWidth(66), 24);
  BOOST_CHECK_EQUAL(hbox.minimumHeightForWidth(122), 16);

  hbox.setGeometry(SGRecti(0, 0, 66, 24));

  BOOST_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0,  16, 24));
  BOOST_CHECK_EQUAL(w2->geometry(), SGRecti(21, 0, 24, 24));
  BOOST_CHECK_EQUAL(w_no_hfw->geometry(), SGRecti(50, 0, 16, 24));

  // vertical
  sc::VBoxLayout vbox;
  vbox.setSpacing(5);
  vbox.addItem(w1);
  vbox.addItem(w2);

  BOOST_CHECK_EQUAL(vbox.heightForWidth(24), 143);
  BOOST_CHECK_EQUAL(vbox.heightForWidth(48), 74);
  BOOST_CHECK_EQUAL(vbox.minimumHeightForWidth(24), 39);
  BOOST_CHECK_EQUAL(vbox.minimumHeightForWidth(48), 22);

  vbox.addItem(w_no_hfw);

  BOOST_CHECK_EQUAL(vbox.heightForWidth(24), 180);
  BOOST_CHECK_EQUAL(vbox.heightForWidth(48), 111);
  BOOST_CHECK_EQUAL(vbox.minimumHeightForWidth(24), 60);
  BOOST_CHECK_EQUAL(vbox.minimumHeightForWidth(48), 43);

  SGVec2i min_size = vbox.minimumSize(),
          size_hint = vbox.sizeHint();

  BOOST_CHECK_EQUAL(min_size, SGVec2i(24, 66));
  BOOST_CHECK_EQUAL(size_hint, SGVec2i(48, 122));

  vbox.setGeometry(SGRecti(0, 0, 24, 122));

  BOOST_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0,  24, 33));
  BOOST_CHECK_EQUAL(w2->geometry(), SGRecti(0, 38, 24, 47));
  BOOST_CHECK_EQUAL(w_no_hfw->geometry(), SGRecti(0, 90, 24, 32));

  // Vertical layouting modifies size hints, so check if they are correctly
  // restored
  BOOST_CHECK_EQUAL(min_size, vbox.minimumSize());
  BOOST_CHECK_EQUAL(size_hint, vbox.sizeHint());

  vbox.setGeometry(SGRecti(0, 0, 50, 122));

  BOOST_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0,  50, 25));
  BOOST_CHECK_EQUAL(w2->geometry(), SGRecti(0, 30, 50, 51));
  BOOST_CHECK_EQUAL(w_no_hfw->geometry(), SGRecti(0, 86, 50, 36));

  // Same geometry as before -> should get same widget geometry
  // (check internal size hint cache updates correctly)
  vbox.setGeometry(SGRecti(0, 0, 24, 122));

  BOOST_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0,  24, 33));
  BOOST_CHECK_EQUAL(w2->geometry(), SGRecti(0, 38, 24, 47));
  BOOST_CHECK_EQUAL(w_no_hfw->geometry(), SGRecti(0, 90, 24, 32));
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( item_alignment_rect )
{
  TestWidgetRef w1( new TestWidget( SGVec2i(16, 16),
                                    SGVec2i(32, 32) ) );

  const SGRecti r(10, 10, 64, 64);

  // Default: AlignFill -> fill up to maximum size
  BOOST_CHECK_EQUAL(w1->alignmentRect(r), r);

  // Horizontal

  // AlignLeft -> width from size hint, positioned on the left
  w1->setAlignment(sc::AlignLeft);
  BOOST_CHECK_EQUAL(w1->alignmentRect(r), SGRecti(10, 10, 32, 64));

  // AlignRight -> width from size hint, positioned on the left
  w1->setAlignment(sc::AlignRight);
  BOOST_CHECK_EQUAL(w1->alignmentRect(r), SGRecti(42, 10, 32, 64));

  // AlignHCenter -> width from size hint, positioned in the center
  w1->setAlignment(sc::AlignHCenter);
  BOOST_CHECK_EQUAL(w1->alignmentRect(r), SGRecti(26, 10, 32, 64));


  // Vertical

  // AlignTop -> height from size hint, positioned on the top
  w1->setAlignment(sc::AlignTop);
  BOOST_CHECK_EQUAL(w1->alignmentRect(r), SGRecti(10, 10, 64, 32));

  // AlignBottom -> height from size hint, positioned on the bottom
  w1->setAlignment(sc::AlignBottom);
  BOOST_CHECK_EQUAL(w1->alignmentRect(r), SGRecti(10, 42, 64, 32));

  // AlignVCenter -> height from size hint, positioned in the center
  w1->setAlignment(sc::AlignVCenter);
  BOOST_CHECK_EQUAL(w1->alignmentRect(r), SGRecti(10, 26, 64, 32));


  // Vertical + Horizontal
  w1->setAlignment(sc::AlignCenter);
  BOOST_CHECK_EQUAL(w1->alignmentRect(r), SGRecti(26, 26, 32, 32));
}

//------------------------------------------------------------------------------
// TODO extend to_nasal_helper for automatic argument conversion
static naRef f_Widget_visibilityChanged(nasal::CallContext ctx)
{
  sc::NasalWidget* w = ctx.from_nasal<sc::NasalWidget*>(ctx.me);

  if( !ctx.requireArg<bool>(0) )
    w->setGeometry(SGRecti(0, 0, -1, -1));

  return naNil();
}

//------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( nasal_widget )
{
  nasal::Context c;
  nasal::Hash globals = c.newHash();

  nasal::Object::setupGhost();
  nasal::Ghost<sc::LayoutItemRef>::init("LayoutItem");
  sc::NasalWidget::setupGhost(globals);

  nasal::Hash me = c.newHash();
  me.set("visibilityChanged", &f_Widget_visibilityChanged);
  sc::NasalWidgetRef w( new sc::NasalWidget(me.get_naRef()) );

  // Default layout sizes (no user set values)
  BOOST_CHECK_EQUAL(w->minimumSize(), SGVec2i(16, 16));
  BOOST_CHECK_EQUAL(w->sizeHint(),    SGVec2i(32, 32));
  BOOST_CHECK_EQUAL(w->maximumSize(), sc::LayoutItem::MAX_SIZE);

  // Changed layout sizes
  w->setLayoutMinimumSize( SGVec2i(2, 12) );
  w->setLayoutSizeHint(    SGVec2i(3, 13) );
  w->setLayoutMaximumSize( SGVec2i(4, 14) );

  BOOST_CHECK_EQUAL(w->minimumSize(), SGVec2i(2, 12));
  BOOST_CHECK_EQUAL(w->sizeHint(),    SGVec2i(3, 13));
  BOOST_CHECK_EQUAL(w->maximumSize(), SGVec2i(4, 14));

  // User set values (overwrite layout sizes)
  w->setMinimumSize( SGVec2i(15, 16) );
  w->setSizeHint(    SGVec2i(17, 18) );
  w->setMaximumSize( SGVec2i(19, 20) );

  BOOST_CHECK_EQUAL(w->minimumSize(), SGVec2i(15, 16));
  BOOST_CHECK_EQUAL(w->sizeHint(),    SGVec2i(17, 18));
  BOOST_CHECK_EQUAL(w->maximumSize(), SGVec2i(19, 20));

  // Only vertical user set values (layout/default for horizontal hints)
  w->setMinimumSize( SGVec2i(0, 21) );
  w->setSizeHint(    SGVec2i(0, 22) );
  w->setMaximumSize( SGVec2i(SGLimits<int>::max(), 23) );

  BOOST_CHECK_EQUAL(w->minimumSize(), SGVec2i(2, 21));
  BOOST_CHECK_EQUAL(w->sizeHint(),    SGVec2i(3, 22));
  BOOST_CHECK_EQUAL(w->maximumSize(), SGVec2i(4, 23));

  w->setVisible(false);
  BOOST_CHECK_EQUAL(w->geometry(), SGRecti(0, 0, -1, -1));
}
