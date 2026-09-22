using System;
using System.IO;
using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering;

public static class BuildIndexedGif
{
    public static void Build()
    {
        PlayerSettings.SetScriptingBackend(UnityEditor.Build.NamedBuildTarget.Android, ScriptingImplementation.IL2CPP);
        PlayerSettings.Android.targetArchitectures = AndroidArchitecture.ARM64;
        PlayerSettings.SetUseDefaultGraphicsAPIs(BuildTarget.Android, false);
        PlayerSettings.SetGraphicsAPIs(BuildTarget.Android,
            new[] { GraphicsDeviceType.OpenGLES3, GraphicsDeviceType.Vulkan });
        string output = Path.GetFullPath(Path.Combine(Application.dataPath, "../../assets/shaders"));
        Directory.CreateDirectory(output);
        var manifest = BuildPipeline.BuildAssetBundles(output, new[] {
            new AssetBundleBuild {
                assetBundleName = "bsml-indexed-gif.bundle",
                assetNames = new[] { "Assets/IndexedGifExpand.shader" }
            }
        }, BuildAssetBundleOptions.ChunkBasedCompression |
           BuildAssetBundleOptions.StrictMode | BuildAssetBundleOptions.ForceRebuildAssetBundle,
           BuildTarget.Android);
        if (manifest == null) throw new Exception("Indexed GIF shader bundle build failed");
        // Catch missing module/build metadata before shipping a shader-only
        // SerializedFile that the player's AssetBundle loader cannot open.
        var bundle = AssetBundle.LoadFromFile(Path.Combine(output, "bsml-indexed-gif.bundle"));
        if (bundle == null) throw new Exception("Built shader bundle could not be opened");
        try
        {
            if (bundle.LoadAsset<Shader>("Assets/IndexedGifExpand.shader") == null)
                throw new Exception("Built bundle does not contain the expansion shader");
        }
        finally { bundle.Unload(true); }
    }
}
