
add_executable(flat_struct
    flat_struct.cpp
)

target_link_libraries(flat_struct ${CONAN_LIBS})
target_include_directories(flat_struct PUBLIC "${CMAKE_SOURCE_DIR}/src")


add_executable(map_struct
    map_struct.cpp
)

target_link_libraries(map_struct ${CONAN_LIBS})
target_include_directories(map_struct PUBLIC "${CMAKE_SOURCE_DIR}/src")

add_executable(vector_struct
    vector_struct.cpp
)

target_link_libraries(vector_struct ${CONAN_LIBS})
target_include_directories(vector_struct PUBLIC "${CMAKE_SOURCE_DIR}/src")


add_executable(custom_param
    custom_param.cpp
)

target_link_libraries(custom_param ${CONAN_LIBS})
target_include_directories(custom_param PUBLIC "${CMAKE_SOURCE_DIR}/src")
