# S3HS regression tests

```powershell
cmake -S test -B build/s3hs-tests
cmake --build build/s3hs-tests --config Release
ctest --test-dir build/s3hs-tests -C Release --output-on-failure
```

The core test checks the `S3HS_sound` adapter over the vendored integer core:
at 48 kHz its output equals the bare core's, three samples late, across register
changes between blocks (all algorithms, feedback, WT/noise, EQ, compressor); the
PCM filter and FM user-wave omissions; PCM RAM loading with `syncPcmMemory()`;
voice stealing releasing instead of holding; pitch, level and block-size
independence at 22.05/44.1/96 kHz; and the FIFO and quantization-setting API.
The core itself is tested against the original float 3HS88PWN4 in the CPT32
repository (`chips/3hs88pwn4`). With the JUCE checkout available, the patch test
also checks JSON round trips and legacy patch fallback.

The optional processor/editor test covers channel-isolated CC values, bank/program,
pitch bend, melodic/drum notes, sustain, velocity-zero note off, CC120/123,
Panic, GM reset and keyboard navigation between the channel monitor and editor.

```powershell
cmake -S . -B build -DTHREEHS_BUILD_UI_TESTS=ON
cmake --build build --config Release --target channel_status_tests -- /m:1 /nodeReuse:false
ctest --test-dir build -C Release --output-on-failure
# Optionally export editor previews at the default and minimum sizes:
build/Release/channel_status_tests.exe build/channel-status-preview
```
