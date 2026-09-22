# Image request lifetime regression checks

Run these checks in Beat Saber with the rebuilt BSML library. Use a local HTTP
server that can hold and release individual PNG and GIF responses, plus a GIF
large enough that decoding spans several frames. Count `onFinished` and `onError`
calls for each request. Use unique URLs to avoid cache hits unless specified.

| Scenario | Expected result |
| --- | --- |
| Start a delayed PNG or GIF download, then destroy the target view with `ClearContents()` or hot reload. Release the response. | Download is aborted; neither callback runs for the old view; no missing-object exception or native crash. |
| Destroy only the `Image`, leaving its GameObject/updater alive, while downloading. | The next coroutine tick disposes the download without invoking either callback. |
| Destroy only `AnimationStateUpdater` while downloading. | Its `OnDestroy` aborts the download and invalidates the completion. |
| Start slow request A, then request B on the same image; finish B before A. Repeat for PNG/GIF combinations and for the same URL twice. | Only B can assign the image or invoke callbacks, including when the paths match. |
| Start A, then replace it with a cached static sprite, registered animation, or `#WhitePixel`. | A is cancelled. The selected replacement remains visible; a base-game sprite is not overwritten by the loading animation. |
| Let a GIF download finish, then destroy the view or replace its image while GIF processing is running. | The old result is not registered or assigned; its generated atlas is destroyed when processing finishes. |
| Trigger a GIF parsing error, then replace/destroy the target before the main-thread error callback runs. | The obsolete error callback is suppressed. For a current request, the callback runs on the main thread. |
| Have A's completion callback immediately start delayed request B. | Cleanup of A does not clear or dispose B's download. Destroying the view still cancels B. |
| Register a one-shot dirty-vertices callback that starts request B when A assigns the loading frame; use a cached sprite, registered GIF, and base-game sprite for A. | A stops immediately after re-entry; it neither overwrites B nor invokes its completion callback. |
| Repeat dirty-callback re-entry while attaching a completed GIF or enabling its updater, and repeat with the callback destroying the target instead. | No subsequent updater/image access occurs for A; B remains selected, or teardown completes without a missing-object exception. |
| Supply invalid static image bytes, or force sprite creation to fail. | The current request reports a parsing error and destroys its unused texture. |
| Complete an ordinary PNG/GIF load and repeat from cache; test with loading animation enabled and disabled. | Normal assignment and completion still work; cached resources remain usable. |
| Switch between two GIFs with loading animation both enabled and disabled; delay the second download, use a large GIF that takes several frames to parse, and give its first frame a long duration. | The previous GIF keeps playing through download and parsing, then the replacement's first frame appears immediately without a blank interval. |
| Switch to a GIF already playing on another image, including while the target view is inactive, then enable the view. | The target immediately displays the shared animation's current frame, including when playback is paused. |

For the cancelled GIF case, compare live `Texture2D` counts after decoding and
Unity's destruction pass have finished. In-flight GIF decoding is allowed to
finish because the worker owns its frame data; the cancelled result must not
leave an atlas or animation cache entry behind.

These are device integration checks, not a host-side simulation of Unity object
destruction. A successful C++ build alone does not verify them.
