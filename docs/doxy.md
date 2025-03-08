#### File: main.c
A tool to extract Doxygen comments from C source files and generate Markdown documentation.

This utility parses C files for Doxygen-style comments extracting @brief and


#### main
Main entry for the `doxy` command

`<target>` to specify target configuration


``` c
int main(int argc, string* argv)
```  
#### typedef struct doxy_config
Configuration for doxy execution.

``` c
typedef struct doxy_config
```  
#### struct comment_s
Structure representing the full comment

``` c
struct comment_s
```  
#### typedef struct IParser
Interface for parsing Doxy2MD files.

``` c
typedef struct IParser
```  
#### list
Parses a Doxy2MD file and processes its targets.

``` c
list (*parseDoxy)(doxy_config*)
```  
#### trim
Trims whitespace from a string.

``` c
string trim(string str)
```  
#### typedef struct IWriter
Interface for writing generated output.

``` c
typedef struct IWriter
```  
#### int
Writes data to a destination.

``` c
int (*write)(const string, const string)
```  
