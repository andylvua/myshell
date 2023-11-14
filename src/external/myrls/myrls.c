// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
/**
 * @file
 * @brief External ulility `myrls`.
 * @ingroup external
 */

/*
 * myrls - a simple implementation of the ls(1) command for recursively
 * listing directory contents. The closest output ls(1) can produce is
 * achieved with: ls -ARFo --time-style="+%Y-%m-%d %H:%M:%S" [path]
 *
 * Options:
 *     -h, --help -- print usage information and exit
 *
 * Exit status:
 *     0 -- Success,
 *     1 -- minor problems encountered during execution (e.g., cannot access file).
 *     2 -- fatal error (e.g., cannot allocate memory, command-line usage error).
 */


#include <sys/stat.h>
#include <getopt.h>
#include <dirent.h>
#include <pwd.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <locale.h>
#include <stdio.h>
#include <stdint.h>


#define DEFAULT_PATH "./"
#define TIME_FORMAT "%Y-%m-%d %H:%M:%S"

/* initial size of the buffer for storing files information */
#define INIT_BUF_SIZE 16

#define UNAME_MAX 32
#define TIME_LEN 20
#define PERM_LEN 10

#define MINOR_ERROR 1
#define FATAL_ERROR 2


enum file_type {
    REGULAR,
    DIRECTORY,
    SYMLINK,
    FIFO,
    SOCKET,
    EXECUTABLE,
    UNKNOWN
};

struct file_info {
    uid_t uid;
    off_t size;
    time_t mtime;
    mode_t mode;
    enum file_type type;
    char *name;
    char *target;
};

struct user_info {
    char name[UNAME_MAX + 1];
    uid_t uid;
    struct user_info *next;
};


/* string representation of file types */
static char const *const file_type_str[] = {
        [REGULAR]    = "",
        [DIRECTORY]  = "/",
        [SYMLINK]    = "@",
        [FIFO]       = "|",
        [SOCKET]     = "=",
        [EXECUTABLE] = "*",
        [UNKNOWN]    = "?"
};

/* command line options */
static struct option long_opts[] = {
        {"help", no_argument, NULL, 'h'},
        {NULL, 0,             NULL, 0}
};

/* cache the results of the expensive getpwuid() calls */
static struct user_info *user_info_cache;

/* reusable buffer for constructing paths */
static char current_path[PATH_MAX];

/* keep track of the exit status */
static int exit_status;


/* Initialize the timezone and locale information */
static void
init(void) {
    tzset(); /* call tzset() to initialize the timezone information before
    call to reentrant localtime_r() according to POSIX.1-2004 */
    setlocale(LC_COLLATE, ""); /* set the locale for
    collation aware string comparison to comply with task requirements */
}

/* Print usage information and exit with the given status
   If message is not NULL, print it before usage information,
   otherwise exit immediately if status is not EXIT_SUCCESS */
static void
usage(int const status, char const *message) {
    if (message != NULL) {
        error(status, 0, "%s. See '--help'", message);
    }

    if (status == EXIT_SUCCESS) {
        fprintf(stdout,
                "myrls: recursively list directory contents\n"
                "Usage: myrls [path=\"./\"] [-h|--help]\n\n"
                "Exit status:\n"
                "\t0 -- Success,\n"
                "\t1 -- minor problems encountered during execution (e.g., cannot access file).\n"
                "\t2 -- fatal error (e.g., cannot allocate memory, command-line usage error).\n");

    }

    exit(status);
}

/* Compare function for qsort(3). Sort by file name with
   respect to current locale for category LC_COLLATE */
static int
file_name_cmp(void const *a, void const *b) {
    return strcoll(((struct file_info const *) a)->name,
                   ((struct file_info const *) b)->name);
}

/* Wrapper functions for malloc(3) and realloc(3). Call
   error(3) on failure and exit with FATAL_ERROR status */
static void *
xmalloc(size_t const size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        error(FATAL_ERROR, errno, "malloc");
        __builtin_unreachable(); /* if status given to error(3) is a
        nonzero value, error() calls exit(3) to terminate the program,
        so tell the compiler that this line is unreachable */
    }

    return ptr;
}

static void *
xrealloc(void *ptr, size_t const size) {
    void *new_ptr = realloc(ptr, size);
    if (new_ptr == NULL) {
        error(FATAL_ERROR, errno, "realloc");
        __builtin_unreachable();
    }

    return new_ptr;
}

/* Report error with given message and path using
   error(3) and set exit_status to MINOR_ERROR */
static void
file_error(char const *message, char const *path) {
    error(0, errno, "%s: %s", message, path);
    exit_status = MINOR_ERROR;
}

/* Write file permissions specified by mode to
   given buffer in form of three rwx triplets. */
