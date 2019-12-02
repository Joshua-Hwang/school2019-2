/**
 * This implementation is VERY coupled but this was to improve
 * lookup speeds
 */

/**
 * TODO
 * Just a heads up future self.
 * Because this program needs to print all the grains and we're actually
 * destroying all the grains that suffocate you're gonna have to bucketsort
 * the final collection of grains to print them. The empty cells should tell
 * you which ones died.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <float.h>

#include "grain.h"

/* Structs */
/**
 * Contains all information about a collision between two grains
 * time contains the cached value of the time/distance/general metric
 * between two grains. Should be updated every timestep.
 * glst is the global node attached to this GrainPair.
 * g1lst is the grain 1 node attached to this GrainPair.
 * g2lst is the grain 1 node attached to this GrainPair.
 */
struct GrainPair {
    struct Grain* g1;
    struct Grain* g2;
    double time;
    struct GrainPairListNode *glst;
    struct GrainPairListNode *g1lst;
    struct GrainPairListNode *g2lst;
};

/**
 * Linkedlist containing all the grain pairs. Very useful.
 * l and r are left and right
 */
struct GrainPairListNode {
    struct GrainPair *gp;
    struct GrainPairListNode *l;
    struct GrainPairListNode *r;
};

/**
 * Contains all information pertaining to a single grain
 * x,y are spatial coordinates
 * t is the birth time
 * v is growth rate
 * r is maximum radius. Set to -1 if unknown and 0 if suffocated.
 * i is the Grain it collides with if r is positive
 * lst is a linkedlist of all pairs it's a part of
 * spair is a special pair since it's the best pair involving a stationary
 */
struct Grain {
    size_t id;
    double x, y, t;
    double v;
    double r;
    ssize_t i;
    /* GRAINS SHOULD BE ADDED TO THE RIGHT LIKE A STACK */
    struct GrainPairListNode lst; /* this is considered the head of the lst */
    struct GrainPair *spair;
};

/* Private Prototypes */
struct GrainPairListNode **get_gp_glst(struct GrainPair *gp, struct Grain *g);
void free_gpln_l(struct GrainPairListNode *lst);
void free_gpln_r(struct GrainPairListNode *lst);

/* Functions */
/**
 * Creates (calloc) space for a new GrainPair
 * Returns a new GrainPair on success
 * NULL on failure
 */
struct Grain *new_g(size_t id, double x, double y, double t,
        double v) {
    struct Grain *ret = calloc(1, sizeof(*ret));

    ret->id = id;
    ret->x = x;
    ret->y = y;
    ret->t = t;
    ret->v = v;

    ret->r = -1;
    ret->i = -1;
    ret->lst.gp = NULL;
    ret->lst.l = NULL;
    ret->lst.r = NULL;

    return ret;
}

/**
 * Frees and cleans up the Grain.
 * Will also free all GrainPairs ensuring they properly
 * remove their GrainPairListNodes from their parent lists.
 * Does nothing if g is NULL
 */
void free_g(struct Grain *g) {
    if (!g) { /* NULL */
        return;
    }
    /* Free all grain pairs via free_gp */
    for (struct GrainPairListNode *node = get_gpln_r(&(g->lst)); node /* NOTNULL */;) {
        /* free_gp will also remove oldnode so we have to jump prior */
        struct GrainPairListNode *oldnode = node;
        node = get_gpln_r(node);

        free_gp(get_gpln_gp(oldnode));
    }

    /* Free the special spair */
    free_gp(g->spair);

    /* Free itself and also zero memory just to be safe */
    memset(g, 0, sizeof(*g));
    free(g);
}

/**
 * converts a grain to an allocated printable string
 */
char *g_to_str(struct Grain *g) {
    size_t needed = snprintf(NULL, 0,
            "Grain %lu: x=%lf, y=%lf, t=%lf, v=%lf, r=%lf, i=%zd",
            g->id, g->x, g->y, g->t, g->v, g->r, g->i) + 1;

    char *buffer = calloc(needed, sizeof(*buffer));

    snprintf(buffer, needed,
            "Grain %lu: x=%lf, y=%lf, t=%lf, v=%lf, r=%lf, i=%zd",
            g->id, g->x, g->y, g->t, g->v, g->r, g->i);

    return buffer;
}

