# Lab work 5: `myrls` utility
> Authors: Andrii Yaroshevych, Pavlo Kryven, Yurii Kharabara
> 
> Variant: `-`

## Description
`myrls` is a simple implementation of the [`ls(1)`](https://www.man7.org/linux/man-pages/man1/ls.1.html) command for recursively 
listing directory contents. The closest output `ls` can produce is
achieved with: 
```bash
ls -ARFo --time-style="+%Y-%m-%d %H:%M:%S" [path]
```

> `-A`: list all files except `.` and `..`
> 
> `-R`: list subdirectories recursively
> 
> `-F`: append indicator (one of */=>@|) to entries
> 
> `-o`: like `-l`, but do not list group information
> 
> `--time-style="+%Y-%m-%d %H:%M:%S"`: show modification time in the specified format

## Prerequisites

The project is written in C standard 99 with GNU extensions and does not require any additional libraries. 

- C compiler with C99 support (gcc, clang, etc.)
- CMake 3.15 or higher (optional)
- Make (optional)

> **Note**
> 
> Your system must be POSIX-compliant

## Compilation

You can use CMake directly:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
```

use provided `compile.sh` script:
```bash
./compile.sh -O
```

or manually:
```bash
$(C_COMPILER) -std=gnu99 -O3 -o myrls myrls.c
```

## Usage

```bash
myrls [path] [-h|--help]
```

Where `path` is a path to the directory or file to be listed. 
If `path` is not specified, the current working directory is used.

Use `-h` or `--help` to display usage information.

### Exit status

`myrls` returns the following exit values:
- `0` - Success
- `1` - Minor problems encountered during execution (e.g., cannot access file)
- `2` - Fatal error (e.g., cannot allocate memory, command-line usage error)

### Details

If ownership of a file is unknown or cannot be determined, the user ID is displayed instead.

## Results

[//]: # (TODO: add benchmark results)

## Additional tasks

None.

## License

The [MIT](LICENSE) License.
