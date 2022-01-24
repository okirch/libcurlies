/*
 * Handling for libcurly config files.
 * 
 * Copyright (C) 2014-2021 SUSE
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "curlies.h"
#include "internal.h"

static curly_node_t *	__curly_node_new(const char *type, const char *name);
static void		__curly_node_free(curly_node_t *cfg);
static void		__curly_attr_list_free(curly_attr_t **);
static void		__curly_attr_list_assign(curly_attr_t **, const char *, const char *);
static void		__curly_attr_list_append(curly_attr_t **, const char *, const char *);
static void		__curly_attr_list_assign_list(curly_attr_t **, const char *, const char * const *);
static void		__curly_attr_list_copy(curly_attr_t **dst, const curly_attr_t *src);
static void		__curly_attr_list_drop(curly_attr_t **, const char *);
static const char **	__curly_attr_list_get_names(curly_attr_t * const*);
static const char *	__curly_attr_list_get_string(curly_attr_t **, const char *);
static const char * const *__curly_attr_list_get_list_value(curly_attr_t **, const char *);
static curly_attr_t *	__curly_attr_new(const char *name);
static curly_attr_t *	__curly_attr_clone(const curly_attr_t *src_attr);
static void		__curly_attr_free(curly_attr_t *attr);
static void		__curly_attr_clear(curly_attr_t *attr);

static inline int
xstrcmp(const char *a, const char *b)
{
	if (a == NULL || b == NULL)
		return a == b;
	return strcmp(a, b);
}

/*
 * Constructor
 */
curly_node_t *
curly_node_new(void)
{
	return __curly_node_new("root", NULL);
}

static curly_node_t *
__curly_node_new(const char *type, const char *name)
{
	curly_node_t *cfg;

	cfg = (curly_node_t *) calloc(1, sizeof(*cfg));
	cfg->type = type? strdup(type) : NULL;
	cfg->name = name? strdup(name) : NULL;
	return cfg;
}

/*
 * Destructor
 */
void
curly_node_free(curly_node_t *cfg)
{
	__curly_node_free(cfg);
}

static void
__curly_node_clear(curly_node_t *cfg)
{
	curly_node_t *child;

	/* This function clears out all children and attributes,
	 * but leaves the type/name information intact
	 */
	while ((child = cfg->children) != NULL) {
		cfg->children = child->next;
		curly_node_free(child);
	}

	__curly_attr_list_free(&cfg->attrs);
}

static void
__curly_node_free(curly_node_t *cfg)
{
	__curly_node_clear(cfg);

	if (cfg->type)
		free(cfg->type);
	cfg->type = NULL;

	if (cfg->name)
		free(cfg->name);
	cfg->name = NULL;

	free(cfg);
}

const char *
curly_node_name(const curly_node_t *cfg)
{
	return cfg->name;
}

const char *
curly_node_type(const curly_node_t *cfg)
{
	return cfg->type;
}

static void
__curly_node_attach_iterator(curly_node_t *cfg, curly_iter_t *iter)
{
	iter->chain = cfg->iterators;
	cfg->iterators = iter;
}

static void
__curly_node_detach_iterator(curly_node_t *cfg, const curly_iter_t *iter)
{
	curly_iter_t **pos, *rover;

	for (pos = &cfg->iterators; (rover = *pos) != NULL; pos = &rover->chain) {
		if (rover == iter) {
			*pos = rover->chain;
			rover->chain = NULL;
			return;
		}
	}
}

static void
__curly_node_invalidate_iterators(curly_node_t *cfg, const curly_node_t *child)
{
	curly_iter_t *iter;

	for (iter = cfg->iterators; iter; iter = iter->chain) {
		iter->next_item = NULL;
		iter->valid = false;
	}
}

/*
 * Accessor functions for child nodes
 */
curly_node_t *
curly_node_get_child(const curly_node_t *cfg, const char *type, const char *name)
{
	curly_node_t *child;

	for (child = cfg->children; child; child = child->next) {
		if (type && xstrcmp(child->type, type))
			continue;
		if (name && xstrcmp(child->name, name))
			continue;
		return child;
	}
	return NULL;
}


curly_node_t *
curly_node_add_child(curly_node_t *cfg, const char *type, const char *name)
{
	curly_node_t *child, **pos;

	if (curly_node_get_child(cfg, type, name) != NULL) {
		fprintf(stderr, "duplicate %s group named \"%s\"\n", type, name);
		return NULL;
	}

	/* Find the tail of the list */
	for (pos = &cfg->children; (child = *pos) != NULL; pos = &child->next)
		;

	*pos = child = __curly_node_new(type, name);
	return child;
}

unsigned int
curly_node_drop_child(curly_node_t *cfg, const curly_node_t *child)
{
	curly_node_t *cur, **pos;
	unsigned int count = 0;

	__curly_node_invalidate_iterators(cfg, child);

	for (pos = &cfg->children; (cur = *pos) != NULL; ) {
		if (cur == child) {
			*pos = cur->next;
			cur->next = NULL;

			__curly_node_free(cur);
			count += 1;
		} else {
			pos = &cur->next;
		}
	}

	return count;
}

