cmake_minimum_required(VERSION 3.16)
project(qthsharp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Qt6 does its own moc/uic/rcc — we don't declare any Q_OBJECT classes
# with custom signals here, so AUTOMOC isn't strictly required, but it's
# harmless to leave on for anyone who extends src/qthsharp.cpp later.
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_library(qthsharp STATIC
    src/qthsharp.cpp
)

target_include_directories(qthsharp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(qthsharp PUBLIC Qt6::Widgets)

# `extern static [c, "qthsharp"]` in src/qt.h# expects `libqthsharp.a`
# to sit directly under native/ (see native/build.sh), so drop the
# built archive there instead of leaving it in build/.
set_target_properties(qthsharp PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS qthsharp ARCHIVE DESTINATION lib)
install(FILES include/qthsharp.h DESTINATION include)
