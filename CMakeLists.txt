module_version(1.1)
 
# Download demo data
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/rabbit.tif)
	message(STATUS "Downloading volrend demo data...")
    file(DOWNLOAD http://omegalib.s3-website-us-west-2.amazonaws.com/volrend/rabbit.tif ${CMAKE_CURRENT_SOURCE_DIR}/rabbit.tif SHOW_PROGRESS)
endif()


add_definitions(-DFREEIMAGE_LIB)
add_library(stress_tensor MODULE
	volrend.cpp
    VolumeBlock.cpp
    VolumeBlock.h)

target_link_libraries(stress_tensor omega FreeImage)
declare_native_module(stress_tensor)
