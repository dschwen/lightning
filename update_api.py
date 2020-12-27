#!/usr/bin/env python3

import re

#
# Extract all function names declared extern in the lightning.h header
#
extern = re.compile('extern\s+[^\s]+\s*\*?\s*([^(]+)')

f = open("include/lightning.h.in")

for line in f.readlines():
  m = extern.match(line)
  if m is not None:
    print(m.group(1))

f.close()
