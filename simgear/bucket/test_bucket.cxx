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
    COMPARE(sg_bucket_span(0.0), 0.125);
    COMPARE(sg_bucket_span(-20), 0.125);
    COMPARE(sg_bucket_span(-40), 0.25);
    COMPARE(sg_bucket_span(89.9), 12.0);
    COMPARE(sg_bucket_span(88.1), 4.0);
    COMPARE(sg_bucket_span(-89.9), 12.0);
}

void testPolar()
{
    SGBucket b1(0.0, 89.92);
    SGBucket b2(10.0, 89.96);
    COMPARE(b1.get_chunk_lat(), 89);
    COMPARE(b1.get_chunk_lon(), 0);
    COMPARE(b1.get_x(), 0);
    COMPARE(b1.get_y(), 7);
    
    COMPARE(b1.gen_index(), b2.gen_index());
    
    SGBucket b3(-2, 89.88);
    SGBucket b4(-7, 89.88);
    COMPARE(b3.gen_index(), b4.gen_index());
    
    // south pole
    SGBucket b5(-170, -89.88);
    SGBucket b6(-179, -89.88);
    
    COMPARE(b5.get_chunk_lat(), -90);
    COMPARE(b5.get_chunk_lon(), -180);
    COMPARE(b5.get_x(), 0);
    COMPARE(b5.get_y(), 0);
    COMPARE(b5.gen_index(), b6.gen_index());
    
    // no automatic wrapping of these values occurs
    SGBucket b7(200, 89.88);
    COMPARE(b7.get_chunk_lon(), 192);

}

void testNearPolar()
{
    SGBucket b1(1, 88.5);
    SGBucket b2(-1, 88.8);
    COMPARE(b1.get_chunk_lon(), 0);
    COMPARE(b1.get_chunk_lat(), 88);
    VERIFY(b1.gen_index() != b2.gen_index());
    
    SGBucket b3(176.1, 88.5);
    COMPARE(b3.get_chunk_lon(), 176);
    
    SGBucket b4(-178, 88.5);
    // the fix for the polar cap issue, causes this to report -180,
    // not -184
    COMPARE(b4.get_chunk_lon(), -180);
}

void testOffset()
{
    
}

int main(int argc, char* argv[])
{
    testBucketSpans();
    
    SGBucket b1(5.1, 55.05);
    COMPARE(b1.get_chunk_lon(), 5);
    COMPARE(b1.get_chunk_lat(), 55);
    COMPARE(b1.get_x(), 0);
    COMPARE(b1.get_y(), 0);
    COMPARE(b1.gen_index(), 3040320);
    COMPARE(b1.gen_base_path(), "e000n50/e005n55");
    
    SGBucket b2(-10.1, -43.8);
    COMPARE(b2.get_chunk_lon(), -11);
    COMPARE(b2.get_chunk_lat(), -44);
    COMPARE(b2.get_x(), 3);
    COMPARE(b2.get_y(), 1); // latitude chunks numbered bottom to top, it seems
    COMPARE(b2.gen_base_path(), "w020s50/w011s44");
    
    SGBucket b3(123.48, 9.01);
    COMPARE(b3.get_chunk_lon(), 123);
    COMPARE(b3.get_chunk_lat(), 9);
    COMPARE(b3.get_x(), 3);
    COMPARE(b3.get_y(), 0);
    COMPARE(b3.gen_base_path(), "e120n00/e123n09");
    
    testPolar();
    testNearPolar();
    testOffset();
    
    cout << "all tests passed OK" << endl;
    return 0; // passed
}

