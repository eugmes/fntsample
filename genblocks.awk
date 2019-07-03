# This file is in public domain
# Author: Eugeniy Meshcheryakov <eugen@debian.org>

BEGIN {
    print "#include \"unicode_blocks.h\""
    print ""
    print "const struct unicode_block static_unicode_blocks[] = {"
}

/^[^#]/ {
    if (split($0, a, /\.\.|; /) == 3) {
        # remove any ^M characters, like ones in dos line breaks
        # NOTE: gsub() is used because mawk does not support gensub()
        gsub(/\r/, "", a[3])
        print "    {0x" a[1] ", 0x" a[2] ", \"" a[3] "\"},";
    }
}

END {
    print "    {0, 0, NULL},"
    print "};"
}
