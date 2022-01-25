/*
Twopence python bindings - class Config

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


#include "extension.h"

#include <fcntl.h>
#include <sys/wait.h>

#include "curlies.h"

static void		Config_dealloc(curlies_Config *self);
static PyObject *	Config_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int		Config_init(curlies_Config *self, PyObject *args, PyObject *kwds);
static PyObject *	Config_name(curlies_Config *self, PyObject *args, PyObject *kwds);
static PyObject *	Config_workspace(curlies_Config *self, PyObject *args, PyObject *kwds);
static PyObject *	Config_report(curlies_Config *self, PyObject *args, PyObject *kwds);
static PyObject *	Config_nodes(curlies_Config *self, PyObject *args, PyObject *kwds);
static PyObject *	Config_networks(curlies_Config *self, PyObject *args, PyObject *kwds);
static PyObject *	Config_tree(curlies_Config *self, PyObject *args, PyObject *kwds);
static PyObject *	Config_save(curlies_Config *self, PyObject *args, PyObject *kwds);

static PyObject *	Config_node_target(curlies_Config *self, PyObject *args, PyObject *kwds);
static PyObject *	Config_node_internal_ip(curlies_Config *self, PyObject *args, PyObject *kwds);
static PyObject *	Config_node_external_ip(curlies_Config *self, PyObject *args, PyObject *kwds);
static PyObject *	Config_node_internal_ip6(curlies_Config *self, PyObject *args, PyObject *kwds);

static PyObject *	Config_network_subnet(curlies_Config *self, PyObject *args, PyObject *kwds);
static PyObject *	Config_network_gateway(curlies_Config *self, PyObject *args, PyObject *kwds);


/*
 * Define the python bindings of class "Config"
 */
static PyMethodDef curlies_ConfigMethods[] = {
      /* Top-level attributes */
      { "name", (PyCFunction) Config_name, METH_VARARGS | METH_KEYWORDS,
	"Get the name of the test project",
      },
      { "workspace", (PyCFunction) Config_workspace, METH_VARARGS | METH_KEYWORDS,
	"Get the workspace of the test project",
      },
      { "report", (PyCFunction) Config_report, METH_VARARGS | METH_KEYWORDS,
	"Get the report of the test project",
      },

      /* Top-level children */
      { "nodes", (PyCFunction) Config_nodes, METH_VARARGS | METH_KEYWORDS,
	"Get the nodes of the test project",
      },
      { "networks", (PyCFunction) Config_networks, METH_VARARGS | METH_KEYWORDS,
	"Get the networks of the test project",
      },

      /* Node attributes */
      {	"node_target", (PyCFunction) Config_node_target, METH_VARARGS | METH_KEYWORDS,
	"Get the node's target description"
      },
      {	"node_internal_ip", (PyCFunction) Config_node_internal_ip, METH_VARARGS | METH_KEYWORDS,
	"Get the node's internal IPv4 address"
      },
      {	"node_external_ip", (PyCFunction) Config_node_external_ip, METH_VARARGS | METH_KEYWORDS,
	"Get the node's external IPv4 address"
      },
      {	"node_internal_ip6", (PyCFunction) Config_node_internal_ip6, METH_VARARGS | METH_KEYWORDS,
	"Get the node's internal IPv6 address"
      },

      /* Network attributes */
      {	"network_subnet", (PyCFunction) Config_network_subnet, METH_VARARGS | METH_KEYWORDS,
	"Get the networks's IPv4 subnet"
      },
      {	"network_gateway", (PyCFunction) Config_network_gateway, METH_VARARGS | METH_KEYWORDS,
	"Get the networks's IPv4 gateway"
      },

      /* Access to low-level config functions */
      {	"tree", (PyCFunction) Config_tree, METH_VARARGS | METH_KEYWORDS,
	"Get the config tree"
      },
      {	"save", (PyCFunction) Config_save, METH_VARARGS | METH_KEYWORDS,
	"Save configuration to file"
      },

      /* Interface stuff */
      {	NULL }
};

PyTypeObject curlies_ConfigType = {
	PyVarObject_HEAD_INIT(NULL, 0)

	.tp_name	= "curly.Config",
	.tp_basicsize	= sizeof(curlies_Config),
	.tp_flags	= Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_doc		= "Config object for twopence based tests",

	.tp_methods	= curlies_ConfigMethods,
	.tp_init	= (initproc) Config_init,
	.tp_new		= Config_new,
	.tp_dealloc	= (destructor) Config_dealloc,
};

