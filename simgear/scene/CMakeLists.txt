include (SimGearComponent)

include_directories(${PROJECT_SOURCE_DIR})

foreach( mylibfolder 
        material
        model
        sky
        tgdb
        util
        tsync
        viewer
    )

    add_subdirectory(${mylibfolder})

endforeach( mylibfolder )

if(ENABLE_GDAL)
    add_subdirectory(dem)
endif(ENABLE_GDAL)
