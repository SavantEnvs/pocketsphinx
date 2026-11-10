/* Parse a PocketSphinx JSON-ish configuration string. */
#include "common.h"

int
LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    mayhem_quiet();
    return 0;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char *json;
    ps_config_t *config;

    if (size == 0)
        return -1;
    json = mayhem_cstr(data, size);
    if (json == NULL)
        return 0;
    config = ps_config_parse_json(NULL, json);
    free(json);
    if (config == NULL)
        return -1;
    (void)ps_config_serialize_json(config);
    (void)ps_config_int(config, "nfft");
    (void)ps_config_float(config, "lw");
    (void)ps_config_str(config, "hmm");
    ps_config_free(config);
    return 0;
}
