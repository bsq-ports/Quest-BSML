# BSML 2: idiomatic-C++ refactor roadmap

Tracks an incremental effort to make BSML's own internals idiomatic modern C++
(templates, concepts, `std::function`, virtual dispatch, RAII) instead of
hand-rolled reflection/registry systems built on IL2CPP RTTI — while keeping
IL2CPP/beatsaber-hook types (`safe_ptr<T>`, `StringW`, `ArrayW`/`ListW<T>`, raw
codegen'd Unity pointers, `i2c::` reflection helpers) exactly where they're the
real interop boundary (talking to Unity, or binding to a mod author's own
code identified only by a runtime string). Work happens in independent,
reviewable passes — each item below is scoped to be done on its own.

Do these passes one at a time. Update the Status table when a pass lands (date
+ short note), and add newly-discovered items to the backlog rather than
scope-creeping the pass in progress.

## Core idioms (apply to every pass)

1. **Component creation & configuration**: a typed `*Options` aggregate struct
   (C++20 designated initializers) + one canonical free function per
   component, owning real creation logic directly. No reaching into
   `BSMLTag::CreateObject` via the `#define protected public` hack.
2. **Property application dissolves, it isn't reimplemented**: once a
   component's creation+configuration is unified behind idiom #1, there is
   nothing left for a `TypeHandler`-style global-registry + IL2CPP-RTTI
   broadcast-dispatch system to do. Don't port that pattern to something
   "idiomatic" — delete it once nothing calls it.
3. **Never reflect on your own statically-known methods.** When BSML needs to
   hand Unity a delegate/`UnityAction`/`SystemAction` for one of its *own*
   member functions, use `MakeUnityAction(std::bind(&Self::Method, this))` (or
   a lambda) — never
   `i2c::functions::class_get_method_from_name(this->klass, "Method", n)` +
   `MakeUnityAction(this, methodInfo)`. Several files in this codebase already
   do both side by side for different methods — the closure form is proven
   safe wherever the reflective form appears.
4. **Prefer a checked cast over `reinterpret_cast`** when downcasting a
   `UnityEngine::Component*`/`System::Object*` whose exact type isn't locally,
   statically obvious at the point of the cast — `i2c::try_cast<T*>(ptr)`
   (returns `nullptr` on mismatch) instead of blindly trusting an invariant
   established elsewhere (e.g. an RTTI check that ran in a caller three frames
   up). See `ButtonHandler.cpp` for the pattern.
5. **Shim + pure-C++-interface for Unity lifecycle types.** Where BSML
   currently requires behavior to live inside an IL2CPP-declared class
   (forcing either real IL2CPP methods or reflection to reach them), split
   into a minimal IL2CPP shim (the real Unity component, forwards lifecycle
   calls) + a pure C++ virtual interface (owns actual logic, zero IL2CPP
   macros):
   ```cpp
   struct FooAbstract {
       virtual std::string getContent() = 0;
       virtual ~FooAbstract() = default;
   };
   DECLARE_CLASS_CODEGEN(BSML, Foo, HMUI::ViewController) {
       std::unique_ptr<FooAbstract> impl;
       std::string getContent() { return impl->getContent(); }
       // Unity lifecycle overrides forward to impl-> the same way
   };
   ```
   **This applies whenever the implementation is *any* C++ code — including a
   downstream mod's, not just BSML's own** (proven via `BSMLContentProvider`,
   backlog #5). Every Quest mod is a native `.so` built with the same NDK
   toolchain against BSML's own public headers and dynamically linked the same
   way any C++ shared library is — a virtual call through a vtable defined in
   a shared header works fine across that boundary, no reflection needed. The
   one case this *doesn't* cover is dispatching into the **game's own
   IL2CPP-generated C# classes** (or any code identified only by an XML
   string, whose type is genuinely unknown until parse time) — that's idiom
   #6, unrelated to this one. Don't assume "the caller is some other compiled
   binary" alone means reflection is required; check whether that binary is
   another native mod (idiom #5 applies) or the game/host code named only by
   a runtime string (idiom #6 applies) before concluding reflection is
   unavoidable.
6. **Host/mod-author binding: the *lookup* is inherent, but it must stay
   contained to one place.** `BSMLValue`, `BSMLAction`, `BSMLEvent`,
   `StringParseHelper`, `GenericSettingWrapper` resolve a *runtime string*
   (an XML attribute) against a field/method on a type BSML has never seen
   (the mod author's ViewController) — that lookup itself can't become a
   compile-time template/concept, since the type genuinely isn't known until
   the XML is parsed. But the raw `MethodInfo*`/host `System::Object*` pair
   that lookup produces should never leak past the class that resolved it —
   `BSMLAction` already provides `GetFunction<Targs...>()`/`GetSystemAction<Targs...>()`/
   `GetUnityAction<Targs...>()` to wrap `{host, methodInfo}` into a plain
   `std::function`/delegate exactly once; every consumer downstream of that
   should hold a `std::function` field and never re-derive or re-store the
   `MethodInfo*`/host pair itself. `ButtonHandler.cpp` already does this
   correctly (`action->GetUnityAction()`); `ModalColorPickerHandler.cpp`
   didn't (it copied `action->host`/`action->methodInfo` into its own fields,
   duplicating the invocation mechanism) — fixed this session, see backlog #2.
   Check any new `TryGetAction`/`TryGetValue` call site against this before
   assuming a `MethodInfo*` field is needed. The *result* of a lookup
   (found-or-not, wrong-type-or-not) is also exactly the kind of fallible
   outcome idiom #8 targets, so turning today's ad-hoc null-checks/silent-failures
   into `std::optional`/`std::expected`-returning helpers is worth doing too
   (see backlog item 8) — it's the IL2CPP class/method/field lookup itself
   that must stay reflection-based, not how its result is reported or reused.
7. **Prefer RAII over raw ownership** where BSML itself briefly owns a
   GC-tracked handle across a multi-step sequence (e.g. `safe_ptr<T>` fields
   on a short-lived result struct), even though the final handed-back pointer
   is conventionally raw (matches existing codebase convention — Unity's own
   scene graph is what actually keeps things alive once parented).
8. **Reach for C++26 idioms over ad-hoc null/throw conventions.** A lookup
   that can legitimately fail should return `std::optional<T>` rather than a
   nullable raw pointer the caller has to remember is nullable, and rather
   than throwing on an unexceptional "not found"/null-instance case. A fallible
   operation that needs to communicate *why* it failed (not just that it did)
   should return `std::expected<T, E>` instead of a bool-and-out-param or a
   thrown exception used for control flow. Mark functions `noexcept` when
   they provably can't throw (most plain-C++-idiom code following this
   roadmap; not IL2CPP-facing calls, which can). Prefer templates/concepts —
   resolving overloads and constraints at compile time — over IL2CPP RTTI
   (`class_is_assignable_from`, `class_get_method_from_name`) wherever the
   type in question is already statically known; RTTI stays only where the
   type genuinely isn't known until runtime (see idiom #6). Concrete example
   landed this session: `BSML::ExternalComponents::TryGetByType`/`TryGet<T>()`
   (`shared/BSML/Components/ExternalComponents.hpp`) — a null-safe,
   `std::optional`-returning sibling to the older `GetByType`/`Get<T>()`,
   which throws on a null instance and returns a bare possibly-null pointer
   on a miss.
9. **Prefer `std::string`/`std::string_view` over `StringW` for anything that
   stays on the C++ side.** `StringW` should show up in a signature only when
   the value's *final* destination is an IL2CPP call — a parameter that's
   about to be handed to `set_text(StringW)`, a key into an IL2CPP
   `Dictionary<StringW, T>`, a return value coming straight back from a
   reflective `i2c::run_method<StringW>(...)`. Where a function only ever
   does plain string work (comparison, parsing, concatenation) and never
   touches IL2CPP with the value, it should take `std::string_view` (or
   return `std::string`) — `StringW` there just adds an unnecessary
   conversion and, worse, invites calling into `System::String`'s own IL2CPP
   methods (`str->EndsWith(...)`, `str->Substring(...)`) for things plain C++
   already does. `StringW`'s own `operator std::string()` makes the
   conversion at a call site trivial and explicit
   (`IsAnimated(std::string(path))`) when the caller already has a `StringW`
   in hand. Concrete example fixed this session: `Helpers::IsAnimated`
   (`src/Helpers/utilities.cpp`) took a `StringW` purely to call
   `str->EndsWith(...,OrdinalIgnoreCase)` (a real IL2CPP call) four times for
   a file-extension check — switched to `std::string_view` with a plain C++
   case-insensitive suffix check. Most of BSML-Lite's own `Create*(..., StringW
   text, ...)` parameters are *not* violations of this — the text is passed
   straight to a `set_text(StringW)` call, so the "final result becomes
   StringW" exception applies and keeping the parameter typed as `StringW`
   (rather than `std::string_view` + a conversion at the call into
   `set_text`) is both correct and avoids converting twice for a caller who
   already has a `StringW`. Audit new/touched functions against this rather
   than sweeping all ~150 existing `StringW` usages at once — most of them
   are already the legitimate case.
10. **Mind build times and header includes — but keep this low-risk, not a
    restructuring project.** bs-cordl/Unity codegen headers are heavy, and
    this codebase's headers are transitively included everywhere, so a
    careless `#include` in a widely-used header (`BSML-Lite/Creation/*.hpp`,
    `ComponentCreation.hpp`, anything under `BSML/Components/`) costs every
    translation unit that touches BSML, not just its own. Concrete,
    low-risk moves, roughly in order of value for effort:
    - **Forward-declare instead of including** in a header when the header
      only uses a type as a pointer or reference (never calls a member,
      never needs its size) — move the real `#include` to the corresponding
      `.cpp`. This is the single highest-value, lowest-risk change available;
      it doesn't change behavior at all, only compile dependencies.
    - **Don't add a new include to an already-widely-included header** when
      the thing needed is only used in the `.cpp` — put it there instead.
      Check with `grep`/an include graph before adding to `ComponentCreation.hpp`,
      `concepts.hpp`, `TransformWrapper.hpp`/`GameObjectWrapper.hpp`, or any
      `Creation/*.hpp` in `BSML-Lite` specifically, since those are the most
      widely-pulled-in.
    - **Prefer the narrowest codegen header that declares the type actually
      used**, not a broader umbrella header, when both exist.
    - Don't reach for precompiled headers, unity/jumbo builds, or splitting
      existing headers apart purely for compile speed — those are real
      build-time wins but are invasive (change the build graph itself,
      harder to review, easier to get subtly wrong) and out of scope for
      "not super invasive." If build times become a real problem, that's its
      own dedicated pass with its own design discussion, not something to
      fold into an ordinary idiom-#6-migration or Options-struct pass.
    - When a pass touches a header anyway (e.g. backlog #6 migrations),
      it's a reasonable moment to also check that header's includes against
      this idiom — but don't go looking for headers to trim as a task on its
      own; let it ride along with work already happening there.

## Status

| Pass | Status | Landed | Notes |
|---|---|---|---|
| Button proof-of-concept (`BSML::Lite::CreateUIButton`) | ✅ Done | this session | `shared/BSML-Lite/ComponentCreation.hpp`/`.cpp` (new), `Buttons.hpp`/`.cpp` rewritten, no more `#define protected public`. Full NDK build verified (`qpm s build`, links clean). |
| `ButtonHandler.cpp`: `reinterpret_cast` → `i2c::try_cast` | ✅ Done | this session | Both `HandleType`/`HandleTypeAfterParse` casts fixed; `ComponentTypeWithData::data` documented; the two `componentType.data.find(...)` lookups replaced with a small `GetAttribute()` helper for readability. |
| `ExternalComponents`: `std::optional`-returning lookups | ✅ Done | this session | Added `TryGetByType`/`TryGet<T>()`, null-safe siblings to `GetByType`/`Get<T>()` (which throw on a null instance). Shared lookup loop factored into a private `FindByType` so nothing is duplicated. First concrete instance of idiom #8. |
| Backlog #1: self-reflection removal | ✅ Done | this session | `TabSelector.cpp`, `ScrollableContainer.cpp`, `Settings/{ToggleSetting,SliderSetting,ListSliderSetting}.cpp` now use `MakeUnityAction`/`MakeSystemAction(std::bind(...))` instead of `class_get_method_from_name`. Explicit `Targs` don't work with the `T fun` overloads (they bind to `T`, not the pack) — wrap the bind result in an explicitly-typed `std::function<void(Targs...)>` instead and let the dedicated `std::function` overload deduce `Targs`. **`Settings/IncDecSetting.cpp` deliberately excluded** — see note below. |
| Backlog #2: `ModalColorPicker` dual callback collapse | ✅ Done (corrected) | this session | `ColorSetting::Setup()` assigns `modalColorPicker->done`/`cancel` via `std::bind`. **Correction after first pass**: the `onDoneInfo`/`onCancelInfo`/`colorChangeInfo` `MethodInfo*`/host fields were initially kept on the theory that `ModalColorPickerHandler.cpp`'s XML host-binding genuinely needed them — missed that `BSMLAction` already has a `GetFunction<Targs...>()` wrapping `{host, methodInfo}` into a `std::function` (already used correctly by `ButtonHandler.cpp` via `action->GetUnityAction()`). Switched `ModalColorPickerHandler.cpp` to `action->GetFunction<UnityEngine::Color>()`/`GetFunction<>()` assigned straight to the `done`/`cancel`/`onChange` fields, and **deleted** the `MethodInfo*`/host fields and their now-dead `friend`s entirely — the raw reflection stays contained inside `BSMLAction`, nothing else touches it. Also fixed two `reinterpret_cast<ModalColorPicker*>` → `i2c::try_cast` in that handler (idiom #4) while in the area. See idiom #6 update below — this pattern (route host-binding through `BSMLAction::GetFunction`/`GetUnityAction`/`GetSystemAction` instead of storing a raw `MethodInfo*`/host pair a second time) generalizes and is worth checking for at each remaining `TryGetAction`/`TryGetValue` call site. |
| Backlog #3: `TableView` base-call idiom | ✅ Done | this session | `ReloadData`/`DidSelectCellWithIdx` now use `i2c::metadata_getter<&HMUI::TableView::X>::method_info()` instead of a string lookup. |
| Backlog #4: registry consolidation | ⚠️ Investigated, merge deferred | this session | Real merge blocked by Unity-rendering-pinned storage types + public API risk (details below). Applied the one safe fix found: `MenuButtons.cpp`'s `reinterpret_cast` → `i2c::try_cast`. |
| Backlog #5: `BSMLViewController` shim | ✅ Done (corrected twice) | this session | First pass cached the `MethodInfo*`; a second pass wrongly concluded the shim pattern couldn't apply here at all (conflated "the game's IL2CPP-generated classes, genuinely unknown at compile time" with "another mod's C++ code, compiled with the same NDK toolchain and dynamically linked like any shared library" — the latter supports ordinary C++ virtual dispatch across the .so boundary just fine). **Corrected**: added `BSMLContentProvider` (`shared/BSML/ViewControllers/BSMLContentProvider.hpp`/`.cpp`), a pure-virtual interface (`GetContent()` required, `GetFallbackContent()` optional with a default) that `BSMLViewController` now holds via `std::unique_ptr<BSMLContentProvider> contentProvider`. `ParseWithFallback()` prefers `contentProvider` (real virtual call, signature-checked at compile time, zero reflection) and only falls back to the legacy by-name `get_Content`/`get_FallbackContent` resolution when `contentProvider` isn't set, so existing mods keep working unmodified. |
| Backlog #6: `Text`/`ClickableText` migration to `BSML::Lite` | ✅ Done (1 of ~38) | earlier session | First component migrated per the pattern below — `shared/BSML-Lite/Creation/Text.hpp`/`.cpp` rewritten: `TextOptions`/`ClickableTextOptions` structs replace the ~13-overload pyramid, `#define protected public` + `Tags/{TextTag,ClickableTextTag}.hpp` includes removed, creation logic (font/material/color/rect setup, the click-signal/haptics singletons) now owned directly. Tags left untouched at the time (same deliberate duplication as the Button pass) — **now flipped, see next row.** |
| Backlog #6: Image/Layout/Misc/Lists clusters, and flipping Tags to call `BSML::Lite` | ✅ Done (16 of ~38 tags total) | this session | Extends backlog #6 a full step further than Button/Text did: not just extracting creation logic into `BSML::Lite`, but also flipping each Tag's `CreateObject` to call it (step 2 of the backlog #6 plan), while deliberately leaving each `TypeHandler` in place (steps 3/4 are a later pass — XML attribute-driven property setting via the broadcast+RTTI system still needs them until each component's full XML-settable surface is captured in its `*Options` struct too). Covered, each with a real NDK build verified: **Image family** (`ImageTag`, `RawImageTag`, `ClickableImageTag` → `ImageOptions`/`ClickableImageOptions`/`RawImageOptions` in `Image.hpp`/`.cpp`; found and fixed a real pre-existing bug — `CreateRawImage` was instantiating `ImageTag` instead of `RawImageTag`, so `GetComponent<RawImage*>()` always returned null). **Layout family** (`VerticalTag`, `HorizontalTag`, `GridLayoutTag`, `StackLayoutTag`, `ScrollViewTag`, `SettingsContainerTag`, `ModalTag`, `ModifierContainerTag` → all own their creation directly in `Layout.cpp` now; `get_scrollViewTemplate()` kept as a `namespace BSML` free function since it's also forward-declared and called directly by `TextPageScrollViewTag.cpp` and two `TypeHandlers` — moving or duplicating it would've broken those). **Misc**: `TextSegmentedControlTag` → `Misc.cpp`. **Lists**: `ListTag` → `Lists.cpp`. Also extracted a genuinely-duplicated (not one-off) click-signal/haptics-preset singleton helper — previously copy-pasted per-component (`ClickableImageTag`, and `Text.cpp`'s own copy from the earlier Text pass) — into shared `BSML::Lite::GetClickedSignal()`/`GetClickHapticPreset()`/`GetClickHapticFeedbackManager()` in `ComponentCreation.hpp`/`.cpp`, and switched both callers to it. **New idiom established this session**: where a component's `BSML::Lite::Create*` function has always unconditionally overridden position/size (pre-existing behavior, safe for direct C++ callers who pass real values), but the Tag never touched that dimension at all (relying on a separate generic `RectTransformHandler` XML attribute to size it later) — don't route the Tag through the full public function with a made-up `{0,0}`, since that forces a zero-size default the original Tag never had. Instead split out a `Create*Base()` (no position/size override) that both the Tag and the public function build on — done for `CreateListBase`/`CreateTextSegmentedControlBase`, forward-declared cross-TU the same way as `get_scrollViewTemplate()` rather than added to the public header (they're not part of BSML::Lite's public API). All builds verified via real `ninja -C build` after each cluster. |
| Backlog #6: `Settings/` cluster migration | ✅ Done (24 of ~38 tags total; 0 active hack usages remain anywhere) | this session | The biggest remaining cluster, migrated in full: `TextFieldTag`/`ModifierTag`/`ToggleSettingTag`/`IncrementSettingTag`+`ListSettingTag` (via `IncDecSettingTagBase`)/`SliderSettingTag`+`ListSliderSettingTag` (via `GenericSliderSettingTagBase`)/`DropdownListSettingTag`/`ColorSettingTag`/`ModalColorPickerTag` — 8 Tags backing 7 public `Settings.hpp` functions. Two real design subtleties found and handled, both worth remembering for the remaining ~14 Tags: **(1) virtual override points** — `TextFieldTag::get_fieldViewPrefab()` is `virtual` and `protected`, a genuine XML-Tag-subclassing extensibility point for downstream mods; routing `BSML::Lite::CreateStringSetting` through it would've required calling through the Tag (defeating the point of removing the hack) or losing the override hook, so `CreateStringSetting` reimplements the same default-prefab lookup independently and `TextFieldTag.cpp` was left **completely untouched** — check for `virtual` protected members on any Tag before assuming it's safe to fully absorb into `BSML::Lite`. **(2) state-initialization, not just position/size, needs the `Create*Base()` split** — `CreateDropdown`/`CreateColorPicker`/`CreateModifierButton`/`CreateIncrementSetting`/`CreateSliderSetting`/`CreateToggle` all call some combination of `Setup()`/`values`-population/`index`/`UpdateState()`/position that the bare XML-driven Tag never did (left for the `TypeHandler` to trigger via attributes later) — confirmed via `grep -n "\\->Setup()"` that these `Setup()` calls have *always* only existed in the `BSML::Lite` C++-facing wrapper, even before this session, so this isn't a new risk introduced by the migration, but it does mean **every** `Create*`/`Create*Base` split in this cluster needed the same care as the sizing-only cases from the previous cluster. Also fixed a genuine idiom #3 violation found while porting `ModalColorPickerTag`: `i2c::functions::class_get_method_from_name(colorPicker->klass, "OnChange", 2)` reflectively resolving `BSML::ModalColorPicker`'s own statically-known `OnChange` method (confirmed callable directly — `DECLARE_INSTANCE_METHOD` self-inserts `public:`, so no access-control issue) → replaced with `MakeSystemAction(std::bind(&BSML::ModalColorPicker::OnChange, colorPicker, ...))`, matching the established idiom #3 pattern. `ColorSettingTag` (which composes `ModalColorPickerTag` via inheritance, not the hack) was also flipped to call the new `CreateColorPickerBase()` for DRY, even though it didn't strictly need to for hack-removal. Full build (`ninja -C build`) links clean. **`grep -rn "define protected public\|define private public" $(find src -name "*.cpp")` now matches zero active `#define`s anywhere in the codebase** — only explanatory comments remain. |
| Backlog #6: remaining ~14 standalone Tags | ✅ Done (38 of ~38 tags total — step 1+2 complete for every Tag) | this session | Designed and landed a brand-new `BSML::Lite::Create*` for each of the 14 Tags that had no existing facade to extend (real API design, not mechanical hack-removal — none of these had ever used `#define protected public`, they built their GameObjects directly): `CreateGradientText` (`Text.hpp`/`.cpp`), `CreateIconButton`/`CreatePageButton` (`Buttons.hpp`/`.cpp`), `CreateCustomList` (`Lists.hpp`/`.cpp`, takes a raw BSML XML string — the component's data source is inherently XML-shaped, not a hack), `CreateScrollableContainer` (`Layout.hpp`/`.cpp`), and `CreateLoadingIndicator`/`CreateScrollIndicator`/`CreateIconSegmentedControl`/`CreateVerticalIconSegmentedControl`/`CreateLeaderboard`/`CreateTabSelector`/`CreateTab`/`CreateTextPageScrollView` plus an internal-only `CreateProgressBarBase` (`Misc.hpp`/`.cpp`). Each is a faithful line-for-line port of the original Tag's `CreateObject` body — confirmed via `grep` that none of the absorbed `get_*Template()` singleton helpers (`get_loadingTemplate`, `get_scrollIndicatorTemplate`, `get_pageButtonTemplate`, `get_buttonWithIconTemplate`, `get_customListCanvasTemplate`, `get_iconSegmentedControlTemplate`, `get_verticalIconSegmentedControlTemplate`, `get_leaderboardTemplate`, `get_tabSelectorTagTemplate`) had any other callers before deleting them. Two components reused an *already-identical* existing template lookup instead of duplicating it: `CreateCustomList` reuses Lists.cpp's `GetListCanvasTemplate()` (verified byte-for-byte identical DI-resolve chain to the old `get_customListCanvasTemplate()`), and `CreateIconButton` reuses Buttons.cpp's existing `GetPracticeButtonPrefab()` (same prefab source as `ButtonWithIconTag` always used). `TabTag` still nominally extends `BackgroundTag` in its header (untouched, zero risk) but its `CreateObject` no longer calls `Base::CreateObject` — it calls the new `BSML::Lite::CreateTab`, which independently re-implements `BackgroundTag`'s bare body (same accepted duplication tradeoff as `GradientTextTag`/`TextTag` from the previous cluster, since a free function can't call a `protected` inherited method without the hack). **Not yet verified with a real build** — the verifying `ninja -C build` was interrupted (exit 137) and not re-run at the user's request; next session (or whenever a build is next run) should treat this row as the first thing to compile-check. Steps 3/4 (delete each `TypeHandler` once nothing calls `HandleType` for it, then delete `ComponentTypeWithData`/`TypeHandlerBase`) remain untouched for every component migrated so far, including this cluster — see the rewritten backlog #6 section below for why that's a bigger step than it sounds. |
| Idiom #6 follow-through: `ModalColorPicker` fully reflection-free | ✅ Done | this session | Correction on top of backlog #2 — deleted the `MethodInfo*`/host fields entirely, routed through `BSMLAction::GetFunction<Targs...>()` instead. See idiom #6 and backlog #2 above. |
| Idiom #5 correction: `BSMLContentProvider` shim | ✅ Done | this session | `shared/BSML/ViewControllers/BSMLContentProvider.hpp`/`.cpp` (new) — a real pure-virtual interface (`GetContent()` required, `GetFallbackContent()` optional), held via `std::unique_ptr` on `BSMLViewController`, dispatched with a plain virtual call instead of reflection. Legacy by-name path kept as a fallback for existing mods. See backlog #5 above (rewritten — the original conclusion that this pattern couldn't apply was wrong). |
| Idiom #9 (new): `std::string`/`std::string_view` over `StringW` | ✅ Done (1 example) | this session | Added as core idiom #9 above. Fixed `Helpers::IsAnimated` (`src/Helpers/utilities.cpp`) as the first concrete example — took `StringW` purely to make 4 IL2CPP `EndsWith` calls for a file-extension check; now `std::string_view` with a plain C++ suffix check. ~150 other `StringW` usages exist codebase-wide; most already fit the "final result becomes StringW" exception — audit opportunistically per-function, not as a sweep. |
| Idiom #10 (new): header/include hygiene | 📝 Documented only | this session | Added as core idiom #10 + backlog #9 above, at the user's request, deliberately scoped to low-risk moves (forward-declare over include, don't widen widely-included headers) — no PCH/unity-build/header-splitting. Not yet applied anywhere; no baseline build-time measurement taken yet either (see backlog #9). |
| `Create*Base()` split eliminated (user-directed redesign) | ✅ Done | this session | User feedback: "base methods should not be necessary, just use default options." Merged all 6 position/state-only `Create*Base()` functions into their public `Create*()` counterpart, so there's exactly one function per component and Tags just call it with (mostly) default arguments instead of a separate internal-only function: `CreateList`/`CreateListBase` (Lists.cpp), `CreateTextSegmentedControl`/`...Base` and `CreateProgressBar`/`...Base` (Misc.cpp — the ProgressBar case needed no merge at all, the Tag now just calls the existing public `CreateProgressBar({0,0,0}, {0,0,0}, {1,1,1}, "")` directly and reparents itself), `CreateModifierButton`/`...Base`, `CreateToggle`/`...Base`, `CreateDropdown`/`...Base`, `CreateColorPicker`/`...Base` (all in Settings.cpp). Mechanism, confirmed with the user via two clarifying questions before implementing: **position/size fields became `std::optional<UnityEngine::Vector2>` defaulting to `std::nullopt`** ("don't touch, keep the template's natural value" — exactly reproduces old Base behavior; a direct caller who wants `{0,0}` still just passes it), while **state fields (label text, values lists, current-value) got real default values matching what Base already hardcoded** (e.g. `label = "BSMLDropdownSetting"`), and `Setup()`/finalization calls now run unconditionally *except* `CreateDropdown`, which only calls `Setup()`/selects an index when `values` is non-empty (calling `SelectCellWithIdx` on an empty dropdown looked like a real out-of-bounds risk, unlike the other Setup() calls) — the Tag gets this by simply not passing `values`, no separate flag needed. `CreateList` also gained a plain (non-optional) `bool activate = true` parameter, since "is the list left inactive so a TypeHandler can finish sizing/populating it before it's shown" is a genuine timing need, not a "leave at natural value" case — `ListTag` passes `false`. `CreateDropdown`'s return type stays `BSML::DropdownListSetting*` (best for C++ callers) even though the component lives on a child GameObject, not the wrapper `DropdownListSettingTag` needs to return — resolved with `dropdownSetting->GetComponentInParent<BSML::ExternalComponents*>()->get_gameObject()` in the Tag, cleaner than the old two-different-return-types split. **Deliberately NOT merged**: `CreateIncDecSettingBase`/`CreateGenericSliderSettingBase` — these aren't a "Tag needs bare defaults" case at all; they're shared across *two* Tag leaf types each (`IncrementSettingTag`+`ListSettingTag`; `SliderSettingTag`+`ListSliderSettingTag`) via a runtime `System::Type*` the concrete public functions (`CreateIncrementSetting`/`CreateSliderSetting`) never need, since those Tags never call the public wrapper at all — a genuine type-genericity reason to stay separate, not a default-value problem. All Tag `.cpp` call sites updated; grepped for every removed function name across `src`/`shared` to confirm zero remaining references. **Not yet build-verified** — this is a substantial, multi-file signature change and the highest-risk unverified work of the session; should be the first thing compiled. |
| Idiom #10 header hygiene attempt on `Misc.hpp`/`Layout.hpp`/`Text.hpp` | ⚠️ Mostly reverted — 1 real fix landed | this session | Tried forward-declaring `Create*` return types (`HMUI::IconSegmentedControl`/`TextPageScrollView`, `GlobalNamespace::LeaderboardTableView`, `BSML::TextGradientUpdater`) in place of full includes, per idiom #10. **Turned out unsafe for this specific family of headers**: every `BSML::Tag::CreateObject` that delegates to a `BSML::Lite::Create*` function typically does `return BSML::Lite::CreateX(parent)->get_gameObject();` in one expression, in a `.cpp` that only includes the `Creation/*.hpp` header — a forward declaration there makes the member-function call on an incomplete type fail. Caught by grepping each consumer `.cpp` for a direct dereference after the `Create*` call, not by a build (none available this session) — reverted all 4 attempts back to full includes before this could land as a real regression. **One genuine, safe win survived**: `Misc.hpp` had a full `#include` for `BSML/Components/TabSelector.hpp` that was never actually used in any signature (only in a `///` doc comment) — `CreateTabSelector` returns `UnityEngine::GameObject*`, not `TabSelector*` — so that one's gone for real, no forward-declare needed since nothing references the type at all. Recorded as a feedback memory so this isn't attempted again the same way. **Not yet build-verified** (the one real removal, specifically). |
| TypeHandler `reinterpret_cast` → `i2c::try_cast` sweep | ✅ Done | this session | `ButtonHandler.cpp` got this fix long ago (idiom #4), but ~15 sibling `TypeHandler`s doing the exact same `reinterpret_cast<X*>(componentType.component)` pattern were left behind — found via a full `reinterpret_cast` grep across `src/BSML/TypeHandlers`. `componentType.component` is precisely idiom #4's stated case (RTTI-broadcast-matched, "whose exact type isn't locally, statically obvious"), unlike the `GameplaySetup.cpp`/`SettingsMenu`-family casts investigated and correctly left alone elsewhere in this table. Fixed all of them, matching `ButtonHandler.cpp`'s exact idiom (`Base::HandleType(...)` called *first*, then `i2c::try_cast` + `if (!x) { ERROR(...); return; }`): `ModalKeyboardHandler` (both overrides), `ClickableImageHandler`, `ClickableTextHandler`, `CustomCellListTableDataHandler`, `CustomListTableDataHandler`, `IconSegmentedControlHandler`, `InputFieldViewHandler`, `LayoutGroupHandler`, `ModalViewHandler`, `ScrollableContainerHandler` (both overrides), `TabSelectorHandler`, `TextSegmentedControlHandler`, and the 3 `Settings/` handlers (`DropDownListSettingHandler`, `ListSettingHandler`, `ListSliderSettingHandler`). Several of these called `Base::HandleType`/`HandleTypeAfterParse` *after* their type-specific logic instead of before — reordered to match `ButtonHandler.cpp`'s convention (call `Base::X` first) since an early return after a failed cast must not skip it. `TextSegmentedControlHandler.cpp`'s other 2 `reinterpret_cast`s (converting between `List_1<System::Object*>*`/`List_1<StringW>*` IL2CPP generic instantiations) are a different, legitimate pattern — left alone, not part of this fix. Also checked every other `TypeHandler` (23 more, including ones that looked like candidates by filename — `ScrollIndicatorHandler`, `TextGradientUpdaterHandler`, `TabHandler`, etc.): none of them touch `componentType.component` directly at all — they only implement `get_props()`/`get_setters()`, and the generic `TypeHandler<T>::HandleType` in `TypeHandler.hpp` (base class) does the one remaining `reinterpret_cast<T>(componentType.component)` for the whole codebase, immediately after its own `i2c::functions::class_is_assignable_from(...)` check earlier in the *same function* — a provably-safe, locally-verified cast (unlike a "three frames up" assumption), so this one correctly stays a `reinterpret_cast` per idiom #4's own stated exception. One consequence worth noting: for the `HandleType`-overriding files in this fix, `Base::HandleType` already re-verifies the same assignability before the override's own `try_cast` runs, so that particular `try_cast` is defense-in-depth rather than the *only* check (matches `ButtonHandler.cpp`'s own established pattern, so not a problem) — but for every `HandleTypeAfterParse` override fixed here, `TypeHandlerBase::HandleTypeAfterParse`'s default body is empty (no RTTI check at all), so those `try_cast`s are the *only* safety net and are a real fix, not redundant. **Not yet build-verified.** |
| Cross-TU forward-declare cleanup | ✅ Done | this session | User correction: internal helpers shared across translation units (`Create*Base()` functions, `get_scrollViewTemplate()`, `stringToTableType()`, `collect_minfos()`) were being forward-declared locally inside each calling `.cpp` instead of getting a real header declaration — including one pre-existing instance of the same anti-pattern from before this session. Fixed by giving each a real declaration in the most relevant header (`Settings.hpp`/`Misc.hpp`/`Lists.hpp`/`Layout.hpp`, `CustomListTableDataHandler.hpp`, `BSMLValue.hpp`) marked as an internal (not-public-API) step where applicable, and switching every consumer to `#include` that header instead. `get_scrollViewTemplate()` also moved (not just re-declared) from `ScrollViewTag.cpp` into `BSML-Lite/Creation/Layout.cpp` as `BSML::Lite::GetScrollViewTemplate()` — it had no reason to live in Tags-land any more since `ScrollViewTag::CreateObject` already fully delegates to `BSML::Lite::CreateScrollView`; this also fixes the dependency direction (Tags/TypeHandlers → BSML-Lite, not the reverse) for the 2 TypeHandlers that use it. Also found and deleted 3 genuinely dead `extern` declarations in `SubmenuTag.cpp` (`get_textClickedSignal`/`get_textHapticPreset`/`get_textHapticFeedbackManager`) that referenced functions renamed away during this session's earlier Text.cpp work and were never actually called from that file. **Not yet build-verified.** |
| Everything else below | 🔲 Not started | — | — |

**Correction found while doing backlog #1**: `Settings/IncDecSetting.cpp`'s `BaseSetup()` reflects on `IncButtonPressed`/`DecButtonPressed` — but unlike the other 5 files, those methods are **not declared on `IncDecSetting` itself**; only its subclasses `IncrementSetting`/`ListSetting` declare them (each independently, no shared virtual base). `BaseSetup()` is called both from a concretely-typed call site (`BSML-Lite/Creation/Settings.cpp`, knows the real type) *and* reflectively from `BaseSettingHandler.cpp` (doesn't — dispatches whatever `TypeHandler` RTTI-matched). From inside `IncDecSetting::BaseSetup()`, `this` is statically only an `IncDecSetting*`, so there's no C++-visible `IncButtonPressed` to bind to — this is cross-subclass dispatch, not self-reflection, much closer to idiom #6 than idiom #3. Making it a real C++ `virtual` method was considered and rejected: none of the ~15 `DECLARE_CLASS_CUSTOM` hierarchies in this codebase use `virtual`/`override`, and adding a C++ vtable to an IL2CPP-visible type risks disturbing the object layout `custom-types` assumes — exactly the risk idiom #5's shim pattern exists to avoid, and not something to do as a drive-by fix. Left as-is; a real fix belongs with backlog #6's `BSMLViewController`-style shim work, not here.

## Backlog, roughly ordered by risk/effort (lowest first)

### 1. Remove self-reflection on known methods (mechanical, low-risk) — ✅ Done
Files: `Components/ScrollableContainer.cpp`, `Components/TabSelector.cpp`,
`Components/Settings/ToggleSetting.cpp`,
`Components/Settings/SliderSetting.cpp`, `Components/Settings/ListSliderSetting.cpp`.
Replaced `i2c::functions::class_get_method_from_name(this->klass, "X", n)` +
`MakeUnityAction(this, methodInfo)` with `MakeUnityAction(std::bind(&Self::X, this))`
(2+ args: wrap in an explicitly-typed `std::function<void(Targs...)>` first — see
Status table note on why explicit `Targs` alone doesn't compile).
`Components/Settings/IncDecSetting.cpp` turned out **not** to fit this pattern
(cross-subclass dispatch, not self-reflection) — see the Status table note above.

### 2. Collapse `ModalColorPicker`'s dual callback mechanism — ✅ Done
File: `Components/ModalColorPicker.hpp`/`.cpp`. The `MethodInfo*`/host-object
fields were **kept**, not removed — `ModalColorPickerHandler.cpp` needs them for
arbitrary XML host binding (idiom #6). Updated `Components/Settings/ColorSetting.cpp::Setup()`,
which used to populate that reflective path with pointers to its own
`DonePressed`/`CancelPressed` methods, to assign `std::bind` lambdas to the
existing `done`/`cancel` `std::function` fields instead.

### 3. Fix `TableView::ReloadData`/`DidSelectCellWithIdx` base-call idiom — ✅ Done
File: `Components/TableView.cpp`. Switched from a per-call
`i2c::functions::class_get_method_from_name(klass, "ReloadData", 0)` (string
lookup) to the typed `i2c::metadata_getter<&HMUI::TableView::ReloadData>::method_info()`
form already used for `Finalize()` elsewhere (`Animations/AnimationControllerData.cpp`).

### 4. Consolidate the three redundant "register a menu entry" registries — investigated, merge deferred
Read all three in full this session. Finding: a real merge is riskier than the
original description suggested, and was **not** done —
- `MenuButtons::_buttons` and `GameplaySetup::_menus` are both
  `ListW<System::Object*>` (IL2CPP-visible), and both are handed *directly* to
  a live Unity render path (`menuButtonsViewController->buttons = get_buttons();`;
  `GameplaySetup::CellForIdx`/`NumberOfCells` index `_menus` straight off a
  `HMUI::TableView::IDataSource`). Their storage shape is pinned by Unity's
  rendering requirements, not a free choice — replacing `ListW` with e.g. a
  `std::unordered_map` would require a parallel index kept in sync, not a
  drop-in swap.
- `MainMenuRegistration::registrations` is a genuinely different, pure-C++
  `std::vector` holding presentation strategy + cached ViewController/FlowCoordinator
  instances — conceptually paired with a `MenuButton` entry only by a matching
  `buttonText` string (`AddMainMenuRegistration` creates one of each). Its own
  `get_registration(buttonText)` lookup, the only reason it's keyed by string
  at all, has **zero callers anywhere in the codebase** — the actual click
  dispatch bypasses it entirely via a `std::bind(&MainMenuRegistration::Present, reg)`
  captured at registration time.
- All three are exposed as public fields/API (`MenuButton*`, `RegisterMenuButton`,
  `AddTab`, `MainMenuRegistration`) that downstream Quest mods link against
  directly — restructuring their storage shape is a real compatibility break
  for the mod ecosystem, not an internal-only refactor.

Given that, forcing a merge now would trade a cosmetic "three registries" wart
for real rendering/API risk. Applied the one safe, in-scope fix instead:
`MenuButtons::Registerbutton`'s duplicate-name scan used
`reinterpret_cast<MenuButton*>(b)` over a generic `System::Object*` list —
switched to `i2c::try_cast<MenuButton*>` (idiom #4; this one, unlike the
`GameplaySetup.cpp` casts below, checks an externally-registered arbitrary
object, so the RTTI check has real defensive value and it's not a hot path).
Left `GameplaySetup.cpp`'s several `reinterpret_cast<GameplaySetupMenu*>` calls
alone — every element of `_menus` is provably always a `GameplaySetupMenu*` by
construction (BSML's own invariant, not a broadcast/RTTI-matched value like
`ButtonHandler.cpp`'s), and some are on the per-cell render path, so a `try_cast`
there would add per-frame RTTI cost for no real safety gain — doesn't fit
idiom #4's own stated rationale ("whose exact type isn't locally, statically
obvious"). If this is ever revisited, the real unlock is deciding whether
`MenuButton`/`MainMenuRegistration` should merge into one public type (an API
redesign) — not a mechanical pass.

### 5. `BSMLViewController` shim + interface split — ✅ Done
Two corrections happened on this item before landing on the right answer —
worth recording both, since the second mistake is an easy one to repeat.

**First pass**: cached the `get_Content`/`get_FallbackContent` `MethodInfo*`
per klass (`i2c::run_method<StringW>(this, "...")` was re-resolving the string
every single `ParseWithFallback()` call) but concluded the reflection itself
was unavoidable.

**Second pass (wrong)**: reasoned that since the override lives on an
"arbitrary mod author's ViewController subclass, compiled independently of
BSML," there's "no shared C++ vtable slot BSML and a separately-compiled mod
binary could possibly both link against" — concluding the idiom #5 shim
pattern couldn't apply and that this was structurally idiom #6 (host-binding),
like `BSMLValue`/`BSMLAction`. **This conflated two different kinds of
"unknown at compile time."** The *game's* IL2CPP-generated C# classes really
are unknown until runtime — that's genuine idiom #6. But another BSML mod's
C++ code is not: every Quest mod, BSML included, is a native shared library
(`.so`) built with the same NDK toolchain against BSML's own public headers,
and dynamically linked together the same way any C++ shared library is. A
virtual call through a vtable defined in a shared header works correctly
across that boundary — there's nothing IL2CPP-specific about it, and no
reflection is needed at all.

**What actually shipped**: `BSMLContentProvider` (`shared/BSML/ViewControllers/BSMLContentProvider.hpp`/`.cpp`),
a plain C++ abstract interface — `virtual StringW GetContent() = 0` (required)
and `virtual StringW GetFallbackContent()` (optional, defaults to BSML's
built-in "Invalid BSML" page via a shared `DefaultFallbackContent()` free
function so the markup isn't duplicated). `BSMLViewController` gained a
`std::unique_ptr<BSMLContentProvider> contentProvider` field.
`ParseWithFallback()` now calls `contentProvider->GetContent()`/`GetFallbackContent()`
directly (real virtual dispatch, compiler-checked signature, no reflection at
all) when `contentProvider` is set, and only falls back to the legacy by-name
`get_Content`/`get_FallbackContent` resolution (still cached per klass) when
it isn't — so this is purely additive, not a breaking change for existing
mods that still override the old way.

**Generalizes beyond this one class**: any other place in BSML where behavior
is currently resolved by name against a *derived BSML type* (not the game's
own generated code) is a candidate for the same fix — a real interface +
`std::unique_ptr` field, not reflection. Worth checking for during backlog #6.

### 6. Tags/TypeHandlers/Macros → `BSML::Lite` migration (the big one) — steps 1+2 done for all ~38 tags
This is the actual "BSML 2" migration. Button and `Text`/`ClickableText`
(earlier sessions) proved step 1 below generalizes to both prefab-based and
non-prefab components. Sessions since then extended the pattern through step 2
for the **Image**, **Layout**, **Misc**, **Lists**, **`Settings/`** (8 Tags:
`TextFieldTag`, `ModifierTag`, `ToggleSettingTag`,
`IncrementSettingTag`/`ListSettingTag`, `SliderSettingTag`/`ListSliderSettingTag`,
`DropdownListSettingTag`, `ColorSettingTag`, `ModalColorPickerTag`), and finally
the **remaining 14 standalone Tags** (`ButtonWithIcon`, `CustomList`,
`GradientText`, `IconSegmentedControl`, `Leaderboard`, `LoadingIndicator`,
`PageButton`, `ProgressBar`, `ScrollableContainer`, `ScrollIndicator`,
`TabSelector`, `Tab`, `TextPageScrollView`, `VerticalIconSegmentedControl`) —
see the Status table entries above for full specifics, including two real
bugs/violations found (`CreateRawImage` instantiating the wrong Tag; a genuine
idiom #3 violation in `ModalColorPickerTag`'s reflective `OnChange` dispatch)
and two idioms to watch for in every remaining component:
- **The `Create*Base()` split** isn't only for position/size. Any
  state-initialization the bare XML-driven Tag never did but the public
  `BSML::Lite::Create*` wrapper unconditionally does (`Setup()`, populating
  `values`, computing `index`, calling `UpdateState()`, applying
  `anchoredPosition`/`sizeDelta`) needs the same base/public split — the
  `Settings/` cluster needed it far more than the earlier ones did (see the
  Status table row for the specific functions).
- **Watch for `virtual`/`protected` Tag member functions** before assuming a
  Tag can be fully absorbed into `BSML::Lite` — `TextFieldTag::get_fieldViewPrefab()`
  is a genuine downstream-subclassing extensibility point and was left
  completely untouched; `BSML::Lite::CreateStringSetting` reimplements the
  same default lookup independently rather than routing through it.

**Zero active `#define protected public`/`#define private public` uses
remain anywhere in the codebase** (verified: `grep -rn "define protected
public\|define private public" $(find src -name "*.cpp")` matches only
explanatory comments) — every `BSML::Lite::Create*` function now owns its
creation logic directly, and **all ~38 Tags now have a `BSML::Lite` facade
and call it** (steps 1+2 of the plan below are complete for every component).
Per component type:
1. Extract/adapt the Tag's `CreateObject` body into a `BSML::Lite::Create*`
   function taking a typed `*Options` struct (reuse `ComponentCreation.hpp`
   helpers where the component is prefab-based; add new shared helpers there
   if a new *pattern* — not a one-off — emerges). ✅ Done for all ~38.
2. Flip the Tag to call the new `BSML::Lite::Create*` function and drop its
   own duplicate creation/fixup logic — watch for both idioms above. ✅ Done
   for all ~38.
3. Delete the corresponding `TypeHandler` once nothing calls its `HandleType`
   anymore (i.e. once the Tag's property-setting is inlined via the typed
   options struct instead of the string-attribute broadcast). **Not yet done
   for any component.** This is a bigger step than "delete a now-dead file" —
   confirmed by re-reading a few handlers for the just-migrated Tags
   (`ButtonHandler`-style ones): most `TypeHandler`s still do real work no
   `*Options` struct replaces, because `*Options` only covers
   *construction-time* parameters a direct C++ caller would pass, while
   `TypeHandler::HandleTypeAfterParse` is what lets **XML** set those same
   properties by attribute string (`interactable="false"`,
   `on-click="MethodName"`, `text="~someValue"`) *after* construction, via
   `BSMLValue`/`BSMLAction`'s host-binding reflection — which is legitimate
   and stays (idiom #6). Deleting a `TypeHandler` outright would silently
   drop XML support for every attribute it used to bind, not just remove
   dead code. Doing this for real means either (a) confirming a given
   `TypeHandler`'s `get_setters()` map is empty *and* its
   `HandleTypeAfterParse` does nothing beyond what's now in the `*Options`
   struct (a handful may already qualify — worth auditing one at a time,
   starting with handlers for the simplest of the 14 just-migrated Tags), or
   (b) designing how an XML attribute reaches a `*Options` field directly
   without the broadcast+RTTI system, which is a real architecture question
   deserving its own planning pass, not a drive-by deletion.
4. Delete `ComponentTypeWithData` and `TypeHandlerBase`'s registry entirely
   once the last `TypeHandler` is gone.

Do this one component (or one small related cluster) per pass — not all
remaining ones at once.

### 7. Cosmetic/robustness cleanup (no urgency, do opportunistically)
- ~~`StringParseHelper`: collapse ~10 named implicit-conversion operators
  into one `template<typename T> std::optional<T> TryParse(std::string_view)`
  family.~~ **Done (narrower than originally scoped)**: the `tryParseX()`
  named methods themselves turned out to have real, deliberate per-type
  differences worth keeping distinct — `tryParseBool`/`tryParseFloat` rely on
  their *caller* (the operator) to pre-trim whitespace, while
  `tryParseInt`/`tryParseDouble` trim internally; `tryParseVector3` takes an
  extra `defaultZ` parameter; `tryParsePadding` returns a distinct
  `Padding` struct and does CSS-shorthand expansion. Collapsing all of that
  into one generic `TryParse<T>()` would either lose those distinctions or
  need per-type specializations that are no simpler than what's already
  there — not worth the churn/risk for a cosmetic pass. What *was* genuinely
  duplicated 9 times over (`parseVector3` + all 8 non-Vector3 conversion
  operators) was the "unwrap the `optional` or throw `ParseException`"
  wrapper shape — collapsed into a single `ParseOrThrow<T>(std::optional<T>,
  std::invocable auto message)` helper in `StringParseHelper.cpp`'s
  anonymous namespace (the `message` thunk is only invoked, and only builds
  the `fmt::format` string, on the failure path, so this doesn't cost
  anything extra on a successful parse — verified no other files call the
  9 collapsed `operator T()`/`parseVector3()` bodies' old inline logic
  directly, only `StringParseHelper` itself). **Not yet build-verified.**
- ~~`Components/Backgroundable.cpp`: hardcoded string→scene-path maps + a full
  `Resources::FindObjectsOfTypeAll` scan + ~25 manual `set_X(get_X())` calls
  to "clone" a found template.~~ **Partially done**: the 3 separate
  `std::map<std::string, std::string>` globals (`backgrounds`/`objectNames`/
  `objectParentNames`), keyed by the same background name and easy to let
  drift out of sync with each other, are now one
  `std::map<std::string, BackgroundTemplateNames>` (a 3-field struct) —
  `FindTemplate`'s signature changed from `(StringW name, StringW
  backgroundName)` (re-deriving `objectName`/`parentName` via two more map
  lookups inside the function) to `(std::string_view spriteName,
  std::string_view objectName, std::string_view parentName)`, called once
  with the already-looked-up struct's fields. Also gave the maps/cache/alias
  internal linkage (moved into an anonymous namespace) — they were
  file-scope globals with external linkage before, serving no external
  caller. **Left alone, still real remaining work**: the full-scene
  `Resources::FindObjectsOfTypeAll<HMUI::ImageView*>()` linear scan and the
  ~25-line manual `set_X(get_X())` property clone in `ApplyBackground` — a
  "cached-typed-handle rewrite" of those is a bigger, riskier change
  (behavioral, not just structural) and wasn't attempted this pass. **Not
  yet build-verified.**
- ~~`Settings/SettingsMenu.cpp`: unchecked `reinterpret_cast<SettingsMenu*>`s
  — replace with `i2c::try_cast` per core idiom #4.~~ **Investigated, no
  change needed.** There are 5 of these, not 1, spread across
  `BSMLSettings.cpp` (×3), `SettingsMenuListViewController.cpp`, and
  `ModSettingsFlowCoordinator.cpp` — all downcasting an element of
  `BSMLSettings::settingsMenus` (`ListW<BSML::CustomCellInfo*>`) back to
  `SettingsMenu*`. Traced every write site: the list is only ever appended to
  by `BSMLSettings::TryAddSettingsMenu(SettingsMenu*)` (`menus->Add(menu)`/
  `menus->Insert(i, menu)`), so every element is provably a `SettingsMenu*`
  by construction — this is BSML's own internal invariant, not a
  broadcast/RTTI-matched external value. Exactly the same shape as the
  `GameplaySetup.cpp` `_menus` casts backlog #4 already decided to leave
  alone (see that section above) — doesn't fit idiom #4's own stated
  rationale ("whose exact type isn't locally, statically obvious"), so
  converting these to `try_cast` would just be defensive-cast churn with a
  small per-call RTTI cost and no real safety gain. Left as-is.

### 8. Sweep for `std::optional`/`std::expected`/`noexcept` opportunities (idiom #8)
Exploratory, do opportunistically alongside other passes rather than as one
big sweep. Candidates:
- Any function that currently returns a raw pointer where null means "not
  found" *and* the caller has no other way to distinguish that from "found
  but null" — `ExternalComponents::GetByType`/`Get<T>()` had exactly this
  shape; `TryGetByType`/`TryGet<T>()` are the template to follow elsewhere
  (`Components/TableView.cpp`, `GameplaySetup`'s registry lookups, etc. —
  audit as each is touched by its own backlog item, not all at once).
- `BSMLValue`/`BSMLAction`/`StringParseHelper`'s host-binding resolution
  (idiom #6): today a missing field/method/attribute is usually a silent
  `nullptr` or a logged error with no way for the caller to branch on *why*
  it failed. A `std::expected<T, BindError>`-shaped result (enum: not found /
  wrong type / host null) would let call sites make better decisions without
  touching the underlying reflection lookup itself.
- Plain-C++ helper functions with no IL2CPP calls in their body (string
  parsing, geometry math, the `ComponentCreation.hpp` helpers) are candidates
  for `noexcept` once each is individually confirmed not to throw.

### 9. Header/include hygiene sweep (idiom #10)
Exploratory and low-risk, do opportunistically — see idiom #10 for the
ground rules (forward-declare over include, don't widen already-widely-included
headers, no PCH/unity-build/header-splitting as part of this). Candidates,
roughly in decreasing order of payoff (most-included headers first):
- `shared/BSML-Lite/ComponentCreation.hpp`, `shared/BSML-Lite/TransformWrapper.hpp`,
  `shared/BSML-Lite/GameObjectWrapper.hpp`, `shared/concepts.hpp` — pulled into
  nearly every `BSML-Lite` consumer; worth an actual `grep`-based check of
  which included types are only ever used as `T*`/`T&` in the header itself
  (forward-declarable) versus genuinely needed there (e.g. anything used by
  value, anything a template body calls a member on).
- `shared/BSML-Lite/Creation/*.hpp` (`Buttons.hpp`, `Text.hpp`, and each
  future backlog #6 migration's `*Options` header) — same check, and worth
  doing as part of backlog #6's per-component passes rather than separately,
  since those headers are being rewritten anyway.
- No baseline has been measured yet (e.g. `time ninja -C build` on a clean
  build, or clang's `-ftime-trace`) — do that once before/after a first real
  attempt here so the payoff is a number, not a guess, before deciding
  whether to keep going.

## How to verify a pass

Dependencies restore from the local QPM cache without network access if
you've built before:
```
qpm restore
```
Full build (real NDK, ~1-3 min from clean):
```
qpm s build
```
or, once configured once, just re-run ninja directly for faster iteration:
```
ninja -C build
```
A green `ninja -C build` (ends with `Linking CXX shared library libbsml.so`)
is a real compile-and-link check, not a syntax approximation — this repo's
NDK toolchain is available in this environment (`qpm s build` auto-detects it),
so there's no need to fall back to host-clang syntax-only tricks.

Runtime verification (on-device) still needs a Quest headset + adb, which
isn't available in an agent sandbox — that step is on whoever's driving the
headset.
