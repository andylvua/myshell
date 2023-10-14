// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>


#define HEX_LIST "0123456789ABCDEF"
#define HEX_ESC_SIZE 4

#define BUFFER_SIZE (1024 * 512) // 512 KB - Optimal for my machine

/**
 * If specified, the error message will include the description of errno
 */
#define USE_ERRNO 1

/**
 * Structure to store file information
 */
struct file {
    int fd;
    char *filename;
    off_t size;
};

/**
 * Error codes
 */
typedef enum {
    UNKNOWN_OPT = 1,
    NO_FILES = 2,
    ALLOC_ERR = 3,
    OPEN_ERR = 4,
    STAT_ERR = 5,
    READ_ERR = 6,
    WRITE_ERR = 7
} error_code;

/**
 * Error messages @related error_code
 */
static const char *error_messages[] = {
        [UNKNOWN_OPT] = "Unknown option",
        [NO_FILES]    = "No input files provided",
        [ALLOC_ERR]   = "Memory allocation error",
        [OPEN_ERR]    = "Cannot open file %s",
        [STAT_ERR]    = "Cannot stat file %s",
        [READ_ERR]    = "Cannot read from file %s",
        [WRITE_ERR]   = "Cannot write to stdout"
};

/**
 * Prints error message to stderr and exits with the specified code @see error_code
 * @param code Error code
 * @param use_errno If set to 1, the description of errno will be appended to the error message
 * @param ... Additional arguments for the error message
 */
void herror(error_code code, int use_errno, ...) {
    va_list args;
    va_start(args, use_errno);
    fprintf(stderr, "mycat: ");
    vfprintf(stderr, error_messages[code], args);
    va_end(args);
    if (use_errno) { // Add description of errno if requested
        fprintf(stderr, ": %s", strerror(errno));
    }
    fprintf(stderr, "\n");
    exit(code);
}

/**
 * Prints help message to stdout
 */
void help(void) {
    printf("Usage: mycat [-h|--help] [-A] <file1> <file2> ... <fileN>\n");
}

/**
 * Writes @c size bytes from @c buffer to @c fd
 * @param fd File descriptor
 * @param buffer Buffer to write
 * @param size Size of the buffer
 * @return 0 on success, -1 on error
 * @note Automatically retries if @c write() is interrupted
 */
int wrbuf(int fd, const char *buffer, ssize_t size) {
    ssize_t written_total = 0, written_now;
    while (written_total < size) {
        if ((written_now = write(fd, buffer + written_total, size - written_total)) == -1) {
            if (errno == EINTR) continue; // Retry if interrupted
            return -1;
        }
        written_total += written_now;
    }
    return 0;
}

/**
 * Reads @c size bytes from @c fd to @c buffer
 * @param fd File descriptor
 * @param buffer Buffer to read to
 * @param size Size of the buffer
 * @return 0 on success, -1 on error
 * @note Automatically retries if @c read() is interrupted
 */
int rdbuf(int fd, char *buffer, ssize_t size) {
    ssize_t read_total = 0, read_now;
    while (read_total < size) {
        if ((read_now = read(fd, buffer + read_total, size - read_total)) == -1) {
            if (errno == EINTR) continue; // Retry if interrupted
            return -1;
        }
        read_total += read_now;
    }
    return 0;
}

/**
 * Converts a character to its hexadecimal escape sequence
 * @param c Character to convert
 * @param hex_escape Buffer to write the escape sequence to
 */
void chtoh(char c, char *hex_escape) {
    unsigned char uc = (unsigned char) c; // Cast to unsigned char to avoid sign extension
    hex_escape[0] = '\\';
    hex_escape[1] = 'x';
    hex_escape[2] = HEX_LIST[uc >> 4]; // Extract high nibble (most significant 4 bits)
    hex_escape[3] = HEX_LIST[uc & 0xF]; // Extract low nibble (least significant 4 bits)
}

/**
 * Escapes non-printable characters in @c buffer and writes the result to @c fbuffer
 * @param buffer Buffer to filter
 * @param size Size of the buffer
 * @param fbuffer Buffer to write the result to
 * @return Number of bytes written to @c fbuffer
 */
