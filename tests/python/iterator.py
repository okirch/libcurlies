#!/usr/bin/python3

# Run from top level source directory with
# LD_PRELOAD=library/libcurlies.so PYTHONPATH=python

import curly

def dump(node, indent = ""):
	type = node.type()
	name = node.name()
	if name is not None:
		print("%s%s %s:" % (indent, type, name))
	else:
		print("%s%s:" % (indent, type))

	indent += "  "

	for attr in node.attributes:
		print("%s%s = %s" % (indent, attr.name, attr.values))

	for child in node:
		dump(child, indent)

cfg = curly.Config("input/twopence.conf")
node = cfg.tree()

dump(node)
