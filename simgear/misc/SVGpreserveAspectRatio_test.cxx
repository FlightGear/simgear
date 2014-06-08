/// Unit tests for SVGpreserveAspectRatio
#define BOOST_TEST_MODULE misc
#include <BoostTestTargetConfig.h>

#include "SVGpreserveAspectRatio.hxx"

namespace simgear
{
  std::ostream& operator<<( std::ostream& strm,
                            const SVGpreserveAspectRatio& ar )
  {
    strm << "[ align_x=" << ar.alignX() <<
            ", align_y=" << ar.alignY() <<
            ", meet=" << ar.meet() <<
            "]";
    return strm;
  }
}

BOOST_AUTO_TEST_CASE( parse_attribute )
{
  using simgear::SVGpreserveAspectRatio;

  SVGpreserveAspectRatio ar = SVGpreserveAspectRatio::parse("none");
  BOOST_CHECK( ar.scaleToFill() );
  BOOST_CHECK( !ar.scaleToFit() );
  BOOST_CHECK( !ar.scaleToCrop() );
  BOOST_CHECK_EQUAL( ar.alignX(), SVGpreserveAspectRatio::ALIGN_NONE );
  BOOST_CHECK_EQUAL( ar.alignY(), SVGpreserveAspectRatio::ALIGN_NONE );

  SVGpreserveAspectRatio ar_meet = SVGpreserveAspectRatio::parse("none meet");
  SVGpreserveAspectRatio ar_slice = SVGpreserveAspectRatio::parse("none slice");

  BOOST_CHECK_EQUAL( ar, ar_meet );
  BOOST_CHECK_EQUAL( ar, ar_slice );

  ar_meet  = SVGpreserveAspectRatio::parse("xMidYMid meet");
  BOOST_CHECK( !ar_meet.scaleToFill() );
  BOOST_CHECK( ar_meet.scaleToFit() );
  BOOST_CHECK( !ar_meet.scaleToCrop() );
  BOOST_CHECK_EQUAL( ar_meet.alignX(), SVGpreserveAspectRatio::ALIGN_MID );
  BOOST_CHECK_EQUAL( ar_meet.alignY(), SVGpreserveAspectRatio::ALIGN_MID );

  ar_slice = SVGpreserveAspectRatio::parse("xMidYMid slice");
  BOOST_CHECK( !ar_slice.scaleToFill() );
  BOOST_CHECK( !ar_slice.scaleToFit() );
  BOOST_CHECK( ar_slice.scaleToCrop() );
  BOOST_CHECK_EQUAL( ar_slice.alignX(), SVGpreserveAspectRatio::ALIGN_MID );
  BOOST_CHECK_EQUAL( ar_slice.alignY(), SVGpreserveAspectRatio::ALIGN_MID );

  BOOST_CHECK_NE(ar_meet, ar_slice);

  // defer is ignored, meet is default
  ar_meet  = SVGpreserveAspectRatio::parse("defer xMinYMin");
  BOOST_CHECK( !ar_meet.scaleToFill() );
  BOOST_CHECK( ar_meet.scaleToFit() );
  BOOST_CHECK( !ar_meet.scaleToCrop() );
  BOOST_CHECK_EQUAL( ar_meet.alignX(), SVGpreserveAspectRatio::ALIGN_MIN );
  BOOST_CHECK_EQUAL( ar_meet.alignY(), SVGpreserveAspectRatio::ALIGN_MIN );
}
