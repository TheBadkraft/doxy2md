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
	string file;			/**< Doxy2MD config file (e.g., "Doxy2MD") */
	string output;		/**< Output override (e.g., "custom.md") */
	string target;		/**< Target to process (e.g., "default") */
} doxy_config;

/**
 * @brief Interface for parsing Doxy2MD files.
 */
typedef struct IParser {
	/**
	 * @brief Parses a Doxy2MD file and processes its targets.
	 * @param config Configuration for parsing and output.
	 * @return 0 on SUCCESS; otherwise non-0.
	 */
	int (*parseDoxy)(doxy_config*);
} IParser;

extern const IParser Parser;

#endif // PARSER_H