/*
 * Constructor: allocate empty Config object, and set its members.
 */
static PyObject *
Config_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	curlies_Config *self;

	self = (curlies_Config *) type->tp_alloc(type, 0);
	if (self == NULL)
		return NULL;

	/* init members */
	self->config = NULL;
	self->name = NULL;

	return (PyObject *)self;
}

/*
 * Initialize the status object
 */
static int
Config_init(curlies_Config *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {
		"file",
		NULL
	};
	PyObject *arg_object = NULL;
	char *filename = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &arg_object))
		return -1;

	if (arg_object == Py_None || arg_object == NULL) {
		/* create an empty Config object */
		self->config_root = curly_node_new();
		self->config = self->config_root;
	} else {
		filename = PyUnicode_AsUTF8(arg_object);
		if (filename == NULL)
			return -1;

		self->config_root = curly_node_read(filename);
		if (self->config_root == NULL) {
			PyErr_Format(PyExc_SystemError, "Unable to read curlies config from file \"%s\"", filename);
			return -1;
		}

		/* While we're transitioning from the old-style curly stuff to Eric's
		 * XML stuff, there may or may not be a testenv group between the root and
		 * the stuff we're interested in.
		 */
		self->config = curly_node_get_child(self->config_root, "testenv", NULL);
		if (self->config != NULL) {
			self->name = (char *) curly_node_name(self->config);
		} else {
			self->config = self->config_root;
		}

		/* printf("Using curly config file %s\n", filename); */
	}

	return 0;
}

/*
 * Destructor: clean any state inside the Config object
 */
static void
Config_dealloc(curlies_Config *self)
{
	// printf("Destroying %p\n", self);
	/* drop_string(&self->name); */
	if (self->config_root)
		curly_node_free(self->config_root);
	self->config_root = NULL;
	self->config = NULL;
	self->name = NULL;
}

int
Config_Check(PyObject *self)
{
	return PyType_IsSubtype(Py_TYPE(self), &curlies_ConfigType);
}

static bool
__check_void_args(PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {
		NULL
	};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist, NULL))
		return false;

	return true;
}

static bool
__get_single_string_arg(PyObject *args, PyObject *kwds, const char *arg_name, const char **string_arg_p)
{
	char *kwlist[] = {
		(char *) arg_name,
		NULL
	};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, string_arg_p))
		return false;

	return true;
}

static PyObject *
__to_string(const char *value)
{
	if (value != NULL)
		return PyUnicode_FromString(value);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
__to_string_list(const char * const*values)
{
	PyObject *result;

	result = PyList_New(0);
	while (values && *values)
		PyList_Append(result, PyUnicode_FromString(*values++));

	return result;
}

static PyObject *
__toplevel_string_attr(curlies_Config *self, PyObject *args, PyObject *kwds, const char *attrname)
{
	if (!__check_void_args(args, kwds))
		return NULL;

	return __to_string(curly_node_get_attr(self->config, attrname));
}

static PyObject *
__get_children(curly_node_t *config, const char *type)
{
	const char **values;
	PyObject *result;

	values = curly_node_get_children(config, type);
	if (values == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "failed to get child names for configuration object");
		return NULL;
	}

	result = __to_string_list(values);
	free(values);

	return result;
}

static PyObject *
__get_attr_names(curly_node_t *config)
{
	const char **values;
	PyObject *result;

	values = curly_node_get_attr_names(config);
	if (values == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "failed to get attribute names for configuration object");
		return NULL;
	}

	result = __to_string_list(values);
	free(values);

	return result;
}

static PyObject *
__toplevel_name_list(curlies_Config *self, PyObject *args, PyObject *kwds, const char *type)
{
	if (!__check_void_args(args, kwds))
		return NULL;

	return __get_children(self->config, type);
}

static PyObject *
__firstlevel_string_attr(curlies_Config *self, PyObject *args, PyObject *kwds, const char *type, const char *attrname, const char *compat_attrname)
{
	const char *name, *value;
	curly_node_t *child;

	if (!__get_single_string_arg(args, kwds, "name", &name))
		return NULL;

	child = curly_node_get_child(self->config, type, name);
	if (child == NULL) {
		PyErr_Format(PyExc_AttributeError, "Unknown %s \"%s\"", type, name);
		return NULL;
	}

	value = curly_node_get_attr(child, attrname);
	if (value == NULL && compat_attrname)
		value = curly_node_get_attr(child, compat_attrname);

	return __to_string(value);
}

