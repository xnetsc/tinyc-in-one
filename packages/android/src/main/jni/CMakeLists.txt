# Builds the Android variant of the dual-form file: libtcc kernel +
# tinyc_jni.c in one libtinyc.so (spec 7.4: the JNI layer exists only
# in the Android variant). The kernel build drives this file; the AAR
# itself packages the resulting .so from src/main/jniLibs/<abi>/.
#
# Expected inputs (set by the kernel build):
#   TINYC_LIBTCC  - path to the Android-target libtcc built from the
#                   dual-form kernel tree (static or shared)
#   LIBTCC_INCLUDE - directory containing libtcc.h
cmake_minimum_required(VERSION 3.22)
project(tinyc_android C)

if(NOT DEFINED TINYC_LIBTCC OR NOT DEFINED LIBTCC_INCLUDE)
    message(FATAL_ERROR
        "Set -DTINYC_LIBTCC=<libtcc for the Android target> and "
        "-DLIBTCC_INCLUDE=<dir with libtcc.h>; both come from the "
        "dual-form kernel build for the same Android ABI.")
endif()

add_library(tinyc SHARED tinyc_jni.c)
target_include_directories(tinyc PRIVATE ${LIBTCC_INCLUDE} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tinyc PRIVATE ${TINYC_LIBTCC} log)
