# BSML stability audit — 2026-09-22

Reviewed the working tree at commit `5a7164f`, concentrating on parsing, view reconstruction, bindings, events, image/GIF loading, settings, lists, and UI lifetimes. Existing changes to `CMakeLists.txt` and `qpm.shared.json` were not modified.

These are 18 actionable findings, not a guarantee that every defect has been found. Finding 1 was reproduced with AddressSanitizer against the actual repository header on Windows, and finding 7 was confirmed on a connected Quest 3 with a temporary diagnostic probe. The other findings are source-level analysis; they have not been exercised inside Beat Saber on a Quest. P1 means prioritize for a fix; P2 means a narrower trigger or less immediate stability impact. Finding 7's native integer attribute reads have since been fixed as described below.

## 1. [P1] Adding listeners during event dispatch causes use-after-free

Location: [BSMLEvent.hpp:14](/D:/BeatSaber/Quest-BSML/shared/BSML/Parsing/BSMLEvent.hpp:14).

`Invoke()` iterates `funcs` by reference and calls each `std::function` in place. A listener that calls `Add()` on the same event can reallocate the vector, destroying both the running callable's storage and the iteration state. This also applies when a listener registers another handler through `BSMLParserParams::AddEvent()`.

**Verified:** a small program including the actual header, with a listener that adds 100 listeners and a second existing listener, exits with AddressSanitizer reporting `heap-use-after-free`. Reproduction: [event-reentrancy.cpp](/D:/BeatSaber/Quest-BSML/build/stability-audit/event-reentrancy.cpp). Output: [event-reentrancy.log](/D:/BeatSaber/Quest-BSML/build/stability-audit/event-reentrancy.log).

**Fixed:** callback storage is individually owned through `shared_ptr`, and each dispatch snapshots those owners before invoking callbacks. Adding listeners cannot move or destroy an executing callable. Added listeners are eligible for subsequent invocations (including nested invocations); the current dispatch retains its original order. Mutable callback state persists between invocations, while copying an event still copies each callback's state independently.

The existing `vector<std::function<void()>>` storage type/layout is retained; each newly added function wraps a shared owner of its callable. Consumers must recompile to pick up the fixed inline dispatch implementation.

**Verified:** the original reproduction and [expanded regression test](/D:/BeatSaber/Quest-BSML/tests/event-reentrancy.cpp) both pass with AddressSanitizer against the fixed header. The regression covers vector reallocation, new-listener timing, mutable state, event copies, nested invocation, exception propagation/recovery, and empty listeners. Logs: [expanded test](/D:/BeatSaber/Quest-BSML/build/stability-audit/event-reentrancy-fixed.log), [original reproduction after fix](/D:/BeatSaber/Quest-BSML/build/stability-audit/event-original-repro-fixed.log).

## 2. [P1] Image completion callbacks use destroyed UI objects

Locations: [utilities.cpp:299](/D:/BeatSaber/Quest-BSML/src/Helpers/utilities.cpp:299), [utilities.cpp:352](/D:/BeatSaber/Quest-BSML/src/Helpers/utilities.cpp:352).

Both animated and static image loaders capture raw `image`/`stateUpdater` pointers and use them after asynchronous download or GIF processing. The work runs through the shared coroutine starter, so destroying the target view does not cancel it. A hot reload or `ClearContents()` while a download is pending destroys those components; completion still calls their Unity methods. This can produce missing-object exceptions or a crash. An older request can also overwrite a newer image request on a surviving component.

Trigger: display a slow URL image, rebuild/destroy the view before completion, then let the download finish.

Fix: retain a suitable managed reference, check native Unity object validity before each completion, cancel on teardown, and use a request generation token to reject obsolete results. Dispose unused generated textures.

## 3. [P1] A missing raw-image resource dereferences a null array

Locations: [RawImageHandler.cpp:31](/D:/BeatSaber/Quest-BSML/src/BSML/TypeHandlers/RawImageHandler.cpp:31), [utilities.cpp:435](/D:/BeatSaber/Quest-BSML/src/Helpers/utilities.cpp:435).

`GetData()` reports a missing cache key by calling its callback with `nullptr`. The raw-image handler forwards that directly to `LoadTextureRaw()`, which calls `data.size()` without checking for null. `ArrayW::size()` dereferences its underlying pointer. A missing asset therefore becomes a native null dereference, rather than a recoverable parse error.

Trigger: `<raw-image src='missing-resource-key'/>`.

Fix: guard null in the loader and make the handler report a controlled error or install a placeholder.

## 4. [P1] Empty dropdown choices produce an unchecked out-of-bounds read

