using System;
using UnityEditor;
using UnityEngine;

// Run in a graphics-enabled Editor process. This tests the actual shader,
// point sampling, frame/palette addressing, GPU copy, alpha, and orientation.
public static class ValidateIndexedGif
{
    public static void RunBoth()
    {
        var previous = PlayerSettings.colorSpace;
        try
        {
            foreach (var colorSpace in new[] { ColorSpace.Gamma, ColorSpace.Linear })
            {
                PlayerSettings.colorSpace = colorSpace;
                if (QualitySettings.activeColorSpace != colorSpace)
                    throw new Exception($"Could not select {colorSpace} for validation");
                Run();
            }
        }
        finally { PlayerSettings.colorSpace = previous; }
    }

    public static void Run()
    {
        const int width = 17, height = 19, frames = 4, columns = 2;
        var shader = AssetDatabase.LoadAssetAtPath<Shader>("Assets/IndexedGifExpand.shader");
        if (shader == null || !shader.isSupported || ShaderUtil.ShaderHasError(shader))
            throw new Exception("Indexed shader is unavailable or failed compilation");
        var indices = new Texture2D(width * columns, height * 2, TextureFormat.R8, false, true);
        var palette = new Texture2D(256, frames, TextureFormat.RGBA32, false, false);
        var output = new Texture2D(width, height, TextureFormat.RGBA32, false, false);
        var readback = new Texture2D(width, height, TextureFormat.RGBA32, false, false);
        var target = new RenderTexture(width, height, 0, output.graphicsFormat);
        var verify = new RenderTexture(width, height, 0, output.graphicsFormat);
        var material = new Material(shader);
        var previous = RenderTexture.active;
        bool previousSrgb = GL.sRGBWrite;
        try
        {
            indices.filterMode = palette.filterMode = FilterMode.Point;
            indices.wrapMode = palette.wrapMode = TextureWrapMode.Clamp;
            var indexBytes = new byte[width * height * frames];
            var colors = new Color32[256 * frames];
            for (int f = 0; f < frames; ++f)
            {
                for (int i = 0; i < 256; ++i)
                    colors[f * 256 + i] = new Color32((byte)i, (byte)(255 - i),
                        (byte)(i * 37 + f * 53), (byte)(i % 3 == 0 ? 0 : 255));
                for (int y = 0; y < height; ++y)
                    for (int x = 0; x < width; ++x)
                        indexBytes[(f / columns * height + y) * width * columns + f % columns * width + x]
                            = (byte)(y * width + x + f * 29);
            }
            indices.LoadRawTextureData(indexBytes);
            indices.Apply(false, true);
            palette.SetPixels32(colors);
            palette.Apply(false, true);
            output.Apply(false, true);
            if (!target.Create() || !verify.Create()) throw new Exception("RenderTexture creation failed");
            material.SetTexture("_PaletteTex", palette);
            // Change order and repeat frames, including both atlas rows.
            foreach (int f in new[] { 0, 3, 1, 2, 0, 2 })
            {
                material.SetVector("_FrameRect", new Vector4(f % columns / 2f, f / columns / 2f, 0.5f, 0.5f));
                material.SetFloat("_PaletteRow", (f + 0.5f) / frames);
                GL.sRGBWrite = QualitySettings.activeColorSpace == ColorSpace.Linear;
                Graphics.Blit(indices, target, material, 0);
                Graphics.CopyTexture(target, 0, 0, output, 0, 0);
                Graphics.CopyTexture(output, 0, 0, verify, 0, 0);
                RenderTexture.active = verify;
                readback.ReadPixels(new Rect(0, 0, width, height), 0, 0, false);
                var actual = readback.GetPixels32();
                for (int p = 0; p < actual.Length; ++p)
                {
                    Color32 expected = colors[f * 256 + (byte)(p + f * 29)];
                    Color32 got = actual[p];
                    if (Math.Abs(got.r - expected.r) > 1 || Math.Abs(got.g - expected.g) > 1 ||
                        Math.Abs(got.b - expected.b) > 1 || got.a != expected.a)
                        throw new Exception($"Frame {f}, pixel {p}: {got} != {expected}");
                }
            }
            Debug.Log($"BSML indexed shader GPU validation passed ({SystemInfo.graphicsDeviceType}, {QualitySettings.activeColorSpace})");
        }
        finally
        {
            RenderTexture.active = previous;
            GL.sRGBWrite = previousSrgb;
            foreach (var obj in new UnityEngine.Object[] { material, indices, palette, output, readback, target, verify })
                UnityEngine.Object.DestroyImmediate(obj);
        }
    }
}