/**
 * In this particular instance the time is actually the global time
 */
double calc_g_r(struct Grain *g, double time) {
    double t = get_g_t(g);
    return time > t ? get_g_v(g) * (time - t) : 0;
}

/**
 * Goes through its list and removes the pairs with two stationary in them.
 * Also consider setting itself as a grains spair if it's the best.
 * Keep in mind when we're replacing spair we have to remove an entry from glst
 */
void set_g(struct Grain *g, double r, size_t i) {
    g->r = r;
    g->i = i;

    /*
     * If we have a pair of stationary grains or we have a suffocated grain.
     * In that case we should throw out ALL pairs involved with it
     * Luckily we're not destorying someone's spair because an overlap
     * only ever occurs between two growing grains.
     */
    if (r == 0) {
        for (struct GrainPairListNode *node = get_gpln_r(&(g->lst)); node;) {
            /* free_gp will also remove oldnode so we have to jump prior */
            struct GrainPairListNode *oldnode = node;
            node = get_gpln_r(node);

            free_gp(get_gpln_gp(oldnode));
        }

        /* Free the special spair */
        free_gp(g->spair);

        return;
    }

    for (struct GrainPairListNode *node = get_gpln_r(&(g->lst)); node;) {
        /* free_gp will also remove oldnode so we have to jump prior */
        struct GrainPairListNode *oldnode = node;
        node = get_gpln_r(node);

        struct GrainPair *gp = get_gpln_gp(oldnode);
        if (is_gp_done(gp)) {
            free_gp(gp);
        } else { /* Otherwise consider replacing spair */
            /* Remove self from other grain (og) list (assuming og has lst) */
            struct Grain *og = get_gp_other(gp, g);
            struct GrainPairListNode **oglstnode = get_gp_glst(gp, og);
            free_gpln(*(oglstnode));
            *oglstnode = NULL;

            /* Compare our time to spair */
            /* Update our gp time */
            double gptime = calc_gp_time(gp);
            set_gp_time(gp, gptime);

            double sptime = (og->spair) ? get_gp_time(og->spair) : DBL_MAX;
            /* Which of these isn't us */
            if (gptime < sptime) { /* replace the old spair */
                og->spair = gp;
            } else { /* Free pair since both grains don't need it anymore */
                free_gp(gp);
            }
        }
    }
}

size_t get_g_id(struct Grain *g) {
    return g->id;
}

double get_g_x(struct Grain *g) {
    return g->x;
}

double get_g_y(struct Grain *g) {
    return g->y;
}

double get_g_t(struct Grain *g) {
    return g->t;
}

double get_g_v(struct Grain *g) {
    return g->v;
}

/**
 * r is maximum radius. Set to -1 if unknown and 0 if suffocated.
 */
double get_g_r(struct Grain *g) {
    return g->r;
}

/**
 * i is the Grain it collides with if r is positive
 * otherwise return -1
 */
ssize_t get_g_i(struct Grain *g) {
    return g->i;
}

/**
 * Returns true if the grain has a radius
 */
bool is_g_done(struct Grain *g) {
    return get_g_r(g) >= 0;
}

/**
 * Creates (calloc) space for a new GrainPair
 * Returns a new GrainPair on success
 * NULL on failure
 *
 * [IMPL] This function should append itself to each grains linked
 * list. We can do this because these pairs are expected to be unique
 * unlike the GrainPairListNodes
 */
struct GrainPair *new_gp(struct Grain *g1, struct Grain *g2) {
    struct GrainPair *ret = calloc(1, sizeof(*ret));
    ret->g1 = g1;
    ret->g2 = g2;
    /* update time */
    set_gp_time(ret, calc_gp_time(ret));
    ret->glst = NULL;

    /* add this pair to g1 and g2 then save the nodes we created */
    struct GrainPairListNode *g1lst = new_gpln(ret);
    ret->g1lst = g1lst;
    add_gpln_r(&(g1->lst), g1lst);

    struct GrainPairListNode *g2lst = new_gpln(ret);
    ret->g2lst = g2lst;
    add_gpln_r(&(g2->lst), g2lst);

    return ret;
}

/**
 * Frees and cleans up the GrainPair.
 * Will also free all GrainPairListNodes ensuring they're properly
 * removed from their lists.
 * Doesn't get rid of the Grains.
 * Does nothing if gp is NULL.
 */
