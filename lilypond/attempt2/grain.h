#ifndef GRAIN_H
#define GRAIN_H

#include <stdbool.h>
#include <stddef.h>

/* Definitions in grain.c */
struct Grain;
struct GrainPair;
struct GrainPairListNode;

/**
 * Creates (calloc) space for a new Grain
 * Returns a new Grain on success
 * NULL on failure
 */
struct Grain *new_g(size_t id, double x, double y, double t,
        double v);
/**
 * Frees and cleans up the Grain.
 * Will also free all GrainPairs ensuring they properly
 * remove their GrainPairListNodes from their parent lists.
 */
void free_g(struct Grain *g);

/**
 * converts a grain to an allocated printable string
 */
char *g_to_str(struct Grain *g);

/**
 * Once we've solved for r and i we do a lot of behind-the-scenes
 * updating.
 *
 * [IMPL] This function actually does a LOT of updating.
 * Removes the spair
 * Searches through its lst in search of other grains
 * Removes it's own entry from other grains and replaces spair
 * if it's better otherwise don't update.
 */
void set_g(struct Grain *g, double r, size_t i);

size_t get_g_id(struct Grain *g);
double get_g_x(struct Grain *g);
double get_g_y(struct Grain *g);
double get_g_t(struct Grain *g);
double get_g_v(struct Grain *g);

/**
 * r is maximum radius. Is -1 if unknown and 0 if suffocated.
 */
double get_g_r(struct Grain *g);

/**
 * i is the Grain it collides with if r is positive
 * otherwise return -1
 */
ssize_t get_g_i(struct Grain *g);

/**
 * Creates (calloc) space for a new GrainPair
 * Returns a new GrainPair on success
 * NULL on failure
 *
 * [IMPL] This function should append itself to each grains linked
 * list. We can do this because these pairs are expected to be unique
 * unlike the GrainPairListNodes
 */
struct GrainPair *new_gp(struct Grain *g1, struct Grain *g2);

/**
 * Frees and cleans up the GrainPair.
 * Will also free all GrainPairListNodes ensuring they're properly
 * removed from their lists.
 * Doesn't get rid of the Grains.
 */
void free_gp(struct GrainPair *gp);

/**
 * Forces this grain pair to have the radius they claim to have
 * Essentially calls set_g
 */
void set_gp(struct GrainPair *gp);

/**
 * Gets the first grain of the grain pair
 */
struct Grain *get_gp_1(struct GrainPair *gp);

/**
 * Gets the second grain of the grain pair
 */
struct Grain *get_gp_2(struct GrainPair *gp);

/**
 * Assuming g is actually in the grain pair return the other
 * Returns NULL on failure
 */
struct Grain *get_gp_other(struct GrainPair *gp, struct Grain *g);

/**
 * Checks if both grains in the pair are already realised
 */
bool is_gp_done(struct GrainPair *gp);

/**
 * Sets the glst to what the GrainPair considers is the GlobalListNode it resides in
 */
void set_gp_glst(struct GrainPair *gp, struct GrainPairListNode *glst);

/**
 * Computes the time of the GrainPair but doesn't store it
 */
double calc_gp_time(struct GrainPair *gp);

/**
 * Gets the time of the grain pair
 */
double get_gp_time(struct GrainPair *gp);

/**
 * Sets the time of the grain pair
 */
void set_gp_time(struct GrainPair *gp, double newtime);

/**
 * Create (calloc) space for a new GrainPairListNode
 * gp can also be NULL to create an empty node
 * Returns a new GrainPairListNode on success
 * NULL on failure
 */
struct GrainPairListNode *new_gpln(struct GrainPair *gp);

/**
 * Frees and cleans up a node.
 * Doesn't get rid of the GrainPair.
 */
void free_gpln(struct GrainPairListNode *node);

/**
 * Gets the left node of the GrainPairListNode
 * Returns NULL if there isn't one
 */
struct GrainPairListNode *get_gpln_l(struct GrainPairListNode *node);

/**
 * Gets the right node of the GrainPairListNode
 * Returns NULL if there isn't one
 */
struct GrainPairListNode *get_gpln_r(struct GrainPairListNode *node);

/**
 * Gets the GrainPair from the GrainPairListNode
 * Returns NULL if there isn't one
 */
struct GrainPair *get_gpln_gp(struct GrainPairListNode *node);

/**
 * Inserts the node to the left of the list
 */
void add_gpln_l(struct GrainPairListNode *lst, struct GrainPairListNode *node);

/**
 * Inserts the node to the right of the list
 */
void add_gpln_r(struct GrainPairListNode *lst, struct GrainPairListNode *node);

/**
 * Removes the current GrainPairListNode from its list
 */
void rm_gpln(struct GrainPairListNode *node);

/**
 * Performs free_gpln on all nodes of lst including self
 */
void free_gpln_a(struct GrainPairListNode *lst);

#endif
