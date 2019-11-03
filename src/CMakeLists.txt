configure_file(config.h.in config.h ESCAPE_QUOTES @ONLY)
configure_file(fntsample.1.in fntsample.1 @ONLY)

add_executable(gen-unicode-blocks EXCLUDE_FROM_ALL
    unicode_blocks.cpp
    file_unicode_blocks.cpp
    gen_unicode_blocks.cpp)

target_compile_features(gen-unicode-blocks PRIVATE cxx_std_11)

set(unicode_blocks_table "${CMAKE_CURRENT_BINARY_DIR}/static_unicode_blocks.cpp")

add_custom_command(
  OUTPUT "${unicode_blocks_table}"
  COMMAND "$<TARGET_FILE:gen-unicode-blocks>" static_unicode_blocks.cpp.in "${UNICODE_BLOCKS}" > "${unicode_blocks_table}"
  DEPENDS static_unicode_blocks.cpp.in "${UNICODE_BLOCKS}" gen-unicode-blocks
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  VERBATIM
)

add_executable(fntsample
  fntsample.cpp
  unicode_blocks.cpp
  file_unicode_blocks.cpp
  "${unicode_blocks_table}"
)

add_translatable_sources(fntsample.cpp read_blocks.cpp)

target_compile_features(fntsample PRIVATE cxx_std_11)

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
  -Wnon-virtual-dtor -Wcast-align -Wchar-subscripts -Wall -W
  -Wshadow -Wpointer-arith -Wno-format-security -Wundef
) # TODO Add -Wformat-security

# TODO use improved install handling in CMake 3.14
install(TARGETS fntsample DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fntsample.1"
        DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