static PyObject *
Config_name(curlies_Config *self, PyObject *args, PyObject *kwds)
{
	return __to_string(self->name);
}

static PyObject *
Config_workspace(curlies_Config *self, PyObject *args, PyObject *kwds)
{
	return __toplevel_string_attr(self, args, kwds, "workspace");
}

static PyObject *
Config_report(curlies_Config *self, PyObject *args, PyObject *kwds)
{
	return __toplevel_string_attr(self, args, kwds, "report");
}

static PyObject *
Config_nodes(curlies_Config *self, PyObject *args, PyObject *kwds)
{
	return __toplevel_name_list(self, args, kwds, "node");
}

static PyObject *
Config_node_target(curlies_Config *self, PyObject *args, PyObject *kwds)
{
	return __firstlevel_string_attr(self, args, kwds, "node", "target", NULL);
}

static PyObject *
Config_node_internal_ip(curlies_Config *self, PyObject *args, PyObject *kwds)
{
	return __firstlevel_string_attr(self, args, kwds, "node", "ipv4_address", "ipv4_addr");
}

static PyObject *
Config_node_external_ip(curlies_Config *self, PyObject *args, PyObject *kwds)
{
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Config_node_internal_ip6(curlies_Config *self, PyObject *args, PyObject *kwds)
{
	return __firstlevel_string_attr(self, args, kwds, "node", "ipv6_address", "ipv6_addr");
}

static PyObject *
Config_networks(curlies_Config *self, PyObject *args, PyObject *kwds)
{
	return __toplevel_name_list(self, args, kwds, "network");
}

static PyObject *
Config_network_subnet(curlies_Config *self, PyObject *args, PyObject *kwds)
{
	return __firstlevel_string_attr(self, args, kwds, "network", "subnet", NULL);
}

static PyObject *
Config_network_gateway(curlies_Config *self, PyObject *args, PyObject *kwds)
{
	return __firstlevel_string_attr(self, args, kwds, "network", "gateway", NULL);
}


static PyObject *
Config_tree(curlies_Config *self, PyObject *args, PyObject *kwds)
{
	PyObject *tuple;
	PyObject *nodeObj;

	if (!__check_void_args(args, kwds))
		return NULL;

	tuple = PyTuple_New(1);

	PyTuple_SetItem(tuple, 0, (PyObject *) self);
	Py_INCREF(self);

	nodeObj = curlies_callType(&curlies_ConfigNodeType, tuple, NULL);

	Py_DECREF(tuple);
	return nodeObj;
}

static PyObject *
Config_save(curlies_Config *self, PyObject *args, PyObject *kwds)
{
	const char *filename;

	if (!__get_single_string_arg(args, kwds, "filename", &filename))
		return NULL;

	if (curly_node_write(self->config_root, filename) < 0) {
		PyErr_Format(PyExc_OSError, "unable to write config file %s", filename);
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *	ConfigNode_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int		ConfigNode_init(curlies_ConfigNode *self, PyObject *args, PyObject *kwds);
static void		ConfigNode_dealloc(curlies_ConfigNode *self);
static PyObject *	ConfigNode_iter(curlies_ConfigNode *self);
static PyObject *	ConfigNode_attribute_iter(curlies_ConfigNode *self);
static PyObject *	ConfigNode_getattro(curlies_ConfigNode *self, PyObject *name);
static PyObject *	ConfigNode_str(curlies_ConfigNode *self);
static PyObject *	ConfigNode_get_child(curlies_ConfigNode *self, PyObject *args, PyObject *kwds);
static PyObject *	ConfigNode_add_child(curlies_ConfigNode *self, PyObject *args, PyObject *kwds);
static PyObject *	ConfigNode_drop_child(curlies_ConfigNode *self, PyObject *args, PyObject *kwds);
static PyObject *	ConfigNode_get_children(curlies_ConfigNode *self, PyObject *args, PyObject *kwds);
static PyObject *	ConfigNode_get_attributes(curlies_ConfigNode *self, PyObject *args, PyObject *kwds);
static PyObject *	ConfigNode_get_value(curlies_ConfigNode *self, PyObject *args, PyObject *kwds);
static PyObject *	ConfigNode_set_value(curlies_ConfigNode *self, PyObject *args, PyObject *kwds);
static PyObject *	ConfigNode_unset_value(curlies_ConfigNode *self, PyObject *args, PyObject *kwds);
static PyObject *	ConfigNode_get_values(curlies_ConfigNode *self, PyObject *args, PyObject *kwds);

static PyObject *	ConfigAttr_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int		ConfigAttr_init(curlies_Attr *self, PyObject *args, PyObject *kwds);
static void		ConfigAttr_dealloc(curlies_Attr *self);
static PyObject *	ConfigAttr_getattro(curlies_Attr *self, PyObject *name);

static PyObject *	ConfigIter_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int		ConfigIter_init(curlies_Iterator *self, PyObject *args, PyObject *kwds);
static void		ConfigIter_dealloc(curlies_Iterator *self);
static PyObject *	ConfigIter_iter(curlies_Iterator *self);
static PyObject *	ConfigNodeIter_iternext(curlies_Iterator *self);
static PyObject *	ConfigAttrIter_iternext(curlies_Iterator *self);

/* should prolly be in a public header (if we had one) */
extern int		ConfigIter_Check(PyObject *self);

/*
 * Define the python bindings of class "Config"
 * Much of this cruft is no longer needed and can go away.
 */
static PyMethodDef curly_ConfigNodeMethods[] = {
      /* Top-level attributes */
      { "get_child", (PyCFunction) ConfigNode_get_child, METH_VARARGS | METH_KEYWORDS,
	"Find the child node with given type and name",
      },
      { "add_child", (PyCFunction) ConfigNode_add_child, METH_VARARGS | METH_KEYWORDS,
	"Add a child node with given type and name",
      },
      { "drop_child", (PyCFunction) ConfigNode_drop_child, METH_VARARGS | METH_KEYWORDS,
	"Drop the given child",
      },
      { "get_children", (PyCFunction) ConfigNode_get_children, METH_VARARGS | METH_KEYWORDS,
	"Get all child nodes with given type",
      },
      { "get_attributes", (PyCFunction) ConfigNode_get_attributes, METH_VARARGS | METH_KEYWORDS,
	"Get the names of all attributes of this node",
      },
      { "get_value", (PyCFunction) ConfigNode_get_value, METH_VARARGS | METH_KEYWORDS,
	"Get the value of the named attribute as a single string"
      },
      { "set_value", (PyCFunction) ConfigNode_set_value, METH_VARARGS | METH_KEYWORDS,
	"Set the value of the named attribute"
      },
      { "drop", (PyCFunction) ConfigNode_unset_value, METH_VARARGS | METH_KEYWORDS,
	"Drop the named attribute"
      },
      { "get_values", (PyCFunction) ConfigNode_get_values, METH_VARARGS | METH_KEYWORDS,
	"Get the value of the named attribute as list of strings"
      },

      {	NULL }
};

PyTypeObject curlies_ConfigNodeType = {
	PyVarObject_HEAD_INIT(NULL, 0)

	.tp_name	= "curly.ConfigNode",
	.tp_basicsize	= sizeof(curlies_ConfigNode),
	.tp_flags	= Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_doc		= "Config object representing a curly config file",

	.tp_methods	= curly_ConfigNodeMethods,
	.tp_init	= (initproc) ConfigNode_init,
	.tp_new		= ConfigNode_new,
	.tp_dealloc	= (destructor) ConfigNode_dealloc,
	.tp_getattro	= (getattrofunc) ConfigNode_getattro,
	.tp_iter	= (getiterfunc) ConfigNode_iter,
	.tp_str		= (reprfunc) ConfigNode_str,
};

PyTypeObject curlies_ConfigAttrType = {
	PyVarObject_HEAD_INIT(NULL, 0)

	.tp_name	= "curly.Attr",
	.tp_basicsize	= sizeof(curlies_Attr),
	.tp_flags	= Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_doc		= "Config object representing a curly node attribute",

	.tp_methods	= NULL,
	.tp_init	= (initproc) ConfigAttr_init,
	.tp_new		= ConfigAttr_new,
	.tp_dealloc	= (destructor) ConfigAttr_dealloc,
	.tp_getattro	= (getattrofunc) ConfigAttr_getattro,
};


static PyMethodDef curly_ConfigIterMethods[] = {
      {	NULL }
};

PyTypeObject curlies_NodeIteratorType = {
	PyVarObject_HEAD_INIT(NULL, 0)

	.tp_name	= "curly.NodeIterator",
	.tp_basicsize	= sizeof(curlies_Iterator),
	.tp_flags	= Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_doc		= "Object representing an iterator over the children of a Curly config node",

	.tp_methods	= curly_ConfigIterMethods,
	.tp_init	= (initproc) ConfigIter_init,
	.tp_new		= ConfigIter_new,
	.tp_dealloc	= (destructor) ConfigIter_dealloc,
	.tp_iter	= (getiterfunc) ConfigIter_iter,
	.tp_iternext	= (iternextfunc) ConfigNodeIter_iternext,
};

PyTypeObject curlies_AttrIteratorType = {
	PyVarObject_HEAD_INIT(NULL, 0)

	.tp_name	= "curly.AttrIterator",
	.tp_basicsize	= sizeof(curlies_Iterator),
	.tp_flags	= Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	.tp_doc		= "Object representing an iterator over the attributes of a Curly config node",

	.tp_methods	= curly_ConfigIterMethods,
	.tp_init	= (initproc) ConfigIter_init,
	.tp_new		= ConfigIter_new,
	.tp_dealloc	= (destructor) ConfigIter_dealloc,
	.tp_iter	= (getiterfunc) ConfigIter_iter,
	.tp_iternext	= (iternextfunc) ConfigAttrIter_iternext,
};

/*
 * Constructor: allocate empty Config object, and set its members.
 */
static PyObject *
ConfigNode_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	curlies_ConfigNode *self;

	self = (curlies_ConfigNode *) type->tp_alloc(type, 0);
	if (self == NULL)
		return NULL;

	/* init members */
	self->config_object = NULL;
	self->node = NULL;

	return (PyObject *)self;
}

static inline void
__ConfigNode_attach(curlies_ConfigNode *self, PyObject *config_object, curly_node_t *node)
{
	assert(self->config_object == NULL);

	self->node = node;
	self->config_object = config_object;
	Py_INCREF(config_object);

	// printf("ConfigNode %p references %p count=%ld\n", self, config_object, config_object->ob_refcnt);
}

static inline void
__ConfigNode_detach(curlies_ConfigNode *self)
{
	if (self->config_object) {
		// printf("ConfigNode %p releases %p count=%ld\n", self, self->config_object, self->config_object->ob_refcnt);
		Py_DECREF(self->config_object);
	}
	self->config_object = NULL;
}

/*
 * Initialize the node object
 */
static int
ConfigNode_init(curlies_ConfigNode *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {
		"config",
		NULL
	};
	PyObject *config_object = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &config_object))
		return -1;

	if (config_object && !Config_Check(config_object)) {
		PyErr_SetString(PyExc_RuntimeError, "config argument must be an instance of curly.Config");
		return -1;
	}

	if (config_object)
		__ConfigNode_attach(self, config_object, ((curlies_Config *) config_object)->config_root);

	return 0;
}

