cmake_minimum_required(VERSION 3.16)

project(SnipText VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        selectionoverlay.cpp
        selectionoverlay.h
        capturesession.cpp
        capturesession.h
        ocrservice.cpp
        ocrservice.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(SnipText
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET SnipText APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(SnipText SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(SnipText
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(SnipText PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# ------------------------------------------------------------
# Tesseract/Leptonica detection (macOS-focused for now)
set(TESSERACT_ROOT "" CACHE PATH "Prefix where Tesseract is installed")

set(_TESS_SEARCH_PATHS
    ${TESSERACT_ROOT}
    /opt/homebrew
    /usr/local
    /opt/local
)

list(REMOVE_DUPLICATES _TESS_SEARCH_PATHS)

find_path(TESSERACT_INCLUDE_DIR
    NAMES tesseract/baseapi.h
    PATH_SUFFIXES include
    PATHS ${_TESS_SEARCH_PATHS}
)

find_library(TESSERACT_LIB
    NAMES tesseract
    PATH_SUFFIXES lib
    PATHS ${_TESS_SEARCH_PATHS}
)

find_library(LEPTONICA_LIB
    NAMES lept leptonica
    PATH_SUFFIXES lib
    PATHS ${_TESS_SEARCH_PATHS}
)

if(NOT TESSERACT_INCLUDE_DIR OR NOT TESSERACT_LIB OR NOT LEPTONICA_LIB)
    message(FATAL_ERROR
        "Could not find Tesseract/Leptonica. Install via Homebrew (brew install tesseract) "
        "or set TESSERACT_ROOT to the installation prefix.")
endif()

target_include_directories(SnipText PRIVATE "${TESSERACT_INCLUDE_DIR}")
target_link_libraries(SnipText PRIVATE "${TESSERACT_LIB}" "${LEPTONICA_LIB}")

get_filename_component(_TESS_LIB_DIR "${TESSERACT_LIB}" DIRECTORY)
get_filename_component(_LEPT_LIB_DIR "${LEPTONICA_LIB}" DIRECTORY)
set(_OCR_RPATHS "${_TESS_LIB_DIR}" "${_LEPT_LIB_DIR}")
list(REMOVE_DUPLICATES _OCR_RPATHS)

set_target_properties(SnipText PROPERTIES
    BUILD_RPATH   "${_OCR_RPATHS}"
    INSTALL_RPATH "${_OCR_RPATHS}"
)
# ------------------------------------------------------------

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.SnipText)
endif()
set_target_properties(SnipText PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS SnipText
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(SnipText)
endif()
