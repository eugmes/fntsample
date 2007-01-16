# This file is in public domain
# Author: Eugeniy Meshcheryakov <eugen@debian.org>

BEGIN {
	print "#include \"unicode_blocks.h\""
	print ""
	print "const struct unicode_block unicode_blocks[] = {"
}

/^[^#]/ {
	print(gensub(/([^.]+)\.\.([^;]+); (.*)/, "\t{0x\\1, 0x\\2, \"\\3\"},", "g"))
}

END {
	print "\t{0, 0, NULL},"
	print "};"
}