/*
 * Destructor: clean any state inside the Config object
 */
static void
ConfigNode_dealloc(curlies_ConfigNode *self)
{
	__ConfigNode_detach(self);
}

int
ConfigNode_Check(PyObject *self)
{
	return PyType_IsSubtype(Py_TYPE(self), &curlies_ConfigNodeType);
}

static curly_node_t *
ConfigNode_GetPointer(PyObject *self)
{
	curly_node_t *node;

	if (!ConfigNode_Check(self)) {
		PyErr_SetString(PyExc_RuntimeError, "node argument must be an instance of curly.ConfigNode");
		return NULL;
	}

	node = ((curlies_ConfigNode *) self)->node;
	if (node == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "ConfigNode object does not refer to anything");
		return NULL;
	}

	return node;
}

static PyObject *
ConfigNode_getattro(curlies_ConfigNode *self, PyObject *nameo)
{
	if (self->node) {
		const char *name = PyUnicode_AsUTF8(nameo);
		const char *const *values;

		if (name == NULL)
			return NULL;

		if (!strcmp(name, "attributes"))
			return ConfigNode_attribute_iter(self);
		if (!strcmp(name, "type"))
			return __to_string(curly_node_type(self->node));
		if (!strcmp(name, "name"))
			return __to_string(curly_node_name(self->node));
		if (!strcmp(name, "origin")) {
			char buffer[4096];
			const char *path;

			if (!(path = curly_node_get_source_file(self->node))) {
				Py_INCREF(Py_None);
				return Py_None;
			}

			snprintf(buffer, sizeof(buffer), "%s, line %u", path,
					curly_node_get_source_line(self->node));
			return __to_string(buffer);
		}

		values = curly_node_get_attr_list(self->node, name);
		if (values) {
			if (values[0] == NULL || values[1] == NULL)
				return __to_string(values[0]);

			return __to_string_list(values);
		}
	}

	return PyObject_GenericGetAttr((PyObject *) self, nameo);
}

