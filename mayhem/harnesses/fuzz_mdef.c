/* Read a binary acoustic-model definition (mdef). The text parser exits via E_FATAL on every
   malformed line, so only inputs carrying the binary byte-order mark ("BMDF"/"FDMB") are fed. */
#include "common.h"
#include "bin_mdef.h"

static ps_config_t *config;

int
LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    mayhem_quiet();
    config = ps_config_init(NULL);
    if (config == NULL)
        abort();
    return 0;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char *path;
    bin_mdef_t *mdef;
    int i;

    if (size < 8 || (memcmp(data, "BMDF", 4) != 0 && memcmp(data, "FDMB", 4) != 0))
        return -1;
    path = mayhem_tmpfile(data, size);
    if (path == NULL)
        return 0;
    mdef = bin_mdef_read(config, path);
    mayhem_rmtmp(path);
    if (mdef == NULL)
        return -1;
    for (i = 0; i < bin_mdef_n_ciphone(mdef) && i < 64; i++) {
        (void)bin_mdef_ciphone_str(mdef, i);
        (void)bin_mdef_phone_id_nearest(mdef, i, 0, 0, WORD_POSN_SINGLE);
    }
    (void)bin_mdef_ciphone_id(mdef, "AA");
    (void)bin_mdef_ciphone_id(mdef, "SIL");
    bin_mdef_free(mdef);
    return 0;
}