static void
format_permissions(mode_t const mode, char *formatted) {
    formatted[0] = (mode & S_IRUSR) ? 'r' : '-';
    formatted[1] = (mode & S_IWUSR) ? 'w' : '-';
    formatted[2] = (mode & S_IXUSR) ? 'x' : '-';
    formatted[3] = (mode & S_IRGRP) ? 'r' : '-';
    formatted[4] = (mode & S_IWGRP) ? 'w' : '-';
    formatted[5] = (mode & S_IXGRP) ? 'x' : '-';
    formatted[6] = (mode & S_IROTH) ? 'r' : '-';
    formatted[7] = (mode & S_IWOTH) ? 'w' : '-';
    formatted[8] = (mode & S_IXOTH) ? 'x' : '-';
    formatted[9] = '\0';
}

/* Get the owner name for given uid. If the name is not found
   in the cache, call getpwuid(3) and store the result in the cache.
   Return the malloc'ed string with the owner name, or the string
   representation of uid if getpwuid(3) fails or uid is not found */
static char *
get_owner(uid_t const uid) {
    struct user_info *user_info;
    for (user_info = user_info_cache; user_info; user_info = user_info->next) {
        if (user_info->uid == uid) {
            return user_info->name;
        }
    }

    user_info = xmalloc(sizeof(struct user_info));

    user_info->uid = uid;

    errno = 0; /* required by getpwuid(3) */
    struct passwd const *pwd = getpwuid(uid);
    if (pwd == NULL) {
        sprintf(user_info->name, "%ju", (uintmax_t) uid);
        if (errno != 0) {
            user_info->uid = (uid_t) -1; /* if getpwuid() fails, set
            invalid uid to prevent writing garbage to the cache */
        }
    } else {
        strncpy(user_info->name, pwd->pw_name, UNAME_MAX);
    }

    user_info->next = user_info_cache;
    user_info_cache = user_info;

    return user_info->name;
}

/* Format the time specified by mtime and write it to
   given buffer using the format string TIME_FORMAT */
static void
format_time(time_t const mtime, char *formatted) {
    struct tm tm_info;
    localtime_r(&mtime, &tm_info);
    /* use reentrant version of localtime, as it's not
       required to call tzset() that causes overhead */
    strftime(formatted, TIME_LEN, TIME_FORMAT, &tm_info);
}

/* Get the file type for given mode. */
static int
get_file_type(mode_t const st_mode) {
    switch (st_mode & S_IFMT) {
        case S_IFDIR:
            return DIRECTORY;
        case S_IFLNK:
            return SYMLINK;
        case S_IFIFO:
            return FIFO;
        case S_IFSOCK:
            return SOCKET;
        default:
            if (st_mode & S_IXUSR) { /* first check for executable bit,
            as regular files can be executable too */
                return EXECUTABLE;
            } else if (S_ISREG(st_mode)) {
                return REGULAR;
            } else {
                return UNKNOWN;
            }
    }
}

/* Print the file information specified by file_info to stdout.
   owner_w and size_w are the max lengths of owner and size fields */
static void
print_file_info(struct file_info const *file_info, int const owner_w, int const size_w) {
    char permissions[PERM_LEN], date[TIME_LEN];
    char *owner;

    format_permissions(file_info->mode, permissions);
    format_time(file_info->mtime, date);
    owner = get_owner(file_info->uid);

    printf("%s %-*s %*ld %s %s%s",
           permissions,
           owner_w, owner,
           size_w, file_info->size,
           date,
           file_type_str[file_info->type],
           file_info->name);

    if (file_info->target != NULL) {
        printf(" -> %s", file_info->target);
    }

    printf("\n");
}

/* Read the target of the symbolic link specified by path.
   expected_size is the hint for the size of the target,
   returned by lstat(2). Return the null-terminated string
   or NULL if readlink(2) fails or the target is too long */
static char *
read_link(char const *path, size_t expected_size) {
    size_t buf_size;
    ssize_t read;

    if (expected_size == 0) { /* in some cases lstat(2) can report zero
        size for the pseudo-files like /proc/self/exe. We can't rely on
        this value, so instead fallback to _POSIX_SYMLINK_MAX as a guess
        to prevent multiple calls to readlink() and realloc() */
        expected_size = _POSIX_SYMLINK_MAX;
    }

    buf_size = expected_size < PATH_MAX ? expected_size + 1 : PATH_MAX;
    char *buffer = xmalloc(buf_size);

    while ((read = readlink(path, buffer, buf_size)) != -1) {
        if ((size_t) read < buf_size) {
            buffer[read] = '\0'; /* some linters may complain about
            out-of-bounds access, but it's false positive here */
            return buffer;
        }

        if (buf_size >= PATH_MAX) {
            break;
        }

        buf_size = buf_size <= PATH_MAX / 2 ? buf_size * 2 : PATH_MAX;
        buffer = xrealloc(buffer, buf_size);
    }

    file_error("failed to read symbolic link", path);
    free(buffer);
    return NULL;
}

/* Read the file information for given dir_path and file_name
   and store it in file_info. file-info->path is set to NULL
   if the file is not a directory. Return EXIT_SUCCESS on
   success or MINOR_ERROR on failure */