static bool
__check_node(curlies_ConfigNode *self)
{
	if (self->node == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "ConfigNode object does not refer to any config data");
		return false;
	}

	return true;
}

static bool
__check_call(curlies_ConfigNode *self, PyObject *args, PyObject *kwds)
{
	return __check_void_args(args, kwds) && __check_node(self);
}

static PyObject *
__wrap_node(curly_node_t *node, curlies_ConfigNode *parent)
{
	PyObject *result;

	result = curlies_callType(&curlies_ConfigNodeType, NULL, NULL);
	if (result == NULL)
		return NULL;

	if (!ConfigNode_Check(result)) {
		PyErr_SetString(PyExc_RuntimeError, "cannot create ConfigNode object");
		result = NULL;
	} else {
		__ConfigNode_attach((curlies_ConfigNode *) result, parent->config_object, node);
	}

	return (PyObject *) result;
}

/*
 * __str__()
 */
PyObject *
ConfigNode_str(curlies_ConfigNode *self)
{
	curly_node_t *node;
	char buf1[256], buf2[4096];
	const char *type, *name, *path;
	unsigned int line;

	if (!__check_node(self))
		return NULL;

	node = self->node;
	type = curly_node_type(node);
	name = curly_node_name(node);

	if (name)
		snprintf(buf1, sizeof(buf1), "%s \"%s\" { ... }", type, name);
	else
		snprintf(buf1, sizeof(buf1), "%s { ... }", type);

	path = curly_node_get_source_file(node);
	line = curly_node_get_source_line(node);

	if (path != NULL) {
		snprintf(buf2, sizeof(buf2), "%s (defined in %s, line %u)", buf1, path, line);
		return PyUnicode_FromString(buf2);
	}
	return PyUnicode_FromString(buf1);
}


