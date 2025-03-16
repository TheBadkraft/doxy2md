# doxy2md

A lightweight tool to extract Doxygen-style comments from C source files and generate Markdown documentation. Built with the `sigcore` library, `doxy2md` is portable and designed to simplify creating structured Markdown for READMEs or API docs.

## Features
- Extracts `@brief`, `@details`, `@param`, `@return`, and file-level `@file` comments.
- Supports multi-line `@details` comments.
- Outputs Markdown with file names and function signatures.
- Configurable via a `Doxy2MD` file with target and `outdir` options.
- CLI-driven with version info and debug mode.

## Installation
1. **Prerequisites**: GCC, [`sigcore` library][1].
2. **Clone**: `git clone <https://github.com/TheBadkraft/doxy2md.git>`
3. **Build**:  
   `cd doxy2md`  
   `make`  
   `make install`  # Installs to ~/bin  
4. **Verify**: `doxy2md --version` (should print `doxy2md version 1.0`).

## Usage  
`doxy2md [<target>] [-o <output.md>] [--debug] [--version] [<config_file>]`  
- `<target>`: Specify a target from `Doxy2MD` (default: `default`).
- `-o <output.md>`: Override the output file (default: `<target>.md` or `outdir/<target>.md`).
- `--debug`: Print parsing details to stdout.
- `--version`: Show version and exit.
- `<config_file>`: Custom config file (default: `Doxy2MD`).

### Example `Doxy2MD` Config  
*If you're familiar with **Makefile** this will be very similar*  
``` plaintext
default: doxy` 
doxy: src/main.c include/*.h outdir=docs
```
- **Run**: `doxy2md doxy --debug`
- **Output**: Generates `docs/doxy.md` with comments from `src/main.c` and headers.

## Example Output  
For this code in `src/main.c`:  
``` c
/**
 * @file main.c
 * @brief A tool to extract Doxygen comments.
 * @details This utility parses C files for comments.
 */
int main(int argc, char** argv) {
   return 0;
}
```

Running `doxy2md doxy` with `outdir=docs` produces `docs/doxy.md`:
``` plaintext  
#### File: src/main.c
A tool to extract Doxygen comments.

This utility parses C files for comments.

#### main  
int main(int argc, char** argv)
```  
Results:      
> #### File: src/main.c  
> A tool to extract Doxygen comments.  
>  
> This utility parses C files for comments.  
>  
> #### main  
> int main(int argc, char** argv)  
  
## Building from Source
- **Requirements**: `gcc`, `make`, `sigcore`.
- **Compile**: `make`.
- **Clean**: `make clean`.

## License
[GNU GENERAL PUBLIC LICENSE][2]
- Version 3, 29 June 2007

## Version
Current: 1.0 (Initial release with core parsing and Markdown generation).

[1]: https://github.com/TheBadkraft/sigcore  
[2]: https://github.com/TheBadkraft/doxy2md/blob/main/LICENSE
