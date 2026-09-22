# Color picker interaction tests

Open **Mod Settings → Color Interaction Test**. The scripted suite starts once
when the page is constructed. Leave controllers idle until the result appears.
Use **Run scripted tests again** to repeat without restarting.

## Recorded Quest run

### Latest override run: 110 passed, 2 failed

Quest 3 run after removing the nested picker registration and adding six runtime
centering opt-out checks: **110 passed, 2 failed**. Full log:
`build/color-interactions-override-layout-quest.log`.

The updated fixture binds `deferred` as a `ColorSetting*`, accesses
`deferred->modalColorPicker`, and verifies that the picker is not registered
externally. All 106 existing checks, including these updated binding checks,
passed.

The `move-to-center='false'` fixture is positioned at X=12/Y=-8 in its parent
and excluded from automatic layout with `LayoutElement.ignoreLayout=true`.
Measured positions relative to the viewport:

| Opening | Before Show X/Y | After Show X/Y |
| --- | --- | --- |
| First, at scroll top | 12 / -42.83499 | 12 / -5.0000043 |
| Second, at scroll bottom | 12 / 64.66998 | 12 / 64.66998 |

Failures: first opening does not preserve its pre-show Y position, and the
top-to-bottom displayed displacement therefore differs from the expected
displacement. Both dialogs open/close and restore their original parent;
the bottom opening preserves its position. The same discrepancy occurred before
excluding automatic layout (`build/color-interactions-override-quest.log`).
The cause of the first-open shift remains unresolved; no production fix or
relaxation of the assertions has been applied.

### Earlier centering run

- Device: Quest 3, 2026-09-15.
- In-game result after the centering fix and a fresh game restart:
  **106 passed, 0 failed**.
- Build and BSML XML validation passed.
- Full log: `build/color-interactions-scroll-quest.log`, including all 106
  passing-check lines and measured top/bottom modal positions.
- Earlier interaction run: 85 passed, 0 failed; excerpt in
  `build/color-interactions-quest.log`.
- The previous installed library is backed up at
  `build/test-backup/libbsml-before-interactions.so`.

| Cases | Passing checks |
| --- | ---: |
| Picker ID, external lookup, and isolated value binding | 3 |
| BSML immediate setting | 16 |
| BSML deferred setting | 17 |
| BSML standalone with change/done/cancel actions on a foreign receiver | 13 |
| BSML-Lite setting row | 18 |
| BSML-Lite standalone modal | 13 |
| BSML-Lite row/modal with omitted callbacks | 4 |
| Standalone click-off behavior | 1 |
| Centering default, top/bottom scrolling, stable position, parent restoration (five cases) | 20 |
| Explicit centering opt-out | 1 |
| Total | 106 |

The suite dispatches Unity button pointer-click handlers and RGB/HSV panel
change handlers. It checks synchronization, preview/save separation, callback
counts and ordering, black/gray hue movement, OK, Cancel, reopening, deferred
Apply, disabled buttons, reactivation, repeated Setup, optional callbacks, and
click-off. It does not synthesize a tracked VR controller or validate raycasting.

## Manual controller and visual pass (pending)

The manual pass exposed a scroll-dependent modal offset: color pickers inherited
the scrolled row position because `moveToCenter` defaulted to false. The shared
color-picker factory now defaults it to true, matching PC. An explicit
`move-to-center='false'` still overrides it.

The expanded suite adds top/bottom scroll position comparisons for all five
BSML/Lite cases, verifies parent restoration after closing, and checks the default
and explicit opt-out. All passed on-device. Every case had identical top/bottom
positions relative to the viewport.

Exercise both BSML rows, the BSML standalone, both Lite rows, and the Lite
standalone. The row with no callbacks should work without application callbacks.

1. Check initial swatches, modal position, clipping, and button hover highlights.
   Open the same picker at the top and bottom of the page; its position should
   stay fixed.
2. Drag each RGB slider and the HSV controls. Release both over and outside the
   control. Confirm all panels and the preview agree.
3. Test black, gray, saturated colors, and channel endpoints. Hue should stay
   movable when changing it does not visibly change the color.
4. Choose OK and reopen. Choose a different color, Cancel, and reopen again.
5. For the deferred BSML row, OK retains a draft; Apply saves it. Its pending
   draft remains selected when reopening before Apply.
6. Toggle Lite row interactability and check that a real controller click cannot
   open it while disabled. Enable it again and repeat.
7. Repeat after closing/reopening Mod Settings and after a game restart/reload.

The unbound Lite standalone intentionally retains its in-memory draft after
Cancel. It has no bound setting to reload; its caller owns persistence. Setting
rows restore the previous preview on Cancel. Click-off closes a standalone
without treating the dismissal as OK or an explicit Cancel callback, matching
current PC behavior.

Fresh game launches succeeded for both recorded runs. The manual controller and
visual pass after the centering fix remains pending.
