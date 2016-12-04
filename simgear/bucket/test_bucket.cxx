/**************************************************************************
 * test_bucket.cxx -- unit-tests for SGBucket class
 *
 * Copyright (C) 2014  James Turner - <zakalawe@mac.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * $Id$
 **************************************************************************/

#include <simgear/compiler.h>

#include <iostream>
#include <cstdlib>
#include <cstring>

using std::cout;
using std::cerr;
using std::endl;

#include <simgear/bucket/newbucket.hxx>
#include <simgear/misc/test_macros.hxx>

void testBucketSpans()
{
    SG_CHECK_EQUAL(sg_bucket_span(0.0), 0.125);
    SG_CHECK_EQUAL(sg_bucket_span(-20), 0.125);
    SG_CHECK_EQUAL(sg_bucket_span(-40), 0.25);
    SG_CHECK_EQUAL(sg_bucket_span(89.9), 12.0);
    SG_CHECK_EQUAL(sg_bucket_span(88.1), 4.0);
    SG_CHECK_EQUAL(sg_bucket_span(-89.9), 12.0);
}

void testBasic()
{
    SGBucket b1(5.1, 55.05);
    SG_CHECK_EQUAL(b1.get_chunk_lon(), 5);
    SG_CHECK_EQUAL(b1.get_chunk_lat(), 55);
    SG_CHECK_EQUAL(b1.get_x(), 0);
    SG_CHECK_EQUAL(b1.get_y(), 0);
    SG_CHECK_EQUAL(b1.gen_index(), 3040320);
    SG_CHECK_EQUAL(b1.gen_base_path(), "e000n50/e005n55");
    SG_VERIFY(b1.isValid());
    
    SGBucket b2(-10.1, -43.8);
    SG_CHECK_EQUAL(b2.get_chunk_lon(), -11);
    SG_CHECK_EQUAL(b2.get_chunk_lat(), -44);
    SG_CHECK_EQUAL(b2.get_x(), 3);
    // Latitude chunks numbered bottom to top, it seems
    SG_CHECK_EQUAL(b2.get_y(), 1);
    SG_CHECK_EQUAL(b2.gen_base_path(), "w020s50/w011s44");
    SG_VERIFY(b2.isValid());
    
    SGBucket b3(123.48, 9.01);
    SG_CHECK_EQUAL(b3.get_chunk_lon(), 123);
    SG_CHECK_EQUAL(b3.get_chunk_lat(), 9);
    SG_CHECK_EQUAL(b3.get_x(), 3);
    SG_CHECK_EQUAL(b3.get_y(), 0);
    SG_CHECK_EQUAL(b3.gen_base_path(), "e120n00/e123n09");
    SG_VERIFY(b3.isValid());
    
    SGBucket defBuck;
    SG_VERIFY(!defBuck.isValid());
    
    b3.make_bad();
    SG_VERIFY(!b3.isValid());

    SGBucket atAntiMeridian(180.0, 12.3);
    SG_VERIFY(atAntiMeridian.isValid());
    SG_CHECK_EQUAL(atAntiMeridian.get_chunk_lon(), -180);
    SG_CHECK_EQUAL(atAntiMeridian.get_x(), 0);

    SGBucket atAntiMeridian2(-180.0, -78.1);
    SG_VERIFY(atAntiMeridian2.isValid());
    SG_CHECK_EQUAL(atAntiMeridian2.get_chunk_lon(), -180);
    SG_CHECK_EQUAL(atAntiMeridian2.get_x(), 0);

// check comparisom operator overload
    SGBucket b4(5.11, 55.1);
    SG_VERIFY(b1 == b4); // should be equal
    SG_VERIFY(b1 == b1);
    SG_VERIFY(b1 != defBuck);
    SG_VERIFY(b1 != b2);
    
// check wrapping/clipping of inputs
    SGBucket wrapMeridian(-200.0, 45.0);
    SG_CHECK_EQUAL(wrapMeridian.get_chunk_lon(), 160);

    SGBucket clipPole(48.9, 91);
    SG_CHECK_EQUAL(clipPole.get_chunk_lat(), 89);
}

