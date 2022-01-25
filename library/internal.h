/*
 * Handling for twopence config files.
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


#ifndef CURLIES_INTERNAL_H
#define CURLIES_INTERNAL_H

#include "config.h"

typedef struct curly_shared_string curly_shared_string_t;
typedef struct curly_origin curly_origin_t;

struct curly_shared_string {
	char *		value;
	unsigned int	refcount;
};

struct curly_origin {
	curly_shared_string_t *path;
	unsigned int	line;
};


#define CURLIES_NODE_SHORTLIST_MAX	2
struct curly_attr {
	curly_attr_t *	next;
	char *		name;

	unsigned int	nvalues;
	char **		values;
	char *		short_list[CURLIES_NODE_SHORTLIST_MAX+1];
};

struct curly_node {
	curly_node_t *	next;

	curly_origin_t	origin;

	/* The group's type (eg "node") and name (eg "client", "server") */
	char *		type;
	char *		name;

	/* Attributes */
	curly_attr_t *	attrs;

	/* Attach active iterators here */
	curly_iter_t *	iterators;

	curly_node_t *	children;
};

struct curly_iter {
	curly_iter_t *	chain;

	bool		valid;

	curly_node_t *	node;
	curly_node_t *	next_item;
	curly_attr_t *	next_attr;
};

extern curly_node_t *	curly_parse(const char *filename);
extern void		curly_write(const curly_node_t *cfg, const char *filename);
extern void		curly_print(const curly_node_t *cfg, FILE *fp);

extern void		curly_origin_init(curly_origin_t *, const char *path);
extern void		curly_origin_set(curly_origin_t *dst, curly_shared_string_t *fo, unsigned int line);
extern void		curly_origin_destroy(curly_origin_t *dst);

#endif /* CURLIES_INTERNAL_H */
