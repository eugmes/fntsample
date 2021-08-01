include_guard(GLOBAL)

find_package(Gettext)
find_package(Xgettext)

if(NOT GETTEXT_FOUND)
  message(NOTICE "Translations will not be built.")
endif()

define_property(TARGET PROPERTY TRANSLATABLE_SOURCES
  BRIEF_DOCS "Translatable sources"
  FULL_DOCS "List of sources that are used to extract translation templates by xgettext")

add_custom_target(_updatepot)

function(add_translations)
  set(options)
  set(one_value_args POT_FILE)
  set(multi_value_args LANGUAGES XGETTEXT_ARGS MSGMERGE_ARGS)
  cmake_parse_arguments(PARSE_ARGV 0 ARG
    "${options}" "${one_value_args}" "${multi_value_args}")

  set(pot_file "${ARG_POT_FILE}")
  get_filename_component(pot_file_path "${pot_file}" ABSOLUTE)

  if(XGETTEXT_FOUND)
    add_custom_target(updatepot
      COMMAND "${XGETTEXT_EXECUTABLE}" ${ARG_XGETTEXT_ARGS}
              $<TARGET_PROPERTY:_updatepot,TRANSLATABLE_SOURCES>
              -o "${pot_file_path}"
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      COMMENT "Extracting translation templates into ${pot_file}"
      COMMAND_EXPAND_LISTS
      VERBATIM
    )
  else()
    message(NOTICE "Updating translation templates will not be possible.")
  endif()

  if(NOT GETTEXT_FOUND)
    return()
  endif()

  if(TARGET updatepot)
    add_custom_target(updatepo)
  endif()

  add_custom_target(updatepo-only)

  set(gmo_files)

  foreach(lang IN LISTS ARG_LANGUAGES)
    set(po_file "${lang}.po")
    set(gmo_file "${lang}.gmo")
    set(po_file_path "${CMAKE_CURRENT_SOURCE_DIR}/${po_file}")
    set(gmo_file_path "${CMAKE_CURRENT_BINARY_DIR}/${gmo_file}")

    add_custom_command(OUTPUT ${gmo_file}
      COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} "${po_file_path}" -o "${gmo_file}"
      MAIN_DEPENDENCY "${po_file}"
      COMMENT "Generating binary catalog ${gmo_file} from ${po_file}"
      VERBATIM)

    install(FILES "${gmo_file_path}"
      DESTINATION "${CMAKE_INSTALL_LOCALEDIR}/${lang}/LC_MESSAGES/"
      RENAME "${CMAKE_PROJECT_NAME}.mo")

    list(APPEND gmo_files "${gmo_file_path}")

    macro(add_po_update_target target main_target)
      add_custom_target(${target}
        COMMAND "${GETTEXT_MSGMERGE_EXECUTABLE}" ${ARG_MSGMERGE_ARGS}
                --update "${po_file}" "${pot_file}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        DEPENDS ${ARGN}
        COMMENT "Merging ${pot_file} into ${po_file}"
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
  # Use paths relative to the project directory for source files. This way
  # the template file will contain the same file names independen of how
  # the build was configured.
  foreach(src IN LISTS ARGV)
    get_filename_component(src_abs_path "${src}" ABSOLUTE)
    file(RELATIVE_PATH src_path "${PROJECT_SOURCE_DIR}" "${src_abs_path}")
    set_property(TARGET _updatepot APPEND PROPERTY TRANSLATABLE_SOURCES "${src_path}")
  endforeach()
endfunction()
