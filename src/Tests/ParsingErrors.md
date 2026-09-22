# Parsing error checks

Open **Component Test → Anchors and alignment (PC parity)** and choose
**Run names, layout and error checks**. The same suite is `RunRectTransformTests()`.
Malformed samples are constructed separately and their expected exceptions are
caught; the test page itself contains valid markup.

The suite requires `BSML::ParseException` for empty/malformed RectTransform
vectors, scalar coordinates, scale, and active flags; invalid text and child
alignment; and null `~` references. Error messages must identify the
property/type or referenced value. Valid whitespace, scalar vector shorthand,
two-component scale (Z = 1), empty names, macro-defined values, and all existing
name/geometry checks must still pass. The `tryParseVector2/3` APIs return
`std::nullopt` on malformed input instead of throwing or replacing components.

Callers that handle construction errors can include
`BSML/Parsing/ParseException.hpp` and catch `BSML::ParseException`. Construction
does not roll back Unity objects already created under the caller's parent;
the test fixtures destroy their temporary roots on both success and failure.

Strict conversion applies to all numeric, boolean, color and vector conversions.
Padding and enum handlers (alignment, overflow, fitting, page/list directions and
list style) also throw for invalid values. Optional `tryParse*` APIs remain
nonthrowing. Numeric parsing uses invariant syntax and checks integer/float range.
Malformed XML and unknown tags throw instead of substituting content or silently
using the root tag. Quest's existing support for multiple top-level elements is
retained. These failures also exercise the controller fallback tests.

Null references now throw during parameter resolution, including for `name`;
PC instead passes null to its setter. Empty strings remain distinct from null.

Missing `~` bindings throw during parameter resolution, as on PC, for both
string and typed properties. The error identifies the attribute and missing
value name. This includes `~version` and an empty `~` reference; unresolved
references can no longer be used as literal placeholders. Valid field and
macro-defined bindings continue to resolve normally.

For unattended device verification, create
`/sdcard/ModData/com.beatgames.beatsaber/Mods/BSML/run-parsing-tests` before launching
a build containing `ComponentTestPage.cpp`. The runner waits for an active main
menu, runs padding checks, and runs the parsing/layout and fallback suites twice,
with two seconds between passes. Read `PARSING-VERIFY`, `RECT-TEST RESULT` and
`VIEW-TEST RESULT` in logcat. Remove the marker afterward to disable autorun.

`macro.as-host` resolves `host` as a value name without numeric conversion.
A missing value throws; an existing null value parses children with no host;
an omitted `host` attribute skips children. Tests also cover `host='~hostName'`,
a fresh nested scope, and preservation of the outer host for following siblings.
Macro attributes use the same strict reference resolver as component attributes.

For a Solo integration check after both passes, also create
`/sdcard/ModData/com.beatgames.beatsaber/Mods/BSML/run-solo-smoke-test` before
launching. This invokes the main-menu Solo button and logs survival after ten
seconds. Remove both markers when finished.
