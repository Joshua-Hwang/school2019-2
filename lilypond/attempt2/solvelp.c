/**
 * Another optimisation that can be done is to order the list.
 * Recall that pairs only get worse values thus it only requires
 * us to find the pair that has remained on top and "semi sort" the
 * old head.
 * Of course as we continue the ordering breaks but at least the longest
 * lists get sped up.
 * I'm writing this here in the hope that nothing needs to change in the backend
 * since this optimisation is focussed on finding the minimum in the global list.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <float.h>

/* For timing the operation */
#include <time.h>

#include "grain.h"

#define MAX_BUFFER 256

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
 * Pass a pointer to ret to get a allocated array back.
 * Returns the number of elements parsed
 */
size_t parse_stdin(struct Grain ***ret) {
    /* read each line from stdin and parse it */
    size_t id = 0, len = 2;
    struct Grain **grains = calloc(len, sizeof(*grains));
    char buffer[MAX_BUFFER];
    while (fgets(buffer, MAX_BUFFER, stdin)) { /* NOTNULL */
        /* Check if it's a comment or the real deal */
        if (buffer[0] == '#') {
            continue;
        }

        /* it's a legit row. Now check if the row gets successfully parsed */
        double x, y, t, v, r;
        size_t i;
        /* We actually don't care about r or i but we parse them anyway */
        if (sscanf(buffer, "%lf,%lf,%lf,%lf,%lf,%lu\n", &x, &y, &t, &v, &r, &i)
                < 6) {
            err(EXIT_FAILURE, "Invalid row: %s", buffer);
        }

        grains[id] = new_g(id, x, y, t, v);

        /* check if our array is too small */
        id++;
        while (id >= len) {
            /* resize */
            len = len * 1.5;
            grains = realloc(grains, len * sizeof(*grains));
        }
    }

    /* throw error if we only parse two points */
    if (len <= 2) {
        err(EXIT_FAILURE, "Only parsed two points");
    }

    (*ret) = grains;
    return id;
}

void solve(int argc, char **argv) {
    parse_args(argc, argv);

    struct Grain **grains;
    size_t len = parse_stdin(&grains);

    /* generate the head with a grain pair of NULL */
    struct GrainPairListNode *head = new_gpln(NULL);

    /* while we're here might as well find the minimum pair */
    struct GrainPairListNode *minnode;
    double mintime = DBL_MAX;

    /* generate a global list of pairs */
    /* the grain.c should handle everything on the backend all we need to do is set_gp_glst */
    for (int i = 0; i < len; i++) {
        for (int j = i+1; j < len; j++) {
            struct GrainPair *gp = new_gp(grains[i], grains[j]);
            struct GrainPairListNode *gpln = new_gpln(gp);

            double time = calc_gp_time(gp);
            set_gp_time(gp, time);
            /* check if minimum */
            if (time < mintime) {
                mintime = time;
                minnode = gpln;
            }

            /* register new node in all the right places */
            add_gpln_r(head, gpln);
            set_gp_glst(gp, gpln);
        }
    }

    /* when we have a minimum it's important to them remove the gpln */
    set_gp(get_gpln_gp(minnode));

    while (get_gpln_r(head)) { /* NOTNULL */
        minnode = NULL;
        mintime = DBL_MAX;
        /* find minimum */
        for (struct GrainPairListNode *gpln = get_gpln_r(head); gpln;
                gpln = get_gpln_r(gpln)) {
            /* check if minimum */
            double time = get_gp_time(get_gpln_gp(gpln));
            if (time < mintime) {
                mintime = time;
                minnode = gpln;
            }
        }

        /* when we have a minimum it's important to them remove the gpln */
        rm_gpln(minnode);
        set_gp(get_gpln_gp(minnode));
    }

    for (int i = 0; i < len; i++) {
        struct Grain *g = grains[i];
        double x = get_g_x(g);
        double y = get_g_y(g);
        double t = get_g_t(g);
        double v = get_g_v(g);
        double r = get_g_r(g);
        ssize_t i = get_g_i(g);
        printf("%lf,%lf,%lf,%lf,%lf,%zu\n",x,y,t,v,r,i);
    }
}

/**
 * Prints the result to stdout if no filename specified
 */
int main(int argc, char **argv) {
    /* Print the amount of time spent */
    clock_t begin = clock();
    solve(argc, argv);
    clock_t end = clock();

    double time = (double)(end - begin) / CLOCKS_PER_SEC;

    fprintf(stderr, "%s solve time: %lf\n", argv[0], time);

    return 0;
}