/*
 * def __iter__():
 *	return ConfigIter(self)
 */
static PyObject *
ConfigNode_iter(curlies_ConfigNode *self)
{
	PyObject *tuple, *result;

	if (!__check_node(self))
		return NULL;

	tuple = PyTuple_New(1);

	PyTuple_SetItem(tuple, 0, (PyObject *) self);
	Py_INCREF(self);

	result = curlies_callType(&curlies_NodeIteratorType, tuple, NULL);

	Py_DECREF(tuple);
	return result;
}

static PyObject *
ConfigNode_attribute_iter(curlies_ConfigNode *self)
{
	PyObject *tuple, *result;

	if (!__check_node(self))
		return NULL;

	tuple = PyTuple_New(1);

	PyTuple_SetItem(tuple, 0, (PyObject *) self);
	Py_INCREF(self);

	result = curlies_callType(&curlies_AttrIteratorType, tuple, NULL);

	Py_DECREF(tuple);
	return result;
}

static PyObject *
ConfigNode_get_children(curlies_ConfigNode *self, PyObject *args, PyObject *kwds)
{
	const char *type;

	if (!__get_single_string_arg(args, kwds, "type", &type))
		return NULL;

	if (!__check_node(self))
		return NULL;

	return __get_children(self->node, type);
}

static PyObject *
ConfigNode_get_child(curlies_ConfigNode *self, PyObject *args, PyObject *kwds)
{
	char *kwlist[] = {
		"type",
		"name",
		NULL
	};
	const char *type, *name = NULL;
	curly_node_t *child;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|s", kwlist, &type, &name))
		return NULL;

	if (!__check_node(self))
		return NULL;

	child = curly_node_get_child(self->node, type, name);
	if (child == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	return __wrap_node(child, self);
}

extern curly_node_t *      curly_node_add_child(curly_node_t *cfg, const char *type, const char *name);

static PyObject *
ConfigNode_add_child(curlies_ConfigNode *self, PyObject *args, PyObject *kwds)
{
	char *kwlist[] = {
		"type",
		"name",
		NULL
	};
	const char *type, *name = NULL;
	curly_node_t *child;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|s", kwlist, &type, &name))
		return NULL;

	if (!__check_node(self))
		return NULL;

	child = curly_node_add_child(self->node, type, name);
	if (child == NULL) {
		PyErr_Format(PyExc_SystemError, "Unable to create a %s node name \"%s\"", type, name);
		return NULL;
	}

	return __wrap_node(child, self);
}

static PyObject *
ConfigNode_drop_child(curlies_ConfigNode *self, PyObject *args, PyObject *kwds)
{
	char *kwlist[] = {
		"child",
		NULL
	};
	PyObject *childObject;
	unsigned int count;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &childObject))
		return NULL;

	if (!__check_node(self))
		return NULL;

	if (!ConfigNode_Check(childObject)) {
		PyErr_SetString(PyExc_ValueError, "Argument is not a ConfigNode instance");
		return NULL;
	}

	count = curly_node_drop_child(self->node, ((curlies_ConfigNode *) childObject)->node);
	return PyLong_FromUnsignedLong(count);
}

