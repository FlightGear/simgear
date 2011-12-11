// ReaderWriterSPT.cxx -- Provide a paged database for flightgear scenery.
//
// Copyright (C) 2010 - 2011  Mathias Froehlich
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <cstdlib>
#include <iostream>
#include <set>

#include "BucketBox.hxx"

namespace simgear {

static std::set<BucketBox> boxes;
static std::set<long> buckets;

void createTestTree(const BucketBox& bucketBox)
{
    if (!boxes.insert(bucketBox).second) {
        std::cerr << "Duplicate BucketBox: " << bucketBox << std::endl;
        exit(EXIT_FAILURE);
    }
  
    if (bucketBox.getIsBucketSize()) {
        // We have a leaf node
        if (!buckets.insert(bucketBox.getBucket().gen_index()).second) {
            std::cerr << "Duplicate BucketBox: " << bucketBox << std::endl;
            exit(EXIT_FAILURE);
        }
    } else {
        BucketBox bucketBoxList[100];
        unsigned numTiles = bucketBox.getSubDivision(bucketBoxList, 100);

        for (unsigned i = 0; i < numTiles; ++i) {
            createTestTree(bucketBoxList[i]);
        }
    }
}

}

int
main()
{
    simgear::createTestTree(simgear::BucketBox(0, -90, 360, 180));
    return EXIT_SUCCESS;
}