const char **
curly_node_get_children(const curly_node_t *cfg, const char *type)
{
	const curly_node_t *node;
	unsigned int n, count;
	const char **result;

	for (count = 0, node = cfg->children; node; node = node->next, ++count)
		;

	result = calloc(count + 1, sizeof(result[0]));
	for (n = 0, node = cfg->children; node; node = node->next) {
		if (type == NULL || !xstrcmp(node->type, type))
			result[n++] = node->name;
	}
	result[n++] = NULL;

	return result;
}


const char **
curly_node_get_attr_names(const curly_node_t *cfg)
{
	return __curly_attr_list_get_names(&cfg->attrs);
}

/*
 * Copy all attributes and children from one config node to another
 */
void
curly_node_copy(curly_node_t *dst, const curly_node_t *src)
{
	const curly_node_t *src_child;
	curly_node_t **pos;

	__curly_node_clear(dst);
	__curly_attr_list_copy(&dst->attrs, src->attrs);

	pos = &dst->children;
	for (src_child = src->children; src_child; src_child = src_child->next) {
		curly_node_t *clone;

		/* Recursively create a deep copy of the child node */
		clone = __curly_node_new(src_child->type, src_child->name);
		curly_node_copy(clone, src_child);

		/* Append to list */
		*pos = clone;
		pos = &clone->next;
	}
}

/*
 * Attribute accessors
 */
void
curly_node_set_attr(curly_node_t *cfg, const char *name, const char *value)
{
	__curly_attr_list_assign(&cfg->attrs, name, value);
}

void
curly_node_set_attr_list(curly_node_t *cfg, const char *name, const char * const *values)
{
	__curly_attr_list_assign_list(&cfg->attrs, name, values);
}

void
curly_node_add_attr_list(curly_node_t *cfg, const char *name, const char *value)
{
	__curly_attr_list_append(&cfg->attrs, name, value);
}

const char *
curly_node_get_attr(curly_node_t *cfg, const char *name)
{
	return __curly_attr_list_get_string(&cfg->attrs, name);
}

const char * const *
curly_node_get_attr_list(curly_node_t *cfg, const char *name)
{
	return __curly_attr_list_get_list_value(&cfg->attrs, name);
}

static curly_attr_t *
__curly_attr_list_get_attr(curly_attr_t **list, const char *name, int create)
{
	curly_attr_t **pos, *attr;

	for (pos = list; (attr = *pos) != NULL; pos = &attr->next) {
		if (!strcmp(attr->name, name))
			return attr;
	}

	if (!create)
		return NULL;

	*pos = __curly_attr_new(name);
	return *pos;
}

static void
__curly_attr_list_drop(curly_attr_t **list, const char *name)
{
	curly_attr_t **pos, *attr;

	for (pos = list; (attr = *pos) != NULL; pos = &attr->next) {
		if (!strcmp(attr->name, name)) {
			*pos = attr->next;
			__curly_attr_free(attr);
			return;
		}
	}
}

static void
__curly_attr_append(curly_attr_t *attr, const char *value)
{
	char *s;

	if (attr->nvalues >= CURLIES_NODE_SHORTLIST_MAX) {
		unsigned int new_size;

		new_size = (attr->nvalues + 2) * sizeof(char *);
		if (attr->values == attr->short_list) {
			attr->values = malloc(new_size);
			memcpy(attr->values, attr->short_list, sizeof(attr->short_list));
		} else {
			attr->values = realloc(attr->values, new_size);
		}
	}

	attr->values[attr->nvalues++] = s = strdup(value);
	attr->values[attr->nvalues] = NULL;

	/* Replace newlines with a blank */
	while ((s = strchr(s, '\n')) != NULL)
		*s = ' ';
}

void
__curly_attr_list_assign(curly_attr_t **list, const char *name, const char *value)
{
	curly_attr_t *attr;

	if (value == NULL || *value == '\0') {
		__curly_attr_list_drop(list, name);
	} else {
		attr = __curly_attr_list_get_attr(list, name, 1);
		__curly_attr_clear(attr);
		__curly_attr_append(attr, value);
	}
}

void
__curly_attr_list_assign_list(curly_attr_t **attr_list, const char *name, const char * const *values)
{
	curly_attr_t *attr;

	if (values == NULL || *values == NULL) {
		__curly_attr_list_drop(attr_list, name);
	} else {
		attr = __curly_attr_list_get_attr(attr_list, name, 1);
		__curly_attr_clear(attr);

		while (values && *values)
			__curly_attr_append(attr, *values++);
	}
}

void
__curly_attr_list_append(curly_attr_t **attr_list, const char *name, const char *value)
{
	curly_attr_t *attr;

	attr = __curly_attr_list_get_attr(attr_list, name, 1);
	if (value == NULL)
		return;

	__curly_attr_append(attr, value);
}

