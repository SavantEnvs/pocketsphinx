# Unbounded 3-D allocation from the MLLR header

`repro.mllr` declares a huge `n_class`/feature length; `ps_mllr.c:86-88` calls `ckd_calloc_3d(n_class, veclen, veclen, ...)` with the
product unchecked (ASan `allocation-size-too-big`, 0x39ccb1af3210 bytes). Replay:
`/mayhem/fuzz_mllr-standalone repro.mllr`.
Impact: crash / memory exhaustion when loading an untrusted adaptation file.
Fix: validate the dimensions against the file size (or sane maxima) before allocating.
