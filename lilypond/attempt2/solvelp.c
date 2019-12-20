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
#include <ctype.h>
#include <math.h>

/* For timing the operation */
#include <time.h>

#include "grain.h"

#define C_DIMENSIONS 'D'
#define C_GRAINS 'Z'
#define MAX_BUFFER 1024

void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s [-i inputfile] [-o outputfile]\n", progname);
}

/* Prototypes */
void print_gp(struct GrainPair *gp);

/* Functions */

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
 * Helper function for getting the next "important line"
 * ignores comments
 *
 * Has the same signature as fgets
 */
char *fgets_ignore_comments(char *s, int size, FILE *stream) {
    char *ret;
    while ((ret = fgets(s, size, stream))) {
        /* Check if it's a comment or the real deal */
        if (s[0] != '#') {
            break;
        }
    }
    return ret;
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
 * Helper function that checks the current line is a section header.
 * A section header is a line of reoccurring alphebetical
 * characters (at least 2) that make up the entire line.
 *
 * NOTE: if len is -1 then we assumes the line is null terminated
 */
bool is_section_header(const char *line, const ssize_t len) {
    char c = line[0];
    if (!isalpha(c)) {
        return false;
    }

    /* [IMPL] Just a linear check for reoccurrence */
    /* Though ugly looking I'm pretty sure this makes things faster */
    if (len < 0) {
        /* Assume null terminated */
        for (int i = 1; line[i] != '\0' && line[i] != '\n'; i++) {
            if (line[i] != c) {
                return false;
            }
        }
    } else {
        if (len < 2) {
            /* not long enough to be a header */
            return false;
        }
        for (int i = 1; i < len; i++) {
            if (line[i] != c) {
                return false;
            }
        }
    }

    /* Nothing failed so it must be true */
    return true;
}

/**
 * Helper function that parses the dimensions given
 * from a single line.
 *
 * Using strtok_r so will destroy the first argument
 */
bool parser_dimensions(char *line, struct SystemInfo *si) {
    /* A line in the Dimensions section consists of the following
     * Z,bmin,bmax,W
     * Where Z is the dimensions (currenly only X or Y)
     * bmin and bmax are boundary minimum and boundary maximum
     * W is either W or H and defines wrapping or hard boundaries
     */
    /* I'm using strtok_r just to be different */
    /* parse the first parameter like an unsigned char */
    char *endptr = NULL, *saveptr = NULL;
    char *token = strtok_r(line, ",", &saveptr);
    /* very generous parsing */
    Dimension d = atoi(token);
    if (d > NUM_DIMENSIONS) {
        return false;
    }

    /* parse the second and third parameter like doubles */
    token = strtok_r(NULL, ",", &saveptr);
    if (!token) { /* NULL */
        return false;
    }
    errno = 0;
    double dmin = strtod(token, &endptr);
    if (errno == ERANGE || endptr == token) {
        return false;
    }

    token = strtok_r(NULL, ",", &saveptr);
    if (!token) { /* NULL */
        return false;
    }
    errno = 0;
    double dmax = strtod(token, &endptr);
    if (errno == ERANGE || endptr == token) {
        return false;
    }

    /* parse the final parameter as character and convert to enum */
    token = strtok_r(NULL, ",", &saveptr);
    if (!token) { /* NULL */
        return false;
    }
    /* I'm sure this won't go wrong at all :) */
    double dstyle = (enum BorderStyle) toupper((unsigned char)token[0]);

    /* All parsing succeeded */
    set_si_dim(si, d, dmin, dmax, dstyle);
    return true;
}

/**
 * Helper function that does nothing
 */
bool parser_skip(char *line, struct SystemInfo *ai) {
    return true;
}

/**
 * Function designed to handle the other sections of the
 * lps files that weren't handled by parse_grains.
 * Specifically handling the various sections that can be added to
 * the file. Only DDD sections are handled. All others are
 * ignored.
 *
 * ai will contain the information parsed.
 * c determines what kind of section to pass.
 *
 * Returns number of sections skipped unless an error occurs which will
 * return -1
 *
 * NOTE: Sections that aren't parsed will not appear in this binaries output.
 * Please only use this function before calling parse_grains. Undefined behaviour
 * occurs otherwise.
 */
ssize_t parse_headers(struct SystemInfo *si, char *c) {
    if (!isalpha(*c)) {
        /* Not a real header so just return */
        return 0;
    }

    bool skipped = false; /* Have we skipped this sections */
    bool (*parser)(char *, struct SystemInfo *);
    switch (*c) {
        case C_DIMENSIONS:
            parser = parser_dimensions;
            break;
        case C_GRAINS:
            /* We've reached grains */
            return skipped;
        default:
            /* It's a header we don't understand so skip */
            parser = parser_skip;
            skipped = true;
    }
    /* Check if we're already at the end of the file */
    char buffer[MAX_BUFFER];
    while (fgets_ignore_comments(buffer, sizeof(buffer), stdin)) { /* NOTNULL */
        /* Check if we're done parsing and we've reached a new section */
        if (is_section_header(buffer, -1)) {
            /* ignore if same header */
            if (*c == buffer[0]) {
                continue;
            }
            *c = buffer[0];
            break; 
        }

        if (!parser(buffer, si)) {
            /* parsing that line failed */
            return -1;
        }
    }

    /* We'll recursively call parse_headers because why not */
    ssize_t res = parse_headers(si, c);
    return (res >= 0) ? skipped + res : -1;
}

/**
 * Pass a pointer to ret to get a allocated array back.
 * If the address at c is not NULL then the first character taken from
 * fgets is put into c.
 * If c is alphabetical then additional sections need to be parsed.
 *
 * Returns the number of elements parsed
 */
size_t parse_grains(struct SystemInfo *si, struct Grain ***ret, char *c) {
    size_t id = 0, len = 2;
    struct Grain **grains = calloc(len, sizeof(*grains));
    char buffer[MAX_BUFFER];
    while (fgets_ignore_comments(buffer, sizeof(buffer), stdin)) { /* NOTNULL */
        /* Check if we're done parsing and we've reached a new section */
        if (is_section_header(buffer, -1)) {
            /* ignore if same header */
            if (*c == buffer[0]) {
                continue;
            }
            *c = buffer[0];
            break; 
        }
        /* it's a legit row. Now check if the row gets successfully parsed */
        double x, y, t, v, r;
        size_t i;
        /* We actually don't care about r or i but we parse them anyway */
        if (sscanf(buffer, "%lf,%lf,%lf,%lf,%lf,%lu\n", &x, &y, &t, &v, &r, &i)
                < 6) {
            errx(EXIT_FAILURE, "Invalid row: %s", buffer);
        }

        grains[id] = new_g(si, id, x, y, t, v);

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
        errx(EXIT_FAILURE, "Only parsed two points");
    }

    (*ret) = grains;
    return id;
}

size_t parse_footer(struct SystemInfo *si, char *c) {
    if (!isalpha(*c)) {
        /* Not a real header so just return */
        return 0;
    }

    bool skipped = 0; /* The number of sections we've currently skipped */
    bool (*parser)(char *, struct SystemInfo *);
    switch (*c) {
        default:
            /* It's a footer we don't understand so skip */
            parser = parser_skip;
            skipped = true;
    }
    /* Check if we're already at the end of the file */
    char buffer[MAX_BUFFER];
    while (fgets_ignore_comments(buffer, sizeof(buffer), stdin)) { /* NOTNULL */
        /* Check if we're done parsing and we've reached a new section */
        if (is_section_header(buffer, -1)) {
            /* ignore if same header */
            if (*c == buffer[0]) {
                continue;
            }
            *c = buffer[0];
            break; 
        }

        if (!parser(buffer, si)) {
            /* parsing that line failed */
            return -1;
        }
    }

    /* We'll recursively call parse_headers because why not */
    ssize_t res = parse_headers(si, c);
    return (res >= 0) ? skipped + res : -1;
}

void solve(struct SystemInfo *si, struct Grain **grains, size_t len,
        struct GrainPairArray *gpa) {
    /* generate the head with a grain pair of NULL */
    struct GrainPairListNode *head = new_gpln(NULL);

    /* while we're here might as well find the minimum pair */
    struct GrainPairListNode *minnode = NULL;
    double mintime = INFINITY;

    /* generate a global list of pairs */
    /* the grain.c should handle everything on the backend all we need to do is set_gp_glst */
    for (int i = 0; i < len; i++) {
        /* register the stationary pair created with the new_g() */
        struct GrainPair *gp = get_g_spair(grains[i]);
        struct GrainPairListNode *gpln = new_gpln(gp);

        double time = get_gp_time(gp);
        /* check if minimum */
        if (time < mintime) {
            mintime = time;
            minnode = gpln;
        }

        /* register new node in all the right places */
        add_gpln_r(head, gpln);
        set_gp_glst(gp, gpln);

        for (int j = i+1; j < len; j++) {
            gp = new_gp(si, grains[i], grains[j]);
            gpln = new_gpln(gp);

            double time = get_gp_time(gp);
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
    set_gp(get_gpln_gp(minnode), gpa);

    while (get_gpln_r(head)) { /* NOTNULL */
        minnode = NULL;
        mintime = INFINITY;
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
        set_gp(get_gpln_gp(minnode), gpa);
    }

    sort_gpa(gpa);
}

/**
 * Helper function used to iterate and print each grain pair
 */
void print_gp(struct GrainPair *gp) {
    struct Grain *g1 = get_gp_1(gp);
    struct Grain *g2 = get_gp_2(gp);

    printf("%zu,%zd,%lf\n",
            get_g_id(g1), g2 ? get_g_id(g2) : NO_INDEX,
            get_gp_dist(gp));
}

/**
 * All information that had been successfully parsed into SystemInfo
 * are regurgitated here.
 */
void print_headers(struct SystemInfo *si) {
    for (int i = 0; i < 10; i++) {
        putchar('D');
    }
    putchar('\n');
    for (Dimension d = 0; d < NUM_DIMENSIONS; d++) {
        if (get_si_dstyle(si, d) != BORDER_NONE) {
            printf("%u,%lf,%lf,%c\n", d,
                    get_si_dmin(si, d), get_si_dmax(si, d),
                    /* This part works because how the enum is defined */
                    get_si_dstyle(si, d));
        }
    }
}

void print_grains(struct Grain **grains, size_t len) {
    for (int i = 0; i < 10; i++) {
        putchar('Z');
    }
    putchar('\n');

    for (int i = 0; i < len; i++) {
        struct Grain *g = grains[i];
        double x = get_g_x(g);
        double y = get_g_y(g);
        double t = get_g_t(g);
        double v = get_g_v(g);
        double r = get_g_r(g);
        ssize_t i = get_g_i(g);
        printf("%lf,%lf,%lf,%lf,%lf,%zd\n",x,y,t,v,r,i);
    }
}

void print_footers(struct SystemInfo *si, struct GrainPairArray *gpa) {
    for (int i = 0; i < 10; i++) {
        putchar('X');
    }
    putchar('\n');
    for_gpa(gpa, print_gp);
}

/**
 * Prints the result to stdout if no filename specified
 */
int main(int argc, char **argv) {
    parse_args(argc, argv);
    struct Grain **grains;
    struct GrainPairArray *gpa = new_gpa();
    struct SystemInfo *si = new_si();
    char c[2]; /* store single character and null terminator */
    fgets_ignore_comments(c, sizeof(c), stdin);
    ssize_t skippedheaders = parse_headers(si, c);
    if (skippedheaders < 0) {
        /* Errors have occurred */
        err(EXIT_FAILURE, "parse_headers");
    }
    size_t len = parse_grains(si, &grains, c);
    ssize_t skippedfooters = parse_footer(si, c);
    if (skippedfooters < 0) {
        /* Errors have occurred */
        err(EXIT_FAILURE, "parse_headers");
    }

    /* Print the amount of time spent */
    clock_t begin = clock();
    solve(si, grains, len, gpa);
    clock_t end = clock();

    double time = (double)(end - begin) / CLOCKS_PER_SEC;

    fprintf(stderr, "%s skipped sections: %lu\n", argv[0],
            skippedheaders + skippedfooters);
    fprintf(stderr, "%s solve grain: %lu\n", argv[0], len);
    fprintf(stderr, "%s solve time: %lf\n", argv[0], time);

    /* Print the information */
    print_headers(si);
    print_grains(grains, len);
    print_footers(si, gpa);

    return 0;
}
