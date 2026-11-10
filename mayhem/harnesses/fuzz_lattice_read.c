/* Read a word lattice (DAG) file and run rescoring over it. */
#include "common.h"
#include <pocketsphinx/lattice.h>
#include <pocketsphinx/model.h>

static ps_decoder_t *ps;

int
LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    mayhem_quiet();
    ps = mayhem_decoder();
    return 0;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char *path;
    ps_lattice_t *dag;
    ps_latnode_iter_t *ni;
    ps_latlink_t *link;
    ngram_model_t *lm;

    if (size == 0)
        return -1;
    path = mayhem_tmpfile(data, size);
    if (path == NULL)
        return 0;
    dag = ps_lattice_read(ps, path);
    mayhem_rmtmp(path);
    if (dag == NULL)
        return -1;

    for (ni = ps_latnode_iter(dag); ni; ni = ps_latnode_iter_next(ni)) {
        ps_latnode_t *node = ps_latnode_iter_node(ni);
        int16_t fef, lef;
        ps_latnode_times(node, &fef, &lef);
        (void)ps_latnode_word(dag, node);
        (void)ps_latnode_prob(dag, node, NULL);
    }
    lm = ps_get_lm(ps, NULL);
    link = ps_lattice_bestpath(dag, lm, 9.5, 0.5);
    if (link) {
        ps_lattice_posterior(dag, lm, 0.5);
        ps_lattice_posterior_prune(dag, -1000);
    }
    ps_lattice_free(dag);
    return 0;
}
