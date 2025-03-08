// writer.c
#include "writer.h"

static int write_file(const string data, const string filepath) {
	FILE* out = fopen(filepath, "w");
	if (!out) {
		fprintf(stderr, "Failed to open '%s' for writing\n", filepath);
		return 1;
	}
	
	fprintf(out, "%s", data);
	fclose(out);
	return 0;
}

const IWriter FileWriter = {
    .write = write_file
};
