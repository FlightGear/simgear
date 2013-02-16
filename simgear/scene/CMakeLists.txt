include (SimGearComponent)

include_directories(${PROJECT_SOURCE_DIR})

foreach( mylibfolder 
        material
        model
        sky
        tgdb
        util
        tsync
    )

    add_subdirectory(${mylibfolder})

endforeach( mylibfolder )

