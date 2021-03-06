/*
Twopence python bindings - extension glue

Copyright (C) 2014-2021 SUSE

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


#include "extension.h"

#include <fcntl.h>
#include <sys/wait.h>

#include "curlies.h"

/*
 * Methods belonging to the module itself.
 * None so far
 */
static PyMethodDef curlies_methods[] = {
      {	NULL }
};

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
# define PyMODINIT_FUNC void
#endif

static void
curlies_registerType(PyObject *m, const char *name, PyTypeObject *type)
{
	if (PyType_Ready(type) < 0)
		return;

	Py_INCREF(type);
	PyModule_AddObject(m, name, (PyObject *) type);
}

PyObject *
curlies_importType(const char *moduleName, const char *typeName)
{
	PyObject *module, *type;

	module = PyImport_ImportModule(moduleName);
	if (module == NULL)
		return NULL;

	type = PyDict_GetItemString(PyModule_GetDict(module), typeName);
	if (type == NULL) {
		PyErr_Format(PyExc_TypeError, "%s.%s: not defined", moduleName, typeName);
		return NULL;
	}

	if (!PyType_Check(type)) {
		PyErr_Format(PyExc_TypeError, "%s.%s: not a type object", moduleName, typeName);
		return NULL;
	}
	return type;
}

PyObject *
curlies_callType(PyTypeObject *typeObject, PyObject *args, PyObject *kwds)
{
	PyObject *obj;

	if (args == NULL) {
		args = PyTuple_New(0);
		obj = PyObject_Call((PyObject *) typeObject, args, NULL);
		Py_DECREF(args);
	} else {
		obj = PyObject_Call((PyObject *) typeObject, args, kwds);
	}

	return obj;
}

static struct PyModuleDef curlies_module_def = {
	PyModuleDef_HEAD_INIT,
	"curly",		/* m_name */
	"Module for accessing curly config files",
				/* m_doc */
	-1,			/* m_size */
	curlies_methods,	/* m_methods */
	NULL,			/* m_reload */
	NULL,			/* m_traverse */
	NULL,			/* m_clear */
	NULL,			/* m_free */
};



PyMODINIT_FUNC
PyInit_curly(void)
{
	PyObject* m;

	m = PyModule_Create(&curlies_module_def);

	curlies_registerType(m, "Config", &curlies_ConfigType);
	curlies_registerType(m, "ConfigNode", &curlies_ConfigNodeType);
	curlies_registerType(m, "Attr", &curlies_ConfigAttrType);
	curlies_registerType(m, "NodeIterator", &curlies_NodeIteratorType);
	curlies_registerType(m, "AttrIterator", &curlies_AttrIteratorType);
	return m;
}