void testPolar()
{
    SGBucket b1(0.0, 89.92);
    SGBucket b2(10.0, 89.96);
    SG_CHECK_EQUAL(b1.get_chunk_lat(), 89);
    SG_CHECK_EQUAL(b1.get_chunk_lon(), 0);
    SG_CHECK_EQUAL(b1.get_x(), 0);
    SG_CHECK_EQUAL(b1.get_y(), 7);
    
    SG_CHECK_EQUAL_EP(b1.get_highest_lat(), 90.0);
    SG_CHECK_EQUAL_EP(b1.get_width_m(), 10.0);
    
    SG_CHECK_EQUAL(b2.get_chunk_lat(), 89);
    SG_CHECK_EQUAL(b2.get_chunk_lon(), 0);
    SG_CHECK_EQUAL(b2.get_x(), 0);
    SG_CHECK_EQUAL(b2.get_y(), 7);
    
    SG_CHECK_EQUAL(b1.gen_index(), b2.gen_index());
    
    SGGeod actualNorthPole1 = b1.get_corner(2);
    SGGeod actualNorthPole2 = b1.get_corner(3);
    SG_CHECK_EQUAL_EP(actualNorthPole1.getLatitudeDeg(), 90.0);
    SG_CHECK_EQUAL_EP(actualNorthPole1.getLongitudeDeg(), 12.0);
    SG_CHECK_EQUAL_EP(actualNorthPole2.getLatitudeDeg(), 90.0);
    SG_CHECK_EQUAL_EP(actualNorthPole2.getLongitudeDeg(), 0.0);

    SGBucket b3(-2, 89.88);
    SGBucket b4(-7, 89.88);
    SG_CHECK_EQUAL(b3.gen_index(), b4.gen_index());
    
    // south pole
    SGBucket b5(-170, -89.88);
    SGBucket b6(-179, -89.88);
    
    SG_CHECK_EQUAL(b5.get_chunk_lat(), -90);
    SG_CHECK_EQUAL(b5.get_chunk_lon(), -180);
    SG_CHECK_EQUAL(b5.get_x(), 0);
    SG_CHECK_EQUAL(b5.get_y(), 0);
    SG_CHECK_EQUAL(b5.gen_index(), b6.gen_index());
    SG_CHECK_EQUAL_EP(b5.get_highest_lat(), -90.0);
    SG_CHECK_EQUAL_EP(b5.get_width_m(), 10.0);
    
    SGGeod actualSouthPole1 = b5.get_corner(0);
    SGGeod actualSouthPole2 = b5.get_corner(1);
    SG_CHECK_EQUAL_EP(actualSouthPole1.getLatitudeDeg(), -90.0);
    SG_CHECK_EQUAL_EP(actualSouthPole1.getLongitudeDeg(), -180);
    SG_CHECK_EQUAL_EP(actualSouthPole2.getLatitudeDeg(), -90.0);
    SG_CHECK_EQUAL_EP(actualSouthPole2.getLongitudeDeg(), -168);
    
    SGBucket b7(200, 89.88);
    SG_CHECK_EQUAL(b7.get_chunk_lon(), -168);

}

// test the tiles just below the pole (between 86 & 89 degrees N/S)
void testNearPolar()
{
    SGBucket b1(1, 88.5);
    SGBucket b2(-1, 88.8);
    SG_CHECK_EQUAL(b1.get_chunk_lon(), 0);
    SG_CHECK_EQUAL(b1.get_chunk_lat(), 88);
    SG_VERIFY(b1.gen_index() != b2.gen_index());

    SGBucket b3(176.1, 88.5);
    SG_CHECK_EQUAL(b3.get_chunk_lon(), 176);
    
    SGBucket b4(-178, 88.5);
    SG_CHECK_EQUAL(b4.get_chunk_lon(), -180);
}

