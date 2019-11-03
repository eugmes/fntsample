include_guard(GLOBAL)

find_package(Gettext)
find_package(Xgettext)

if(NOT GETTEXT_FOUND)
  message(NOTICE "Translations will not be built.")
endif()

define_property(TARGET PROPERTY XGETTEXT_OPTIONS
  BRIEF_DOCS "Options for xgettext"
  FULL_DOCS "Options to pass to xgettext")

define_property(TARGET PROPERTY TRANSLATABLE_SOURCES
  BRIEF_DOCS "Translatable sources"
  FULL_DOCS "List of sources that are used to extract translation templates by xgettext")

define_property(TARGET PROPERTY POT_FILE
  BRIEF_DOCS "POT File"
  FULL_DOCS "Translation template file to update in updatepot target")

if(XGETTEXT_FOUND)
  add_custom_target(updatepot
    COMMAND "${XGETTEXT_EXECUTABLE}" $<TARGET_PROPERTY:updatepot,XGETTEXT_OPTIONS>
            $<TARGET_PROPERTY:updatepot,TRANSLATABLE_SOURCES>
            -o $<TARGET_PROPERTY:updatepot,POT_FILE>
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    # COMMENT "Extracting translation templates into $<TARGET_PROPERTY:updatepot,POT_FILE>"
    COMMAND_EXPAND_LISTS
    VERBATIM
  )
else()
  message(NOTICE "Updating translation templates will not be possible.")
endif()

function(_add_language lang out_var)
  set(po_file "${CMAKE_CURRENT_SOURCE_DIR}/${lang}.po")
  set(gmo_file "${CMAKE_CURRENT_BINARY_DIR}/${lang}.gmo")

  add_custom_command(OUTPUT ${gmo_file}
    COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} "${po_file}" -o "${gmo_file}"
    DEPENDS "${po_file}"
    COMMENT "Generating binary catalog ${lang}.gmo from ${lang}.gmo"
    VERBATIM)

  set(${out_var} "${gmo_file}" PARENT_SCOPE)

  install(FILES ${gmo_file}
    DESTINATION "${CMAKE_INSTALL_LOCALEDIR}/${lang}/LC_MESSAGES/"
    RENAME "${CMAKE_PROJECT_NAME}.mo")
endfunction()

function(add_translations)
  set(options)
  set(one_value_args POT_FILE)
  set(multi_value_args LANGUAGES XGETTEXT_ARGS MSGMERGE_ARGS)
  cmake_parse_arguments(PARSE_ARGV 0 ADD_TRANSLATIONS
    "${options}" "${one_value_args}" "${multi_value_args}")

  if(TARGET updatepot)
    get_filename_component(pot_file_path "${ADD_TRANSLATIONS_POT_FILE}" ABSOLUTE)
    set_property(TARGET updatepot PROPERTY POT_FILE "${pot_file_path}")
    set_property(TARGET updatepot PROPERTY XGETTEXT_OPTIONS ${ADD_TRANSLATIONS_XGETTEXT_ARGS})
  endif()

  if(NOT GETTEXT_FOUND)
    return()
  endif()

  if(TARGET updatepot)
    add_custom_target(updatepo)
  endif()

  add_custom_target(updatepo-only)

  set(gmo_files)

  foreach(lang IN LISTS ADD_TRANSLATIONS_LANGUAGES)
    _add_language(${lang} gmo_file)
    list(APPEND gmo_files "${gmo_file}")

    set(po_file "${lang}.po")

    macro(add_po_update_target target main_target)
      add_custom_target(${target}
        COMMAND "${GETTEXT_MSGMERGE_EXECUTABLE}" ${ADD_TRANSLATIONS_MSGMERGE_ARGS}
                --update "${po_file}" "${ADD_TRANSLATIONS_POT_FILE}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        DEPENDS ${ARGN}
        COMMENT "Merging ${ADD_TRANSLATIONS_POT_FILE} into ${po_file}"
        VERBATIM
      )
      add_dependencies(${main_target} ${target})
    endmacro()

    if(TARGET updatepot)
      add_po_update_target(updatepo-${lang} updatepo updatepot)
    endif()
    add_po_update_target(updatepo-only-${lang} updatepo-only)
  endforeach()

  add_custom_target(pofiles ALL DEPENDS ${gmo_files})
endfunction()

function(add_translatable_sources)
  if(NOT TARGET updatepot)
    return()
  endif()

  get_property(_SOURCES TARGET updatepot PROPERTY TRANSLATABLE_SOURCES)

  # Use paths relative to the project directory for source files. This way
  # the template file will contain the same file names independen of how
  # the build was configured.
  foreach(src IN LISTS ARGV)
    get_filename_component(src_abs_path "${src}" ABSOLUTE)
    file(RELATIVE_PATH src_path "${PROJECT_SOURCE_DIR}" "${src_abs_path}")
    list(APPEND _SOURCES "${src_path}")
  endforeach()

  set_property(TARGET updatepot PROPERTY TRANSLATABLE_SOURCES ${_SOURCES})
endfunction()