void free_gp(struct GrainPair *gp) {
    if (!gp) { /* NULL */
        return;
    }

    free_gpln(gp->glst);
    free_gpln(gp->g1lst);
    free_gpln(gp->g2lst);

    /* Just to be safe memset it */
    memset(gp, 0, sizeof(*gp));
    free(gp);
}

/**
 * Gets the first grain of the grain pair
 */
struct Grain *get_gp_1(struct GrainPair *gp) {
    return gp->g1;
}

/**
 * Gets the second grain of the grain pair
 */
struct Grain *get_gp_2(struct GrainPair *gp) {
    return gp->g2;
}

/**
 * Assuming g is actually in the grain pair return the other
 * Returns NULL on failure
 * [IMPL] This is done by comparing the id of each grain
 */
struct Grain *get_gp_other(struct GrainPair *gp, struct Grain *g) {
    size_t id = get_g_id(g);
    size_t id1 = get_g_id(get_gp_1(gp));
    size_t id2 = get_g_id(get_gp_2(gp));

    if (id == id1) {
        return get_gp_2(gp);
    } else if (id == id2) {
        return get_gp_1(gp);
    } else {
        return NULL;
    }
}

/**
 * Assuming g is actually in the grain pair, return the entry in the grainpair
 * about the grain's list node entry for the pair gp
 * Returns NULL on failure
 * [IMPL] This is done by comparing the id of each grain
 */
struct GrainPairListNode **get_gp_glst(struct GrainPair *gp,
        struct Grain *g) {
    size_t id = get_g_id(g);
    size_t id1 = get_g_id(get_gp_1(gp));
    size_t id2 = get_g_id(get_gp_2(gp));

    if (id == id1) {
        return &(gp->g1lst);
    } else if (id == id2) {
        return &(gp->g2lst);
    } else {
        return NULL;
    }
}

/**
 * Checks if both grains in the pair are already realised
 */
bool is_gp_done(struct GrainPair *gp) {
    return is_g_done(get_gp_1(gp)) && is_g_done(get_gp_2(gp));
}

/**
 * Sets the glst to what the GrainPair considers is the GlobalListNode it resides in
 * Will override an old value
 */
void set_gp_glst(struct GrainPair *gp, struct GrainPairListNode *glst) {
    gp->glst = glst;
}

/**
 * Computes the time of the GrainPair but doesn't store it
 * If overlap it will be time of overlap.
 */
double calc_gp_time(struct GrainPair *gp) {
    struct Grain *g1 = get_gp_1(gp);
    struct Grain *g2 = get_gp_2(gp);

    double distx = get_g_x(g1) - get_g_x(g2);
    double disty = get_g_y(g1) - get_g_y(g2);
    double dist = sqrt(distx*distx + disty*disty);

    double v1 = get_g_v(g1);
    double v2 = get_g_v(g2);
    double t1 = get_g_t(g1);
    double t2 = get_g_t(g2);

    /* check if one of them is realised */
    if (!is_g_done(g1) && !is_g_done(g2)) {
        /* Both are growing */
        /* Check for overlap */
        if ((dist/v1 + t1) < t2) {
            return dist/v1 + t1;
        }
        if ((dist/v2 + t2) < t1) {
            return dist/v2 + t2;
        }
        /* Everything works fine */
        return (dist + v1 * t1 + v2 * t2)/(v1 + v2);
    }

    /* One of them is stationary */
    /* vg is the growth rate of the grower */
    /* rs is the growth rate of the stopper */
    double vg, tg, rs;
    if ((rs = get_g_r(g1)) >= 0) {
        /* g1 is the stationary */
        vg = v2;
        tg = t2;
    } else {
        rs = get_g_r(g2);
        vg = v1;
        tg = t1;
    }
    /* Everything works out fine */
    return (dist - rs)/(vg) + tg;
}

/**
 * Gets the time of the grain pair
 */
double get_gp_time(struct GrainPair *gp) {
    return gp->time;
}

/**
 * Sets the time of the grain pair
 */
void set_gp_time(struct GrainPair *gp, double newtime) {
    gp->time = newtime;
}

