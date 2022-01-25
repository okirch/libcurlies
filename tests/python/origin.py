#!/usr/bin/python3

# Run from top level source directory with
# LD_PRELOAD=library/libcurlies.so PYTHONPATH=python

import curly

filename = "input/simple.conf"

cfg = curly.Config(filename)
node = cfg.tree()

child = node.get_child("node", "server")
origin = child.origin

print(f"node origin is {origin}")
assert(filename in origin)
assert("line 6" in origin)

print("OK")
