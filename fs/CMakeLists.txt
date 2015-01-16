cmake_minimum_required(VERSION 2.8.8)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules" ${CMAKE_MODULE_PATH})
project(custom_fs)


find_package(PkgConfig REQUIRED)

######################
#       General
######################

pkg_check_modules(LIBNOTIFY libnotify)
pkg_check_modules(FUSE fuse)

set(TARGET_NAME "bin")
set(TARGET ${TARGET_NAME})
set(SRC_DIR "src/")


set (CMAKE_C_FLAGS "-Wall -D_FILE_OFFSET_BITS=64") 

file(GLOB_RECURSE SRC_NAMES ${SRC_DIR}*.c)
add_executable(${TARGET} ${SRC_NAMES})

include_directories( 
    ${PROJECT_BINARY_DIR} 
    ${SRC_DIR}
    ${FUSE_INCLUDE_DIRS}
    ${LIBNOTIFY_INCLUDE_DIRS}

)

target_link_libraries(
    ${TARGET}
    ${FUSE_LIBRARIES}
    ${LIBNOTIFY_LIBRARIES}
)


##################
# Install target
##################

install(TARGETS ${TARGET} DESTINATION ${TARGET_NAME})
