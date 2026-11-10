/* Read an MLLR adaptation matrix file and apply it to a decoder. The decoder is upstream's own an4_ci_cont
   test model (the one test_mllr.c adapts with test/data/mllr_matrices, our seed): the en-us model has a
   different stream layout, so a matrix written for it would not even be shaped like the seed.
   ps_update_mllr() reloads the means and variances from the model files before transforming them, so
   nothing carries over between inputs. */
#include "common.h"
#include <pocketsphinx/mllr.h>

#ifndef TESTDATADIR
#define TESTDATADIR MODELDIR "/../test/data"
#endif

static ps_config_t *config;
static ps_decoder_t *ps;

int
LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    mayhem_quiet();
    config = ps_config_parse_json(
        NULL,
        "hmm: \"" TESTDATADIR "/an4_ci_cont\","
        "lm: \"" TESTDATADIR "/turtle.lm.bin\","
        "dict: \"" TESTDATADIR "/turtle.dic\","
        "samprate: 16000, loglevel: \"FATAL\"");
    if (config == NULL || (ps = ps_init(config)) == NULL)
        abort();
    return 0;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char *path;
    ps_mllr_t *mllr;

    if (size == 0)
        return -1;
    path = mayhem_tmpfile(data, size);
    if (path == NULL)
        return 0;
    mllr = ps_mllr_read(path);
    mayhem_rmtmp(path);
    if (mllr == NULL)
        return -1;
    (void)ps_update_mllr(ps, mllr);
    ps_mllr_free(mllr);
    return 0;
}
