configure_file(config.h.in config.h ESCAPE_QUOTES @ONLY)
configure_file(fntsample.1.in fntsample.1 @ONLY)

add_executable(gen-unicode-blocks EXCLUDE_FROM_ALL
  gen_unicode_blocks.c
  read_blocks.c
)

target_compile_features(gen-unicode-blocks PRIVATE c_std_99)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/static_unicode_blocks.c
  COMMAND "$<TARGET_FILE:gen-unicode-blocks>" "${UNICODE_BLOCKS}" "${CMAKE_CURRENT_BINARY_DIR}/static_unicode_blocks.c"
  DEPENDS ${UNICODE_BLOCKS} gen-unicode-blocks
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM
)

add_executable(fntsample
  fntsample.c
  read_blocks.c
  ${CMAKE_CURRENT_BINARY_DIR}/static_unicode_blocks.c
)

add_translatable_sources(fntsample.c read_blocks.c)

target_compile_features(fntsample PRIVATE c_std_99)

target_include_directories(fntsample PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(fntsample PRIVATE
  m
  Intl::Intl
  PkgConfig::pkgs
)

target_compile_options(fntsample PRIVATE
  -Wcast-align -Werror-implicit-function-declaration -Wchar-subscripts
  -Wall -W -Wpointer-arith -Wwrite-strings -Wformat-security
  -Wmissing-format-attribute -fno-common -Wundef)

# TODO use improved install handling in CMake 3.14
install(TARGETS fntsample DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fntsample.1"
        DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
