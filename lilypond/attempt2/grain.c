/**
 * I don't like how coupled this is all getting...
 */

#include "grain.h"

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
    struct GrainPairListNode *lst;
    struct GrainPair *spair;
};

/**
 * Contains all information about a collision between two grains
 * score contains the cached value of the time/distance/general metric
 * between two grains. Should be updated every timestep.
 * gnode is the global node attached to this GrainPair.
 */
struct GrainPair {
    struct Grain* g1;
    struct Grain* g2;
    double score;
    struct GrainPairListNode *glst;
}

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
 * Performs free_gpln on all nodes to the left of lst
 */
void free_gpln_l(struct GrainPairListNode *lst) {
    if (lst) { /* NULL */
        return;
    }

    struct GrainPairListNode *l = lst->l;

    free_gpln(lst);

    free_gpln_l(l);
}

/**
 * Performs free_gpln on all nodes to the right of lst
 */
void free_gpln_r(struct GrainPairListNode *lst) {
    if (lst) { /* NULL */
        return;
    }

    struct GrainPairListNode *r = lst->r;

    free_gpln(lst);

    free_gpln_r(r);
}

/**
 * Creates (calloc) space for a new GrainPair
 * Returns a new GrainPair on success
 * NULL on failure
 */
struct Grain *new_g(size_t id, double x, double y, double t,
        double v) {
    struct Grain *ret = calloc(1, sizeof(ret));

    ret->id = id;
    ret->x = x;
    ret->y = y;
    ret->t = t;
    ret->v = v;

    ret->r = -1;
    ret->i = -1;
    ret->lst = NULL;

    return ret;
}

/**
 * Frees and cleans up the Grain.
 * Will also free all GrainPairs ensuring they properly
 * remove their GrainPairListNodes from their parent lists.
 */
void free_g(struct Grain *g) {
    for (struct GrainPairListNode *node = g->lst;
    free(g);
}

/**
 * Goes through its list and removes the pairs with two stationary in them.
 *
 */
void set_g(struct Grain *g, double r, size_t i) {

}

size_t get_g_id(struct Grain *g) {

}

double get_g_x(struct Grain *g) {

}

double get_g_y(struct Grain *g) {

}

double get_g_t(struct Grain *g) {

}

double get_g_v(struct Grain *g) {

}

/**
 * r is maximum radius. Set to -1 if unknown and 0 if suffocated.
 */
double get_g_r(struct Grain *g) {

}

/**
 * i is the Grain it collides with if r is positive
 * otherwise return -1
 */
ssize_t get_g_i(struct Grain *g) {

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

}

/**
 * Frees and cleans up the GrainPair.
 * Will also free all GrainPairListNodes ensuring they're properly
 * removed from their lists.
 * Doesn't get rid of the Grains.
 */
void free_gp(struct GrainPair *gp) {

}

/**
 * Gets the first grain of the grain pair
 */
double get_gp_1(struct GrainPair *gp) {

}

/**
 * Gets the second grain of the grain pair
 */
double get_gp_2(struct GrainPair *gp) {

}

/**
 * Checks if both grains in the pair are already realised
 */
bool is_gp_done(struct GrainPair *gp) {

}

/**
 * Sets the glst to what the GrainPair considers is the GlobalListNode it resides in
 */
void set_gp_glst(struct Grainpair *gp, struct GrainPairListNode *glst) {

}

/**
 * Computes the score of the GrainPair but doesn't store it
 */
double calc_gp_score(struct GrainPair *gp) {

}

/**
 * Gets the score of the grain pair
 */
double get_gp_score(struct GrainPair *gp) {

}

/**
 * Sets the score of the grain pair
 */
void set_gp_score(struct GrainPair *gp, double newscore) {

}

/**
 * Create (calloc) space for a new GrainPairListNode
 * Returns a new GrainPairListNode on success
 * NULL on failure
 */
struct GrainPairListNode *new_gpln(struct GrainPair *gp) {

}

/**
 * Frees and cleans up a node.
 * Doesn't get rid of the GrainPair.
 */
void free_gpln(struct GrainPairListNode *node) {
    if (!node) { /* NULL */
        return NULL;
    }

    free(node);
}

/**
 * Gets the left node of the GrainPairListNode
 * Returns NULL if there isn't one
 */
struct GrainPairListNode *get_gpln_l(struct GrainPairListNode *node) {
    if (!node) { /* NULL */
        return NULL;
    }

    return node->r;
}

/**
 * Gets the right node of the GrainPairListNode
 * Returns NULL if there isn't one
 */
struct GrainPairListNode *get_gpln_r(struct GrainPairListNode *node) {
    if (!node) { /* NULL */
        return NULL;
    }

    return node->r;
}

/**
 * Gets the GrainPair from the GrainPairListNode
 * Returns NULL if there isn't one
 */
struct GrainPair *get_gpln_gp(struct GrainPairListNode *node) {
    if (!node) { /* NULL */
        return NULL;
    }

    return node->gp;
}

/**
 * Inserts the node to the left of the list
 */
void add_gpln_l(struct GrainPairListNode *lst, struct GrainPairListNode *node) {
    if (!(lst && node)) { /* NULL */
        return;
    }

    struct GrainPairListNode oldl = get_gpln_l(lst);

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
    if (!(lst && node)) { /* NULL */
        return;
    }

    struct GrainPairListNode oldr = get_gpln_r(lst);

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
    if (!node) { /* NULL */
        return;
    }

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
 * Performs free_gpln on all nodes of lst including self
 */
void free_gpln_a(struct GrainPairListNode *lst) {
    if (!lst) { /* NULL */
        return;
    }

    free_gpln_l(lst->l);
    free_gpln_r(lst->r);
    free_gpln(lst);
}
