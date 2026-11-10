/* Read a pronunciation dictionary against the stock en-us phone set. */
#include "common.h"
#include "bin_mdef.h"
#include "dict.h"

static bin_mdef_t *mdef;

int
LLVMFuzzerInitialize(int *argc, char ***argv)
{
    ps_config_t *config;

    (void)argc;
    (void)argv;
    mayhem_quiet();
    config = ps_config_init(NULL);
    if (config == NULL)
        abort();
    mdef = bin_mdef_read(config, MODELDIR "/en-us/en-us/mdef");
    if (mdef == NULL)
        abort();
    ps_config_free(config);
    return 0;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char *path;
    ps_config_t *config;
    dict_t *dict;

    if (size == 0)
        return -1;
    path = mayhem_tmpfile(data, size);
    if (path == NULL)
        return 0;
    config = ps_config_init(NULL);
    if (config == NULL) {
        mayhem_rmtmp(path);
        return 0;
    }
    ps_config_set_str(config, "dict", path);
    ps_config_set_str(config, "fdict", MODELDIR "/en-us/en-us/noisedict");
    dict = dict_init(config, mdef);
    mayhem_rmtmp(path);
    ps_config_free(config);
    if (dict == NULL)
        return -1;
    (void)dict_wordid(dict, "hello");
    (void)dict_wordid(dict, "<s>");
    dict_free(dict);
    return 0;
}
