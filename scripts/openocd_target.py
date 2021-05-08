#
# Print an OpenOCD target name for a given libopencm3 device name
# This could be done with sed from Make, but I expect this to grow
#

from __future__ import print_function

import sys
import re

if len(sys.argv) < 2:
    print("Usage: %s <DEVICE>")
    sys.exit(1)

device = sys.argv[1]

if device.startswith('stm32'):
    match = re.match(r'(stm32f[0-9]).*', device)
    print(match.group(1) + "x")

else:
    sys.stderr.write("Unsure how to determine OpenOCD target config for '%s'" % device)
    sys.exit(2)