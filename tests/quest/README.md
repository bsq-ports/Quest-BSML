# Quest GIF regression tests

## Standalone decoder test (no game modification)

From the repository root, with QPM dependencies and `ndkpath.txt` already set up:

```powershell
./tests/quest/run-decoder-probe.ps1 -Run -Device YOUR_QUEST_SERIAL
```

Use `-Adb C:/path/to/adb.exe` if needed. Without `-Run`, this only builds.
The script pushes a standalone executable to `/data/local/tmp/bsml-gif-decoder-probe`
and runs it with a 30-second timeout. It does not replace BSML or restart the game.
It links the same giflib archive and EasyGifReader source used by BSML.

Eight cases compare every decoded pixel and frame delay against independent
Pillow-generated references: two animations, a single non-square frame,
transparent/opaque-black pixels with disposal 1/2/3 and mixed zero/short delays,
invalid signature, truncation, no frames, and corrupt LZW data. A nonzero exit is
failure, including timeout. This tests native decoding, not Unity callbacks.

## In-game GIF Tests page

The page is opt-in and absent from normal builds. Build from the repository root:

```powershell
cmake -S . -B build -DBSML_QUEST_TESTS=ON
cmake --build build --parallel 12
```

Install `build/libbsml.so` using your normal mod deployment workflow, then restart
Beat Saber. Open **GIF Tests** from the main menu. **Offline tests** checks:

- Cold decoding of all eight fixtures, exact pixels/delays, callback thread,
  exactly one terminal callback, and recovery after malformed input.
- An invalid replacement reports `GifParsingError` and preserves the current GIF.
- A superseded decode cannot replace the newest image.
- The old animation remains attached while B loads, and B's current frame is
  assigned immediately; cached loads also work while inactive and after enabling.
- Animation playback safely removes an independently destroyed `Image`.
- `ClearContents()` during decoding destroys the old image and suppresses its
  callback and animation-cache registration.

Every asynchronous completion has a 10-second test deadline. Cancellation cases
observe the result for three seconds; they are regression probes, not proof
against arbitrarily late completion. Stopping or leaving the page cancels its
active request and invalidates the runner. Do not press manual controls during
an automated suite; doing so stops that suite. Results appear in the page,
logcat (`[GIF TEST]`), and `/sdcard/ModData/com.beatgames.beatsaber/bsml-gif-tests.log`.

For **Network tests**, start the fixture server in another terminal:

```powershell
python tests/quest/serve_fixtures.py
```

Then connect the server to the Quest:

```powershell
adb -s YOUR_QUEST_SERIAL reverse tcp:8765 tcp:8765
```

The server is local-only and requires no third-party URLs. GIF and PNG responses
under `/slow/` wait two seconds. Each test uses a unique URL to avoid caches.
The suite first verifies connectivity, then checks replacement, real
`ClearContents()`, destruction of only the image, and destruction of only the
updater during delayed downloads. Failed setup stops the suite with a failure;
it never counts an unreachable server as a successful cancellation test.

For the black-flash check, click **GIF A**, then **Slow B**. A should continue
playing until B appears. Repeat with **Loading on/off** in both settings and
click **No flash observed** or **Saw a flash** to record the visual result.
The first B frame lasts 1.5 seconds, exposing delayed first-frame assignment.
**Corrupt GIF** should preserve the current image and report an error. **Rebuild**
exercises `ClearContents()` while loading. The [manual matrix](image-load-lifetime.md)
covers additional dirty-callback and shared-animation cases.

Pull the report:

```powershell
adb -s YOUR_QUEST_SERIAL pull /sdcard/ModData/com.beatgames.beatsaber/bsml-gif-tests.log build/quest-gif-tests.log
```

Before producing a normal library, explicitly disable the cached CMake option:

```powershell
cmake -S . -B build -DBSML_QUEST_TESTS=OFF
cmake --build build --parallel 12
```

## Fixtures and decoder policy

Fixtures are bundled; Python/Pillow is only needed to regenerate them with
`python tests/quest/generate_fixtures.py`. The fixture server uses only Python's
standard library. Test it with:

```powershell
python -m unittest discover -s tests/quest -p 'test_*.py'
```

The production validator rejects incomplete GIF containers, missing palettes,
frames outside the logical screen, screens above 4096 pixels on either axis,
more than 1024 frames, encoded input above 64 MiB, or composited frames above
128 MiB. These are per-request limits, not a global memory budget. Normal image
loading overlaps decoding with texture creation through a queue capped at two
completed RGBA frames. The producer can hold one additional pending frame, and
the consumer can hold one frame during upload. Each decoded buffer is released
after upload, before the consumer yields. The atlas is published only after
the whole decode succeeds; partial failures are freed and reported once.
Consumer teardown wakes a producer blocked on the full queue. The public
`GifDecoder::Process` API still returns all completed frames for callers that
explicitly use it, including the direct pixel/timing checks above.

This bounds reachable decoded RGBA buffers, not total loading memory: giflib
still retains indexed raster data, managed arrays await GC after release, and
`PackTextures` still needs all temporary Unity frame textures until packing.
Frame delays of 0 or 10 ms use EasyGifReader's 100 ms fallback.
Existing GIFs continue playing during replacement decoding and atlas creation.
These changes do not claim support for every GIF encoder or eliminate the need
for on-device visual testing.

## ADB command interface (test build only)

The optional build consumes one command at a time from
`/sdcard/ModData/com.beatgames.beatsaber/bsml-gif-tests.command` on Unity's main
thread. Write a local ASCII text file containing a command, then `adb push` it
to that path. Wait for `COMMAND ...` and the result in the report before sending
the next command. `open` waits for the main menu and opens the real page.

Supported commands: `open`, `streaming`, `offline`, `network`, `reentry`, `a`, `b`, `slow-b`, `bad`,
`loading`, `active`, `rebuild`, `stop`, `stats`, and `gc`. `stats` records live
Unity texture/sprite/image/updater counts; `gc` requests managed collection.
Allow several frames for native finalizer cleanup before comparing counts.
The command interface is compiled out along with the page in normal builds.

`streaming` checks exact pixels/delays through the production stream, a producer
held at the two-frame queue limit during managed GC, consumer abandonment while
the producer is blocked, release of shared state, and actual four-frame atlas
creation/display. Malformed fixtures must report a single main-thread error.

`reentry` checks replacement using the same URL, a download completion that
starts another download, obsolete corruption errors, and dirty-vertices callback
re-entry during cached GIF assignment. Run repeatable suites and collect memory
snapshots with `python tests/quest/run_device_tests.py --device YOUR_QUEST_SERIAL
--rounds 3` (with the page already open and fixture server running). Each round
runs streaming, offline, network, and re-entry suites before collecting resource
counts and a process memory snapshot.
