# Signed integer overflow in the WebRTC resampler used by the VAD

`repro.bin` (first byte: VAD mode / sample rate, rest: PCM) makes `ps_endpointer_process` resample 32/48 kHz
input; `common_audio/signal_processing/resample_fractional.c:73` adds two 32-bit accumulators and overflows
(UBSan `signed integer overflow: 2122845016 + 64991730`). Replay: `/mayhem/fuzz_endpointer-standalone repro.bin`.
Impact: undefined behaviour on loud input at 32/48 kHz (benign on wrapping hardware, but it halts under UBSan).
Fix: accumulate in `uint32_t`/`int64_t` or saturate.
