
add_executable(test
    test.cpp
    ClassParserTest.cpp
    ParamParserTest.cpp
    MapParserTest.cpp
    VectorParserTest.cpp
)

target_link_libraries(test ${CONAN_LIBS})
target_include_directories(test PUBLIC "${CMAKE_SOURCE_DIR}/src")