const char *
__curly_attr_list_get_string(curly_attr_t **list, const char *name)
{
	curly_attr_t *attr;

	attr = __curly_attr_list_get_attr(list, name, 0);
	if (attr && attr->nvalues)
		return attr->values[0];
	return NULL;
}

const char * const *
__curly_attr_list_get_list_value(curly_attr_t **attr_list, const char *name)
{
	curly_attr_t *attr;

	attr = __curly_attr_list_get_attr(attr_list, name, 0);
	if (attr && attr->nvalues)
		return (const char * const *) attr->values;
	return NULL;
}

void
__curly_attr_list_copy(curly_attr_t **dst, const curly_attr_t *src_attr)
{
	__curly_attr_list_free(dst);

	while (src_attr != NULL) {
		*dst = __curly_attr_clone(src_attr);
		src_attr = src_attr->next;
		dst = &(*dst)->next;
	}
}

const char **
__curly_attr_list_get_names(curly_attr_t * const*list)
{
	curly_attr_t *attr;
	unsigned int n, count = 0;
	const char **result;

	for (attr = *list, count = 0; attr; attr = attr->next, ++count)
		;

	result = calloc(count + 1, sizeof(*result));
	for (attr = *list, n = 0; attr; attr = attr->next) {
		/* assert(n < count); */
		result[n++] = attr->name;
	}
	result[n] = NULL;

	return result;
}

static void
__curly_attr_clear(curly_attr_t *attr)
{
	unsigned int n;

	for (n = 0; n < attr->nvalues; ++n)
		free(attr->values[n]);
	if (attr->values != attr->short_list)
		free(attr->values);
	attr->values = attr->short_list;
	attr->nvalues = 0;
}

static curly_attr_t *
__curly_attr_new(const char *name)
{
	curly_attr_t *attr;

	attr = calloc(1, sizeof(*attr));
	attr->name = strdup(name);
	attr->values = attr->short_list;
	return attr;
}

static curly_attr_t *
__curly_attr_clone(const curly_attr_t *src_attr)
{
	curly_attr_t *attr;
	char **values;

	attr = __curly_attr_new(src_attr->name);

	values = src_attr->values;
	while (values && *values)
		__curly_attr_append(attr, *values++);
	return attr;
}

static void
__curly_attr_free(curly_attr_t *attr)
{
	free(attr->name);
	__curly_attr_clear(attr);
	free(attr);
}

void
__curly_attr_list_free(curly_attr_t **list)
{
	curly_attr_t *attr;

	while ((attr = *list) != NULL) {
		*list = attr->next;

		__curly_attr_free(attr);
	}
}

/*
 * Public attr accessor functions
 * (used when iterating over a node's attributes)
 */
const char *
curly_attr_get_name(const curly_attr_t *attr)
{
	return attr->name;
}

unsigned int
curly_attr_get_count(const curly_attr_t *attr)
{
	return attr->nvalues;
}

const char *
curly_attr_get_value(const curly_attr_t *attr, unsigned int i)
{
	if (i >= attr->nvalues)
		return NULL;
	return attr->values[i];
}

const char * const *
curly_attr_get_values(const curly_attr_t *attr)
{
	return (const char * const *) attr->values;
}

/*
 * Iterate over curly nodes
 */
curly_iter_t *
curly_node_iterate(curly_node_t *node)
{
	curly_iter_t *iter;

	iter = calloc(1, sizeof(*iter));

	/* Attach to iterator */
	__curly_node_attach_iterator(node, iter);

	/* Prime the next value */
	iter->next_item = node->children;
	iter->valid = true;

	return iter;
}

curly_node_t *
curly_iter_next_node(curly_iter_t *iter)
{
	curly_node_t *item;

	if (!iter->valid)
		return NULL;

	if ((item = iter->next_item) != NULL)
		iter->next_item = item->next;

	return item;
}

void
curly_iter_free(curly_iter_t *iter)
{
	if (iter->node) {
		__curly_node_detach_iterator(iter->node, iter);
	}
}

/*
 * I/O routines
 */
int
__curly_node_write(curly_node_t *cfg, const char *path)
{
	FILE *fp;

	if ((fp = fopen(path, "w")) == NULL) {
		fprintf(stderr, "Unable to open %s: %m\n", path);
		return -1;
	}

	curly_print(cfg, fp);
	fclose(fp);
	return 0;
}

curly_node_t *
__curly_node_read(const char *path)
{
	curly_node_t *cfg;

	cfg = curly_parse(path);
	return cfg;
}

int
curly_node_write(curly_node_t *cfg, const char *path)
{
	return __curly_node_write(cfg, path);
}

int
curly_node_write_fp(curly_node_t *cfg, FILE *fp)
{
	curly_print(cfg, fp);
	return 0;
}

curly_node_t *
curly_node_read(const char *path)
{
	return __curly_node_read(path);
}