ssize_t buftoh(const char *buffer, ssize_t size, char *fbuffer) {
    ssize_t location = 0;
    for (int i = 0; i < size; i++) {
        if (isprint(buffer[i]) || isspace(buffer[i])) {
            fbuffer[location++] = buffer[i];
        } else {
            chtoh(buffer[i], fbuffer + location);
            location += HEX_ESC_SIZE;
        }
    }
    return location;
}

/**
 * Opens files from @c argv and saves their metadata to @c files
 * @param argc Number of files
 * @param argv Array of filenames
 * @param files Array of @c struct file
 * @note Returns nothing. Exits on error.
 */
void rfiles(int argc, char *argv[], struct file *files) {
    for (int i = 0; i < argc; i++) {
        char *filename = argv[i];
        int fd;
        struct stat st;
        if ((fd = open(filename, O_RDONLY)) == -1) {
            herror(OPEN_ERR, USE_ERRNO, filename);
        }
        if (fstat(fd, &st) == -1) {
            herror(STAT_ERR, USE_ERRNO, filename);
        }
        files[i] = (struct file) {
                .fd = fd,
                .filename = filename,
                .size = st.st_size
        };
    }
}

/**
 * Reads file @c file and writes it to stdout
 * @param file File to read
 * @param a_flag If set to 1, non-printable characters will be replaced with their hexadecimal escape sequences
 * @param buffer Buffer to read to
 * @param fbuffer Auxiliary buffer for @c a_flag
 * @note Returns nothing. Exits on error.
 * @warning @c fbuffer may be @c NULL if @c a_flag is not set, otherwise behavior is undefined.
 * @c fbuffer must be at least @code 4 * BUFFER_SIZE @endcode bytes long if @c a_flag is set.
 */
void cat(const struct file *file, int a_flag, char *buffer, char *fbuffer) {
    int fd = file->fd;
    char *filename = file->filename;
    off_t size = file->size;

    while (size > 0) {
        ssize_t to_read = size > BUFFER_SIZE ? BUFFER_SIZE : size;
        if (rdbuf(fd, buffer, to_read) == -1) {
            herror(READ_ERR, USE_ERRNO, filename);
        }

        if (a_flag) {
            ssize_t to_write = buftoh(buffer, to_read, fbuffer);
            if (wrbuf(STDOUT_FILENO, fbuffer, to_write) == -1) {
                herror(WRITE_ERR, USE_ERRNO);
            }
        } else {
            if (wrbuf(STDOUT_FILENO, buffer, to_read) == -1) {
                herror(WRITE_ERR, USE_ERRNO);
            }
        }
        size -= to_read;
    }
}

int main(int argc, char *argv[]) {
    int opt;
    int a_flag = 0;

    static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {0, 0,                0, 0}
    };
    while ((opt = getopt_long(argc, argv, "Ah", long_options, NULL)) != -1) {
        switch (opt) {
            case 'A':
                a_flag = 1;
                break;
            case 'h':
                help();
                return 0;
            case '?':
                herror(UNKNOWN_OPT, !USE_ERRNO);
            default:; // Should never happen
        }
    }
    if (optind == argc) {
        herror(NO_FILES, !USE_ERRNO);
    }

    struct file *files = malloc((argc - optind) * sizeof(struct file));
    if (files == NULL) {
        herror(ALLOC_ERR, USE_ERRNO);
    }
    rfiles(argc - optind, argv + optind, files);

    char *buffer = malloc(BUFFER_SIZE);
    char *fbuffer = malloc(HEX_ESC_SIZE * BUFFER_SIZE);
    if (buffer == NULL || fbuffer == NULL) {
        herror(ALLOC_ERR, !USE_ERRNO);
    }

    for (int i = optind; i < argc; i++) {
        struct file file = files[i - optind];
        cat(&file, a_flag, buffer, fbuffer);
        close(file.fd);
    }

    free(files);
    free(buffer);
    free(fbuffer);
    return 0;
}
