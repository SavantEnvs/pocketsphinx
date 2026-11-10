# Out-of-range float-to-int conversion in the quantised-trie LM code

An ARPA file with a huge log-probability reaches `weights_comparator` (`lm/lm_trie_quant.c:229`), which does
`(int)(*(float32 *)a - *(float32 *)b)`; the difference does not fit an `int` (UBSan
`2.76324e+13 is outside the range of representable values of type 'int'`, which also makes the sort order wrong).
Replay: `/mayhem/fuzz_ngram_read-standalone repro.arpa`.
Impact: undefined behaviour while loading an untrusted language model.
Fix: compare the floats (`return (x > y) - (x < y);`) instead of casting their difference.
