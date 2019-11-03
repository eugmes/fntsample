set(XGETTEXT_ARGS
  --keyword=_ --keyword=N_ --keyword=__ --keyword=__x
  --default-domain=${CMAKE_PROJECT_NAME}
  --add-comments=TRANSLATORS: --foreign-user
  --package-name ${CMAKE_PROJECT_NAME}
  --msgid-bugs-address=eugen@debian.org)

add_translations(
  POT_FILE fntsample.pot
  LANGUAGES uk
  XGETTEXT_ARGS ${XGETTEXT_ARGS}
  MSGMERGE_ARGS --backup=none --verbose
)
