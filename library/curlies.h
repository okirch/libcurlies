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

#ifndef CURLIES_H
#define CURLIES_H

#include <stdbool.h>

/*
 * Handling curly config files
 */
typedef struct curly_node	curly_node_t;
typedef struct curly_attr	curly_attr_t;
typedef struct curly_iter	curly_iter_t;

extern curly_node_t *		curly_node_new(void);
extern void			curly_node_free(curly_node_t *);
extern void			curly_node_copy(curly_node_t *dst, const curly_node_t *src);
extern int			curly_node_write(curly_node_t *cfg, const char *path);
extern int			curly_node_write_fp(curly_node_t *cfg, FILE *fp);
extern curly_node_t *		curly_node_read(const char *path);
extern const char *		curly_node_name(const curly_node_t *cfg);
extern const char *		curly_node_type(const curly_node_t *cfg);
extern curly_node_t *		curly_node_get_child(const curly_node_t *cfg, const char *type, const char *name);
extern curly_node_t *		curly_node_add_child(curly_node_t *cfg, const char *type, const char *name);
extern unsigned int		curly_node_drop_child(curly_node_t *cfg, const curly_node_t *child);
extern const char **		curly_node_get_children(const curly_node_t *, const char *type);
extern const char **		curly_node_get_attr_names(const curly_node_t *);
extern void			curly_node_set_attr(curly_node_t *cfg, const char *name, const char *value);
extern void			curly_node_set_attr_list(curly_node_t *cfg, const char *name, const char * const *value);
extern void			curly_node_add_attr_list(curly_node_t *cfg, const char *name, const char *value);
extern const char *		curly_node_get_attr(curly_node_t *cfg, const char *name);
extern const char * const *	curly_node_get_attr_list(curly_node_t *cfg, const char *name);

extern curly_iter_t *		curly_node_iterate(curly_node_t *);
extern curly_node_t *		curly_iter_next_node(curly_iter_t *);
extern curly_attr_t *		curly_iter_next_attr(curly_iter_t *);
extern void			curly_iter_free(curly_iter_t *);

extern const char *		curly_attr_get_name(const curly_attr_t *);
extern unsigned int		curly_attr_get_count(const curly_attr_t *);
extern const char *		curly_attr_get_value(const curly_attr_t *, unsigned int);
extern const char * const *	curly_attr_get_values(const curly_attr_t *);

extern int			curly_node_format_from_string(const char *s);
extern const char *		curly_node_format_to_string(int fmt);

extern const char *		curly_node_get_source_file(const curly_node_t *);
extern unsigned int		curly_node_get_source_line(const curly_node_t *);

#endif /* CURLIES_H */
