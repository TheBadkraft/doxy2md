// parser.c
#include "parser.h"
#include <dirent.h>
#include <ctype.h>

const string OUTDIR = "outdir=";
const string WILDSEP = "/*";
const char O_PAREN = '(';
const string DOXTAG = "/**";
const string DEFTRGET = "default";
const string DOXFILE = "* @file";
const string DOXBRIEF = "* @brief";
const string DOXDTAIL = "* @details";
const string DOXPARAM = "* @param";
const string DOXRETRN = "* @return";

static int IS_DEBUG = 0;

//	Typedefs
enum tag_type {
	NONE,
	BRIEF,
	DTAIL,
	PARAM,
	RETRN
};
typedef enum tag_type TAGTYPE;
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


//	Forward declarations / Function prototypes
static int doxy_index_targets(FILE*, list);
static long find_offset(list, const string);
static target doxy_parse_line(const string);
static void free_target(target);
static int process_target(target, list, doxy_config*);
static int process_file(const string, list);
static comment init_comment();
static string parse_comment_line(const string, TAGTYPE*);
static string extract_signature(FILE*, string*, string*);

//	to md_generator ???
//static void append_comment_to_md(string_builder, comment);

/**
 * @brief Parses a Doxy2MD file and processes its targets.
 * @param filename Name of the Doxy2MD file to parse.
 * @return 0 on success, non-zero on failure.
 */
