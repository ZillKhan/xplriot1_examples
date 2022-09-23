# Include ubxlib
include($ENV{UBXLIB_DIR}/zephyr/ubxlib.cmake)
# Add XPLR-IOT-1 specifics, remove this for other boards
include(${CMAKE_CURRENT_LIST_DIR}/xplriot1/xplriot1.cmake)
# And Zephyr
find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

file(GLOB_RECURSE APP_SOURCES ${CMAKE_SOURCE_DIR} "src/*.c")
target_sources(app PRIVATE ${APP_SOURCES})
