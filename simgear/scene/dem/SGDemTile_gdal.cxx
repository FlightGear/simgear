// dem_gdal.cxx -- perform gdal warp - based on gdalwarp.cpp

/******************************************************************************
 * $Id: gdalwarp.cpp 29153 2015-05-04 17:51:41Z rouault $
 *
 * Project:  High Performance Image Reprojector
 * Purpose:  Test program for high performance warper API.
 * Author:   Frank Warmerdam <warmerdam@pobox.com>
 *
 ******************************************************************************
 * Copyright (c) 2002, i3 - information integration and imaging
 *                          Fort Collin, CO
 * Copyright (c) 2007-2013, Even Rouault <even dot rouault at mines-paris dot org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/
#include <iostream>
#include <iomanip>
#include <fstream>

#include <cpl_conv.h> // for CPLMalloc()
#include "ogr_spatialref.h"

#include <gdal.h>
#include <gdalwarper.h>
#include <gdal_priv.h>

#include <simgear/scene/dem/SGDem.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/stdint.hxx>
#include <simgear/misc/sg_dir.hxx>

using namespace simgear;

int GDALExit( int nCode )
{
    GDALDumpOpenDatasets( stderr );
    CPLDumpSharedList( NULL );

    GDALDestroyDriverManager();
    OGRCleanupAll();

    exit( nCode );
}

GDALDatasetH SGDemTile::createTile( char **papszSrcFiles, const char *pszFilename,
                                    int nForceLines, int nForcePixels,
                                    double dfMinX, double dfMinY,
                                    double dfMaxX, double dfMaxY,
                                    char **papszTO )
{
    GDALDriverH  	hDriver = NULL;
    GDALDatasetH 	hDstDS;
    GDALColorTableH 	hCT = NULL;
    double dfWrkMinX=0, dfWrkMaxX=0, dfWrkMinY=0, dfWrkMaxY=0;
    double  dfXRes=0.0, dfYRes=0.0;
    int nDstBandCount = 0;
    std::vector<GDALColorInterp> apeColorInterpretations;

    /* -------------------------------------------------------------------- */
    /*      Find the output driver.                                         */
    /* -------------------------------------------------------------------- */
    hDriver = GDALGetDriverByName( "GTiff" );
    GDALDataType eDT = GDT_Unknown;
    if( hDriver == NULL || GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATE, NULL ) == NULL )
    {
        int	iDr;

        printf( "Output driver 'GTiff' not recognised or does not support\n" );
        printf( "direct output file creation.  The following format drivers are configured\n"
                "and support direct output:\n" );

        for( iDr = 0; iDr < GDALGetDriverCount(); iDr++ )
        {
            GDALDriverH hDriver = GDALGetDriver(iDr);

            if( GDALGetMetadataItem( hDriver, GDAL_DCAP_RASTER, NULL) != NULL &&
                GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATE, NULL) != NULL )
            {
                printf( "  %s: %s\n",
                        GDALGetDriverShortName( hDriver  ),
                        GDALGetDriverLongName( hDriver ) );
            }
        }
        printf( "\n" );
    }

    char    *pszThisTargetSRS = (char*)CSLFetchNameValue( papszTO, "DST_SRS" );
    if( pszThisTargetSRS != NULL ) {
        pszThisTargetSRS = CPLStrdup( pszThisTargetSRS );
    } else {
        printf("GDALWarpCreateOutput: pszThisTargetSRS is NULL\n");
    }

    /* -------------------------------------------------------------------- */
    /*      Loop over all input files to collect extents.                   */
    /* -------------------------------------------------------------------- */
    for( int iSrc = 0; papszSrcFiles[iSrc] != NULL; iSrc++ )
    {
        GDALDatasetH hSrcDS;
        const char *pszThisSourceSRS = CSLFetchNameValue(papszTO,"SRC_SRS");

        hSrcDS = GDALOpenEx( papszSrcFiles[iSrc], GDAL_OF_RASTER, NULL, NULL, NULL );
        if( hSrcDS == NULL )
            GDALExit( 1 );

        /* -------------------------------------------------------------------- */
        /*      Check that there's at least one raster band                     */
        /* -------------------------------------------------------------------- */
        if ( GDALGetRasterCount(hSrcDS) == 0 )
        {
            fprintf(stderr, "Input file %s has no raster bands.\n", papszSrcFiles[iSrc] );
            GDALExit( 1 );
        }

        if( eDT == GDT_Unknown ) {
            eDT = GDALGetRasterDataType(GDALGetRasterBand(hSrcDS,1));
        }

        /* -------------------------------------------------------------------- */
        /*      If we are processing the first file, and it has a color         */
        /*      table, then we will copy it to the destination file.            */
        /* -------------------------------------------------------------------- */
        if( iSrc == 0 )
        {
            nDstBandCount = GDALGetRasterCount(hSrcDS);
            hCT = GDALGetRasterColorTable( GDALGetRasterBand(hSrcDS,1) );
            if( hCT != NULL )
            {
                hCT = GDALCloneColorTable( hCT );
                printf( "Copying color table from %s to new file.\n", papszSrcFiles[iSrc] );
            }

            for(int iBand = 0; iBand < nDstBandCount; iBand++)
            {
                apeColorInterpretations.push_back(
                    GDALGetRasterColorInterpretation(GDALGetRasterBand(hSrcDS,iBand+1)) );
            }
        }

        /* -------------------------------------------------------------------- */
        /*      Get the sourcesrs from the dataset, if not set already.         */
        /* -------------------------------------------------------------------- */
        if( pszThisSourceSRS == NULL )
        {
            const char *pszMethod = CSLFetchNameValue( papszTO, "METHOD" );

            if( GDALGetProjectionRef( hSrcDS ) != NULL
                && strlen(GDALGetProjectionRef( hSrcDS )) > 0
                && (pszMethod == NULL || EQUAL(pszMethod,"GEOTRANSFORM")) ) {

                pszThisSourceSRS = GDALGetProjectionRef( hSrcDS );
            } else if( GDALGetGCPProjection( hSrcDS ) != NULL
                     && strlen(GDALGetGCPProjection(hSrcDS)) > 0
                     && GDALGetGCPCount( hSrcDS ) > 1
                     && (pszMethod == NULL || EQUALN(pszMethod,"GCP_",4)) ) {
                pszThisSourceSRS = GDALGetGCPProjection( hSrcDS );
            } else if( pszMethod != NULL && EQUAL(pszMethod,"RPC") ) {
#ifndef SRS_WKT_WGS84
                pszThisSourceSRS = SRS_WKT_WGS84_LAT_LONG;
#else
                pszThisSourceSRS = SRS_WKT_WGS84;
#endif
            } else {
                pszThisSourceSRS = "";
            }
        }

        if( pszThisTargetSRS == NULL ) {
            pszThisTargetSRS = CPLStrdup( pszThisSourceSRS );
        }

        GDALClose( hSrcDS );
    }

    /* -------------------------------------------------------------------- */
    /*      Did we have any usable sources?                                 */
    /* -------------------------------------------------------------------- */
    if( nDstBandCount == 0 )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "No usable source images." );
        CPLFree( pszThisTargetSRS );
        return NULL;
    }

    /* -------------------------------------------------------------------- */
    /*      Turn the suggested region into a geotransform and suggested     */
    /*      number of pixels and lines.                                     */
    /* -------------------------------------------------------------------- */
    double adfDstGeoTransform[6] = { 0, 0, 0, 0, 0, 0 };
    int nPixels = 0, nLines = 0;

    /* -------------------------------------------------------------------- */
    /*      Did the user override some parameters?                          */
    /* -------------------------------------------------------------------- */
    if( nForcePixels != 0 && nForceLines != 0 )
    {
        if( dfMinX == 0.0 && dfMinY == 0.0 && dfMaxX == 0.0 && dfMaxY == 0.0 )
        {
            dfMinX = dfWrkMinX;
            dfMaxX = dfWrkMaxX;
            dfMaxY = dfWrkMaxY;
            dfMinY = dfWrkMinY;
        }

        dfXRes = (dfMaxX - dfMinX) / nForcePixels;
        dfYRes = (dfMaxY - dfMinY) / nForceLines;

        adfDstGeoTransform[0] = dfMinX;
        adfDstGeoTransform[3] = dfMaxY;
        adfDstGeoTransform[1] = dfXRes;
        adfDstGeoTransform[5] = -dfYRes;

        nPixels = nForcePixels;
        nLines = nForceLines;
    }
    else
    {
        fprintf(stderr, "UHOH - need force pixels/lines\n");
    }

    /* -------------------------------------------------------------------- */
    /*      Create the output file.                                         */
    /* -------------------------------------------------------------------- */
    char** papszCreateOptions = CSLAddString( NULL, "COMPRESS=DEFLATE" );
    hDstDS = GDALCreate( hDriver, pszFilename, nPixels, nLines,
                         nDstBandCount, eDT, papszCreateOptions );

    CSLDestroy( papszCreateOptions );
    papszCreateOptions = NULL;

    if( hDstDS == NULL )
    {
        printf( "Error creating file - return NULL\n" );
        CPLFree( pszThisTargetSRS );
        return NULL;
    }

    /* -------------------------------------------------------------------- */
    /*      Write out the projection definition.                            */
    /* -------------------------------------------------------------------- */
    const char *pszDstMethod = CSLFetchNameValue(papszTO,"DST_METHOD");
    if( pszDstMethod == NULL || !EQUAL(pszDstMethod, "NO_GEOTRANSFORM") )
    {
        if( GDALSetProjection( hDstDS, pszThisTargetSRS ) == CE_Failure ||
            GDALSetGeoTransform( hDstDS, adfDstGeoTransform ) == CE_Failure )
        {
            printf( "Set projection of hDstDS - error\n" );
            CPLFree( pszThisTargetSRS );
            return NULL;
        }
    }
    else
    {
        adfDstGeoTransform[0] = 0.0;
        adfDstGeoTransform[3] = 0.0;
        adfDstGeoTransform[5] = fabs(adfDstGeoTransform[5]);
    }

    /* -------------------------------------------------------------------- */
    /*      Copy the color table, if required.                              */
    /* -------------------------------------------------------------------- */
    if( hCT != NULL )
    {
        printf( "copy color table\n" );
        GDALSetRasterColorTable( GDALGetRasterBand(hDstDS,1), hCT );
        GDALDestroyColorTable( hCT );
    }

    printf( "free targetSRS\n" );
    CPLFree( pszThisTargetSRS );

    printf( "GDALWarpCreateOutput complete\n" );

    return hDstDS;
}

