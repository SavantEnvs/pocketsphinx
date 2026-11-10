/* Read a finite-state grammar in Sphinx FSG format and write it back out. */
#include "common.h"

static logmath_t *lmath;

int
LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    mayhem_quiet();
    lmath = logmath_init(1.0001, 0, 0);
    if (lmath == NULL)
        abort();
    return 0;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FILE *in, *out;
    fsg_model_t *fsg;
    char *buf = NULL;
    size_t buflen = 0;

    if (size == 0)
        return -1;
    in = fmemopen((void *)data, size, "r");
    if (in == NULL)
        return 0;
    fsg = fsg_model_read(in, lmath, 1.0f);
    fclose(in);
    if (fsg == NULL)
        return -1;
    fsg_model_accept(fsg, "go forward ten meters");
    out = open_memstream(&buf, &buflen);
    if (out) {
        fsg_model_write(fsg, out);
        fsg_model_write_fsm(fsg, out);
        fsg_model_write_symtab(fsg, out);
        fclose(out);
        free(buf);
    }
    fsg_model_free(fsg);
    return 0;
}
