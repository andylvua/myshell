# Lab work 2. mycat utility

> Team members: Andrii Yaroshevych, Pavlo Krynenko

## Description
This is a simple implementation of the `cat` utility. It is used to concatenate files and print on the standard output.
The goal of this lab is to learn how to work with system calls and file descriptors.

## Prerequisites

This project is written in C and doesn't use any third-party libraries. So, you need to have only:
- C compiler (gcc, clang, etc.)
- CMake 3.15 or higher
- Make

### Compilation

#### Using CMake directly

```bash
mkdir build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
```

#### Using `compile.sh` script

```bash
./compile.sh -O
```

### Usage

```bash
mycat [-h|--help] [-A] <file1> <file2> ... <fileN>
```

For example:

```bash
./mycat README.md
```

#### Options

| Option                        | Description                                                  |
|-------------------------------|--------------------------------------------------------------|
| `-h`, `--help`                | Print help message and exit                                  |
| `-A`                          | Print all characters, including non-printable ones as `\xNN` |
| `<file1> <file2> ... <fileN>` | Files to concatenate                                         |

> **Note**
> 
> If any of provided files cannot be opened, the program will exit with code 4 without printing _any_ output.

#### Exit codes

| Code | Description               |
|------|---------------------------|
| 1    | Unknown option provided   |
| 2    | No input files provided   |
| 3    | Memory allocation error   |
| 4    | Error opening file        |
| 5    | Error getting file status |
| 6    | Error reading from file   |
| 7    | Error writing to stdout   |


### Results

My implementation of `cat` utility is working around 7% faster than the original one.

```bash
❯ ./mycat 10g_zero | pv > /dev/null
9.77GiB 0:00:17 [ 570MiB/s]
❯ cat 10g_zero | pv > /dev/null
9.77GiB 0:00:18 [ 532MiB/s]
```

# Additional tasks

None of the additional tasks were implemented.