void SGDemTile::doWarp( int iSrc, char* pszSrcFile, GDALDatasetH hDstDS, char** papszTO, char** papszWarpOptions )
{
    GDALDatasetH hSrcDS;
    GDALResampleAlg eResampleAlg = GRA_NearestNeighbour;
    int     bEnableDstAlpha = FALSE, bEnableSrcAlpha = FALSE;
    GDALDataType    eWorkingType = GDT_Unknown;
    void* hTransformArg = NULL;
    GDALTransformerFunc pfnTransformer = NULL;
    double             dfErrorThreshold = 0.125;

    /* -------------------------------------------------------------------- */
    /*      Open this file.                                                 */
    /* -------------------------------------------------------------------- */
    hSrcDS = GDALOpenEx( pszSrcFile, GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR, NULL, NULL, NULL );
    if( hSrcDS == NULL )
        GDALExit( 2 );

    /* -------------------------------------------------------------------- */
    /*      Check that there's at least one raster band                     */
    /* -------------------------------------------------------------------- */
    if ( GDALGetRasterCount(hSrcDS) == 0 )
    {
        fprintf(stderr, "Input file %s has no raster bands.\n", pszSrcFile );
        GDALExit( 1 );
    }

    printf( "Processing input file %s.\n", pszSrcFile );

#if 0 // do we need metadata?
    /* -------------------------------------------------------------------- */
    /*      Get the metadata of the first source DS and copy it to the      */
    /*      destination DS. Copy Band-level metadata and other info, only   */
    /*      if source and destination band count are equal. Any values that */
    /*      conflict between source datasets are set to pszMDConflictValue. */
    /* -------------------------------------------------------------------- */
    if ( true )
    {
        char **papszMetadata = NULL;
        const char *pszSrcInfo = NULL;
        const char *pszDstInfo = NULL;
        GDALRasterBandH hSrcBand = NULL;
        GDALRasterBandH hDstBand = NULL;

        /* copy metadata from first dataset */
        if ( iSrc == 0 )
        {
            CPLDebug("WARP", "Copying metadata from first source to destination dataset");
            /* copy dataset-level metadata */
            papszMetadata = GDALGetMetadata( hSrcDS, NULL );

            char** papszMetadataNew = NULL;
            for( int i = 0; papszMetadata != NULL && papszMetadata[i] != NULL; i++ )
            {
                // Do not preserve NODATA_VALUES when the output includes an alpha band
                if( bEnableDstAlpha &&
                    EQUALN(papszMetadata[i], "NODATA_VALUES=", strlen("NODATA_VALUES=")) )
                {
                    continue;
                }

                papszMetadataNew = CSLAddString(papszMetadataNew, papszMetadata[i]);
            }

            if ( CSLCount(papszMetadataNew) > 0 ) {
                if ( GDALSetMetadata( hDstDS, papszMetadataNew, NULL ) != CE_None )
                    fprintf( stderr, "Warning: error copying metadata to destination dataset.\n" );
            }

            CSLDestroy(papszMetadataNew);

            /* copy band-level metadata and other info */
            if ( GDALGetRasterCount( hSrcDS ) == GDALGetRasterCount( hDstDS ) )
            {
                for ( int iBand = 0; iBand < GDALGetRasterCount( hSrcDS ); iBand++ )
                {
                    hSrcBand = GDALGetRasterBand( hSrcDS, iBand + 1 );
                    hDstBand = GDALGetRasterBand( hDstDS, iBand + 1 );
                    /* copy metadata, except stats (#5319) */
                    papszMetadata = GDALGetMetadata( hSrcBand, NULL);
                    if ( CSLCount(papszMetadata) > 0 )
                    {
                        //GDALSetMetadata( hDstBand, papszMetadata, NULL );
                        char** papszMetadataNew = NULL;
                        for( int i = 0; papszMetadata != NULL && papszMetadata[i] != NULL; i++ )
                        {
                            if (strncmp(papszMetadata[i], "STATISTICS_", 11) != 0)
                                papszMetadataNew = CSLAddString(papszMetadataNew, papszMetadata[i]);
                        }
                        GDALSetMetadata( hDstBand, papszMetadataNew, NULL );
                        CSLDestroy(papszMetadataNew);
                    }
                    /* copy other info (Description, Unit Type) - what else? */
                    if ( bCopyBandInfo ) {
                        pszSrcInfo = GDALGetDescription( hSrcBand );
                        if(  pszSrcInfo != NULL && strlen(pszSrcInfo) > 0 )
                            GDALSetDescription( hDstBand, pszSrcInfo );
                        pszSrcInfo = GDALGetRasterUnitType( hSrcBand );
                        if(  pszSrcInfo != NULL && strlen(pszSrcInfo) > 0 )
                            GDALSetRasterUnitType( hDstBand, pszSrcInfo );
                    }
                }
            }
        }
        /* remove metadata that conflicts between datasets */
        else
        {
            CPLDebug("WARP", "Removing conflicting metadata from destination dataset (source #%d)", iSrc );
            /* remove conflicting dataset-level metadata */
            RemoveConflictingMetadata( hDstDS, GDALGetMetadata( hSrcDS, NULL ), pszMDConflictValue );

            /* remove conflicting copy band-level metadata and other info */
            if ( GDALGetRasterCount( hSrcDS ) == GDALGetRasterCount( hDstDS ) )
            {
                for ( int iBand = 0; iBand < GDALGetRasterCount( hSrcDS ); iBand++ )
                {
                    hSrcBand = GDALGetRasterBand( hSrcDS, iBand + 1 );
                    hDstBand = GDALGetRasterBand( hDstDS, iBand + 1 );
                    /* remove conflicting metadata */
                    RemoveConflictingMetadata( hDstBand, GDALGetMetadata( hSrcBand, NULL ), pszMDConflictValue );
                    /* remove conflicting info */
                    if ( bCopyBandInfo ) {
                        pszSrcInfo = GDALGetDescription( hSrcBand );
                        pszDstInfo = GDALGetDescription( hDstBand );
                        if( ! ( pszSrcInfo != NULL && strlen(pszSrcInfo) > 0  &&
                                pszDstInfo != NULL && strlen(pszDstInfo) > 0  &&
                                EQUAL( pszSrcInfo, pszDstInfo ) ) )
                            GDALSetDescription( hDstBand, "" );
                        pszSrcInfo = GDALGetRasterUnitType( hSrcBand );
                        pszDstInfo = GDALGetRasterUnitType( hDstBand );
                        if( ! ( pszSrcInfo != NULL && strlen(pszSrcInfo) > 0  &&
                                pszDstInfo != NULL && strlen(pszDstInfo) > 0  &&
                                EQUAL( pszSrcInfo, pszDstInfo ) ) )
                            GDALSetRasterUnitType( hDstBand, "" );
                    }
                }
            }
        }
    }
#endif

    /* -------------------------------------------------------------------- */
    /*      Warns if the file has a color table and something more          */
    /*      complicated than nearest neighbour resampling is asked          */
    /* -------------------------------------------------------------------- */
    if ( eResampleAlg != GRA_NearestNeighbour && eResampleAlg != GRA_Mode &&
        GDALGetRasterColorTable(GDALGetRasterBand(hSrcDS, 1)) != NULL)
    {
        fprintf( stderr, "Warning: Input file %s has a color table, which will likely lead to "
                "bad results when using a resampling method other than "
                "nearest neighbour or mode. Converting the dataset prior to 24/32 bit "
                "is advised.\n", pszSrcFile );
    }

    /* -------------------------------------------------------------------- */
    /*      Do we have a source alpha band?                                 */
    /* -------------------------------------------------------------------- */
    if( GDALGetRasterColorInterpretation( GDALGetRasterBand(hSrcDS,GDALGetRasterCount(hSrcDS)) ) == GCI_AlphaBand && !bEnableSrcAlpha )
    {
        printf( "SHOULD NOT HAPPEN Using band %d of source image as alpha.\n", GDALGetRasterCount(hSrcDS) );
        bEnableSrcAlpha = TRUE;
    }

    /* -------------------------------------------------------------------- */
    /*      Create a transformation object from the source to               */
    /*      destination coordinate system.                                  */
    /* -------------------------------------------------------------------- */
    hTransformArg = GDALCreateGenImgProjTransformer2( hSrcDS, hDstDS, papszTO );
    if( hTransformArg == NULL ) {
        printf( "SHOULD NOT HAPPEN hTransformArg is NULL\n" );
        GDALExit( 1 );
    }

    /* -------------------------------------------------------------------- */
    /*      Warp the transformer with a linear approximator                 */
    /* -------------------------------------------------------------------- */
    hTransformArg = GDALCreateApproxTransformer( GDALGenImgProjTransform,
                                                 hTransformArg, dfErrorThreshold);
    pfnTransformer = GDALApproxTransform;
    GDALApproxTransformerOwnsSubtransformer(hTransformArg, TRUE);

    /* -------------------------------------------------------------------- */
    /*      Clear temporary INIT_DEST settings after the first image.       */
    /* -------------------------------------------------------------------- */
    if( iSrc == 1 )
        papszWarpOptions = CSLSetNameValue( papszWarpOptions,
                                            "INIT_DEST", NULL );

    /* -------------------------------------------------------------------- */
    /*      Setup warp options.                                             */
    /* -------------------------------------------------------------------- */
    GDALWarpOptions *psWO = GDALCreateWarpOptions();

    psWO->papszWarpOptions = CSLDuplicate(papszWarpOptions);
    psWO->eWorkingDataType = eWorkingType;
    psWO->eResampleAlg = eResampleAlg;

    psWO->hSrcDS = hSrcDS;
    psWO->hDstDS = hDstDS;

    psWO->pfnTransformer = pfnTransformer;
    psWO->pTransformerArg = hTransformArg;

    psWO->pfnProgress = GDALTermProgress;
    //psWO->pfnProgress = NULL;

    /* -------------------------------------------------------------------- */
    /*      Setup band mapping.                                             */
    /* -------------------------------------------------------------------- */
    if( bEnableSrcAlpha )
        psWO->nBandCount = GDALGetRasterCount(hSrcDS) - 1;
    else
        psWO->nBandCount = GDALGetRasterCount(hSrcDS);

    psWO->panSrcBands = (int *) CPLMalloc(psWO->nBandCount*sizeof(int));
    psWO->panDstBands = (int *) CPLMalloc(psWO->nBandCount*sizeof(int));

    for( int i = 0; i < psWO->nBandCount; i++ )
    {
        psWO->panSrcBands[i] = i+1;
        psWO->panDstBands[i] = i+1;
    }

    /* -------------------------------------------------------------------- */
    /*      Setup alpha bands used if any.                                  */
    /* -------------------------------------------------------------------- */
    if( bEnableSrcAlpha )
        psWO->nSrcAlphaBand = GDALGetRasterCount(hSrcDS);

    if( !bEnableDstAlpha
        && GDALGetRasterCount(hDstDS) == psWO->nBandCount+1
        && GDALGetRasterColorInterpretation(
            GDALGetRasterBand(hDstDS,GDALGetRasterCount(hDstDS)))
        == GCI_AlphaBand )
    {
        printf( "Using band %d of destination image as alpha.\n",
                GDALGetRasterCount(hDstDS) );

        bEnableDstAlpha = TRUE;
    }

    if( bEnableDstAlpha )
        psWO->nDstAlphaBand = GDALGetRasterCount(hDstDS);

    int bHaveNodata = FALSE;
    double dfReal = 0.0;

    for( int i = 0; !bHaveNodata && i < psWO->nBandCount; i++ )
    {
        GDALRasterBandH hBand = GDALGetRasterBand( hSrcDS, i+1 );
        dfReal = GDALGetRasterNoDataValue( hBand, &bHaveNodata );
    }

    if( bHaveNodata )
    {
        if (CPLIsNan(dfReal))
            printf( "Using internal nodata values (e.g. nan) for image %s.\n",
                    pszSrcFile );
        else
            printf( "Using internal nodata values (e.g. %g) for image %s.\n",
                    dfReal, pszSrcFile );

        psWO->padfSrcNoDataReal = (double *)
            CPLMalloc(psWO->nBandCount*sizeof(double));
        psWO->padfSrcNoDataImag = (double *)
            CPLMalloc(psWO->nBandCount*sizeof(double));

        for( int i = 0; i < psWO->nBandCount; i++ )
        {
            GDALRasterBandH hBand = GDALGetRasterBand( hSrcDS, i+1 );

            dfReal = GDALGetRasterNoDataValue( hBand, &bHaveNodata );

            if( bHaveNodata )
            {
                psWO->padfSrcNoDataReal[i] = dfReal;
                psWO->padfSrcNoDataImag[i] = 0.0;
            }
            else
            {
                psWO->padfSrcNoDataReal[i] = -123456.789;
                psWO->padfSrcNoDataImag[i] = 0.0;
            }
        }
    }

    /* else try to fill dstNoData from source bands */
    if ( psWO->padfSrcNoDataReal != NULL )
    {
        psWO->padfDstNoDataReal = (double *)
            CPLMalloc(psWO->nBandCount*sizeof(double));
        psWO->padfDstNoDataImag = (double *)
            CPLMalloc(psWO->nBandCount*sizeof(double));

        printf( "Copying nodata values from source %s \n", pszSrcFile );

        for( int i = 0; i < psWO->nBandCount; i++ )
        {
            int bHaveNodata = FALSE;

            GDALRasterBandH hBand = GDALGetRasterBand( hSrcDS, i+1 );
            GDALGetRasterNoDataValue( hBand, &bHaveNodata );

            CPLDebug("WARP", "band=%d bHaveNodata=%d", i, bHaveNodata);
            if( bHaveNodata )
            {
                psWO->padfDstNoDataReal[i] = psWO->padfSrcNoDataReal[i];
                psWO->padfDstNoDataImag[i] = psWO->padfSrcNoDataImag[i];
                CPLDebug("WARP", "srcNoData=%f dstNoData=%f",
                        psWO->padfSrcNoDataReal[i], psWO->padfDstNoDataReal[i] );
            }

            CPLDebug("WARP", "calling GDALSetRasterNoDataValue() for band#%d", i );
            GDALSetRasterNoDataValue(
                GDALGetRasterBand( hDstDS, psWO->panDstBands[i] ),
                psWO->padfDstNoDataReal[i] );
        }
    }

    /* -------------------------------------------------------------------- */
    /*      Initialize and execute the warp.                                */
    /* -------------------------------------------------------------------- */
    GDALWarpOperation oWO;
    if( oWO.Initialize( psWO ) == CE_None )
    {
        oWO.ChunkAndWarpImage( 0, 0, GDALGetRasterXSize( hDstDS ), GDALGetRasterYSize( hDstDS ) );
    }

    /* -------------------------------------------------------------------- */
    /*      Cleanup                                                         */
    /* -------------------------------------------------------------------- */
    if( hTransformArg != NULL )
        GDALDestroyTransformer( hTransformArg );

    GDALDestroyWarpOptions( psWO );
    GDALClose( hSrcDS );
}
