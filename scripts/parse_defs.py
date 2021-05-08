#
# Extract a particular definition (-D) value from passed compiler args
#
# This is a bit messy as the genlink.py script in libopencm3 isn't written in
# a way that allows reuse of its parsing code in other Python scripts. This
# script primarily exists to parse RAM and ROM size data from genlink.py.
#

from __future__ import print_function

import re
import sys

if len(sys.argv) < 2:
    print("Usage: %s <NAME> [-Dvar=value, ...]")
    sys.exit(1)

search = sys.argv[1]
pattern = "-D%s=" % search
value = None

for arg in sys.argv[2:]:
    if arg.startswith(pattern):
        value = arg[len(pattern):]
        break
else:
    print("Couldn't find variable named '%s' in args: %s" % (search, sys.argv[2:]))
    sys.exit(2)

# Convert kilobytes to bytes
match = re.match(r'([0-9]+)K', value)
if match:
    value = int(match.group(1)) * 1024

print(value)