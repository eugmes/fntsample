cmake_minimum_required(VERSION 3.12..3.21)

project(fntsample
  VERSION 5.4
  DESCRIPTION "PDF and PostScript font samples generator"
  HOMEPAGE_URL "https://github.com/eugmes/fntsample"
  LANGUAGES C)

set(CMAKE_C_STANDARD 99 CACHE STRING "The C standard to use")
set(CMAKE_C_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

include(GNUInstallDirs)
include(CPack)

find_package(PkgConfig REQUIRED)
find_package(Intl REQUIRED)

# The target was added in CMake 3.20.
if(NOT TARGET Intl::Intl)
  add_library(Intl::Intl INTERFACE IMPORTED GLOBAL)
  target_include_directories(Intl::Intl INTERFACE ${Intl_INCLUDE_DIRS})
  target_link_libraries(Intl::Intl INTERFACE ${Intl_LIBRARIES})
endif()

pkg_check_modules(pkgs REQUIRED IMPORTED_TARGET
  cairo>=1.15.4
  fontconfig
  freetype2
  glib-2.0
  pangocairo>=1.37.0
  pangoft2>=1.37.0
)

include(DownloadUnicodeBlocks)

download_unicode_blocks()

string(TIMESTAMP DATE "%Y-%m-%d" UTC)

include(PoFileUtils)

add_subdirectory(src)
add_subdirectory(scripts)
add_subdirectory(po)
