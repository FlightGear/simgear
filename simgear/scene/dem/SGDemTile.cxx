#include <iomanip>
#include <sstream>

#include <simgear/misc/sg_dir.hxx>
#include <simgear/debug/logstream.hxx>

#include <simgear/scene/dem/SGDem.hxx>
#include <simgear/scene/dem/SGDemTile.hxx>
#include <simgear/scene/dem/SGDemSession.hxx>

SGDemTile::SGDemTile( const SGPath& levelDir, unsigned wo, unsigned so, int w, int h, int x, int y, int o, bool cache)
{
    // add the tile name to the level path
    ref_lon = (int)wo/8 - 180;
    ref_lat = (int)so/8 - 90;
    path    = levelDir / getTileName( ref_lon, ref_lat );
    width   = w;
    height  = h;
    resx    = x;
    resy    = y;
    overlap = o;

    pixResX = ((double)width/(double)8.0)/(double)(resx-1);
    pixResY = ((double)height/(double)8.0)/(double)(resy-1);

    if ( cache ) {
        raster = cacheTile( path );
    } else {
        raster = NULL;
    }
}

void SGDemTile::dbgDumpDataset( GDALDataset* poDataset ) const
{
    double adfGeoTransform[6];

    SG_LOG( SG_TERRAIN, SG_INFO, "Driver: " << poDataset->GetDriver()->GetDescription() << "/" << poDataset->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );
    SG_LOG( SG_TERRAIN, SG_INFO, "Size is " << poDataset->GetRasterXSize() << " x " << poDataset->GetRasterYSize() << " x " << poDataset->GetRasterCount() );

    if( poDataset->GetProjectionRef() != NULL ) {
        SG_LOG( SG_TERRAIN, SG_INFO, "Projection is " << poDataset->GetProjectionRef() );
    }

    if( poDataset->GetGeoTransform( adfGeoTransform ) == CE_None ) {
        SG_LOG( SG_TERRAIN, SG_INFO, "Origin = (" << adfGeoTransform[0] << ", " << adfGeoTransform[3] << ")" );
        SG_LOG( SG_TERRAIN, SG_INFO, "Pixel Size = (" << adfGeoTransform[1] << ", " << adfGeoTransform[5] << ")" );
    }
}

void SGDemTile::dbgDumpBand( GDALRasterBand* poBand ) const
{
    int             nBlockXSize, nBlockYSize;
    int             bGotMin, bGotMax;
    double          adfMinMax[2];

    poBand->GetBlockSize( &nBlockXSize, &nBlockYSize );
    SG_LOG( SG_TERRAIN, SG_INFO, "Block=" << nBlockXSize << " x " << nBlockYSize << " Type=" << GDALGetDataTypeName(poBand->GetRasterDataType()) << "ColorInterp=" << GDALGetColorInterpretationName( poBand->GetColorInterpretation()) );

    adfMinMax[0] = poBand->GetMinimum( &bGotMin );
    adfMinMax[1] = poBand->GetMaximum( &bGotMax );
    if( ! (bGotMin && bGotMax) ) {
        GDALComputeRasterMinMax((GDALRasterBandH)poBand, TRUE, adfMinMax);
    }
    SG_LOG( SG_TERRAIN, SG_INFO, "Min=" << adfMinMax[0] << ", Max=" << adfMinMax[1] );
    if( poBand->GetOverviewCount() > 0 ) {
        SG_LOG( SG_TERRAIN, SG_INFO, "Band has " << poBand->GetOverviewCount() << " overviews." );
    }
    if( poBand->GetColorTable() != NULL ) {
        SG_LOG( SG_TERRAIN, SG_INFO, "Band has a color table with " << poBand->GetColorTable()->GetColorEntryCount() << " entries." );
    }
}

