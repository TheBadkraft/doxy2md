/**
 * @file main.c
 * @author The Badkraft, 2025
 * @brief A tool to extract Doxygen comments from C source files and generate Markdown documentation.
 * @details This utility parses C files for Doxygen-style comments extracting @brief and 
 *			@details tags to create a Markdown file. It uses the sigcore library making it 
 *			lightweight and portable. Designed to simplify documentation generation, it outputs 
 *			structured Markdown suitable for READMEs or API docs.
 */
 
#include "parser.h"

#include <stdio.h>

static int run_doxy(const string, string, string);

// Main
/**
 * @brief Main entry for the `doxy` command 
 * @detail `<target>` to specify target configuration
 *				 `-o <output.md>` to override target configuration
 *				 `<config_file>` optional configuration
 * @return 0 on SUCCESS; otherwise non-0;
 */
int main(int argc, string* argv) {
	string config_file = "Doxy2MD";
	string output_file = NULL;
	string target = "default";
	
	int ret = 0;
	/*
	 *	assumptions:
	 *			- if target is supplied, it will be arg[1]
	 *			- -o will have an output file immediately following
	 *      - a trailing file (last arg) will be an alternate configuration
	 *	**/
	int i = 1;
	while (i < argc) {
		if (strcmp(argv[i], "-o") == 0) {
			if (++i >= argc) {
				fprintf(stderr, "Error: '-o' requires an output file\n");
				ret = 1;
				goto EXIT;
			}
			output_file = argv[i];
		} else if (argv[i][0] != '-') { // Positional arg
			if (strcmp(target, "default") == 0) { 		// First positional is target
				target = argv[i];
			} else { // Last positional is config_file
				config_file = argv[i];
			}
		} else {
			fprintf(stderr, "Error: Unknown flag '%s'\n", argv[i]);
			ret = 1;
			goto EXIT;
		}
		i++;
	}	
	
	//	run doxy
	ret = run_doxy(config_file, output_file, target);
	
EXIT:
	printf("Document generation [%s]\n", ret ? "FAILED" : "SUCCESS");
	return ret;
}

static int run_doxy(const string config_file, string output_file, string target) {
	doxy_config* config = Mem.alloc(sizeof(doxy_config));
	if (!config) {
		fprintf(stderr, "Memory allocation failed.\n");
		return 1;
	}
	
	config->file = config_file;
	config->output = output_file;
	config->target = target;
	
	printf("Configuration='%s'\n", config->file);
	printf("Output='%s'\n", config->output ? config->output : "DEFAULT");
	printf("Target='%s'\n", config->target ? config->target : "DEFAULT");
	int ret = Parser.parseDoxy(config);
	Mem.free(config);
	
	return ret;
}