static PyObject *
ConfigNode_get_attributes(curlies_ConfigNode *self, PyObject *args, PyObject *kwds)
{
	if (!__check_call(self, args, kwds))
		return NULL;

	return __get_attr_names(self->node);
}

static PyObject *
ConfigNode_get_value(curlies_ConfigNode *self, PyObject *args, PyObject *kwds)
{
	const char *name;

	if (!__get_single_string_arg(args, kwds, "name", &name))
		return NULL;

	if (!__check_node(self))
		return NULL;

	return __to_string(curly_node_get_attr(self->node, name));
}

static PyObject *
ConfigNode_set_value(curlies_ConfigNode *self, PyObject *args, PyObject *kwds)
{
	char *kwlist[] = {
		"name",
		"value",
		NULL
	};
	const char *name;
	PyObject *valueObj = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "sO", kwlist, &name, &valueObj))
		return NULL;

	if (!__check_node(self))
		return NULL;

	if (PyUnicode_Check(valueObj)) {
		curly_node_set_attr(self->node, name, PyUnicode_AsUTF8(valueObj));
	} else if (PySequence_Check(valueObj)) {
		Py_ssize_t i, len = PySequence_Size(valueObj);

		if (len < 0) {
bad_seq:
			PyErr_SetString(PyExc_ValueError, "bad sequence arg");
			return NULL;
		}
		curly_node_set_attr(self->node, name, NULL);

		for (i = 0; i < len; ++i) {
			PyObject *itemObj = PySequence_GetItem(valueObj, i);

			if (itemObj == NULL)
				goto bad_seq;

			if (!PyUnicode_Check(itemObj)) {
				PyErr_Format(PyExc_ValueError, "bad value at sequence position %d - expected a str", (int) i);
				Py_DECREF(itemObj);
				return NULL;
			}
			curly_node_add_attr_list(self->node, name, PyUnicode_AsUTF8(itemObj));
			Py_DECREF(itemObj);
		}
	} else if (valueObj == Py_None) {
		curly_node_set_attr(self->node, name, NULL);
	} else {
		PyErr_SetString(PyExc_ValueError, "cannot handle values of this type");
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
ConfigNode_unset_value(curlies_ConfigNode *self, PyObject *args, PyObject *kwds)
{
	const char *name;

	if (!__get_single_string_arg(args, kwds, "name", &name))
		return NULL;

	if (!__check_node(self))
		return NULL;

	curly_node_set_attr(self->node, name, NULL);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
ConfigNode_get_values(curlies_ConfigNode *self, PyObject *args, PyObject *kwds)
{
	const char *name;

	if (!__get_single_string_arg(args, kwds, "name", &name))
		return NULL;

	if (!__check_node(self))
		return NULL;

	return __to_string_list(curly_node_get_attr_list(self->node, name));
}

/*
 * Wrapper object for curly attributes
 */
static PyObject *
ConfigAttr_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	curlies_Attr *self;

	self = (curlies_Attr *) type->tp_alloc(type, 0);
	if (self == NULL)
		return NULL;

	/* init members */
	self->node_object = NULL;
	self->attr = NULL;

	return (PyObject *)self;
}

static inline void
__ConfigAttr_attach(curlies_Attr *self, PyObject *node_object, curly_attr_t *attr)
{
	assert(self->node_object == NULL);

	self->attr = attr;
	self->node_object = node_object;
	Py_INCREF(node_object);
}

static inline void
__ConfigAttr_detach(curlies_Attr *self)
{
	self->attr = NULL;

	if (self->node_object)
		Py_DECREF(self->node_object);
	self->node_object = NULL;
}

/*
 * Initialize the iterator object
 */
static int
ConfigAttr_init(curlies_Attr *self, PyObject *args, PyObject *kwds)
{
	if (!__check_void_args(args, kwds))
		return -1;

	return 0;
}

/*
 * Destructor: clean any state inside the Config object
 */
static void
ConfigAttr_dealloc(curlies_Attr *self)
{
	__ConfigAttr_detach(self);
}

int
ConfigAttr_Check(PyObject *self)
{
	return PyType_IsSubtype(Py_TYPE(self), &curlies_ConfigAttrType);
}

PyObject *
ConfigAttr_getattro(curlies_Attr *self, PyObject *nameo)
{
	if (self->attr) {
		const char *name = PyUnicode_AsUTF8(nameo);

		if (name == NULL)
			return NULL;

		if (!strcmp(name, "name"))
			return __to_string(curly_attr_get_name(self->attr));

		if (!strcmp(name, "value"))
			return __to_string(curly_attr_get_value(self->attr, 0));

		if (!strcmp(name, "values"))
			return __to_string_list(curly_attr_get_values(self->attr));
	}

	return PyObject_GenericGetAttr((PyObject *) self, nameo);
}

static PyObject *
__wrap_attr(curly_attr_t *attr, curlies_ConfigNode *parent)
{
	PyObject *result;

	result = curlies_callType(&curlies_ConfigAttrType, NULL, NULL);
	if (result == NULL)
		return NULL;

	if (!ConfigAttr_Check(result)) {
		PyErr_SetString(PyExc_RuntimeError, "cannot create ConfigAttr object");
		result = NULL;
	} else {
		__ConfigAttr_attach((curlies_Attr *) result, parent->config_object, attr);
	}

	return (PyObject *) result;
}


/*
 * Iterator implementation
 */
static PyObject *
ConfigIter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	curlies_Iterator *self;

	self = (curlies_Iterator *) type->tp_alloc(type, 0);
	if (self == NULL)
		return NULL;

	/* init members */
	self->node_object = NULL;
	self->iter = NULL;

	return (PyObject *)self;
}

