// parser.c
#include "parser.h"
#include <dirent.h>

const string OUTDIR = "outdir=";
const string WILDSEP = "/*";
const string DEFTARGET = "default";

//	Typedefs
/*	structure representing a target in Doxy2MD. */
struct target_s {
	string target;		/**< Target name (e.g., "sigcore") */
	list sources; 		/**< List of source files (e.g., "sigcore.h") */
	string outdir;		/**< Output directory (e.g., "docs/") */
	int is_redirect;	/**< Flag: 1 if this redirects to another target, 0 otherwise */
};
typedef struct target_s* target;
/* structure representing a target index */
struct index_s {
	string name;		/**< Target name */
	long offset;		/**< File offset to target line */
};
typedef struct index_s* target_index;
/* structure representing the full comment */
struct comment_s {
	string brief;		/**< Short description */
	string details;	/**< Detailed description */
	list params;		/**< List of param descriptions */
	string ret;			/**< Return value description */
	string signature;	/**< Function signature */
};
typedef struct comment_s* comment;

//	Forward declarations / Function prototypes
static int index_targets(FILE*, list);
static long find_offset(list, const string);
static target parse_line(const string);
static void free_target(target);
static string trim(string);
static int process_target(target, doxy_config*);
static int process_file(const string, string_builder);
static void free_comment(comment);
static comment parse_comment(const string);
/**
 * @brief Parses a Doxy2MD file and processes its targets.
 * @param filename Name of the Doxy2MD file to parse.
 * @return 0 on success, non-zero on failure.
 */
static int parse_doxy2md(doxy_config* config) {
	int ret = 0;
	if (!config || !config->file) return 1;
	FILE* f = fopen(config->file, "r");
	if (!f) {
		fprintf(stderr, "failed to open configuration: %s\n", config->file);
		return 1;
	}
	
	list indices = List.new(10);
	if (index_targets(f, indices) != 0) {
		//	came back empty -- no targets
		printf("Configuration '%s' was empty\n", config->file);
		ret = 1;
		goto cleanup;
	}
	
	string target_name = config->target ? config->target : DEFTARGET;
	long offset = find_offset(indices, target_name);
	target t = NULL;
	char line[MAX_LINE];
	
	if (offset >= 0) {
		fseek(f, offset, SEEK_SET);
		if (fgets(line, MAX_LINE, f)) {
			t = parse_line(line);
		}
	}
	
	if (t && t->is_redirect && t->target) {
		offset = find_offset(indices, t->target);
		free_target(t);
		t = NULL;
		
		if (offset >= 0) {
			fseek(f, offset, SEEK_SET);
			if (fgets(line, MAX_LINE, f)) {
				t = parse_line(line);
			}
		}
	}
	
	if (t && !t->is_redirect) {
		ret = process_target(t, config);
		free_target(t);
	} else {
		fprintf(stderr, "%s: target '%s' not found or invalid default\n", config->file, target_name);
		if (t) free_target(t);
		ret = 1;
	}
	
cleanup:
	iterator it = Array.getIterator(indices, LIST);
	while (Iterator.hasNext(it)) {
		target_index ti = Iterator.next(it);
		Mem.free(ti->name);
		Mem.free(ti);
	}
	Iterator.free(it);
	fclose(f);
	return ret;	
}

/**
 * @brief Parses a single line from Doxy2MD into a Target.
 * @param line Line to parse (e.g., "sigcore: sigcore.h").
 * @return Allocated Target, or NULL on error.
 */
