cmake_minimum_required( VERSION 2.8 )
project( ffmpegtutorial )

if( NOT CMAKE_MODULE_PATH )
  set( CMAKE_MODULE_PATH /Users/matt/videosurf/projects/cmake/Modules )
  message( STATUS "CMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}" )
endif()

#set( Boost_DEBUG              ON  )
set( Boost_USE_STATIC_LIBS    ON  )
set( Boost_USE_MULTITHREADED  ON  )
set( Boost_USE_STATIC_RUNTIME OFF )

find_package( Boost 1.42.0 REQUIRED thread date_time filesystem system )
find_package( SDL          REQUIRED )
find_package( FFMPEG       REQUIRED )
find_package( LOG4CXX      REQUIRED )

include_directories( ${Boost_INCLUDE_DIRS} ${FFMPEG_INCLUDE_DIR} ${LOG4CXX_INCLUDE_DIR} )

configure_file (
  "${PROJECT_SOURCE_DIR}/log4j.properties"
  "${PROJECT_BINARY_DIR}/log4j.properties"
  COPYONLY
  )

add_subdirectory( src )
