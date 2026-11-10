# Heap-buffer-overflow in `fsg_model_null_trans_closure` for a JSGF grammar

`repro.gram` parses; `jsgf_build_fsg_internal` (`lm/jsgf.c:530`) then calls `fsg_model_null_trans_closure`, which
indexes `fsg->trans[tl1->to_state]` (`lm/fsg_model.c:225`) past the end of the `calloc`'d table (ASan
heap-buffer-overflow READ; a heap-use-after-free variant at the same line was also seen).
Replay: `/mayhem/fuzz_jsgf-standalone repro.gram`.
Impact: memory-safety bug from an untrusted grammar.
Fix: make the FSG builder produce only in-range state ids (or check `to_state < n_state` in the closure).
