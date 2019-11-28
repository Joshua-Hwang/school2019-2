#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <string.h>

/* Using the gnu science libraries */
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [-i inputfile] [-o outputfile]\n", progname);
}

/**
 * Redirects a file descriptor to a file by filename
 * Only uses the mode argument if oflag & O_CREAT
 * Exits if there is an error with the filename.
 */
void set_fd(const char *filename, int oldfd, int oflag, ...) {
    /* Annoying part for variadic functions */
    mode_t mode = 0;
    if (oflag & O_CREAT) {
        va_list ap; va_start(ap, oflag); /* initialise variadic arguments */
        mode = va_arg(ap, mode_t);
        va_end(ap);
    }

    /* Opening and duping the file */
    int fd = open(filename, oflag, mode);
    if (fd == -1) {
        err(EXIT_FAILURE, "%s", filename);
    }
    dup2(fd, oldfd);
}

/**
 * Parses arguments.
 * Exits if any of these arguments fail
 */
void parse_args(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "hi:o:")) != -1) {
        switch (opt) {
        case 'h':
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
            break;
        case 'i':
            set_fd(optarg, STDIN_FILENO, O_RDONLY);
            break;
        case 'o':
            set_fd(optarg, STDOUT_FILENO, O_RDWR | O_CREAT, 0666);
            break;
        default: /* '?' */
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) { /* too many arguments */
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
}


/**
 * Prints the result to stdout if no filename specified
 */
int main(int argc, char **argv) {
    parse_args(argc, argv);

    printf("hello\n");

    return 0;
}
