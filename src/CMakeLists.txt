cmake_minimum_required(VERSION 3.5)

project(argon2ui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

set(${PROJECT_NAME}_ARGON2_IMPL ${CMAKE_CURRENT_LIST_DIR}/../lib/argon2/src/ref.c)

if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    set(${PROJECT_NAME}_ARGON2_IMPL ${CMAKE_CURRENT_LIST_DIR}/../lib/argon2/src/opt.c)
endif ()

message(STATUS "Argon2 implementation used: ${${PROJECT_NAME}_ARGON2_IMPL}")

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        ${CMAKE_CURRENT_LIST_DIR}/../lib/argon2/src/blake2/blake2b.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/argon2/src/argon2.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/argon2/src/core.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/argon2/src/encoding.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/argon2/src/thread.c
        ${CMAKE_CURRENT_LIST_DIR}/../lib/argon2/include/argon2.h
        ${${PROJECT_NAME}_ARGON2_IMPL}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(argon2ui
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET argon2ui APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(argon2ui SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(argon2ui
            ${PROJECT_SOURCES}
        )
    endif()
endif()

if (WIN32)
    add_compile_definitions("_CRT_SECURE_NO_WARNINGS=1")
    target_link_libraries(${PROJECT_NAME} PUBLIC bcrypt)
endif ()

target_link_libraries(argon2ui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

target_include_directories(argon2ui PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../lib/argon2/include)

set_target_properties(argon2ui PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER argon2ui.glitchedpolygons.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(argon2ui)
endif()
