set(LANGUAGES uk)

function(process_po_file lang out_var)
  set(po_file "${CMAKE_CURRENT_SOURCE_DIR}/${lang}.po")
  set(gmo_file "${CMAKE_CURRENT_BINARY_DIR}/${lang}.gmo")
  set(gmo_file_tgt "${lang}.gmo")

  add_custom_command(OUTPUT ${gmo_file}
    COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} ${po_file} -o ${gmo_file}
    DEPENDS ${po_file}
    COMMENT "Compiling ${lang}.po to ${lang}.gmo"
    VERBATIM)

  install(FILES ${gmo_file}
          DESTINATION "${CMAKE_INSTALL_LOCALEDIR}/${lang}/LC_MESSAGES/"
          RENAME "${CMAKE_PROJECT_NAME}.mo")
  set(${out_var} ${gmo_file} PARENT_SCOPE)
endfunction()

function(process_po_files langs)
  set(gmo_files)

  foreach(lang ${langs})
    process_po_file(${lang} gmo_file)
    list(APPEND gmo_files ${gmo_file})
  endforeach()

  add_custom_target(pofiles ALL DEPENDS ${gmo_files})
endfunction()

process_po_files(${LANGUAGES})
