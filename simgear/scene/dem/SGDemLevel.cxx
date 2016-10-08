#include <iomanip>
#include <fstream>

#include <simgear/misc/sg_dir.hxx>
#include <simgear/debug/logstream.hxx>

#include <simgear/scene/dem/SGDemLevel.hxx>

bool SGDemLevel::isValid( unsigned int wo, unsigned int so, unsigned int eo, unsigned int no ) const
{
    bool valid = true;

    return valid;
}

void SGDemLevel::addExtent( unsigned int wo, unsigned int so, unsigned int eo, unsigned int no )
{
    // add bitmap to level extent
    unsigned int minLon = (wo+0)/8;
    unsigned int maxLon = (eo+0)/8;

    unsigned int minLat = (so+0)/8;
    unsigned int maxLat = (no+0)/8;

    fprintf(stderr, "addExtent %u, %u - %u, %u   - minLon %u, minLat %u, maxLon %u, maxLat %u\n", wo, so, eo, no, minLon, minLat, maxLon, maxLat );
    for ( unsigned int lat=minLat; lat<=maxLat; lat++ ) {
        // convert lon range to mask range
        unsigned char minMask = minLon/8;
        unsigned char maxMask = maxLon/8;

        fprintf(stderr, "addExtent minMask %u, maxMask %d\n", minMask, maxMask );
        for ( unsigned int msk=minMask; msk<=maxMask; msk++ ) {
            if ( msk == minMask ) {
                // find beginning of mask
                unsigned char bitPos = minLon % 8;
                unsigned char bitMask = 0;
                for ( unsigned char i=0; i<=bitPos; i++ ) {
                    bitMask = (bitMask >> 1) | 0x80;
                }
                fprintf(stderr, "addExtent minMask - lat %u, lonmsk %u, bits %u\n", lat, msk, bitMask );
                extent[lat][msk] |= bitMask;
            } else if ( msk == maxMask ) {
                // find end of mask
                unsigned char bitPos = maxLon % 8;
                unsigned char bitMask = 0;
                for ( unsigned int i=0; i<=bitPos; i++ ) {
                    bitMask = (bitMask >> 1) | 0x80;
                }
                fprintf(stderr, "addExtent maxMask - lat %u, lonmsk %u, bits %x\n", lat, msk, bitMask );
                extent[lat][msk] |= bitMask;
            } else {
                fprintf(stderr, "addExtent middlemask - lat %u, lonmsk %u, bits %x\n", lat, msk, 0xFF );
                extent[lat][msk] = 0xFF;
            }
        }
    }
}

void SGDemLevel::close( void )
{
    // save extent to file
    fprintf( stderr, "closing level\n" );
    SGPath extentFile = path / "demextent.txt";
    std::fstream demExtent(extentFile.c_str(), std::ios_base::out | std::ios_base::binary );

    for ( unsigned char lat = 0; lat < 180; lat++ ) {
        for ( unsigned char lon = 0; lon < 360/8; lon++ ) {
            demExtent << extent[lat][lon];
        }
    }
    fprintf( stderr, "closing level complete\n" );
}