/**
 * Only sets grains that are growing
 * Sets both grains in the grain pair using the time in gp
 */
void set_gp(struct GrainPair *gp) {
    struct Grain *g1 = get_gp_1(gp);
    struct Grain *g2 = get_gp_2(gp);

    double time = get_gp_time(gp);

    if (!is_g_done(g1)) {
        double r1 = calc_g_r(g1, time);
        size_t id2 = get_g_id(g2);
        set_g(g1, r1, id2);
    }
    if (!is_g_done(g2)) {
        double r2 = calc_g_r(g2, time);
        size_t id1 = get_g_id(g1);
        set_g(g2, r2, id1);
    }
}

/**
 * Create (calloc) space for a new GrainPairListNode
 * Returns a new GrainPairListNode on success
 * NULL on failure
 */
struct GrainPairListNode *new_gpln(struct GrainPair *gp) {
    struct GrainPairListNode *ret = calloc(1, sizeof(*ret));
    ret->gp = gp;

    return ret;
}

/**
 * Frees and cleans up a node.
 * Doesn't get rid of the GrainPair.
 * Does nothing if node is NULL
 */
void free_gpln(struct GrainPairListNode *node) {
    if (!node) { /* NULL */
        return;
    }
    /* This is for my own protection because one day I'll forget */
    rm_gpln(node);
    /* for the sake of cleanliness also memset the memory */
    memset(node, 0, sizeof(*node));

    free(node);
}

/**
 * Gets the left node of the GrainPairListNode
 * Returns NULL if there isn't one
 */
struct GrainPairListNode *get_gpln_l(struct GrainPairListNode *node) {
    return node->l;
}

/**
 * Gets the right node of the GrainPairListNode
 * Returns NULL if there isn't one
 */
struct GrainPairListNode *get_gpln_r(struct GrainPairListNode *node) {
    return node->r;
}

/**
 * Gets the GrainPair from the GrainPairListNode
 * Returns NULL if there isn't one
 */
struct GrainPair *get_gpln_gp(struct GrainPairListNode *node) {
    return node->gp;
}

/**
 * Inserts the node to the left of the list
 */
void add_gpln_l(struct GrainPairListNode *lst, struct GrainPairListNode *node) {
    struct GrainPairListNode *oldl = get_gpln_l(lst);

    lst->l = node;
    node->r = lst;

    if (oldl) { /* NOTNULL */
        oldl->r = node;
        node->l = oldl;
    }
}

/**
 * Inserts the node to the right of the list
 */
void add_gpln_r(struct GrainPairListNode *lst, struct GrainPairListNode *node) {
    struct GrainPairListNode *oldr = get_gpln_r(lst);

    lst->r = node;
    node->l = lst;

    if (oldr) { /* NOTNULL */
        oldr->l = node;
        node->r = oldr;
    }
}

/**
 * Removes the current GrainPairListNode from its list
 */
void rm_gpln(struct GrainPairListNode *node) {
    struct GrainPairListNode *l = get_gpln_l(node);
    struct GrainPairListNode *r = get_gpln_r(node);

    node->l = NULL;
    node->r = NULL;

    if (l) { /* NOTNULL */
        l->r = r;
    }
    if (r) { /* NOTNULL */
        r->l = l;
    }
}

/**
 * Performs free_gpln on all nodes to the left of lst
 * This is a helper function that will be used recursively
 */
void free_gpln_l(struct GrainPairListNode *lst) {
    struct GrainPairListNode *l = lst->l;

    free_gpln(lst);

    free_gpln_l(l);
    if (l) { /* NOTNULL */
        free_gpln_l(l);
    }
}

/**
 * Performs free_gpln on all nodes to the right of lst
 * This is a helper function that will be used recursively
 */
void free_gpln_r(struct GrainPairListNode *lst) {
    struct GrainPairListNode *r = lst->r;

    free_gpln(lst);

    if (r) { /* NOTNULL */
        free_gpln_r(r);
    }
}

/**
 * Performs free_gpln on all nodes of lst including self
 */
void free_gpln_a(struct GrainPairListNode *lst) {
    if (lst->l) { /* NOTNULL */
        free_gpln_l(lst->l);
    }
    if (lst->r) { /* NOTNULL */
        free_gpln_r(lst->r);
    }
    free_gpln(lst);
}
