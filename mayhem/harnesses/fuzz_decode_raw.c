/* Decode raw 16 kHz 16-bit PCM with the stock en-us model. */
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
    int16_t *pcm;
    size_t nsamp, chunk, off;
    ps_seg_t *seg;
    ps_nbest_t *nbest;
    int i;

    /* First byte picks whole-utterance vs. chunked feeding. */
    if (size < 3)
        return -1;
    chunk = data[0] & 1 ? 0 : ((size_t)(data[0] >> 1) + 1) * 64;
    data++;
    size--;
    nsamp = size / 2;
    pcm = malloc(nsamp * sizeof(*pcm));
    if (pcm == NULL)
        return 0;
    memcpy(pcm, data, nsamp * sizeof(*pcm));

    /* Noise statistics persist across utterances; reset them per input. */
    ps_start_stream(ps);
    if (ps_start_utt(ps) < 0)
        goto done;
    if (chunk == 0) {
        ps_process_raw(ps, pcm, nsamp, FALSE, TRUE);
    }
    else {
        for (off = 0; off < nsamp; off += chunk)
            ps_process_raw(ps, pcm + off,
                           nsamp - off < chunk ? nsamp - off : chunk,
                           FALSE, FALSE);
    }
    ps_end_utt(ps);

    (void)ps_get_hyp(ps, NULL);
    for (seg = ps_seg_iter(ps); seg; seg = ps_seg_next(seg)) {
        int sf, ef;
        int32_t a, l, b;
        (void)ps_seg_word(seg);
        ps_seg_frames(seg, &sf, &ef);
        ps_seg_prob(seg, &a, &l, &b);
    }
    nbest = ps_nbest(ps);
    for (i = 0; nbest && i < 3; i++) {
        (void)ps_nbest_hyp(nbest, NULL);
        nbest = ps_nbest_next(nbest);
    }
    if (nbest)
        ps_nbest_free(nbest);
done:
    free(pcm);
    return 0;
}
