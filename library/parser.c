/*
 * libcurly parser
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <libgen.h>

#include "curlies.h"
#include "internal.h"

typedef struct curly_file curly_file_t;
typedef struct curly_parser curly_parser_t;

curly_file_t *	curly_file_open(const char *filename, const char *mode);
void		curly_file_close(curly_file_t *file);
static bool	curly_parse_include(curly_parser_t *p, const char *filename, curly_node_t *cfg);


typedef enum {
	Error = -1,
	EndOfFile = 0,

	Identifier,
	StringConstant,
	NumberConstant,
	LeftBrace,
	RightBrace,
	Semicolon,
	Comma,
} curly_token_t;


void		curly_parser_pushback(curly_parser_t *, curly_token_t);
void		curly_parser_error(curly_parser_t *, const char *);
curly_token_t	curly_parser_get_token(curly_parser_t *parser, char **token_string);
const char *	curly_token_name(curly_token_t token);

struct curly_file {
	unsigned int	lineno;
	char *		name;
	FILE *		h;
};

struct curly_parser {
	curly_file_t *	file;

	bool		error;
	bool		trace;

	curly_token_t	save;

	char *		pos;
	char		linebuf[1024];
	char		toknbuf[1024];
};

void
curly_parser_init(curly_parser_t *parser, curly_file_t *file)
{
	memset(parser, 0, sizeof(*parser));
	parser->file = file;
}

void
curly_parser_destroy(curly_parser_t *parser)
{
	if (parser->file) {
		curly_file_close(parser->file);
		parser->file = NULL;
	}
	memset(parser, 0, sizeof(*parser));
}

static inline void
save_string(char **var, const char *value)
{
	if (*var) {
		free(*var);
		*var = NULL;
	}
	if (value)
		*var = strdup(value);
}

bool
curly_parser_do(curly_parser_t *p, curly_node_t *cfg)
{
	curly_token_t tok;
	char *value;

	while ((tok = curly_parser_get_token(p, &value)) != EndOfFile) {
		char *identifier = NULL, *name = NULL;
		curly_node_t *subgroup;

		switch (tok) {
		case Error:
			return false;

		case Semicolon:
			/* empty statement */
			continue;

		case RightBrace:
			curly_parser_pushback(p, tok);
			return true;

		case Identifier:
			/* Just what we expect */
			break;

		default:
			goto unexpected_token_error;
		}

		/* include "blah.conf"; */
		if (!strcmp(value, "include")) {
			tok = curly_parser_get_token(p, &value);
			if (tok != Identifier && tok != StringConstant)
				goto unexpected_token_error;

			save_string(&name, value);
			if (curly_parser_get_token(p, &value) != Semicolon)
				goto unexpected_token_error;

			if (!curly_parse_include(p, name, cfg)) {
				curly_parser_error(p, "unable to process include statement");
				save_string(&name, NULL);
				return false;
			}
			save_string(&name, NULL);
			continue;
		}

		save_string(&identifier, value);

		tok = curly_parser_get_token(p, &value);
		if (tok == LeftBrace) {
			/* identifier { ... group ... } */
		} else
		if (tok == Identifier || tok == StringConstant || tok == NumberConstant) {
			save_string(&name, value);

			tok = curly_parser_get_token(p, &value);
		} else {
			goto unexpected_token_error;
		}

		switch (tok) {
		case Error:
			return false;

		case Semicolon:
			/* identifier value ";" */
			curly_node_set_attr(cfg, identifier, name);
			break;

		case LeftBrace:
			/* identifier { ... }
			 * identifier name { ... } */
			subgroup = curly_node_add_child(cfg, identifier, name);
			if (subgroup == NULL) {
				curly_parser_error(p, "unable to create subgroup");
				return false;
			}

			if (!curly_parser_do(p, subgroup))
				return false;

			tok = curly_parser_get_token(p, &value);
			if (tok != RightBrace) {
				curly_parser_error(p, "missing closing brace");
				return false;
			}
			break;

		case Comma:
			/* identifier value, value, ... */
			curly_node_add_attr_list(cfg, identifier, name);

			while (tok == Comma) {
				tok = curly_parser_get_token(p, &value);
				if (tok == Identifier || tok == StringConstant || tok == NumberConstant) {
					curly_node_add_attr_list(cfg, identifier, value);
					tok = curly_parser_get_token(p, &value);
				} else {
					/* We could be more liberal here and accept things like
					 *   colors	red, green, blue, ;
					 *   food {
					 *      flavors	bland, spicy, salty
					 *   }
					 *
					 * ie excess commas, or missing commas at the end
					 * of a group.
					 */
					goto unexpected_token_error;
				}
			}

			if (tok == RightBrace) {
				curly_parser_pushback(p, tok);
				break;
			}
			if (tok != Semicolon)
				goto unexpected_token_error;
			break;

		default:
			goto unexpected_token_error;
		}

		save_string(&identifier, NULL);
		save_string(&name, NULL);
	}

	return true;

