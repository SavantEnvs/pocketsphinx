# Heap overflow in `acmod_read_scores_internal` from the frame's `n_active` field

In a senone-score dump each frame starts with an `int16 n_active`. `acmod.c:942-952` stores it and then
`fread`s `n_active` bytes into `acmod->senone_active`, which is only `bin_mdef_n_sen()` (5126 for en-us) long,
unless `n_active == n_sen` (the dense branch). A negative or larger-than-`n_sen` value overflows the buffer
(WRITE of size 8711 in `repro.bin`).

Replay: `/mayhem/fuzz_senscr-standalone repro.bin`.
Impact: heap corruption from a crafted score file fed to `ps_decode_senscr`.
Fix: reject `n_active < 0 || n_active > n_sen` before the read.