static target parse_line(const string line) {
	target t = NULL;
	string copy = Mem.alloc(strlen(line) + 1);
	strcpy(copy, line);
	
	//	split on ':'
	string colon = strchr(copy, ':');
	if (!colon) {
		//	invalid target entry
		goto EXIT;
	}
	
	*colon = '\0';
	string target_name = trim(copy);
	
	t = Mem.alloc(sizeof(struct target_s));
	if (!t) {
		goto EXIT;
	}
	
	t->sources = List.new(10);
	t->outdir = NULL;
	t->target = Mem.alloc(strlen(target_name) + 1);
	strcpy(t->target, target_name);
	t->is_redirect = 0;
	
	//	set up token iteration
	string body = trim(colon + 1);
	string token = strtok(body, " ");
	if (strcmp(t->target, "default") == 0) {
		if (token) {
			Mem.free(t->target);		//	free "default"
			t->target = Mem.alloc(strlen(token) + 1);
			strcpy(t->target, token);
			//	assumption that somehow nothin has been allocated to t->sources
			List.free(t->sources);
			t->sources = NULL;
			t->is_redirect = 1;			//	redirecting target
		} else {
			free_target(t);
			t = NULL;
			goto EXIT;
		}
	} else {
		//	parse body tokens
		int offset = 0;
		while (token) {
			if (strncmp(token, OUTDIR, (offset = strlen(OUTDIR))) == 0) {	//	outdir=
				t->outdir = Mem.alloc(strlen(token + offset) + 1);
				strcpy(t->outdir, token + offset);
			} else if (strstr(token, WILDSEP)) {										// wildcards
				string dir = Mem.alloc(strlen(token) + 1);
				strcpy(dir, token);
				char* wild = strstr(dir, WILDSEP);	// locate the wildcard separator: /*
				if (wild) *wild = '\0';					// trim separator
				
				DIR* d = opendir(dir);
				if (d) {
					struct dirent* entry;
					while((entry = readdir(d)) != NULL) {
						if (strstr(entry->d_name, ".c") || strstr(entry->d_name, ".h")) {
							string path = Mem.alloc(strlen(dir) + strlen(entry->d_name) + 2);
							snprintf(path, strlen(dir) + strlen(entry->d_name) + 2, "%s/%s", dir, entry->d_name);
							List.add(t->sources, path);
						}
					}
					closedir(d);
				} else {
					fprintf(stderr, "Failed to open directory '%s'\n", dir);
				}
				Mem.free(dir);
			} else {																			// sources
				List.add(t->sources, Mem.alloc(strlen(token) + 1));
				strcpy(List.getAt(t->sources, List.count(t->sources) - 1), token);
			}
			
			//	advance token
			token = strtok(NULL, " ");
		}
	}
	
EXIT:
	Mem.free(copy);
	return t;
}
/**
 * @brief Builds an index of targets in a Doxy2MD file.
 * @param f Open file handle to Doxy2MD.
 * @param indices List to store target_index tuples.
 * @return 0 on success, non-zero if empty or error.
 */
static int index_targets(FILE* f, list indices) {
	char line[MAX_LINE];
	long pos;
	
	while (fgets(line, MAX_LINE, f)) {
		//	skip comments and new lines
		if (line[0] == '#' || line[0] == '\n') continue;
		
		pos = ftell(f) - strlen(line);	//	start of line
		string copy = Mem.alloc(strlen(line) + 1);
		strcpy(copy, line);
		
		//	split target from body
		string colon = strchr(copy, ':');
		if (!colon) {
			//	invalid target configuration
			Mem.free(copy);
			continue;
		}
		
		*colon = '\0';
		string target_name = trim(copy);
		
		target_index ti = Mem.alloc(sizeof(struct index_s));
		//	set the target name
		ti->name = Mem.alloc(strlen(target_name) + 1);
		strcpy(ti->name, target_name);
		//	set the target line start
		ti->offset = pos;
		List.add(indices, ti);
		
		Mem.free(copy);
	}
	
	return List.count(indices) > 0 ? 0 : 1;
}
/**
 * @brief Finds a target’s file offset in the index.
 * @param indices List of target_index tuples.
 * @param name Target name to find.
 * @return File offset if found, -1 if not.
 */
static long find_offset(list indices, const string name) {
	long offset = -1;
	iterator it = Array.getIterator(indices, LIST);
	while (Iterator.hasNext(it)) {
		target_index ti = Iterator.next(it);
		if (strcmp(ti->name, name) == 0) {
			offset = ti->offset;
			goto cleanup;
		}
	}
	
cleanup:
	Iterator.free(it);
	return offset;
}
/**
 * @brief Frees a Target and its resources.
 * @param t Target to free.
 */
