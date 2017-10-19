set(LANGUAGES uk)

add_custom_target(pofiles)

function(process_po_files _langs)
  foreach(_lang ${_langs})
    set(_po_file "${CMAKE_CURRENT_SOURCE_DIR}/${_lang}.po")
    set(_gmo_file "${CMAKE_CURRENT_BINARY_DIR}/${_lang}.gmo")
    set(_gmo_file_tgt "${_lang}.gmo")

    add_custom_command(OUTPUT ${_gmo_file}
      COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} ${_po_file} -o ${_gmo_file}
      DEPENDS ${_po_file}
      COMMENT "Compiling ${_lang}.po to ${_lang}.gmo"
      VERBATIM)

    install(FILES ${_gmo_file}
            DESTINATION "${CMAKE_INSTALL_LOCALEDIR}/${_lang}/LC_MESSAGES/"
            RENAME "${CMAKE_PROJECT_NAME}.mo")

    add_custom_target(${_gmo_file_tgt} ALL DEPENDS ${_gmo_file})
    add_dependencies(pofiles ${_gmo_file_tgt})
  endforeach()
endfunction()

process_po_files(${LANGUAGES})
