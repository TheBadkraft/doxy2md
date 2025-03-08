// writer.h
#ifndef WRITER_H
#define WRITER_H

#include <sigcore.h>

/**
 * @brief Interface for writing generated output.
 */
typedef struct IWriter {
	/**
	* @brief Writes data to a destination.
	* @param data String to write.
	* @param filepath Destination file path, or NULL for default output.
	* @return 0 on success, non-zero on failure.
	*/
    int (*write)(const string, const string);
} IWriter;

extern const IWriter FileWriter;

#endif // WRITER_H
