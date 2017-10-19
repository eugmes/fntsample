find_path(Iconv_INCLUDE_DIR
          NAMES "iconv.h"
          DOC "iconv include directory")
mark_as_advanced(Iconv_INCLUDE_DIR)

find_library(Iconv_LIBRARY iconv libiconv libiconv-2 c DOC "iconv library")
mark_as_advanced(Iconv_LIBRARY)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Iconv
                                  FOUND_VAR Iconv_FOUND
                                  REQUIRED_VARS Iconv_INCLUDE_DIR
                                  FAIL_MESSAGE "Failed to find libiconv")

if(Iconv_FOUND)
  set(Iconv_INCLUDE_DIRS "${Iconv_INCLUDE_DIR}")
  if(Iconv_LIBRARY)
    set(Iconv_LIBRARIES "${Iconv_LIBRARY}")
  else()
    unset(Iconv_LIBRARIES)
  endif()
endif()