unexpected_token_error:
	curly_parser_error(p, "unexpected token");
	return false;
}

static bool
__curly_parse(const char *filename, curly_node_t *cfg)
{
	curly_parser_t parser;
	curly_file_t *file;
	bool rv = true;

	if (!(file = curly_file_open(filename, "r")))
		return false;

	curly_parser_init(&parser, file);
	//parser.trace = true;
	rv = curly_parser_do(&parser, cfg);
	curly_parser_destroy(&parser);

	return rv;
}

curly_node_t *
curly_parse(const char *filename)
{
	curly_node_t *cfg;

	cfg = curly_node_new();
	if (!__curly_parse(filename, cfg)) {
		curly_node_free(cfg);
		cfg = NULL;
	}

	return cfg;
}

static const char *
__curly_resolve_include(curly_parser_t *p, const char *filename)
{
	static char resolved[PATH_MAX];
	char pathbuf[PATH_MAX];
	char *pathcopy = NULL, *dir = NULL;

	if (filename[0] == '/')
		return filename;

	save_string(&pathcopy, p->file->name);
	dir = dirname(pathcopy);

	if (dir == NULL)
		dir = ".";
	snprintf(pathbuf, sizeof(pathbuf), "%s/%s", dir, filename);

	if (p->trace)
		fprintf(stderr, "### include \"%s\" from \"%s\" -> \"%s\"\n", filename, p->file->name, pathbuf);
	save_string(&pathcopy, NULL);

	if (realpath(pathbuf, resolved) == NULL) {
		fprintf(stderr, "Error: Cannot resolve include file \"%s\": %m\n", pathbuf);
		return NULL;
	}

	return resolved;
}

static bool
curly_parse_include(curly_parser_t *p, const char *filename, curly_node_t *cfg)
{
	const char *include_path;

	include_path = __curly_resolve_include(p, filename);
	if (include_path == NULL)
		return false;

	if (p->trace)
		fprintf(stderr, "### including \"%s\"\n", include_path);

	return __curly_parse(include_path, cfg);
}

void
__curly_print(const curly_node_t *cfg, FILE *fp, int indent)
{
	const curly_attr_t *attr;
	const curly_node_t *child;

	for (attr = cfg->attrs; attr; attr = attr->next) {
		unsigned int n = 0;

		fprintf(fp, "%*.*s%-12s ",
				indent, indent, "",
				attr->name);
		for (n = 0; n < attr->nvalues; ++n)  {
			if (n)
				fprintf(fp, ",\n%*.*s", indent + 13, indent + 13, "");
			fprintf(fp, " \"%s\"", attr->values[n]);
		}
		fprintf(fp, ";\n");
	}

	for (child = cfg->children; child; child = child->next) {
		if (child->name) {
			fprintf(fp, "%*.*s%s \"%s\" {\n",
					indent, indent, "",
					child->type,
					child->name);
		} else {
			fprintf(fp, "%*.*s%s {\n",
					indent, indent, "",
					child->type);
		}

		__curly_print(child, fp, indent + 4);
		fprintf(fp, "%*.*s}\n",
				indent, indent, "");
	}
}

void
curly_print(const curly_node_t *cfg, FILE *fp)
{
	__curly_print(cfg, fp, 0);
}

curly_file_t *
curly_file_open(const char *filename, const char *mode)
{
	curly_file_t *file;
	FILE *fp;

	if (!(fp = fopen(filename, mode)))
		return NULL;

	file = calloc(1, sizeof(*file));
	file->name = strdup(filename);
	file->h = fp;
	return file;
}

void
curly_file_close(curly_file_t *file)
{
	if (file->name)
		free(file->name);
	if (file->h)
		fclose(file->h);
	free(file);
}

int
curly_file_gets(char *buffer, size_t size, curly_file_t *file)
{
	unsigned int offset = 0, k;

	buffer[0] = '\0';
	while (offset == 0 || buffer[offset-1] == '\\') {
		if (offset)
			buffer[offset-1] = ' ';

		if (!fgets(buffer + offset, size - offset, file->h))
			break;

		file->lineno++;

		/* If we parsed a continuation line, collapse the
		 * leading white space */
		if (offset && isspace(buffer[offset])) {
			for (k = offset; isspace(buffer[offset]); ++offset)
				;
			do {
				buffer[k++] = buffer[offset++];
			} while (buffer[offset]);
			buffer[k] = '\0';
		}

		offset = strlen(buffer);
		if (offset && buffer[offset-1] == '\n')
			buffer[--offset] = '\0';
	}

	return offset != 0;
}

