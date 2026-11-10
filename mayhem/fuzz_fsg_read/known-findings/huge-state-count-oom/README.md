# Unbounded allocation from the `NUM_STATES` header of an FSG file

`repro.fsg` declares `NUM_STATES 4668507704`; `fsg_model_init` (`lm/fsg_model.c:511`) does
`ckd_calloc(n_state, sizeof(*fsg->trans))` with no sanity limit (libFuzzer: `out-of-memory (malloc(17238822448))`).
Replay: `/mayhem/fuzz_fsg_read -rss_limit_mb=2048 repro.fsg`.
Impact: memory exhaustion / DoS from a 129-byte file.
Fix: cap `NUM_STATES` (and check it against the number of transitions actually present).
