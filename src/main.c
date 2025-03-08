/**
 * @file main.c
 * @author The Badkraft, 2025
 * @brief A tool to extract Doxygen comments from C source files and generate Markdown documentation.
 * @details This utility parses C files for Doxygen-style comments extracting @brief and 
 *			@details tags to create a Markdown file. It uses the sigcore library making it 
 *			lightweight and portable. Designed to simplify documentation generation, it outputs 
 *			structured Markdown suitable for READMEs or API docs.
 */
 
#include "doxy2md.h"

#include <stdio.h>

static int run_doxy(const string, int, string, string*);

// Main
/**
 * @brief Main entry for the `doxy` command 
 * @detail `<target>` to specify target configuration
 *			  `-o <output.md>` to override target configuration
 *			  `<config_file>` optional configuration
 * @return 0 on SUCCESS; otherwise non-0;
 */
int main(int argc, string* argv) {
	string config_file = "Doxy2MD";
	string output_file = NULL;
	string target = "default";
	int is_debug = 0;
	int ret = 0;
	// **
	//	assumptions:
	//			- if target is supplied, it will be arg[1]
	//			- -o will have an output file immediately following
	//      - a trailing file (last arg) will be an alternate configuration
	//	**
	int i = 1;
	while (i < argc) {
		if (strcmp(argv[i], "--version") == 0) {
			printf("doxy2md version=%s\n", VERSION);
			goto exit;
		} else if (strcmp(argv[i], "--debug") == 0) {
			is_debug = 1;
		} else if (strcmp(argv[i], "-o") == 0) {
			if (++i >= argc) {
				fprintf(stderr, "Error: '-o' requires an output file\n");
				ret = 1;
				goto cleanup;
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
			goto cleanup;
		}
		i++;
	}	
	
	//	run doxy
	ret = run_doxy(config_file, is_debug, target, &output_file);
	
cleanup:
	printf("Document '%s' generated [%s]\n", output_file, ret ? "FALSE" : "TRUE");
exit:
	return ret;
}

static int run_doxy(const string config_file, int is_debug, string target, string* output_file) {
	int ret = 0;
	
	doxy_config* config = Mem.alloc(sizeof(doxy_config));
	if (!config) {
		fprintf(stderr, "Memory allocation failed.\n");
		return 1;
	}
	
	config->is_debug = is_debug;
	config->file = config_file;
	config->output = *output_file;
	config->target = target;
	
	printf("Configuration='%s'\n", config->file);
	if (is_debug) printf("Output='%s'\n", config->output ? config->output : "DEFAULT");
	if (is_debug) printf("Target='%s'\n", config->target ? config->target : "DEFAULT");
	
	list comments = Parser.parseDoxy(config);
	if (is_debug) printf("Parsed %d comments\n", List.count(comments));
	
	string_builder sb = StringBuilder.new(1024);
	MDGenerator.generate(sb, comments, NULL); // No template yet
	
	string genMD = StringBuilder.toString(sb);
	if (FileWriter.write(genMD, config->output) == 0) {
		printf("Generated markdown to %s\n", config->output);
	} else {
		printf("Failed to generate markdown to file (%s)\n", *output_file);
	}
	
	*output_file = Mem.alloc(strlen(config->output) + 1);
	strcpy(*output_file, config->output);	
	
	Mem.free(genMD);
	Mem.free(config);
	StringBuilder.free(sb);
	
	//	clean up comments list
	iterator it = Array.getIterator(comments, LIST);
	while (Iterator.hasNext(it)) {
		free_comment(Iterator.next(it));
	}
	Iterator.free(it);
	List.free(comments);
	
	return ret;
}