int
curly_parser_fillbuf(curly_parser_t *parser)
{
	return curly_file_gets(parser->linebuf, sizeof(parser->linebuf), parser->file);
}

static char *
__curly_parser_current(curly_parser_t *parser)
{
	while (parser->pos == NULL) {
		if (!curly_parser_fillbuf(parser))
			return NULL;

		if (parser->trace)
			fprintf(stderr, "### ---- new buffer: \"%s\"\n", parser->linebuf);
		parser->pos = parser->linebuf;
	}
	return parser->pos;
}

void
curly_parser_skip_ws(curly_parser_t *parser)
{
	while (1) {
		char *pos;

		if ((pos = __curly_parser_current(parser)) == NULL)
			return;

		pos = parser->pos;
		while (isspace(*pos))
			++pos;

		if (*pos && *pos != '#') {
			parser->pos = pos;
			break;
		}

		/* Hit a comment, discard the rest of the line */
		parser->pos = NULL;
	}
}

void
curly_parser_pushback(curly_parser_t *parser, curly_token_t token)
{
	if (parser->trace)
		fprintf(stderr, "### pushback token %s(%u)\n", curly_token_name(token), token);
	if (parser->save) {
		curly_parser_error(parser, "Trying to push back more than one token - no workee");
	}
	parser->save = token;
}

curly_token_t
curly_parser_get_token(curly_parser_t *parser, char **token_string)
{
	curly_token_t token;
	char *pos, *dst;

	if (parser->error)
		return Error;

	if (parser->save != EndOfFile) {
		token = parser->save;
		*token_string = parser->toknbuf;
		parser->save = EndOfFile;

		return token;
	}

	curly_parser_skip_ws(parser);

	if ((pos = __curly_parser_current(parser)) == NULL) {
		*token_string = NULL;
		return EndOfFile;
	}

	dst = parser->toknbuf;
	if (isalnum(*pos)) {
		while (isalnum(*pos) || (*pos && strchr("_.:/-", *pos)))
			*dst++ = *pos++;

		token = Identifier;
	} else
	if (isdigit(*pos)) {
		while (isdigit(*pos))
			*dst++ = *pos++;

		token = NumberConstant;
	} else
	if (*pos == '"') {
		char cc;

		++pos;
		while ((cc = *pos++) != '"') {
			if (cc == '\0') {
				curly_parser_error(parser, "missing closing double quote");
				return Error;
			}
			if (cc == '\\' && (cc = *pos++) == '\0') {
				curly_parser_error(parser, "missing closing double quote");
				return Error;
			}
			*dst++ = cc;
		}

		token = StringConstant;
	} else
	if (*pos == '{') {
		*dst++ = *pos++;
		token = LeftBrace;
	} else
	if (*pos == '}') {
		*dst++ = *pos++;
		token = RightBrace;
	} else
	if (*pos == ';') {
		*dst++ = *pos++;
		token = Semicolon;
	} else
	if (*pos == ',') {
		*dst++ = *pos++;
		token = Comma;
	} else {
		return Error;
	}
	*dst++ = '\0';

	if (parser->trace)
		fprintf(stderr, "### %s(%u) \"%s\"\n", curly_token_name(token), token, parser->toknbuf);

	*token_string = parser->toknbuf;
	parser->pos = pos;

	return token;
}

void
curly_parser_error(curly_parser_t *p, const char *msg)
{
	curly_file_t *file = p->file;

	fprintf(stderr, "%s: line %u: %s\n", file->name, file->lineno, msg);
	if (p->pos && p->linebuf <= p->pos && p->pos < p->linebuf + sizeof(p->linebuf)) {
		int hoff = p->pos - p->linebuf;
		char *cp;

		/* Convert all spaces to ' ' so that the ^-- HERE pointer doesn't get
		 * confused by TABs and the like */
		for (cp = p->linebuf; cp < p->pos; ++cp) {
			if (isspace(*cp))
				*cp = ' ';
		}

		/* Position the ^-- HERE pointer to the start of the token we just consumed */
		if (hoff >= strlen(p->toknbuf))
			hoff -= strlen(p->toknbuf);

		fprintf(stderr, "%s\n", p->linebuf);
		fprintf(stderr, "%*.*s^--- HERE\n", hoff, hoff, "");
	}
	p->error = true;
}

const char *
curly_token_name(curly_token_t token)
{
	switch (token) {
	case Error:
		return "Error";
	case EndOfFile:
		return "EndOfFile";
	case Identifier:
		return "Identifier";
	case StringConstant:
		return "StringConstant";
	case NumberConstant:
		return "NumberConstant";
	case LeftBrace:
		return "LeftBrace";
	case RightBrace:
		return "RightBrace";
	case Semicolon:
		return "Semicolon";
	case Comma:
		return "Comma";
	default:
		return "???";
	}
}