Location: [DropdownListSetting.cpp:97](/D:/BeatSaber/Quest-BSML/src/BSML/Components/Settings/DropdownListSetting.cpp:97).

`ValidateRange()` leaves `index` at zero for an empty list, but `get_Value()` still returns `values[0]`. `Setup()` reaches this through `ReceiveValue() -> set_Value() -> UpdateState()`. The handler only logs missing/empty choices and continues; the Lite `CreateDropdown()` path also accepts an empty span. A null options field introduces additional null dereferences.

The local `ListW` and `ArrayW` `operator[]` implementations are unchecked, so this is not reliably an ordinary managed bounds exception.

Fix: handle null/empty choices before selection, return null from the getter, and disable the control or throw a parse error before setup.

## 5. [P1] A null dropdown entry falls through into ToString()

Location: [DropdownListSetting.cpp:46](/D:/BeatSaber/Quest-BSML/src/BSML/Components/Settings/DropdownListSetting.cpp:46).

For a null option, `UpdateChoices()` adds `"NULL"` but does not continue. With the default formatter it immediately calls `v->ToString()` on null. With a custom formatter it invokes that formatter on null and can append a second label, misaligning displayed choices with data indices.

Trigger: bind a choices list containing one null entry.

Fix: continue after the placeholder or use mutually exclusive branches.

## 6. [P1] A tab selector dereferences tagged objects that are not tabs

Location: [TabSelector.cpp:50](/D:/BeatSaber/Quest-BSML/src/BSML/Components/TabSelector.cpp:50).

`GetObjectsWithTag()` can return any tagged GameObject. `Setup()` assumes each has a `Tab` component, adds the result to `tabs`, and immediately dereferences it. Accidentally sharing a tag with an ordinary layout or text crashes during construction instead of producing a BSML diagnostic.

Trigger: a `<tab-selector tab-tag='pages'/>` together with `<text tags='pages' text='hello'/>`.

Fix: verify the component before storing or accessing it and report the offending tag.

## 7. [P1] Native-sized integer bindings use a four-byte destination on ARM64

Location: [ComponentTypeWithData.cpp:120](/D:/BeatSaber/Quest-BSML/src/BSML/ComponentTypeWithData.cpp:120).

`IL2CPP_TYPE_I` and `IL2CPP_TYPE_U` are read using `GetValue<int>()` and `GetValue<uint>()`. On the Quest's ARM64 target these are four-byte C++ integers, while native-sized integers are eight bytes. The field path in `BSMLValue::GetValue<T>()` passes a pointer to that small local to raw `field_get_value()`, which copies the field payload. Binding such a field through a `~value` attribute can therefore overwrite stack memory.

**Verified on Quest 3:** a temporary probe invoked the same raw field read against Unity's `m_CachedPtr` field. The runtime reported native-integer type enum `24`; an adjacent four-byte canary changed from `0x13579BDF` to `0x00000076`, confirming an eight-byte write into the four-byte destination. The exact `BSMLValue::GetValue<int>()` path returned a corrupted/truncated value and the process survived that run. This is confirmed memory corruption; whether it becomes an immediate crash depends on the surrounding stack layout.

**Fixed:** the `IL2CPP_TYPE_I` and `IL2CPP_TYPE_U` attribute conversion paths now read `std::intptr_t` and `std::uintptr_t`. Both destinations are eight bytes on Quest ARM64, preserving the full value and preventing this overwrite. The Android build passed. Generic `GetValue<T>()` still requires callers to choose a type matching the field; broader validation of arbitrary callers is outside this fix.

**Fix verified on Quest 3 (2026-09-22 06:08:23 device time):** the diagnostic fixture in `tests/quest/NativeIntegerRegression.hpp` registered real `System::IntPtr` and `System::UIntPtr` fields; runtime metadata reported types 24 and 25. All eight cases passed 100 iterations each: zero, signed positive/negative values beyond 32 bits, signed minimum/maximum, unsigned high-bit value, and unsigned maximum. Raw native-width field reads preserved adjacent 64-bit guards; separate calls to the production `GetValue<T>()` and `ComponentTypeWithData::GetParameters()` (`~value` resolution) preserved the complete numeric value. The guards surround the diagnostic raw-read destination, not the production getter's local variable. Saved output: [native-integer-fixed-quest.log](/D:/BeatSaber/Quest-BSML/build/stability-audit/native-integer-fixed-quest.log). The startup test is excluded from the regular build.

## 8. [P1] A failed hot reload destroys the good view and escapes Update()

