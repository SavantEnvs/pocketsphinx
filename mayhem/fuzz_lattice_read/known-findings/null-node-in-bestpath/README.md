# NULL link/node dereference in `ps_lattice_bestpath` after `ps_lattice_read`

`repro.lat` is accepted by `ps_lattice_read` (no validation that the start node has usable exits / that every
edge endpoint exists), then `ps_lattice_bestpath` -> `ps_lattice.c:1243` reads `x->link->to->basewid` through a
null `ps_latnode_t` (UBSan `member access within null pointer`; ASan SEGV without UBSan).

Replay: `/mayhem/fuzz_lattice_read-standalone repro.lat`.
Impact: crash (denial of service) when a decoder rescoring an untrusted lattice file.
Fix: reject DAGs in `ps_lattice_read` whose start/end node has no path, or guard the null `link->to`.
