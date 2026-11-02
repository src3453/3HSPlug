# 3HS816 / 3HS88PWN4 integer models (vendored)

Copies of the CPT32 repository's `chips/3hs816` and `chips/3hs88pwn4`
(`include/` and `src/` only), the same core Furnace's S3HS uses. CPT32 is the
source of truth (spec: `doc/spec_3HS816.md`); change the model there, run its
tests and copy both directories here again. Do not edit these files in place.

`../sound.cpp` adapts this core to the plugin's `S3HS_sound` interface.