static void free_target(target t) {
	if (!t) return;
	if (t->sources) {
		for (int i = 0; i < List.count(t->sources); i++) {
			Mem.free(List.getAt(t->sources, i));
		}
		List.free(t->sources);
	}
	if (t->target) Mem.free(t->target);
	if (t->outdir) Mem.free(t->outdir);
	Mem.free(t);
}
/**
 * @brief Trims whitespace from a string.
 * @param str String to trim.
 * @return Trimmed string.
 */
static string trim (string str) {
	while (*str == ' ' || *str == '\t') str++;
	char* end = str + strlen(str) - 1;
	while (end > str && (*end == ' ' || *end == '\t' || *end == '\n')) end--;
	*(end + 1) = '\0';
	return str;
}
/**
 * @brief Processes a Target, generating its Markdown output.
 * @param t Target to process.
 */
static int process_target(target t, doxy_config* config) {
	int ret = 0;
	string_builder sb = StringBuilder.new(1024);
	
	printf("processing target (%s)\n", t->target);
	iterator it = Array.getIterator(t->sources, LIST);
	int i = 0;
	while (Iterator.hasNext(it)) {
		string source = Iterator.next(it);
		printf("   src[%d]=%s\n", i, source);
		ret = process_file(source, sb);
		
		if (ret != 0) {
			StringBuilder.free(sb);
			Iterator.free(it);
			goto EXIT;
		}
		++i;
	}
	Iterator.free(it);
	
	//	build output file w/ output directory
	string_builder outfile_sb = StringBuilder.new(MAX_TARGET);
	if (t->outdir) {
		StringBuilder.appendf(outfile_sb, "%s/", t->outdir);
	}
	StringBuilder.append(outfile_sb, config->output ? config->output : t->target);
	if (!config->output) StringBuilder.append(outfile_sb, ".md");
	
	//	output documentation - if an output override is not give we have to allocate the space
	string outfile = StringBuilder.toString(outfile_sb);
	
	FILE* out = fopen(outfile, "w");
	if (out) {
		StringBuilder.toStream(sb, out);
		//fprintf(out, "%s", StringBuilder.toString(sb));
		fclose(out);
	} else {
		fprintf(stderr, "Failed to open target %s\n", outfile);
		ret = 1;
	}
	
	Mem.free(outfile);
	StringBuilder.free(outfile_sb);
	StringBuilder.free(sb);

EXIT:
	return ret;
}
/**
 * @brief Processes a source file, appending Doxygen comments to the string builder.
 * @param filename Source file to process.
 * @param sb StringBuilder to append documentation to.
 */
