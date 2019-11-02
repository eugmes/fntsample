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

set(DEFAULT_UNICODE_BLOCKS_URL "https://unicode.org/Public/UNIDATA/Blocks.txt")
set(UNICODE_BLOCKS_URL "${DEFAULT_UNICODE_BLOCKS_URL}"
    CACHE STRING "Unicode blocks file download URL")

set(UNICODE_BLOCKS UNICODE_BLOCKS-NOTFOUND CACHE FILEPATH "Unicode blocks file")

if(NOT UNICODE_BLOCKS)
  set(DOWNLOAD_DEST "${CMAKE_CURRENT_BINARY_DIR}/Blocks.txt")
  message(STATUS "Downloading ${UNICODE_BLOCKS_URL}...")
  file(DOWNLOAD
       "${UNICODE_BLOCKS_URL}"
       "${DOWNLOAD_DEST}"
       SHOW_PROGRESS
       STATUS status
       TLS_VERIFY ON)

  list(GET status 0 err)

  if(err)
    list(GET status 1 msg)
    message(FATAL_ERROR "Download failed (${err}): ${msg}")
  endif()

  set(UNICODE_BLOCKS "${DOWNLOAD_DEST}"
      CACHE FILEPATH "Unicode blocks file"
      FORCE)
endif()

if(NOT EXISTS ${UNICODE_BLOCKS})
  message(FATAL_ERROR "Unicode blocks file not found. "
          "Use -DUNICODE_BLOCKS=<path> or -DUNICODE_BLOCKS_URL=<url> to specify location of this file.\n"
          "Blocks.txt file is available at the Unicode web site: ${DEFAULT_UNICODE_BLOCKS_URL}")
endif()

string(TIMESTAMP DATE "%Y-%m-%d" UTC)

add_subdirectory(src)
add_subdirectory(scripts)
add_subdirectory(po)
