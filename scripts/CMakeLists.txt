configure_file(pdf-extract-outline.1.in pdf-extract-outline.1 @ONLY)
configure_file(pdfoutline.1.in pdfoutline.1 @ONLY)
configure_file(pdfoutline.pl pdfoutline ESCAPE_QUOTES @ONLY)
configure_file(pdf-extract-outline.pl pdf-extract-outline ESCAPE_QUOTES @ONLY)

install(
  PROGRAMS
    "${CMAKE_CURRENT_BINARY_DIR}/pdfoutline"
    "${CMAKE_CURRENT_BINARY_DIR}/pdf-extract-outline"
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/pdfoutline.1"
    "${CMAKE_CURRENT_BINARY_DIR}/pdf-extract-outline.1"
  DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
)