Location: [HotReloadFileWatcher.cpp:53](/D:/BeatSaber/Quest-BSML/src/BSML/Components/HotReloadFileWatcher.cpp:53).

`Reload()` stores the new content hash and destroys the old hierarchy before parsing. Invalid XML, a missing binding, or a handler exception then propagates out through `Update()` with no recovery here. The old view is gone, a construction failure can leave a partial replacement, and the failed content is already recorded as loaded. Calling `Reload()` again with unchanged content is suppressed by the hash check.

Trigger: save malformed markup while the view is open, or let a file copy expose incomplete contents during polling.

Fix: catch reload failures, validate before tearing down, construct into a temporary owned container where feasible, and commit the hash only after successful replacement. Preserve the previous view or show a controlled error view. Swapping host bindings also needs to be accounted for if construction is transactional.

## 9. [P2] Invalid tab page counts reach division and indexing unchecked

Locations: [TabSelector.cpp:23](/D:/BeatSaber/Quest-BSML/src/BSML/Components/TabSelector.cpp:23), [TabSelector.cpp:35](/D:/BeatSaber/Quest-BSML/src/BSML/Components/TabSelector.cpp:35).

The setter accepts zero and negative counts other than the `-1` sentinel. A zero count reaches integer division in `get_page()` and `Refresh()`; other negative counts can reach negative capacities and invalid pagination indices. These values are accepted directly from `page-count` markup.

Fix: accept only `-1` or positive counts and handle an empty tab set without selecting cell zero.

## 10. [P2] Failed GIF decoding leaves coroutines waiting forever

Locations: [GifDecoder.cpp:53](/D:/BeatSaber/Quest-BSML/src/BSML/Animations/GIF/GifDecoder.cpp:53), [GifDecoder.cpp:105](/D:/BeatSaber/Quest-BSML/src/BSML/Animations/GIF/GifDecoder.cpp:105), [AnimationLoader.cpp:78](/D:/BeatSaber/Quest-BSML/src/BSML/Animations/AnimationLoader.cpp:78).

The worker catches a decode error and returns without publishing a failure/completion state. An error opening the file leaves `Process()` waiting for `isInitialized` forever. An error after initialization can leave `ProcessAnimationInfo()` waiting for a frame that will never arrive. The raw `AnimationInfo` allocation and resources retained by these coroutines cannot reach their normal cleanup.

Trigger: repeatedly load corrupt or truncated GIFs.

Fix: publish success/failure/completion, check it in both wait loops, and give the worker and consumer coordinated ownership and cancellation.

## 11. [P2] GIF initialization is published with a data race

Locations: [AnimationInfo.hpp:16](/D:/BeatSaber/Quest-BSML/shared/BSML/Animations/AnimationInfo.hpp:16), [GifDecoder.cpp:74](/D:/BeatSaber/Quest-BSML/src/BSML/Animations/GIF/GifDecoder.cpp:74).

The detached worker writes a plain `bool isInitialized` while the main thread reads it without synchronization. This is a C++ data race. It also sets the flag before assigning width and height. The atomic decoded-frame counter does not make this earlier flag access race-free.

Fix: publish all initialization fields before a release store to an atomic status, then read that status with acquire semantics; a locked shared state or future is another option.

## 12. [P2] GIF error callbacks run on the decoder worker

Location: [GifDecoder.cpp:107](/D:/BeatSaber/Quest-BSML/src/BSML/Animations/GIF/GifDecoder.cpp:107).

`ProcessingThread()` directly invokes `onError`, which the image utility forwards to the public `SetImage` error callback. Successful image completion and network-error callbacks run through the main-thread coroutine path. A callback that updates a label, hides a loading object, or installs a fallback image therefore runs on the wrong thread specifically for a malformed GIF, risking Unity threading errors or crashes.

Fix: marshal decoder errors to the main thread and apply the same target-lifetime checks as successful completion.

## 13. [P2] View controllers discard the lifetime owner of parser events

Locations: [BSMLViewController.cpp:61](/D:/BeatSaber/Quest-BSML/src/BSML/ViewControllers/BSMLViewController.cpp:61), [HotReloadFileWatcher.cpp:62](/D:/BeatSaber/Quest-BSML/src/BSML/Components/HotReloadFileWatcher.cpp:62).

Both paths discard the shared parser returned by construction. Parser parameters own the `BSMLEvent` objects, while button `click-event` listeners hold only weak pointers. Once construction returns, markup such as a button with `click-event='open'` and a modal with `show-event='open'` stops working. The click logs an expired event instead of showing the modal. Direct `on-click` host methods survive because their delegates capture the host and method separately.

