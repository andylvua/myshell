# Lab work 4: `myshell` - a custom shell implementation
Authors (team): 
- [Yurii Kharabara](https://github.com/YuriiKharabara)
- [Andrii Yaroshevych](https://github.com/andylvua)
- [Pavlo Kryven](https://github.com/codefloww)

## Description
This project is a custom command-line interpreter that provides a user interface for executing commands. 
The shell is the primary way of interacting with the operating system, 
allowing users to execute commands and manage files and directories. 

## Compilation and running

### Requirements

The project is build using C++20 standard. 

- [CMake](https://cmake.org/) (version 3.15 or higher)
- [GCC](https://gcc.gnu.org/) (version 12.3.0 or higher) or [Clang](https://clang.llvm.org/) (version 16.0.6 or higher)

> **Note**
> 
> Tested on GCC `13.2.1` and Clang `17.0.3`.

Besides, it heavily relies on the following libraries:
- [Boost](https://www.boost.org/) (version 1.71.0 or higher)
- [Readline](https://tiswww.case.edu/php/chet/readline/rltop.html) (version 8.0 or higher)

> **Note**
> 
> For installation instructions, please refer to official documentation.


### Compilation

1. Use the provided `CMakeLists.txt` to compile the project:
    ```bash
      cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
    ```
2. Run the compiled executable:
    ```bash
    ./build/myshell
    ```

> **Note**
> 
> Alternatively, you can use the provided `compile.sh` script to compile and run the project:
> ```bash
> ./compile.sh -O
> ```

## Usage

The usage of the shell is similar to that of other shells, such as `bash` or `zsh`. 
If you are familiar with these shells, you should have no trouble using `myshell`.

### Adding External Commands

The external commands are compiled separately and can be invoked directly from the shell, much like built-in commands.
They are recognized automatically by the build system. All external commands are placed in the `{CMAKE_BINARY_DIR}/msh/bin/external` directory that is added to the `PATH` environment variable on startup.
This ensures that `myshell` can always find them.

All processing is done by [ExternalPrograms.cmake](./cmake/ExternalPrograms.cmake) script.

For exact instructions on how to add external commands, please refer to the [README.md](./src/external/README.md) file in the `external` directory.

### History

`myshell` supports command history. Its path is predefined by the build system and is set to `{CMAKE_BINARY_DIR}/msh/.msh_history`.
This ensures that the history file is placed in the build directory and it will not be in vain to clog the examiner's computer.
Also, this allows us to use the persistent history between different runs of the shell.

If you want to change the path to the history file, consider changing the `MSH_HISTORY_PATH` variable in the `CMakeLists.txt` file to your desired path.

## Implementation Details

### Tokens
Tokens are the basic building blocks of `myshell`. All internal operations are performed on tokens.
Tokens can have different flags, which determine how they are processed by the shell.

You can find the token and token types definitions in the [msh_token.h](inc/types/msh_token.h) file.
Flags are defined in the [msh_internal.cpp](src/internal/msh_internal.cpp) file.

### Lexer/Tokenizer
The lexer/tokenizer is a crucial component of `myshell`. Basically, it is a simple state machine that tokenizes the user input.

It takes the raw input string from the user and breaks it down into individual tokens. 
These tokens can represent commands, words, variable definitions, shell metacharacters etc. 
By tokenizing the input, `myshell` can understand and act upon user commands effectively.

The lexer/tokenizer is implemented in the [msh_parser.cpp](src/internal/msh_parser.cpp) file.

### Pipeline Overview

`myshell` pipeline is implemented using the following components:

**Setup**

When `myshell` starts, it initializes essential configurations:
- Initializes internal environment variables from its own environment.
- Loads command history from the history file. Its path is determined by the build system.
- Sets up other necessary configurations.

**Main Loop**

`myshell` enters its main loop, continuously awaiting user input. 
Within this loop, the following operations are performed:

- Read User Input <br><br>
The shell waits for the user to enter a command.
<br><br>
- Tokenize <br><br>
The input is tokenized using a lexer/tokenizer, breaking the user input into individual tokens representing commands, arguments, or special symbols.
<br><br>
- Tokens Processing <br><br>
This step involves several sub-steps:
    - _Alias Expansion_: All aliases are expanded into their corresponding values.
    - _Syntax Check_: The syntax of the command is checked to ensure that it is valid.
    - _Variable Expansion_: All variables are expanded into their corresponding values.
    - _Setting Internal Variables_: The shell processes variable declarations and sets the corresponding internal variables.
    - _Wildcard Expansion_: Wildcard characters are expanded into their corresponding file names.
    - _Tokens Merging_: The shell merges adjacent tokens into a single token if necessary.
<br><br>
- Argument Processing <br><br>
The shell splits the processed tokens into command arguments.
<br><br>

- Command Execution <br><br>
Depending on the command's nature:
  - _Internal/Built-in Commands:_ Handled directly by the shell (e.g., changing directories or setting environment variables).
  - _External Commands:_ `myshell` spawns a child process using the `fork` system call and then executes the command in the child process via `execve`/`execvpe`.
  - _Scripts:_ `myshell` can also execute script files, treating them as sequences of commands.

> **Note**
> 
> When `myshell` is executed with an argument, it treats the argument as a script file. 
> The shell terminates after executing the script file.

**Cleanup**

When the user exits the shell, it saves the command history to the history file and performs other necessary cleanup operations.

### Notes on Implementation

- **Wildcard Expansion**:

Suggestion
```text
The mask can contain an absolute or relative path, 
but we believe that the wildcard is taken into account only in the last element of the path. 
For example, here: /usr/doc/*.txt -- we take it into account, and here: /usr/*/abc.txt -- we ignore it. 
Maybe with a warning message from myshell.
```

is ignored. Wildcard expansion is performed on the **entire** path, not just the last element.

- **Variable Expansion**:

If a variable is not defined, it is treated as an empty string. This behavior is consistent with other shells, however, unspecified in the task.

- **`msource` Builtin Command**:

This command is a synonym for the `.` command and operates identically to it.

- **Tilde Expansion**:

`myshell` supports tilde expansion. It expands `~` to the user's home directory.

- **Default Prompt**:

The default prompt is changed from `\w \$ ` specified in the main task to `\d [\u@\h \W] \$ `. 
This is done to demonstrate the `PS1` environment variable expansion.

### Documentation
The whole project is documented. Feel free to read it if something is unclear.

## Additional Tasks

All additional features from `Additional task 2` are supported:

1. **Double Quotation Marks Handling**: The shell supports the use of double quotation marks for processing file names and arguments with spaces.

The behavior of double quotation marks is similar to that of other shells, such as bash or zsh.

You can use double quotation marks inside double quotation marks by escaping them with a backslash.

2. **Wildcard Substitution in Double Quotes**: The shell provides the ability to perform wildcard substitution even within double quotation marks.

> **Note**
> 
> This behavior is disabled by default. To enable it, set the `ENABLE_DOUBLE_QUOTE_WILDCARD_SUBSTITUTION` flag to `ON` in the `CMakeLists.txt` file.
> 
> This decision was due to the fact that this is an unexpected behavior for many shells. Neither bash nor zsh perform wildcard substitutions in double quotes.

3. **Single Quotation Marks**: Single quotation marks function similarly to double quotes, but no variable or wildcard substitution occurs inside them.

The one exception is that single quotation marks can't appear inside single quotation marks even if they are escaped.

4. **Escape Sequences**: We support the following escape sequences: `\#`, `\\`, `\"`, and `\'`. These allow the corresponding characters to be inserted without their special meaning.

Also, other escape sequences are supported. For now, we support parsing of shell metacharacters, such as `|`, `&`, `;`, `(`, `)`, `<`, `>`, but we don't support their functionality.
Due to this fact, `myshell` won't be able to execute commands with these tokens. If you want to use them without their special meaning, you should escape them with a backslash.

5. **Command Support with Equal Sign**: To support commands with an equal sign in their name, escape sequence `\=` is also supported.

6. **`#` in Strings**: The shell supports the use of `#` in strings. This means that `#` is treated as a regular character and doesn't start a comment.

7. **Local Environment Variables**: The shell supports the creation of local environment variables using the `VAR=ABC` syntax. These variables are visible only within the shell and aren't passed to child processes. To promote a local variable to an environment variable for child processes, use the `mexport VAR` command.

> **Note**
> 
> Variable declarations can only appear before a simple command. Otherwise, they are treated as regular arguments.

8. **Customizable Prompt**: The shell prompt can be customized based on the `PS1` environment variable. This provides users with the flexibility to include information such as the username or the current time in the prompt.

Currently, the following variables are supported:
- `\d` - The current date in YYYY-MM-DD format.
- `\t` - The current time in HH:MM:SS format.
- `\u` - The current user.
- `\h` - The current host.
- `\w` - The current working directory.
- `\W` - The current working directory's basename.
- `\n` - A newline character.
- `\r` - A carriage return character.
- `\s` - The current shell.
- `\v` - The current shell version.
- `\$` - The prompt character.

The default value of `PS1` is `\d [\u@\h \W] \$ `.

9. **Alias Support**: Our shell supports the creation and utilization of aliases, allowing users to define custom shortcuts for frequently used commands.

Aliases are defined using the `alias` command. For example, to create an alias named `ll` for the `ls -l` command, you can use the following command:
```bash
alias ll="ls -l"
```

To remove an alias, use the `unalias` command:
```bash
unalias ll
```

For more information, please use the `--help` flag.

> **Note**
> 
> Aliases can appear in alias itself. `myshell` incorporates a robust alias expansion mechanism that prevents infinite alias expansion loops.