static list parse_doxy2md(doxy_config* config) {
	list comments = NULL;
	if (!config || !config->file) return comments;
	IS_DEBUG = config->is_debug;
	
	
	comments = List.new(100);
	if (config->sources && List.count(config->sources) > 0) {
		printf("IS_DEBUG=%s\n", IS_DEBUG ? "TRUE" : "FALSE");
		printf("cfg.file=%s\n", config->file);
		printf("cfg.output=%s\n", config->output);
		printf("cfg.target=%s\n", config->target);
		printf("cfg.template=%s\n", config->template);
		printf("cfg.sources:\n");
		iterator s_it = Array.getIterator(config->sources, LIST);
		while (Iterator.hasNext(s_it)) printf("   src=%s\n", (string)Iterator.next(s_it));
		Iterator.free(s_it);
		
		//	use provided sources directly
		process_target(NULL, comments, config);
	} else {
		FILE* f = fopen(config->file, "r");
		if (!f) {
			fprintf(stderr, "failed to open configuration: %s\n", config->file);
			return comments;
		}
		
		list indices = List.new(10);
		if (doxy_index_targets(f, indices) != 0) {
			//	came back empty -- no targets
			printf("Configuration '%s' was empty\n", config->file);
			List.free(comments);
			comments = NULL;
			
			goto cleanup;
		}
		
		string target_name = config->target ? config->target : DEFTRGET;
		long offset = find_offset(indices, target_name);
		target t = NULL;
		char line[MAX_LINE];
		
		if (offset >= 0) {
			fseek(f, offset, SEEK_SET);
			if (fgets(line, MAX_LINE, f)) {
				t = doxy_parse_line(line);
			}
		}
		
		if (t && t->is_redirect && t->target) {
			offset = find_offset(indices, t->target);
			free_target(t);
			t = NULL;
			
			if (offset >= 0) {
				fseek(f, offset, SEEK_SET);
				if (fgets(line, MAX_LINE, f)) {
					t = doxy_parse_line(line);
				}
			}
		}
		
		if (t && !t->is_redirect) {
			process_target(t, comments, config);
			free_target(t);
		} else {
			fprintf(stderr, "%s: target '%s' not found or invalid default\n", config->file, target_name);
			if (t) free_target(t);
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
	}
	
	return comments;	
}

/**
 * @brief Parses a single line from Doxy2MD into a Target.
 * @param line Line to parse (e.g., "sigcore: sigcore.h").
 * @return Allocated Target, or NULL on error.
 */
static target doxy_parse_line(const string line) {
	target t = NULL;
	string copy = Mem.alloc(strlen(line) + 1);
	strcpy(copy, line);
	
	//	split on ':'
	char* colon = strchr(copy, ':');
	if (!colon) {
		//	invalid target entry
		goto cleanup;
	}
	
	*colon = '\0';
	string target_name = trim(copy);
	
	t = Mem.alloc(sizeof(struct target_s));
	if (!t) {
		goto cleanup;
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
		//	redirecting to the default target
		if (token) {
			Mem.free(t->target);		//	free "default"
			t->target = Mem.alloc(strlen(token) + 1);
			strcpy(t->target, token);
			//	assumption: t->sources allocated but empty
			List.free(t->sources);
			t->sources = NULL;
			t->is_redirect = 1;
		} else {
			free_target(t);
			t = NULL;
			goto cleanup;
		}
	} else {
		//	parse body tokens
		int offset = 0;
		while (token) {
			if (strncmp(token, OUTDIR, (offset = strlen(OUTDIR))) == 0) {	//	outdir=
				t->outdir = Mem.alloc(strlen(token + offset) + 1);
				strcpy(t->outdir, token + offset);
			} else if (strstr(token, WILDSEP)) {									// wildcards
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
					fprintf(stderr, "Failed to open directory '%s/'\n", dir);
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
	
cleanup:
	Mem.free(copy);
	return t;
}
/**
 * @brief Builds an index of targets in a Doxy2MD file.
 * @param f Open file handle to Doxy2MD.
 * @param indices List to store target_index tuples.
 * @return 0 on success, non-zero if empty or error.
 */
static int doxy_index_targets(FILE* f, list indices) {
	char line[MAX_LINE];
	long pos;
	
	//	it is possible to prevent allocation of copy
	while (fgets(line, MAX_LINE, f)) {
		//	skip comments and new lines
		if (line[0] == '#' || line[0] == '\n') continue;
		
		pos = ftell(f) - strlen(line);	//	start of line
		string copy = Mem.alloc(strlen(line) + 1);
		strcpy(copy, line);
		
		//	split target from body
		char* colon = strchr(copy, ':');
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
 * @brief Processes a Target, generating its Markdown output.
 * @param t Target to process.
 */
static int process_target(target t, list comments, doxy_config* config) {
	//	checking sources to process ...
	iterator it = NULL;
	int ret = 0;
	
	if (t && t->sources) {
		it = Array.getIterator(t->sources, LIST);
		if (IS_DEBUG) printf("Processing target (%s)\n", t->target);
	} else if (config->sources) {
		if (IS_DEBUG) printf("Processing target (%s)\n", config->target);
		if (IS_DEBUG) printf("(allocating iterator on config sources)\n");
		it = Array.getIterator(config->sources, LIST);
	} else {
		ret = 1;
		goto cleanup;
	}
	
	int i = 0;
	while (Iterator.hasNext(it)) {
		string source = Iterator.next(it);
		if (IS_DEBUG) printf("   Processing file [%d]=%s\n", i, source);
		ret = process_file(source, comments);
		
		if (ret != 0) {
			goto cleanup;
		}
		++i;
	}
		
	//	build output file w/ output directory
	string_builder outfile_sb = StringBuilder.new(MAX_TARGET);
	if (t->outdir) {
		StringBuilder.appendf(outfile_sb, "%s/", t->outdir);
	}
	StringBuilder.append(outfile_sb, config->output ? config->output : t->target);
	if (!config->output) StringBuilder.append(outfile_sb, ".md");
	
	//	output documentation - if an output override is not give we have to allocate the space
	string outfile = StringBuilder.toString(outfile_sb);
	
	/*
	FILE* out = fopen(outfile, "w");
	if (out) {
		StringBuilder.toStream(sb, out);
		//fprintf(out, "%s", StringBuilder.toString(sb));
		fclose(out);
	} else {
		fprintf(stderr, "Failed to open target %s\n", outfile);
		ret = 1;
	}
	*/
	
	if (config->output) {
		if (strcmp(config->output, outfile) != 0) {
			//	clear the string and copy outfile to config->output
			Mem.free(config->output);
			// allocate config->output and copy outfile
			config->output = Mem.alloc(strlen(outfile) + 1);
			strcpy(config->output, outfile);
		}
	} else {
		// allocate config->output and copy outfile
		config->output = Mem.alloc(strlen(outfile) + 1);
		strcpy(config->output, outfile);
	}
	
	Mem.free(outfile);
	StringBuilder.free(outfile_sb);
	
cleanup:
	if (it) Iterator.free(it);
	return ret;
}
/**
 * @brief Processes a source file, appending Doxygen comments to the string builder.
 * @param filename Source file to process.
 * @param sb StringBuilder to append documentation to.
 * @return 0 on success, non-zero on failure.
 */
static int process_file(const string filename, list comments) {
	FILE* in = fopen(filename, "r");
	if (!in) {
		fprintf(stderr, "Failed to open source '%s'\n", filename);
		return 1;
	}

	char line[MAX_LINE];
	int in_comment = 0;
	comment c = NULL;
	TAGTYPE lastTagType = NONE; // Track last tag for continuation lines
	
	while (fgets(line, MAX_LINE, in)) {
		string trimmed = trim(line);
		//printf("Line: '%s'\n", trimmed); // Debug: see every line
		
		if (strncmp(trimmed, DOXTAG, strlen(DOXTAG)) == 0) {
			in_comment = 1;
			c = init_comment();
			if (!c) {
				fclose(in);
				return 1;
			}
			c->filename = Mem.alloc(strlen(filename) + 1);	// set filename
			strcpy(c->filename, filename);
			if (IS_DEBUG) printf("Start comment block\n"); 	// Debug
			continue;
		}

		if (in_comment && strstr(trimmed, "*/")) {
			in_comment = 0;
			if (!c->is_file) {
				c->signature = extract_signature(in, &c->func_name, &c->ret_type);
			}
			if (IS_DEBUG) printf("End comment block, signature: '%s' (%s -> %s)\n", 
										c->signature ? c->signature : "none",
										c->func_name ? c->func_name : "none",
										c->ret_type ? c->ret_type : "none");
			// keep comment if it has any content
			if (c->brief || StringBuilder.length(c->details) > 0 || List.count(c->params) > 0 || 
			    c->ret || (c->signature && strlen(trim(c->signature)) > 0)) {			//	skip empty signatures
				List.add(comments, c);
			} else {
				free_comment(c);
			}
			c = NULL;
			continue;
		}

		if (in_comment) {
			TAGTYPE tagType = NONE;
			string result = parse_comment_line(trimmed, &tagType);
			if (result) {
				if (IS_DEBUG) printf("Parsed: '%s' (param=%d, ret=%d)\n", result, tagType == PARAM, tagType == RETRN); // Debug
				if (tagType == PARAM) {
					List.add(c->params, result);
				} else if (tagType == RETRN) {
					c->ret = result;
				} else if (tagType == BRIEF) {
					c->brief = result;
				} else if (tagType == DTAIL) {
					StringBuilder.lappends(c->details, result);
					Mem.free(result);		//	free result since sb copies result
				} else if (tagType == NONE && strncmp(trimmed, DOXFILE, strlen(DOXFILE)) == 0) {
					c->is_file = 1;
					Mem.free(result);
				} else {
					Mem.free(result);
				}
				lastTagType = tagType;	//	update last tag
			} else if (lastTagType == DTAIL && trimmed[0] == '*') {	// continuation line
				string continuation = trim(trimmed + 1);					// skip '*'
				if (strlen(continuation) > 0) {
					if (IS_DEBUG) printf("-       '%s' (param=0, ret=0)\n", continuation);
					StringBuilder.lappends(c->details, continuation);
				}
			}
		}
	}

	if (c) free_comment(c);
	fclose(in);
	return 0;
}
/**
 * @brief Initializes a new comment struct.
 * @return Allocated comment or NULL on failure.
 */
static comment init_comment() {
	comment c = Mem.alloc(sizeof(struct comment_s));
	if (c) {
		c->brief = NULL;
		c->details = StringBuilder.new(128);
		c->params = List.new(4);
		c->ret = NULL;
		c->signature = NULL;
		c->func_name = NULL;
		c->ret_type = NULL;
		c->is_file = 0;
		c->filename = NULL;
	}
	
	return c;
}
/**
 * @brief Frees a Comment and its resources.
 * @param c Comment to free.
 */
void free_comment(comment c) {
	if (!c) return;
	if (c->brief) Mem.free(c->brief);
	if (c->details) StringBuilder.free(c->details);
	if (c->params) {
		for (int i = 0; i < List.count(c->params); i++) {
			Mem.free(List.getAt(c->params, i));
		}
		List.free(c->params);
	}
	if (c->ret) Mem.free(c->ret);
	if (c->signature) Mem.free(c->signature);
	if (c->func_name) Mem.free(c->func_name);
	if (c->ret_type) Mem.free(c->ret_type);
	if (c->filename) Mem.free(c->filename);
	
	Mem.free(c);
}
/**
 * @brief Extracts the signature line after a comment block.
 * @param in File pointer positioned after "* /".
 * @return Allocated signature string or NULL.
 */
static string extract_signature(FILE* in, string* func_name, string* ret_type) {
	char line[MAX_LINE];
	while (fgets(line, MAX_LINE, in) && trim(line)[0] == '\0'); // Skip empty lines
	if (feof(in)) return NULL;

	string copy = Mem.alloc(strlen(trim(line)) + 1);
	strcpy(copy, trim(line));
	char* brace = strchr(copy, '{');
	char* semi = strchr(copy, ';');
	if (brace) *brace = '\0';
	if (semi) *semi = '\0';
	string signature = Mem.alloc(strlen(trim(copy)) + 1);
	strcpy(signature, trim(copy));
	
	//	extract func_name and ret_type
	char* paren = strchr(signature, O_PAREN);
	if (paren) {
		// Work backward from ( to find function name start
		string name_end = paren - 1;
		while (name_end > signature && (*name_end == ' ' || *name_end == '\t')) name_end--; // Skip trailing spaces
		string name_start = name_end;
		while (name_start > signature && (*name_start == '*' || isalnum(*name_start) || *name_start == '_')) name_start--; // Include * and identifier
		if (name_start > signature && (*name_start == ' ' || *name_start == '\t')) name_start++; // Move past space

		// Extract function name
		size_t name_len = name_end - name_start + 1;
		*func_name = Mem.alloc(name_len + 1);
		strncpy(*func_name, name_start, name_len);
		(*func_name)[name_len] = '\0';

		// Extract return type (everything before name_start)
		if (name_start > signature) {
			size_t type_len = name_start - signature - 1; // Exclude the space
			*ret_type = Mem.alloc(type_len + 1);
			strncpy(*ret_type, signature, type_len);
			(*ret_type)[type_len] = '\0';
		} else {
			*ret_type = NULL; // No return type found
		}
	} else {
		// No paren: treat whole signature as function name (e.g., struct definition)
		*func_name = Mem.alloc(strlen(signature) + 1);
		strcpy(*func_name, signature);
		*ret_type = NULL;
	}
	
	Mem.free(copy);
	return signature;
}
/**
 * @brief Parses a single Doxygen comment line and identifies its type.
 * @param line Line to parse (e.g., "* @brief Short desc").
 * @param is_param Output flag: 1 if line is a param, 0 otherwise.
 * @param is_ret Output flag: 1 if line is a return, 0 otherwise.
 * @return Allocated string with the content (caller frees), or NULL if not a tagged line.
 */
static string parse_comment_line(const string line, TAGTYPE* tagType) {
	 *tagType = NONE;
	 string trimmed = trim(line);
	 string tag = NULL;

	int tagLen = 0;
	if (strncmp(trimmed, DOXBRIEF, (tagLen = strlen(DOXBRIEF))) == 0) {
		*tagType = BRIEF;
		tag = Mem.alloc(strlen(trimmed + tagLen) + 1);
		strcpy(tag, trim(trimmed + tagLen));
	} else if (strncmp(trimmed, DOXDTAIL, (tagLen = strlen(DOXDTAIL))) == 0 || 
		      strncmp(trimmed, "* @detail", 9) == 0) { // Fixed condition
		*tagType = DTAIL;
		tagLen = strncmp(trimmed, DOXDTAIL, strlen(DOXDTAIL)) == 0 ? strlen(DOXDTAIL) : 9;
		tag = Mem.alloc(strlen(trimmed + tagLen) + 1);
		strcpy(tag, trim(trimmed + tagLen));
	} else if (strncmp(trimmed, DOXPARAM, (tagLen = strlen(DOXPARAM))) == 0) {
		*tagType = PARAM;
		tag = Mem.alloc(strlen(trimmed + tagLen) + 1);
		strcpy(tag, trim(trimmed + tagLen));
	} else if (strncmp(trimmed, DOXRETRN, (tagLen = strlen(DOXRETRN))) == 0) {
		*tagType = RETRN;
		tag = Mem.alloc(strlen(trimmed + tagLen) + 1);
		strcpy(tag, trim(trimmed + tagLen));
	} else if (strncmp(trimmed, DOXFILE, (tagLen = strlen(DOXFILE))) == 0) {
		*tagType = NONE;		// treat as metadata, not content
		tag = Mem.alloc(strlen(trimmed + tagLen) + 1);
		strcpy(tag, trim(trimmed + tagLen));
	}

	return tag;
}
/**
 * @brief Trims whitespace from a string.
 * @param str String to trim.
 * @return Trimmed string.
 */
string trim (string str) {
	while (*str == ' ' || *str == '\t') str++;
	char* end = str + strlen(str) - 1;
	while (end > str && (*end == ' ' || *end == '\t' || *end == '\n')) end--;
	*(end + 1) = '\0';
	return str;
}

/**
 * @brief Appends a comment to the string builder in Markdown format.
 * @param sb StringBuilder to append to.
 * @param c Comment to format.
 *//*
static void append_comment_to_md(string_builder sb, comment c) {
	if (!c->brief) return;
	StringBuilder.appendf(sb, "#### %s \n\n", c->brief);
	if (c->details) StringBuilder.appendf(sb, "%s\n\n", StringBuilder.toString(c->details));
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
*/

//	=============================================================================
const IParser Parser = {
	.parseDoxy = parse_doxy2md
};