static int
read_file_info(char const *path, char const *file_name, struct file_info *file_info) {
    struct stat st;
    if (lstat(path, &st) == -1) {
        file_error("failed to get information", path);
        return MINOR_ERROR;
    }

    file_info->uid = st.st_uid;
    file_info->size = st.st_size;
    file_info->mtime = st.st_mtime;
    file_info->mode = st.st_mode;
    file_info->type = get_file_type(st.st_mode);

    char const *real_name = (file_name[0] == '\0') ? path : file_name;
    file_info->name = strdup(real_name);

    file_info->target = NULL;
    if (file_info->type == SYMLINK) {
        file_info->target = read_link(path, st.st_size);
    }

    return EXIT_SUCCESS;
}

/* Free the count entries starting from file_info. */
static void
free_file_info(struct file_info *file_info, size_t const count) {
    for (size_t i = 0; i < count; i++) {
        free(file_info[i].name);
        free(file_info[i].target);
    }
}

/* Concatenate the file_name to the path and return the
   length of the resulting string. If the path does not
   end with a slash, add it before concatenation */
static size_t
concat_path(char const *file_name, size_t const path_len, char *full_path) {
    size_t new_len = path_len;
    if (full_path[path_len - 1] != '/') {
        full_path[path_len] = '/';
        new_len++;
    }

    char const *end = stpcpy(full_path + new_len, file_name);
    return end - full_path;
}

/* Restore the path to its original state by adding
   the null-terminator to the end of the string.
   Should be called after concat_path() */
static void
restore_path(char *path, size_t const path_len) {
    path[path_len] = '\0';
}

/* Recursively list the contents of the directory specified by path */
static void
list_dir(char *path, size_t const path_len) { // NOLINT(*-no-recursion): recursion is required
    static int first_call = 1; /* flag to prevent printing an extra newline */

    struct file_info *files;
    size_t buf_size = INIT_BUF_SIZE;
    struct dirent const *entry;
    size_t entry_no = 0;

    int owner_w = 0, size_w = 0;

    DIR *dir = opendir(path);
    if (dir == NULL) {
        file_error("failed to open directory", path);
        return;
    }

    files = xmalloc(buf_size * sizeof(struct file_info));

    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        struct file_info info;
        (void) concat_path(entry->d_name, path_len, path);
        if (read_file_info(path, entry->d_name, &info) != 0) {
            continue;
        }
        restore_path(path, path_len);

        files[entry_no] = info;

        int owner_len = (int) strlen(get_owner(info.uid));
        int size_len = snprintf(NULL, 0, "%jd", (intmax_t) info.size);
        owner_w = (owner_len > owner_w) ? owner_len : owner_w;
        size_w = (size_len > size_w) ? size_len : size_w;

        if (entry_no++ == buf_size - 1) {
            buf_size *= 2;
            files = xrealloc(files, buf_size * sizeof(struct file_info));
        }
    }
    closedir(dir);

    qsort(files, entry_no, sizeof(struct file_info), file_name_cmp);

    if (!first_call) {
        printf("\n");
    }

    first_call = 0;
    printf("%s:\n", path);
    for (size_t i = 0; i < entry_no; i++) {
        print_file_info(&files[i], owner_w, size_w);
    }

    for (size_t i = 0; i < entry_no; i++) {
        if (files[i].type != DIRECTORY) {
            continue;
        }
        size_t const new_len = concat_path(files[i].name, path_len, path);
        list_dir(path, new_len);
        restore_path(path, path_len);
    }

    free_file_info(files, entry_no);
    free(files);
}

/* List the contents of the directory specified by path if it
   is a directory, otherwise print the information about the
   file specified by path. Return the value of exit_status */
static int
myrls(char const *path) {
    struct stat st;
    init();

    if (lstat(path, &st) == -1) {
        file_error("cannot access", path);
        return exit_status;
    }

    strcpy(current_path, path);

    if (S_ISDIR(st.st_mode)) {
        list_dir(current_path, strlen(path));
    } else {
        /* if myrls was called with a file argument, pay the price
           of double call to lstat(2) in favor of code simplicity.
           It would not be executed recursively anyway */
        struct file_info info;
        if ((read_file_info(current_path, "", &info)) != 0) {
            return exit_status;
        }
        print_file_info(&info, 0, 0);
        free_file_info(&info, 1);
    }

    return exit_status;
}

int
main(int argc, char *argv[]) {
    char const *path;
    int opt;

    while ((opt = getopt_long(argc, argv, "h", long_opts, NULL)) != -1) {
        usage((opt == 'h') ? EXIT_SUCCESS : FATAL_ERROR, NULL);
    }

    if (optind < argc - 1) {
        usage(FATAL_ERROR, "too many arguments");
    }
    path = (optind < argc) ? argv[optind] : DEFAULT_PATH;

    return myrls(path);
}