unsigned short* SGDemTile::cacheTile( const SGPath& path )
{
    unsigned short* r = NULL;

    GDALDataset*    poDataset = NULL;
    GDALRasterBand* poBand = NULL;

    // check if file exists to supress GDAL errors...
    if ( path.exists() ) {
        poDataset = (GDALDataset *)GDALOpen( path.c_str(), GA_ReadOnly );
    }

    if ( poDataset ) {

#if DEM_DEBUG
        dbgDumpDataset( poDataset );
#endif

        // read the bands into array
        unsigned int numRasters = poDataset->GetRasterCount();
        for ( unsigned int rb=1; rb<=numRasters; rb++ ) {
            poBand = poDataset->GetRasterBand(rb);

#if DEM_DEBUG
            dbgDumpBand( poBand );
#endif

            // if this is the raster we're looking for
            if ( rb == 1 ) {
                int   nXSize = poBand->GetXSize();
                int   nYSize = poBand->GetYSize();
                // SG_LOG( SG_TERRAIN, SG_INFO, "Reading raster " << rb << " (" << nXSize << "x" << nYSize << ")" );

                r = (unsigned short *)CPLMalloc(sizeof(unsigned short)*nXSize*nYSize);
                //fprintf( stderr, "reading raster size %d x %d - buffer is %p\n", nXSize, nYSize, r );
                CPLErr err = poBand->RasterIO( GF_Read, 0, 0, nXSize, nYSize, r, nXSize, nYSize, GDT_UInt16, 0, 0 );
                if ( err ) {
                    fprintf(stderr, "Error reading raster\n");
                }
            }
        }

        GDALClose( poDataset );
    }

    return r;
}

// Create a new DEM tile from multiple source tiles ( at an expected lower resolution )
// code based on gdalwarp, but with most options removed.
// example dgalwarp usage this function is based on:
// gdalwarp -r cubic -ts 1201 1201 -te -85.0 32.0 -83.0 34.0 -dstnodata 0 -co "COMPRESS=DEFLATE" temp/N32W085.hgt temp/merged_cubic.tiff