static int process_file(const string filename, string_builder sb) {
	FILE* in = fopen(filename, "r");
	if (!in) {
		fprintf(stderr, "Failed to open source '%s'\n", filename);
		return 1;
	}
	
	printf("Processing file=%s\n", filename);
	const string type = strstr(filename, ".h") ? "header" : "source";
	StringBuilder.appendf(sb, "### *(%s)* %s\n\n", type, filename);
	
	char line[MAX_LINE];
	int in_comment = 0;
	comment c = NULL;
	
	while (fgets(line, MAX_LINE, in)) {
		string trimmed = trim(line);
		if (strncmp(trimmed, "/**", 3) == 0) {
			in_comment = 1;
			c = Mem.alloc(sizeof(struct comment_s));
			if (!c) return -1;
			
			c->brief = NULL;
			c->details = NULL;
			c->params = List.new(10);
			c->ret = NULL;
			c->signature = NULL;
			continue;
		}
		
		if (in_comment && strstr(trimmed, "*/")) {
			in_comment = 0;
			while (fgets(line, MAX_LINE, in) && trim(line)[0] == '\0'); // Skip empty lines
			if (!feof(in)) {
				string copy = Mem.alloc(strlen(trim(line)) + 1);
				strcpy(copy, trim(line));
				//	if the signature line end with a brace '{' or semi ';' remove it
				string brace = strchr(copy, '{');
				string semi = strchr(copy, ';');
				if (brace) *brace = '\0';
				if (semi) *semi = '\0';
				c->signature = Mem.alloc(strlen(trim(copy)) + 1);
				strcpy(c->signature, trim(copy));
				Mem.free(copy);
			}
			if (c->brief) {
				StringBuilder.appendf(sb, "#### %s \n\n", c->brief);
				if (c->details) StringBuilder.appendf(sb, "%s\n\n", c->details);
				if (List.count(c->params) > 0) {
					StringBuilder.append(sb, "**Parameters:**  \n");
					iterator it = Array.getIterator(c->params, LIST);
					while (Iterator.hasNext(it)) {
						string param = Iterator.next(it);
						StringBuilder.appendf(sb, "- %s\n", param);
					}
					Iterator.free(it);
					StringBuilder.append(sb, "\n");
				}
				if (c->ret) StringBuilder.appendf(sb, "**Returns:** %s  \n", c->ret);
				if (c->signature) StringBuilder.appendf(sb, "``` c\n%s\n```\n", c->signature);
			}
			free_comment(c);
			c = NULL;
			continue;
		}
		
		if (!in_comment) continue;
		
		comment temp = parse_comment(trimmed);
		if(temp) {
			if (temp->brief) { c->brief = temp->brief; temp->brief = NULL; }
			if (temp->details) { c->details = temp->details; temp->details = NULL;}
			if (List.count(temp->params) > 0) {
				iterator it = Array.getIterator(temp->params, LIST);
				while (Iterator.hasNext(it)) {
					List.add(c->params, Iterator.next(it));
				}
				Iterator.free(it);
			}
			if (temp->ret) { c->ret = temp->ret; temp->ret = NULL; }
			//	?? what about temp->signature
			//free_comment(temp);
		}
	}
	
	if (c) free_comment(c);
	fclose(in);	
	return 0;
}
/**
 * @brief Frees a Comment and its resources.
 * @param c Comment to free.
 */
static void free_comment(comment c) {
	if (!c) return;
	if (c->brief) Mem.free(c->brief);
	if (c->details) Mem.free(c->details);
	if (c->params) {
		for (int i = 0; i < List.count(c->params); i++) {
			Mem.free(List.getAt(c->params, i));
		}
		List.free(c->params);
	}
	if (c->ret) Mem.free(c->ret);
	if (c->signature) Mem.free(c->signature);
	Mem.free(c);
}
/**
 * @brief Parses a Doxygen comment line into a Comment struct.
 * @param line Line to parse (e.g., "* @brief Short desc").
 * @return Allocated Comment if valid, NULL otherwise.
 */
static comment parse_comment(const string line) {
	comment c = Mem.alloc(sizeof(struct comment_s));
	if (!c) return NULL;

	c->brief = NULL;
	c->details = NULL;
	c->params = List.new(10);
	c->ret = NULL;
	c->signature = NULL;	
	
	string trimmed = trim(line);
	if (strncmp(trimmed, "* @brief", 7) == 0) {
		c->brief = Mem.alloc(strlen(trimmed + 8) + 1);
		strcpy(c->brief, trimmed + 8);
	} else if (strncmp(trimmed, "* @details", 10) == 0) {
		c->details = Mem.alloc(strlen(trimmed + 11) + 1);
		strcpy(c->details, trimmed + 11);
	} else if (strncmp(trimmed, "* @param", 8) == 0) {
		string param = Mem.alloc(strlen(trimmed + 9) + 1);
		strcpy(param, trimmed + 9);
		List.add(c->params, param);
	} else if (strncmp(trimmed, "* @return", 9) == 0) {
		c->ret = Mem.alloc(strlen(trimmed + 10) + 1);
		strcpy(c->ret, trimmed + 10);
	}
	return c;
}

const IParser Parser = {
    .parseDoxy = parse_doxy2md
};
