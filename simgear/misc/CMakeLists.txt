
include (SimGearComponent)

set(HEADERS
    CSSBorder.hxx
    ListDiff.hxx
    ResourceManager.hxx
    SimpleMarkdown.hxx
    SVGpreserveAspectRatio.hxx
    argparse.hxx
    interpolator.hxx
    make_new.hxx
    sg_dir.hxx
    sg_hash.hxx
    sg_path.hxx
    stdint.hxx
    stopwatch.hxx
    strutils.hxx
    tabbed_values.hxx
    texcoord.hxx
    test_macros.hxx
    lru_cache.hxx
    simgear_optional.hxx
    )

set(SOURCES
    CSSBorder.cxx
    ResourceManager.cxx
    SimpleMarkdown.cxx
    SVGpreserveAspectRatio.cxx
    argparse.cxx
    interpolator.cxx
    sg_dir.cxx
    sg_path.cxx
    sg_hash.cxx
    strutils.cxx
    tabbed_values.cxx
    texcoord.cxx
    )

if (WINDOWS)
    list(APPEND HEADERS sgversionhelpers.hxx)
endif()

if (APPLE)
    list(APPEND SOURCES CocoaHelpers.mm)
endif()

simgear_component(misc misc "${SOURCES}" "${HEADERS}")

if(ENABLE_TESTS)

add_simgear_autotest(test_argparse argparse_test.cxx)
add_simgear_autotest(test_CSSBorder CSSBorder_test.cxx)
add_simgear_autotest(test_tabbed_values tabbed_values_test.cxx)
add_simgear_autotest(test_strutils strutils_test.cxx)
add_simgear_autotest(test_path path_test.cxx )
add_simgear_autotest(test_sg_dir sg_dir_test.cxx)

endif(ENABLE_TESTS)

add_boost_test(SimpleMarkdown
  SOURCES SimpleMarkdown_test.cxx
  LIBRARIES SimGearCore
)

add_boost_test(SVGpreserveAspectRatio
  SOURCES SVGpreserveAspectRatio_test.cxx
  LIBRARIES SimGearCore
)

add_boost_test(utf8tolatin1
  SOURCES utf8tolatin1_test.cxx
  LIBRARIES SimGearCore
)