SGDemTile::SGDemTile( const SGPath& levelDir, unsigned wo, unsigned so, int w, int h, int x, int y, int overlap, const SGDemSession& s, bool& bWritten )
{
    fprintf( stderr, "Writing tile: lon offset is %u, lat offset is %u\n", wo, so );

    // assume failure
    bWritten = false;

    // add the tile name to the level path
    ref_lon = (int)wo/8 - 180;
    ref_lat = (int)so/8 -  90;

    path    = levelDir / getTileName( ref_lon, ref_lat );
    width   = w;
    height  = h;
    resx    = x;
    resy    = y;

    raster = NULL;

    // use gdal warp api to generate tile from session
    char** papszSrcFiles = NULL;
    char** papszWarpOptions = NULL;
    char** papszTO = NULL;

    double dfMinX=0.0, dfMinY=0.0, dfMaxX=0.0, dfMaxY=0.0;  // -te
    double overlapw = (double)w/(double)resx * overlap;
    double overlaph = (double)h/(double)resy * overlap;
    int    nForcePixels = resx+(2*overlap), nForceLines = resy+(2*overlap);     // -ts

    // -dstnodata
    papszWarpOptions = CSLSetNameValue(papszWarpOptions, "INIT_DEST", "0");

    // target extents ( +/- 1 pixel for normals, and skirts )
    dfMinX = (double)ref_lon - overlapw;
    dfMinY = (double)ref_lat - overlaph;
    dfMaxX = (double)ref_lon + w + overlapw;
    dfMaxY = (double)ref_lat + h + overlaph;

    SG_LOG( SG_TERRAIN, SG_INFO, "overlapw: " << overlapw << " overlapw " << overlapw << " resx " << resx << " resy " << resy << " w " << w << " h " << h );
    SG_LOG( SG_TERRAIN, SG_INFO, " dfMinX: " << dfMinX << " dfMinY: " << dfMinY << " dfMaxX: " << dfMaxX << " dfMaxY: " << dfMaxY );

    // generate list of source files in session
    const std::vector<SGDemTileRef>& tiles = s.getTiles();
    SG_LOG( SG_TERRAIN, SG_INFO, " create tile from " << tiles.size() << " source tiles" );

    for ( unsigned int i=0; i<tiles.size(); i++ ) {
        // check if the file exists
        if ( tiles[i]->getPath().exists() ) {
            papszSrcFiles = CSLAddString( papszSrcFiles, tiles[i]->getPath().c_str() );
            SG_LOG( SG_TERRAIN, SG_INFO, " Adding tile " << tiles[i]->getPath() );
        } else {
            // SG_LOG( SG_TERRAIN, SG_INFO, " tile " << tiles[i]->getPath() << " doesn't exist" );
        }
    }

    // create output
    if ( papszSrcFiles ) {
        GDALDatasetH hDstDS = createTile( papszSrcFiles, path.c_str(), nForceLines, nForcePixels,
                                          dfMinX, dfMinY, dfMaxX, dfMaxY,
                                          papszTO );

        if( hDstDS != NULL ) {
            /* -------------------------------------------------------------------- */
            /*      Loop over all source files, processing each in turn.            */
            /* -------------------------------------------------------------------- */
            int iSrc;
            for( iSrc = 0; papszSrcFiles[iSrc] != NULL; iSrc++ )
            {
                doWarp( iSrc, papszSrcFiles[iSrc], hDstDS, papszTO, papszWarpOptions );
            }

            // manually set min/max to all tiles are consistent when viewing in qgis/grass
            // we need to get the raster band
            GDALDataset*    poDataset = (GDALDataset *)hDstDS;
            GDALRasterBand* poBand = NULL;

            // read the bands into array
            unsigned int numRasters = poDataset->GetRasterCount();
            for ( unsigned int rb=1; rb<=numRasters; rb++ ) {
                poBand = poDataset->GetRasterBand(rb);

                // if this is the raster we're looking for
                if ( rb == 1 ) {
                    double pdfMin, pdfMax, pdfMean, pdfStddev;

                    poBand->ComputeStatistics(false, &pdfMin, &pdfMax, &pdfMean, &pdfStddev, NULL, NULL );

                    fprintf(stderr, "Got band min as %lf, max as %lf\n", pdfMin, pdfMax );

                    // force status to sea level / round of mnt everest
                    pdfMin = 0; pdfMax = 9000;
                    poBand->SetStatistics( pdfMin, pdfMax, pdfMean, pdfStddev);

                    fprintf(stderr, "Setting band min to %lf, max to %lf\n", pdfMin, pdfMax );
                }
            }

            /* -------------------------------------------------------------------- */
            /*      Final Cleanup.                                                  */
            /* -------------------------------------------------------------------- */
            CPLErrorReset();
            GDALFlushCache( hDstDS );
            GDALClose( hDstDS );

            CSLDestroy( papszSrcFiles );
            CSLDestroy( papszWarpOptions );
            CSLDestroy( papszTO );

            GDALDumpOpenDatasets( stderr );

            bWritten = true;
        }
    }
}

SGDemTile::~SGDemTile()
{
    // free the raster
    if ( raster ) {
        CPLFree( raster );
    }
}

unsigned short SGDemTile::getAlt( const SGGeod& loc ) const
{
    if ( raster ) {
        bool debug = false;

        // get lon and lat reletive to sw corner
        double offLon = loc.getLongitudeDeg() - (double)ref_lon;
        double offLat = loc.getLatitudeDeg()  - (double)ref_lat;

        // raster has a 1 pixel border on all sides
        // take that into account
        if ( fabs( offLon ) < 0.000001 ) {
            debug = true;
        }

        double fractLon = offLon / (double)width;
        double fractLat = offLat / (double)height;

        int l = (int)( (double)(resy-1) - ((double)(resy-1)*fractLat) + 1 );
        int p = (int)( (double)(resx-1) * fractLon + 1 );

        if ( debug ) {
            printf( "SGDemTile::getAlt at %lf,%lf: offLon is %lf, offLat is %lf, width is %d, height is %d, fractLon is %lf, fractLat is %lf, resx is %d, resy is %d, line %d, pixel %d\n", 
                    loc.getLongitudeDeg(), loc.getLatitudeDeg(),
                    offLon, offLat, width, height, fractLon, fractLat,
                    resx, resy, l, p );
        }

        return raster[l*(resx+2)+p];
    } else {
        return 0;
    }
}

