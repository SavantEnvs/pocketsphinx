/* Voice-activity detection and endpointing over raw PCM. */
#include "common.h"
#include <pocketsphinx/endpointer.h>

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
    static const int rates[4] = { 8000, 16000, 32000, 48000 };
    ps_endpointer_t *ep;
    int16_t *pcm;
    size_t nsamp, fsz, off;

    /* First byte: VAD mode (low 2 bits) and sample rate (next 2 bits). */
    if (size < 3)
        return -1;
    ep = ps_endpointer_init(0.0, 0.0, (ps_vad_mode_t)(data[0] & 3),
                            rates[(data[0] >> 2) & 3], 0.0);
    if (ep == NULL)
        return -1;
    data++;
    size--;
    nsamp = size / 2;
    pcm = malloc(nsamp * sizeof(*pcm));
    if (pcm == NULL) {
        ps_endpointer_free(ep);
        return 0;
    }
    memcpy(pcm, data, nsamp * sizeof(*pcm));
    fsz = ps_endpointer_frame_size(ep);
    for (off = 0; off + fsz <= nsamp; off += fsz) {
        const int16_t *speech = ps_endpointer_process(ep, pcm + off);
        (void)speech;
        (void)ps_endpointer_in_speech(ep);
    }
    {
        size_t n;
        (void)ps_endpointer_end_stream(ep, pcm + off, nsamp - off, &n);
    }
    ps_endpointer_free(ep);
    free(pcm);
    return 0;
}
