include (SimGearComponent)

include_directories(${PROJECT_SOURCE_DIR})

foreach( mylibfolder 
        bvh
        material
        model
        sky
        tgdb
        util
    )

    add_subdirectory(${mylibfolder})

endforeach( mylibfolder )

