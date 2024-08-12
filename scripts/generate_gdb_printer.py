#!/usr/bin/python
# Copyright 2024 Braden Ganetsky and Niall Douglas
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

import os, sys
import datetime

if len(sys.argv) < 3 or len(sys.argv) > 5:
  print(f"{sys.argv[0]} <output.h> <input.py> [protection_macro [disabling_macro]]")
  sys.exit(1)

printers_header = sys.argv[1]
printers_script = sys.argv[2]

if len(sys.argv) > 3:
    protection_macro = sys.argv[3]
else:
    protection_macro = printers_header.lstrip("/").replace("/", "_").replace(".", "_").upper()

if len(sys.argv) > 4:
    disable_macro = sys.argv[4]
elif "INLINE" in protection_macro:
    disable_macro = protection_macro.replace("INLINE", "DISABLE_INLINE")
else:
    disable_macro = protection_macro + "_DISABLE"

timestamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%S")

# Grab the entire script
with open(printers_script, "rt") as script:
    script_contents = script.readlines()

copyright_message = ""
while True:
    line = script_contents[0]
    if line.startswith("#"):
        copyright_message += "//" + line[1:]
        del script_contents[0]
    elif line.startswith("\n"):
        del script_contents[0]
    else:
        break

top_matter = f"""{copyright_message}
// Generated on {timestamp}

#ifndef {protection_macro}
#define {protection_macro}

#ifndef {disable_macro}
#if defined(__ELF__)
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverlength-strings"
#endif
__asm__(".pushsection \\".debug_gdb_scripts\\", \\"MS\\",@progbits,1\\n"
        ".byte 4 /* Python Text */\\n"
        ".ascii \\"gdb.inlined-script\\\\n\\"\\n"
"""
bottom_matter = f"""
        ".byte 0\\n"
        ".popsection\\n");
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif // defined(__ELF__)
#endif // !defined({disable_macro})

#endif // !defined({protection_macro})
"""

# Write the inline asm header
with open(printers_header, "wt") as header:
    header.write(top_matter)
    for line in script_contents:
        if line == '\n':
            header.write("\n")
            continue
        line2 = repr(line)[1:-1]
        line3 = repr(line2)[1:-1]
        print(f"{line} => {line2} => {line3}")
        header.write(f"""        ".ascii \\"{line3}\\"\\n"\n""")
    header.write(bottom_matter)
