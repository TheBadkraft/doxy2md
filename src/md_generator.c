// md_generator.c
#include "md_generator.h"

static void md_generate(string_builder sb, list comments, const string template) {
	iterator it = Array.getIterator(comments, LIST);
	while (Iterator.hasNext(it)) {
		comment c = Iterator.next(it);
		//	minimal rendering for now -- to be expanced
		if (c->brief) {
			if (c->is_file) {
				StringBuilder.appendf(sb, "#### File: %s\n", c->filename);
			} else {
				StringBuilder.appendf(sb, "#### %s\n", c->func_name ? c->func_name : "File");
			}
			StringBuilder.appendf(sb, "%s\n\n", c->brief);
		}
		if (StringBuilder.length(c->details) > 0) {
			string details = StringBuilder.toString(c->details);
			StringBuilder.appendf(sb, "%s\n\n", details);
			Mem.free(details);
		}
		if (c->signature && strlen(trim(c->signature)) > 0) {
			StringBuilder.appendf(sb, "``` c\n%s\n```  \n", c->signature);
		}
	}
	Iterator.free(it);
}

const IGenerator MDGenerator = {
    .generate = md_generate
};
