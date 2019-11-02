cmake_minimum_required(VERSION 3.12)

project(fntsample
  VERSION 5.3
  DESCRIPTION "PDF and PostScript font samples generator"
  HOMEPAGE_URL "https://github.com/eugmes/fntsample"
  LANGUAGES C)

include(GNUInstallDirs)
include(CPack)

find_package(PkgConfig REQUIRED)
find_package(Intl REQUIRED)
find_package(Gettext REQUIRED)
find_package(Iconv REQUIRED)

# CMake does not create an imported target for libintl...
add_library(Intl INTERFACE IMPORTED GLOBAL)
target_include_directories(Intl INTERFACE ${Intl_INCLUDE_DIRS})
target_link_libraries(Intl INTERFACE ${Intl_LIBRARIES})

pkg_check_modules(pkgs REQUIRED IMPORTED_TARGET
  cairo
  fontconfig
  freetype2
  glib-2.0
  pangocairo>=1.37.0
  pangoft2>=1.37.0
)

find_program(AWK NAMES gawk awk mawk)
if(NOT AWK)
  message(FATAL_ERROR "awk is required to build the program but was not found.")
endif()

include(CMake/DownloadUnicodeBlocks.cmake)
download_unicode_blocks()

string(TIMESTAMP DATE "%Y-%m-%d" UTC)

add_subdirectory(src)
add_subdirectory(scripts)
add_subdirectory(po)