void testOffset()
{
    // bucket just below the 22 degree cutoff, so the next tile north
    // is twice the width
    SGBucket b1(-59.8, 21.9);
    SG_CHECK_EQUAL(b1.get_chunk_lat(), 21);
    SG_CHECK_EQUAL(b1.get_chunk_lon(), -60);
    SG_CHECK_EQUAL(b1.get_x(), 1);
    SG_CHECK_EQUAL(b1.get_y(), 7);
    
    // offset vertically
    SGBucket b2(b1.sibling(0, 1));
    SG_CHECK_EQUAL(b2.get_chunk_lat(), 22);
    SG_CHECK_EQUAL(b2.get_chunk_lon(), -60);
    SG_CHECK_EQUAL(b2.get_x(), 0);
    SG_CHECK_EQUAL(b2.get_y(), 0);

    SG_CHECK_EQUAL(b2.gen_index(), sgBucketOffset(-59.8, 21.9, 0, 1));
    
    // offset vertically and horizontally. We compute horizontal (x)
    // movement at the target latitude, so this should move 0.25 * -3 degrees,
    // NOT 0.125 * -3 degrees.
    SGBucket b3(b1.sibling(-3, 1));
    SG_CHECK_EQUAL(b3.get_chunk_lat(), 22);
    SG_CHECK_EQUAL(b3.get_chunk_lon(), -61);
    SG_CHECK_EQUAL(b3.get_x(), 1);
    SG_CHECK_EQUAL(b3.get_y(), 0);
    
    SG_CHECK_EQUAL(b3.gen_index(), sgBucketOffset(-59.8, 21.9, -3, 1));
}

void testPolarOffset()
{
    SGBucket b1(-11.7, -89.6);
    SG_CHECK_EQUAL(b1.get_chunk_lat(), -90);
    SG_CHECK_EQUAL(b1.get_chunk_lon(), -12);
    SG_CHECK_EQUAL(b1.get_x(), 0);
    SG_CHECK_EQUAL(b1.get_y(), 3);
    
    // offset horizontally
    SGBucket b2(b1.sibling(-2, 0));
    SG_CHECK_EQUAL(b2.get_chunk_lat(), -90);
    SG_CHECK_EQUAL(b2.get_chunk_lon(), -36);
    SG_CHECK_EQUAL(b2.get_x(), 0);
    SG_CHECK_EQUAL(b2.get_y(), 3);
    
    SG_CHECK_EQUAL(b2.gen_index(), sgBucketOffset(-11.7, -89.6, -2, 0));
    
// offset and wrap
    SGBucket b3(-170, 89.1);
    SGBucket b4(b3.sibling(-1, 0));
    SG_CHECK_EQUAL(b4.get_chunk_lat(), 89);
    SG_CHECK_EQUAL(b4.get_chunk_lon(), 168);
    SG_CHECK_EQUAL(b4.get_x(), 0);
    SG_CHECK_EQUAL(b4.get_y(), 0);
    
    SG_CHECK_EQUAL(b4.gen_index(), sgBucketOffset(-170, 89.1, -1, 0));

    
    SGBucket b5(177, 87.3);
    SGBucket b6(b5.sibling(1, 1));
    SG_CHECK_EQUAL(b6.get_chunk_lat(), 87);
    SG_CHECK_EQUAL(b6.get_chunk_lon(), -180);
    SG_CHECK_EQUAL(b6.get_x(), 0);
    SG_CHECK_EQUAL(b6.get_y(), 3);
    
    SG_CHECK_EQUAL(b6.gen_index(), sgBucketOffset(177, 87.3, 1, 1));

    // offset vertically towards the pole
    SGBucket b7(b1.sibling(0, -5));
    SG_VERIFY(!b7.isValid());
    
    SG_VERIFY(!SGBucket(0, 90).sibling(0, 1).isValid());
}

// test behaviour of bucket-offset near the anti-meridian (180-meridian)
void testOffsetWrap()
{
    // near the equator
    SGBucket b1(-179.8, 16.8);
    SG_CHECK_EQUAL(b1.get_chunk_lat(), 16);
    SG_CHECK_EQUAL(b1.get_chunk_lon(), -180);
    SG_CHECK_EQUAL(b1.get_x(), 1);
    SG_CHECK_EQUAL(b1.get_y(), 6);
    
    SGBucket b2(b1.sibling(-2, 0));
    SG_CHECK_EQUAL(b2.get_chunk_lat(), 16);
    SG_CHECK_EQUAL(b2.get_chunk_lon(), 179);
    SG_CHECK_EQUAL(b2.get_x(), 7);
    SG_CHECK_EQUAL(b2.get_y(), 6);
    SG_CHECK_EQUAL(b2.gen_index(), sgBucketOffset(-179.8, 16.8, -2, 0));
    
    
}

int main(int argc, char* argv[])
{
    testBucketSpans();
    
    testBasic();
    testPolar();
    testNearPolar();
    testOffset();
    testOffsetWrap();
    testPolarOffset();
    
    cout << "all tests passed OK" << endl;
    return 0; // passed
}

