/* Decode a precomputed senone-score dump. */
#include "common.h"

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
    FILE *fh;

    /* Without the "s3" header the legacy path hits E_FATAL -> exit() on every input. */
    if (size < 3 || memcmp(data, "s3\n", 3) != 0)
        return -1;
    fh = fmemopen((void *)data, size, "r");
    if (fh == NULL)
        return 0;
    ps_start_stream(ps);
    ps_decode_senscr(ps, fh);
    fclose(fh);
    (void)ps_get_hyp(ps, NULL);
    return 0;
}
