cmake_minimum_required (VERSION 3.12)
project(lc3-test)
set (CMAKE_CXX_STANDARD 11)

# fink pkgconfig
find_package(PkgConfig REQUIRED)

# portaudio
pkg_check_modules(PORTAUDIO portaudio-2.0)
if(PORTAUDIO_FOUND)
    include_directories(${PORTAUDIO_INCLUDE_DIRS})
    link_directories(${PORTAUDIO_LIBRARY_DIRS})
    link_libraries(${PORTAUDIO_LIBRARIES})
    add_compile_definitions(HAVE_PORTAUDIO)
endif()

# lc3plus
pkg_check_modules(LC3PLUS LC3plus)
if(LC3PLUS_FOUND)
	message("HAVE_LC3PLUS")
	include_directories(${LC3PLUS_INCLUDE_DIRS})
	link_directories(${LC3PLUS_LIBRARY_DIRS})
	link_libraries(${LC3PLUS_LIBRARIES})
	add_definitions(${LC3PLUS_CFLAGS})
	add_compile_definitions(HAVE_LC3PLUS)
endif()

include_directories(../include)

file(GLOB SOURCES "*.c" "../src/*.c")
add_executable(plc_test ${SOURCES} )
