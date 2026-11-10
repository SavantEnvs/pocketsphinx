/* Read an N-gram language model (ARPA, DMP or binary trie) and query it. */
#include "common.h"
#include <pocketsphinx/model.h>

static ps_config_t *config;
static logmath_t *lmath;

int
LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    mayhem_quiet();
    config = ps_config_init(NULL);
    lmath = logmath_init(1.0001, 0, 0);
    if (config == NULL || lmath == NULL)
        abort();
    return 0;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    ngram_model_t *lm;
    char *path;
    int32_t n, i, hist[2], n_used;
    const uint32_t *counts;

    if (size == 0)
        return -1;
    path = mayhem_tmpfile(data, size);
    if (path == NULL)
        return 0;
    lm = ngram_model_read(config, path, NGRAM_AUTO, lmath);
    mayhem_rmtmp(path);
    if (lm == NULL)
        return -1;

    counts = ngram_model_get_counts(lm);
    n = counts ? ngram_model_get_size(lm) : 0;
    for (i = 0; i < n; i++)
        (void)counts[i];
    for (i = 0; i < 16; i++) {
        const char *w = ngram_word(lm, i);
        if (w == NULL)
            break;
        hist[0] = i;
        hist[1] = ngram_wid(lm, w);
        (void)ngram_ng_score(lm, i, hist, 2, &n_used);
        (void)ngram_ng_prob(lm, i, hist, 1, &n_used);
    }
    ngram_model_casefold(lm, NGRAM_UPPER);
    ngram_model_add_word(lm, "mayhemword", 1.0);
    (void)ngram_score(lm, "mayhemword", "<s>", NULL);
    ngram_model_free(lm);
    return 0;
}
