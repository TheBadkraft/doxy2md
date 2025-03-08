// parser.h
#ifndef PARSER_H
#define PARSER_H

#include <sigcore.h>
#include <stdio.h>
#include <string.h>

#define MAX_LINE 1024
#define MAX_TARGET 256

/**
 * @brief Configuration for doxy execution.
 */
typedef struct doxy_config {
	string file;		/**< Doxy2MD config file (e.g., "Doxy2MD") */
	string output;		/**< Output override (e.g., "custom.md") */
	string target;		/**< Target to process (e.g., "default") */
	int is_debug;		/**< Set a debug flag */
	string template;	/**< Markdown formatting template */
	list sources;		/**< Source files for comment extraction */
} doxy_config;

/**
 * @brief Structure representing the full comment
 */
struct comment_s {
	string brief;		/**< Short description */
	string_builder details;	/**< Detailed description */
	list params;		/**< List of param descriptions */
	string ret;			/**< Return value description */
	string signature;	/**< Function signature */
	string func_name; /**< Function name */
	string ret_type;	/**< Return type */
	int is_file;		/**< Flag for file-level comments */
};
typedef struct comment_s* comment;

/**
 * @brief Interface for parsing Doxy2MD files.
 */
typedef struct IParser {
	/**
	 * @brief Parses a Doxy2MD file and processes its targets.
	 * @param config Configuration for parsing and output.
	 * @return 0 on SUCCESS; otherwise non-0.
	 */
	list (*parseDoxy)(doxy_config*);
} IParser;

extern const IParser Parser;

/**
 * @brief Trims whitespace from a string.
 * @param str String to trim.
 * @return Trimmed string.
 */
string trim(string str); // Declaration only


#endif // PARSER_H
