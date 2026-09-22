# Image lifetime tests

Run the host regression suite separately from the Android build:

```powershell
cmake -S tests/host -B build/host-tests -G Ninja -DCMAKE_CXX_COMPILER=clang++
cmake --build build/host-tests
ctest --test-dir build/host-tests --output-on-failure
```

The suite compiles the production `ImageLoadRequest.hpp` and
`AnimationStateUpdater.cpp` against small Unity/IL2CPP stubs. It exercises
same-path replacement, dirty-callback re-entry, destroyed targets, inactive
views, immediate frame assignment, cancellation, exception cleanup, and disposal
of an old download after a new one has started. It does not copy or extract the
implementation into the tests. Assertions remain enabled in every build type.

These tests do not simulate managed garbage collection, Unity rendering, the
network coroutine scheduler, or GIF decoding. The GIF validator test additionally
checks container truncation at every byte, invalid bounds, and allocation limits.
Run the [Quest decoder probe and GIF test page](quest/README.md) for actual decoding
and Unity integration. The [manual matrix](quest/image-load-lifetime.md) lists
additional re-entry and visual checks.

The active-image iteration test compiles the production traversal helper against
a list/image stub. It checks zero C++ heap allocations and one list read per
image during steady playback, destroyed-image removal, and setter callbacks
that remove, clear, shrink, grow, or re-enter the list. The pass walks backwards;
newly enabled images receive their frame through `OnEnable`. These allocation
checks cover traversal, not allocations inside Unity setters or user callbacks.

The streaming test compiles the production GIF queue and `AnimationInfo.cpp`.
It checks complete-frame publication and ordering across threads, producer
blocking at two queued frames, cancellation of a blocked producer, early/late
failure, and shared ownership after consumer teardown. A 1,000-frame run checks
that at most four decoded pixel buffers are reachable (two queued, one producer,
one consumer). Its array stub models reachability, not actual managed GC timing.
It excludes giflib's own raster/compositing buffers and Unity frame textures.

Request lifetime rules:

- Use image-loading helpers on Unity's main thread. Marshal decoder error
  callbacks to that thread before checking targets.
- Every `SetImage` call invalidates the previous generation, including cache hits
  and base-game sprites. Cancelled requests do not invoke completion/error handlers.
- UI setters can invoke user callbacks synchronously. A failed transition check
  means the caller must stop touching that request's target.
- The download coroutine owns disposal; the component only borrows the download
  for `Abort`. Cleanup must not detach a newer request's download.
- Keep the current animation attached while a replacement is loading. GIF
  processing already in flight may finish; an obsolete atlas is destroyed before
  it can be registered or displayed.

Known limits outside this change: the public `AnimationStateUpdater` layout has
changed, so prebuilt subclasses are not ABI compatible. The decoder changes add
no public fields or signature changes. Corrupt GIF failures now terminate through
a single error callback on the coroutine thread, releasing partial frame data.
