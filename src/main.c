/**
 * @file main.c
 * @author The Badkraft, 2025
 * @brief A tool to extract Doxygen comments from C source files and generate Markdown documentation.
 * @details This utility parses C files for Doxygen-style comments extracting @brief and 
 *			@details tags to create a Markdown file. It uses the sigcore library making it 
 *			lightweight and portable. Designed to simplify documentation generation, it outputs 
 *			structured Markdown suitable for READMEs or API docs.
 */
#include "md_generator.h"
#include "writer.h"

#include <stdio.h>

static void free_debug_comment(comment c) {
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
	Mem.free(c);
}

/**
 * @brief Main entry for the `doxy` command 
 * @detail `<target>` to specify target configuration
 *         `-o <output.md>` to override target configuration
 *         `<config_file>` optional configuration
 * @return 0 on SUCCESS; otherwise non-0;
 */
int main(int argc, string* argv) {
	printf("starting doxy test ... \n");
	doxy_config config = {"Doxy2MD", "docs/doxy.md", "doxy", 1, NULL, List.new(10)};
	List.add(config.sources, "src/main.c");
	List.add(config.sources, "include/parser.h");
	List.add(config.sources, "include/md_generator.h");
	List.add(config.sources, "include/writer.h");
	
	list comments = Parser.parseDoxy(&config);
	printf("Parsed %d comments\n", List.count(comments));

	string_builder sb = StringBuilder.new(1024);
	MDGenerator.generate(sb, comments, NULL); // No template yet
	
	string output = StringBuilder.toString(sb);
	if (FileWriter.write(output, config.output) == 0) {
		printf("Generated output to %s\n", config.output);
	} else {
		printf("Failed to generate output to file\n");
	}
	
	Mem.free(output);
	StringBuilder.free(sb);
	//	clean up comments list
	iterator it = Array.getIterator(comments, LIST);
	while (Iterator.hasNext(it)) {
		free_debug_comment(Iterator.next(it));
	}
	Iterator.free(it);
	List.free(comments);
	List.free(config.sources);
	
	return 0;
}
 
 
/* 
int main(int argc, string* argv) {
	printf("starting doxy test ... \n");
	doxy_config config = {"Doxy2MD", NULL, "doxy", 1, NULL, List.new(10)};
	List.add(config.sources, "src/main.c");
	
	list comments = Parser.parseDoxy(&config);
	printf("Parsed %d comments\n", List.count(comments));
	
	List.free(config.sources);
	
	return 0;
}
*/