static inline void
__ConfigIter_attach(curlies_Iterator *self, PyObject *node_object, curly_iter_t *iter)
{
	assert(self->node_object == NULL);

	self->iter = iter;
	self->node_object = node_object;
	Py_INCREF(node_object);
}

static inline void
__ConfigIter_detach(curlies_Iterator *self)
{
	if (self->iter)
		curly_iter_free(self->iter);
	self->iter = NULL;

	if (self->node_object)
		Py_DECREF(self->node_object);
	self->node_object = NULL;
}

/*
 * Initialize the iterator object
 */
static int
ConfigIter_init(curlies_Iterator *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {
		"config",
		NULL
	};
	PyObject *node_object = NULL;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &node_object))
		return -1;

	if (node_object) {
		curly_node_t *node;
		curly_iter_t *iter;

		if (!(node = ConfigNode_GetPointer(node_object)))
			return -1;

		if (!(iter = curly_node_iterate(node))) {
			PyErr_SetString(PyExc_RuntimeError, "unable to create iterator for ConfigNode");
			return -1;
		}

		/* printf("Attach node %s iter %p\n", curly_node_name(node), iter); */
		__ConfigIter_attach(self, node_object, iter);
	}

	return 0;
}

/*
 * Destructor: clean any state inside the Config object
 */
static void
ConfigIter_dealloc(curlies_Iterator *self)
{
	__ConfigIter_detach(self);
}

int
ConfigIter_Check(PyObject *self)
{
	return PyType_IsSubtype(Py_TYPE(self), &curlies_NodeIteratorType);
}

PyObject *
ConfigIter_iter(curlies_Iterator *self)
{
	Py_INCREF(self);
	return (PyObject *) self;
}

PyObject *
ConfigNodeIter_iternext(curlies_Iterator *self)
{
	curly_node_t *node = NULL;

	if (self->iter)
		node = curly_iter_next_node(self->iter);

	//printf("Next child for iter %p is %p\n", self->iter, node);
	if (node == NULL) {
		PyErr_SetString(PyExc_StopIteration, "stop");
		return NULL;
	}

	return __wrap_node(node, (curlies_ConfigNode *) self->node_object);
}

PyObject *
ConfigAttrIter_iternext(curlies_Iterator *self)
{
	curly_attr_t *attr = NULL;

	if (self->iter)
		attr = curly_iter_next_attr(self->iter);

	//printf("Next child for iter %p is %p\n", self->iter, attr);
	if (attr == NULL) {
		PyErr_SetString(PyExc_StopIteration, "stop");
		return NULL;
	}

	return __wrap_attr(attr, (curlies_ConfigNode *) self->node_object);
}
