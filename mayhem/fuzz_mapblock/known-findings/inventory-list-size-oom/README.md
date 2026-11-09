# Inventory list size taken from untrusted data allocates gigabytes

`Inventory::deSerialize` (src/inventory.cpp:1019) reads `List <name> <size>` with `iss >> listsize`
into a `u32` and constructs `InventoryList(name, listsize, ...)`, whose `clearItems()` loop
(src/inventory.cpp:532) then `emplace_back`s `listsize` `ItemStack`s. A node metadata blob inside a
map block (or an inventory string anywhere else) that claims a list of tens of millions of slots makes
the server allocate 2+ GB (`malloc(2483027968)` here) from a few bytes of input: a memory-exhaustion
DoS for anyone who can supply a map database or block (world import, mod storage, network block data).

Reproduce: `/mayhem/fuzz_mapblock mayhem/fuzz_mapblock/known-findings/inventory-list-size-oom/repro.bin`
(libFuzzer reports `out-of-memory` at the default 2 GB malloc limit; without the limit it just allocates).

Suggested upstream fix: cap `listsize` (the client UI never uses lists beyond a few thousand slots)
and reject the inventory with a `SerializationError` above it.
