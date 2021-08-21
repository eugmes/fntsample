configure_file(config.h.in config.h ESCAPE_QUOTES @ONLY)
configure_file(fntsample.1.in fntsample.1 @ONLY)

set(
  C_WARNING_FLAGS
    -Wcast-align -Werror-implicit-function-declaration -Wchar-subscripts
    -Wall -W -Wpointer-arith -Wwrite-strings -Wformat-security
    -Wmissing-format-attribute -fno-common -Wundef
  CACHE STRING "Warning flags for C compiler"
)

add_executable(gen-unicode-blocks EXCLUDE_FROM_ALL
  gen_unicode_blocks.c
  read_blocks.c
)

target_compile_options(gen-unicode-blocks PRIVATE ${C_WARNING_FLAGS})

add_custom_command(
  OUTPUT static_unicode_blocks.c
  COMMAND gen-unicode-blocks "${UNICODE_BLOCKS}" static_unicode_blocks.c
  MAIN_DEPENDENCY "${UNICODE_BLOCKS}"
  DEPENDS gen-unicode-blocks
  VERBATIM
)

add_executable(fntsample
  fntsample.c
  read_blocks.c
  ${CMAKE_CURRENT_BINARY_DIR}/static_unicode_blocks.c
)

add_translatable_sources(fntsample.c read_blocks.c)

target_include_directories(fntsample PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(fntsample PRIVATE Intl::Intl PkgConfig::pkgs)

find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
  target_link_libraries(fntsample PRIVATE ${MATH_LIBRARY})
endif()

target_compile_options(fntsample PRIVATE ${C_WARNING_FLAGS})

# TODO use improved install handling in CMake 3.14
install(TARGETS fntsample DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fntsample.1"
        DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