void SGDemTile::getGeods( unsigned wo, unsigned so, unsigned eo, unsigned no, int grid_width, int grid_height, unsigned subx, unsigned suby, int incw, int inch, ::std::vector<SGGeod>& geods, bool Debug1, bool Debug2 )
{
    // grid width and height include the skirt
    // sw and ne do not
    // we need to find the starting and ending l and p;
    int startl;//, endl;
    int startp;//, endp;
    double startlat, startlon;
    double endlat,   endlon;

    startlon = SGDem::offsetToLongitudeDeg(wo) - incw*pixResX;
    startlat = SGDem::offsetToLatitudeDeg(so)  - inch*pixResY;

    endlon   = SGDem::offsetToLongitudeDeg(eo) + incw*pixResX;
    endlat   = SGDem::offsetToLatitudeDeg(no)  + inch*pixResY;

    // todo : how to calculate 15- from given data
    if ( Debug1 ) {
        printf("resx is %d resy is %d incw is %d incy is %d\n", resx, resy, incw, inch  );
    }
    if ( raster ) {
        int    di, dj;
        int    p, l;
        double lonPos, latPos;
        double maxlon = startlon;
        double maxlat = startlat;

        startl = (resy-1) + overlap - ( suby * (153-3) ) + inch;
        startp = 0 + overlap + ( subx * (153-3) ) - incw;

        for ( di = 0, p = startp, lonPos = startlon; di < grid_width; di++, p+=incw, lonPos += incw*pixResX ) {
            maxlon = lonPos;

            for ( dj = 0, l = startl, latPos = startlat; dj < grid_height; dj++, l-=inch, latPos += inch*pixResY ) {
                maxlat = latPos;

                SGGeod pos = SGGeod::fromDeg( SGMiscd::normalizePeriodic( -180.0, 180.0, lonPos ), 
                                            SGMiscd::normalizePeriodic( -180.0, 180.0, latPos ) );

                if ( raster ) {
                    pos.setElevationM( raster[l*(resx+(2*overlap))+p] );
                }

                geods[di*grid_height+dj] = pos;
            }
        }

        if ( fabs( endlon - maxlon ) > 0.0001 ) {
            printf(" tile overlap error %lf : lon is %lf, startlon is %lf, endlon is %lf, maxlon is %lf. grid_width is %d, incw is %d, pixResX is %lf, (grid_width-1)*incw*pixResX is %lf\n",
                endlon-maxlon, SGDem::offsetToLongitudeDeg(wo), startlon, endlon, maxlon, grid_width, incw, pixResX, (grid_width-1)*incw*pixResX );
        }

        if ( fabs( endlat - maxlat ) > 0.0001 ) {
            printf(" tile overlap error %lf : lat is %lf, startlat is %lf, endlat is %lf, maxlat is %lf. grid_height is %d, inch is %d, pixResY is %lf, (grid_height-1)*inch*pixResY is %lf\n",
                endlat-maxlat, SGDem::offsetToLatitudeDeg(so), startlat, endlat, maxlat, grid_height, inch, pixResY, (grid_height-1)*inch*pixResY );
        }
    }
}

std::string SGDemTile::getTileName( int lon, int lat )
{
    std::stringstream ss;

    if ( lat >= 0 ) {
        ss << "N" << std::setw(2) << std::setfill('0') << lat;
    } else {
        ss << "S" << std::setw(2) << std::setfill('0') << -lat;
    }

    if ( lon >= 0 ) {
        ss << "E" << std::setw(3) << std::setfill('0') << lon;
    } else {
        ss << "W" << std::setw(3) << std::setfill('0') << -lon;
    }
    ss << ".hgt";

    printf("created tile string %s from %d,%d\n", ss.str().c_str(), lon, lat );
    return ss.str();
}
