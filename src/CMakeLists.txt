add_executable( tutorial01 tutorial01.cc )
target_link_libraries( tutorial01 ${Boost_LIBRARIES} ${FFMPEG_LIBRARIES} ${LOG4CXX_LIBRARIES} )
