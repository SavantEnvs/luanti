# Schematic dimensions are trusted: signed size product overflows, negative sizes size allocations

`Schematic::deserializeFromMts` (src/mapgen/mg_schematic.cpp:291) reads the X/Y/Z size with
`readV3S16`, so each dimension is a signed 16-bit value taken straight from the file, and uses it
unchecked: `new u8[size.Y]` (line 296) and `new MapNode[size.X * size.Y * size.Z]` (line 325). The
product is computed in `int` and overflows (`647626142 * 27764 cannot be represented in type 'int'`,
reported at line 69 in `~Schematic`, same expression at 83/98/322), which is undefined behaviour; a
negative dimension turns into a ~2^64-byte request (ASan `allocation-size-too-big` in `operator new[]`
was also hit by the fuzzer).

Reproduce: `/mayhem/fuzz_schematic mayhem/fuzz_schematic/known-findings/size-product-overflow/repro.bin`
(UBSan halts on the overflow).

Suggested upstream fix: reject non-positive dimensions after reading them, compute the node count in
`u64`, and refuse files whose count exceeds a sane bound (e.g. 2^24 nodes) before allocating.
