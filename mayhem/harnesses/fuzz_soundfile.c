/* Sniff and parse WAV / NIST SPHERE audio headers. */
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
    int i;

    if (size == 0)
        return -1;
    for (i = 0; i < 3; i++) {
        ps_config_t *config = ps_config_init(NULL);
        FILE *fh = fmemopen((void *)data, size, "r");
        if (config == NULL || fh == NULL) {
            if (fh)
                fclose(fh);
            if (config)
                ps_config_free(config);
            return 0;
        }
        if (i == 0)
            ps_config_soundfile(config, fh, "fuzz.wav");
        else if (i == 1)
            ps_config_wavfile(config, fh, "fuzz.wav");
        else
            ps_config_nistfile(config, fh, "fuzz.sph");
        (void)ps_config_int(config, "samprate");
        fclose(fh);
        ps_config_free(config);
    }
    return 0;
}
