# Out-of-bounds read in `bin_mdef_read` on a binary mdef with an out-of-range senone id

`repro.mdef` is a mutated binary mdef (BMDF). `bin_mdef.c:494-497` indexes `m->sen2cimap[s]` with a senone id
`s` taken from `m->sseq` without checking `s < m->n_sen`, giving a heap-buffer-overflow read (ASan) / SEGV.

Replay: `/mayhem/fuzz_mdef-standalone repro.mdef`.
Impact: crash / info leak when loading an untrusted acoustic model.
Fix: validate every `sseq` entry against `n_sen` right after reading the sseq table.
