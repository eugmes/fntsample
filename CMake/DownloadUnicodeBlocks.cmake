function(download_unicode_blocks)
  set(default_url "https://unicode.org/Public/UNIDATA/Blocks.txt")
  set(UNICODE_BLOCKS "UNICODE_BLOCKS-NOTFOUND" CACHE FILEPATH "Unicode blocks file")
  set(CHECK_UNICODE_CERT ON CACHE BOOL "Check certificate while downloading Unicode blocks file")

  if(NOT UNICODE_BLOCKS_URL)
    set(UNICODE_BLOCKS_URL "${default_url}")
  endif()

  if(NOT UNICODE_BLOCKS)
    set(download_dest "${CMAKE_BINARY_DIR}/Blocks.txt")
    message(STATUS "Downloading ${UNICODE_BLOCKS_URL}...")
    file(DOWNLOAD
         "${UNICODE_BLOCKS_URL}"
         "${download_dest}"
         SHOW_PROGRESS
         STATUS status
       TLS_VERIFY ${CHECK_UNICODE_CERT})

    list(GET status 0 err)

    if(err)
      list(GET status 1 msg)
      message(FATAL_ERROR "Download failed (${err}): ${msg}")
    endif()

    set(UNICODE_BLOCKS "${download_dest}" CACHE FILEPATH "Unicode blocks file" FORCE)
  endif()

  if(NOT EXISTS "${UNICODE_BLOCKS}")
    set(UNICODE_BLOCKS "UNICODE_BLOCKS-NOTFOUND" CACHE FILEPATH "Unicode blocks file" FORCE)
    message(FATAL_ERROR "Unicode blocks file not found. "
            "Use -DUNICODE_BLOCKS=<path> or -DUNICODE_BLOCKS_URL=<url> to specify location of this file.\n"
            "Blocks.txt file is available at the Unicode web site: ${default_url}")
  endif()
endfunction()
