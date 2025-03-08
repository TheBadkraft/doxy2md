starting doxy test ... 
IS_DEBUG=TRUE
cfg.file=Doxy2MD
cfg.output=docs/doxy.md
cfg.target=doxy
cfg.template=(null)
cfg.sources:
   src=src/main.c
   src=include/parser.h
   src=include/md_generator.h
   src=include/writer.h
Processing target (doxy)
(allocating iterator on config sources)
   Processing file [0]=src/main.c
Start comment block
Parsed: 'main.c' (param=0, ret=0)
Parsed: 'A tool to extract Doxygen comments from C source files and generate Markdown documentation.' (param=0, ret=0)
Parsed: 'This utility parses C files for Doxygen-style comments extracting @brief and' (param=0, ret=0)
End comment block, signature: 'none' (none -> none)
Start comment block
Parsed: 'Main entry for the `doxy` command' (param=0, ret=0)
Parsed: '`<target>` to specify target configuration' (param=0, ret=0)
Parsed: '0 on SUCCESS; otherwise non-0;' (param=0, ret=1)
End comment block, signature: 'int main(int argc, string* argv)' (main -> int)
   Processing file [1]=include/parser.h
Start comment block
Parsed: 'Configuration for doxy execution.' (param=0, ret=0)
End comment block, signature: 'typedef struct doxy_config' (typedef struct doxy_config -> none)
Start comment block
Parsed: 'Structure representing the full comment' (param=0, ret=0)
End comment block, signature: 'struct comment_s' (struct comment_s -> none)
Start comment block
Parsed: 'Interface for parsing Doxy2MD files.' (param=0, ret=0)
End comment block, signature: 'typedef struct IParser' (typedef struct IParser -> none)
Start comment block
Parsed: 'Parses a Doxy2MD file and processes its targets.' (param=0, ret=0)
Parsed: 'config Configuration for parsing and output.' (param=1, ret=0)
Parsed: '0 on SUCCESS; otherwise non-0.' (param=0, ret=1)
End comment block, signature: 'list (*parseDoxy)(doxy_config*)' (list -> none)
Start comment block
Parsed: 'Trims whitespace from a string.' (param=0, ret=0)
Parsed: 'str String to trim.' (param=1, ret=0)
Parsed: 'Trimmed string.' (param=0, ret=1)
End comment block, signature: 'string trim(string str)' (trim -> string)
   Processing file [2]=include/md_generator.h
   Processing file [3]=include/writer.h
Start comment block
Parsed: 'Interface for writing generated output.' (param=0, ret=0)
End comment block, signature: 'typedef struct IWriter' (typedef struct IWriter -> none)
Start comment block
Parsed: 'Writes data to a destination.' (param=0, ret=0)
Parsed: 'data String to write.' (param=1, ret=0)
Parsed: 'filepath Destination file path, or NULL for default output.' (param=1, ret=0)
Parsed: '0 on success, non-zero on failure.' (param=0, ret=1)
End comment block, signature: 'int (*write)(const string, const string)' (int -> none)
Parsed 9 comments
Generated output to docs/doxy.md
