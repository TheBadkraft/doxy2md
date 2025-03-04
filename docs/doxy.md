### *(source)* src/parser.c

####  Parses a Doxy2MD file and processes its targets. 

**Parameters:**  
- filename Name of the Doxy2MD file to parse.

**Returns:** 0 on success, non-zero on failure.  
``` c
static int parse_doxy2md(doxy_config* config)
```
####  Parses a single line from Doxy2MD into a Target. 

**Parameters:**  
- line Line to parse (e.g., "sigcore: sigcore.h").

**Returns:** Allocated Target, or NULL on error.  
``` c
static target parse_line(const string line)
```
####  Builds an index of targets in a Doxy2MD file. 

**Parameters:**  
- f Open file handle to Doxy2MD.
- indices List to store target_index tuples.

**Returns:** 0 on success, non-zero if empty or error.  
``` c
static int index_targets(FILE* f, list indices)
```
####  Finds a target’s file offset in the index. 

**Parameters:**  
- indices List of target_index tuples.
- name Target name to find.

**Returns:** File offset if found, -1 if not.  
``` c
static long find_offset(list indices, const string name)
```
####  Frees a Target and its resources. 

**Parameters:**  
- t Target to free.

``` c
static void free_target(target t)
```
####  Trims whitespace from a string. 

**Parameters:**  
- str String to trim.

**Returns:** Trimmed string.  
``` c
static string trim (string str)
```
####  Processes a Target, generating its Markdown output. 

**Parameters:**  
- t Target to process.

``` c
static int process_target(target t, doxy_config* config)
```
####  Processes a source file, appending Doxygen comments to the string builder. 

**Parameters:**  
- filename Source file to process.
- sb StringBuilder to append documentation to.

``` c
static int process_file(const string filename, string_builder sb)
```
####  Frees a Comment and its resources. 

**Parameters:**  
- c Comment to free.

``` c
static void free_comment(comment c)
```
####  Parses a Doxygen comment line into a Comment struct. 

**Parameters:**  
- line Line to parse (e.g., "* @brief Short desc").

**Returns:** Allocated Comment if valid, NULL otherwise.  
``` c
static comment parse_comment(const string line)
```
### *(source)* src/main.c

####  A tool to extract Doxygen comments from C source files and generate Markdown documentation. 

This utility parses C files for Doxygen-style comments extracting @brief and

``` c


```
####  Main entry for the `doxy` command 

**Returns:** 0 on SUCCESS; otherwise non-0;  
``` c
int main(int argc, string* argv)
```
### *(header)* include/parser.h

####  Configuration for doxy execution. 

``` c
typedef struct doxy_config
```
####  Interface for parsing Doxy2MD files. 

``` c
typedef struct IParser
```
####  Parses a Doxy2MD file and processes its targets. 

**Parameters:**  
- config Configuration for parsing and output.

**Returns:** 0 on SUCCESS; otherwise non-0.  
``` c
int (*parseDoxy)(doxy_config*)
```
