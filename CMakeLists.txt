cmake_minimum_required(VERSION 3.12)
project(fntsample C)

include(CheckCCompilerFlag)
include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)
find_package(Intl REQUIRED)
find_package(Gettext REQUIRED)
find_package(Iconv REQUIRED)

# CMake does not create an imported target for libintl...
add_library(Intl INTERFACE IMPORTED)
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

find_file(UNICODE_BLOCKS Blocks.txt
  PATHS /usr/share/unicode
  DOC "Unicode blocks file"
  NO_DEFAULT_PATH)

if(NOT UNICODE_BLOCKS)
  message(FATAL_ERROR "Unicode blocks file (Blocks.txt) not found. \
Use -DUNICODE_BLOCKS=<path> to specify location of this file. \
Blocks.txt file is available at Unicode site: http://unicode.org/Public/UNIDATA/Blocks.txt"
)
endif()

string(TIMESTAMP DATE "%Y-%m-%d" UTC)

set(CMAKE_REQUIRED_FLAGS -Wl,--as-needed)
check_c_compiler_flag("" HAVE_AS_NEEDED) #empty because we are testing a linker flag
if(HAVE_AS_NEEDED)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
endif()

configure_file(config.h.in config.h @ONLY)
configure_file(fntsample.1.in fntsample.1 @ONLY)
configure_file(pdf-extract-outline.1.in pdf-extract-outline.1 @ONLY)
configure_file(pdfoutline.1.in pdfoutline.1 @ONLY)
configure_file(pdfoutline.pl pdfoutline @ONLY)
configure_file(pdf-extract-outline.pl pdf-extract-outline @ONLY)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/unicode_blocks.c
  COMMAND ${AWK} -f genblocks.awk ${UNICODE_BLOCKS} > ${CMAKE_CURRENT_BINARY_DIR}/unicode_blocks.c
  DEPENDS ${UNICODE_BLOCKS} genblocks.awk
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM
)

add_executable(fntsample
  fntsample.c
  read_blocks.c
  unicode_blocks.h
  ${CMAKE_CURRENT_BINARY_DIR}/unicode_blocks.c
  ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

target_compile_features(fntsample PRIVATE c_std_99)

target_include_directories(fntsample PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(fntsample PRIVATE
  m
  Intl
  Iconv::Iconv
  PkgConfig::pkgs
)

target_compile_options(fntsample PRIVATE
  -Wall -W -Wwrite-strings -Wstrict-prototypes -pedantic)

install(TARGETS fntsample DESTINATION ${CMAKE_INSTALL_BINDIR})

install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/pdfoutline"
                 "${CMAKE_CURRENT_BINARY_DIR}/pdf-extract-outline"
        DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES "${PROJECT_BINARY_DIR}/fntsample.1"
              "${PROJECT_BINARY_DIR}/pdfoutline.1"
              "${PROJECT_BINARY_DIR}/pdf-extract-outline.1"
        DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")

add_subdirectory(po)
