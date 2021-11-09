# libcurlies

This is a utility library for handling simple, hierarchical configuration files.
The syntax is fairly simple, and this repo provides a C level interfaces as well as
a python3 binding.

## Concepts and file syntax

A curly config file provides two types of data, attributes and nodes.

An attribute is a named value, or list of values.

```console
workspace	"/tmp/my-workspace";
flavors		chili, cumin, coriander, curcuma;
```

Programmatically, the library does not distinguish between attributes
that hold a single value, and those that contain several. This means
any attribute can hold an arbitrary number values, including zero.
"getting" the value of an attribute will always return the first value
of its list of values, and "appending" a value to an attribute will always
extend its list of values.

A node has a type, commonly a name, and can hold an arbitrary number of
attributes:

```console
network fixed {
        prefix          192.168.1/24;
        uuid            "0011223344-5566-778899";
}
network private {
        prefix          192.168.8/24;
}
```

The contents of a configuration file are represented as a node as
well, with empty type and name.


