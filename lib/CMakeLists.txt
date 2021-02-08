
add_library(pqrs STATIC
        binarization.cpp
        ppm.cpp
        grayscale.cpp
        linear_external_contours.cpp
        contour_container.cpp
        interpolation.cpp
        contour_edge_intensity.cpp
        contour_to_tetragon.cpp
        finder_pattern_detector.cpp
        qr_decoder.cpp
        homography_dlt.cpp
        qr_scanner.cpp
        qr_version_info.cpp
        qr_bitstream_reader.cpp)
target_link_libraries(pqrs PUBLIC xtensor sul::dynamic_bitset)
target_include_directories(pqrs PUBLIC include)

if (${CMAKE_BUILD_TYPE} EQUAL "DEBUG")
    target_compile_definitions(pqrs PUBLIC -DXTENSOR_ENABLE_ASSERT -DXTENSOR_ENABLE_CHECK_DIMENSION)
endif()
