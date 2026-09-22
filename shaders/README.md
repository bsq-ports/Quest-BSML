# Indexed GIF expansion

`IndexedGifExpand.shader` expands an R8 index atlas and a 256-entry RGBA palette
per frame into one reusable RGBA image. It runs only when the GIF advances, once
per shared animation. Images keep Beat Saber's existing UI material, so tint,
curved canvases, stencil/rect clipping, stereo rendering and bilinear filtering
continue through the game's normal UI shader.

## Build

The checked-in Android bundle is embedded in `libbsml.so`; normal native builds
do not need Unity. Rebuild it after changing the shader:

```powershell
./scripts/build-shaders.ps1
cmake -S . -B build
cmake --build build --parallel 12
```

Use Unity **6000.0.40f1**, matching the current game, with Android build support
and an activated Editor license. Pass `-Unity <path>` for another installation.
The build includes OpenGL ES 3 and Vulkan. Shader source cannot be compiled by
the game's runtime, and a bundle for another platform is not interchangeable.
See Unity's [AssetBundle build documentation](https://docs.unity3d.com/6000.0/Documentation/Manual/AssetBundles-Building.html).

At runtime BSML writes the embedded bundle to its application cache and loads
the shader once using the game's available `AssetBundle.LoadFromFile` API.
No separate file installation or download is required.

## Data and ownership

- Index the fully composited RGBA frames from EasyGifReader. Local GIF palette
  changes, transparency and disposal are already resolved; all four channels
  participate in the palette key. No quantization is performed.
- Retain indexed frames while consuming the existing bounded decoding queue.
  A frame with more than 256 colors restores previously indexed frames and
  falls back to the existing RGBA atlas for the whole animation.
- Use an unscaled, tightly tiled R8 atlas, point filtering, no mipmaps and
  linear sampling for index bytes. Palette rows use RGBA32, point filtering,
  no mipmaps and ordinary sRGB color sampling.
- Expand at native frame resolution into a render target matching the output
  texture's graphics format. Copy it to a non-readable RGBA32 `Texture2D` for
  `Sprite` compatibility. Never call `Apply` after GPU copies. Preserve the
  active render target and `GL.sRGBWrite` around each expansion.
- The animation controller owns the displayed texture and schedules all GPU
  resource destruction on the main thread. Cancelled/stale loads dispose of
  unregistered expansion resources. Shared images use the same expanded frame.
- Small animations, atlases exceeding the device limit, unsupported R8/copy
  capabilities and unavailable shaders use the existing RGBA path. The public
  `AnimationLoader::Process` API still returns an ordinary RGBA atlas; the
  optimization is used internally by `SetImage`.

GPU-only copying follows Unity's [Graphics.CopyTexture requirements](https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Graphics.CopyTexture.html);
the render-target state follows its [Graphics.Blit guidance](https://docs.unity3d.com/6000.0/Documentation/ScriptReference/Graphics.Blit.html).

For 100 frames at 256 x 256, GPU pixel storage is about **6.85 MiB**: 6.25 MiB
indices, 0.10 MiB palettes and two 0.25 MiB RGBA surfaces. The original frames
occupy 25 MiB before atlas padding. This excludes driver overhead, sprites and
giflib's decoding buffers. No frame-time improvement is claimed: the expansion
draw and GPU copy must be measured on Quest, especially for high-frame-rate GIFs.

## Validation

```powershell
cmake -S tests/host -B build/host-tests
cmake --build build/host-tests
ctest --test-dir build/host-tests --output-on-failure
./scripts/test-indexed-shader.ps1
```

Host tests cover exact indexed round-trips, all 256 entries, alpha distinctions,
257-color rejection, random palettes and atlas limits. The graphics-enabled
Unity test renders the actual shader, copies into the displayed texture and
reads it back. It checks both color spaces, all palette entries, transparent
and opaque pixels, non-square frames, multiple atlas rows and repeated frame
changes. RGB tolerates one byte of sRGB conversion rounding; alpha is exact.
This desktop test does not replace Quest integration or GPU timing tests.

On Quest, use the existing GIF Tests page and a sufficiently large animation
with at least four frames. Compare colors, transparent edges and orientation
with the RGBA path; test shared images, scroll masks, curved canvases, both
eyes, pause/resume, replacement and destruction. Profile GPU time at both
display refresh and GIF frame boundaries. A >256-color *composited* frame
must display correctly through fallback, even if it occurs late in the GIF.
