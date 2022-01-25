/*
Twopence python bindings

Copyright (C) 2014, 2015 SUSE

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef CURLIES_PYTHON_EXT_H
#define CURLIES_PYTHON_EXT_H

#include <Python.h>
#include <string.h>
#include <curlies.h>

typedef struct {
	PyObject_HEAD

	char *		name;
	curly_node_t *	config_root;
	curly_node_t *	config;
} curlies_Config;

typedef struct {
	PyObject_HEAD

	PyObject *	config_object;
	curly_node_t *	node;
} curlies_ConfigNode;

typedef struct {
	PyObject_HEAD

	PyObject *	node_object;
	curly_attr_t *	attr;
} curlies_Attr;

typedef struct {
	PyObject_HEAD

	/* Strictly speaking, it should not be necessary to keep a reference
	 * to the node object we're currently iterating. The low-level curly
	 * iterator implementation should invalidate the iterator when it
	 * detects that the node goes away. But better safe than start hunting
	 * for weird crashes... */
	PyObject *	node_object;
	curly_iter_t *	iter;
} curlies_Iterator;

extern PyTypeObject	curlies_ConfigType;
extern PyTypeObject	curlies_ConfigNodeType;
extern PyTypeObject	curlies_ConfigAttrType;
extern PyTypeObject	curlies_NodeIteratorType;
extern PyTypeObject	curlies_AttrIteratorType;

extern PyObject *	curlies_importType(const char *module, const char *typeName);
extern PyObject *	curlies_callType(PyTypeObject *typeObject, PyObject *args, PyObject *kwds);

static inline void
assign_string(char **var, const char *str)
{
	if (*var == str)
		return;
	if (*var)
		free(*var);
	*var = str?  strdup(str) : NULL;
}

static inline void
drop_string(char **var)
{
	assign_string(var, NULL);
}

static inline void
assign_object(PyObject **var, PyObject *obj)
{
	if (obj) {
		Py_INCREF(obj);
	}
	if (*var) {
		Py_DECREF(*var);
	}
	*var = obj;
}

static inline void
drop_object(PyObject **var)
{
	assign_object(var, NULL);
}


#endif /* CURLIES_PYTHON_EXT_H */