The independent scopes in `macro.as-host`, `macro.for-each`, and `macro.repeat` also discard their child parser parameters, so preserving only the outer parser does not fix events inside those scopes.

Fix: retain parser parameters for the lifetime of each live content scope and release/replace them with that scope's UI.

## 14. [P2] Repeated parsing leaks actions rejected as duplicate names

Location: [BSMLAction.cpp:25](/D:/BeatSaber/Quest-BSML/src/BSML/Parsing/BSMLAction.cpp:25).

`MakeActions()` allocates an action before inserting it into a map keyed only by method name. Overloads and hidden/inherited methods with duplicate names cause insertion to fail, leaking the newly allocated action. Host types such as view controllers have many such methods, so this leaks repeatedly during hot reload. The field collection in `BSMLValue::MakeValues()` has the same allocation-before-emplace problem for hidden field names.

Fix: check whether the name exists before allocation or retain ownership in a `unique_ptr` until insertion succeeds. Overload resolution should be considered separately from leak prevention.

## 15. [P2] Macro values are deleted through a base without a virtual destructor

Locations: [BSMLValue.hpp:9](/D:/BeatSaber/Quest-BSML/shared/BSML/Parsing/BSMLValue.hpp:9), [BSMLParserParams.cpp:8](/D:/BeatSaber/Quest-BSML/src/BSML/Parsing/BSMLParserParams.cpp:8).

`macro.define` allocates `BSMLStringValue`, which owns a `std::string`, and stores it as `BSMLValue*`. Parser cleanup deletes through that base pointer, but `BSMLValue` has no virtual destructor. This is undefined behavior; the derived string destructor is not reliably run and larger macro values leak on repeated parsing.

Fix: add a virtual destructor and rebuild dependents that rely on this exported class's ABI.

## 16. [P2] Slider increments can generate non-finite or overflowing step counts

Location: [SliderSetting.cpp:42](/D:/BeatSaber/Quest-BSML/src/BSML/Components/Settings/SliderSetting.cpp:42).

Setup divides the slider range by `increments`, converts the result to `int`, and adds one without validating either the input or result. `increment='0'`, a tiny positive increment, non-finite bounds, or a reversed range can produce undefined float-to-int conversion, signed overflow, or an invalid step count passed into HMUI. The Lite API reaches the same calculation.

Fix: require finite ordered bounds and a positive finite increment, then range-check the computed count before conversion and addition.

## 17. [P2] Custom-cell list bounds check accepts the end index

Location: [CustomCellListTableData.cpp:27](/D:/BeatSaber/Quest-BSML/src/BSML/Components/CustomCellListTableData.cpp:27).

`CellForIdx()` rejects `idx > Count` but accepts `idx == Count` and negative indices, then uses unchecked indexing. It also dereferences `data` before a null check, although `NumberOfCells()` explicitly supports null data. A caller using an invalid index or a list that shrinks after the table cached its count can read outside the live data and treat that value as a host object.

Fix: check `data && idx >= 0 && idx < data->get_Count()` before access. Callers still need to reload the table after changing its data.

## 18. [P2] Hot reload silently misses same-second and older-timestamp edits

Location: [HotReloadFileWatcher.cpp:38](/D:/BeatSaber/Quest-BSML/src/BSML/Components/HotReloadFileWatcher.cpp:38).

Polling only hashes the file when its whole-second `st_mtime` is strictly greater than the recorded value. Two observed saves in the same second can leave the second edit unapplied indefinitely, even if their contents differ. Replacing the file with one retaining an older timestamp has the same problem. Polling every 0.5 seconds, as in the usage example, does not overcome this gate.

Fix: compare a sufficiently precise file identity/change tuple or periodically hash contents independently of strictly increasing timestamps. Combine that with failure-safe reload and write stabilization.

## Validation and limits

- Compiled the event reproduction with local Clang 22, C++20, `-O0 -g -fsanitize=address`, including `shared/BSML/Parsing/BSMLEvent.hpp` directly. The executable exited with code 1 and reported heap-use-after-free.
- Traced missing raw-image data through `GetData()` to the null-unsafe loader.
- Inspected the bundled `ListW`/`ArrayW` implementations to verify that `size()` is null-unsafe and indexing is unchecked.
- Traced parser event ownership and compared the controllers with `SettingsMenu`, which explicitly retains `parserParams`.
- No Android build, headset execution, GPU resource profiling, or thread sanitizer run was performed. Runtime-specific crash frequency and exception handling at the Unity boundary remain unmeasured.
- Start with findings 1–8. The earlier usage examples demonstrate the API but do not protect against these implementation defects.
