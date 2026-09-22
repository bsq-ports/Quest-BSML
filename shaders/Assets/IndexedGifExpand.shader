Shader "Hidden/BSML/IndexedGifExpand"
{
    Properties
    {
        _MainTex ("Index atlas", 2D) = "black" {}
        _PaletteTex ("RGBA palettes", 2D) = "black" {}
        _FrameRect ("Frame UV rectangle", Vector) = (0, 0, 1, 1)
        _PaletteRow ("Palette row center", Float) = 0.5
    }
    SubShader
    {
        Cull Off ZWrite Off ZTest Always Blend Off
        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #pragma target 3.0
            #include "UnityCG.cginc"

            // Mobile low-precision samplers can round an R8 value far enough
            // to select an adjacent palette entry after multiplication by 255.
            sampler2D_float _MainTex;
            sampler2D_float _PaletteTex;
            float4 _FrameRect;
            float _PaletteRow;

            struct Interpolators { float4 vertex : SV_POSITION; float2 uv : TEXCOORD0; };
            Interpolators vert(appdata_img input)
            {
                Interpolators output;
                output.vertex = UnityObjectToClipPos(input.vertex);
                output.uv = input.texcoord;
                return output;
            }

            float4 frag(Interpolators input) : SV_Target
            {
                // Both textures are point sampled, without mipmaps. Decode at
                // native frame resolution; the game's UI filters the RGBA result.
                float2 uv = _FrameRect.xy + input.uv * _FrameRect.zw;
                float index = floor(tex2D(_MainTex, uv).r * 255.0 + 0.5);
                return tex2D(_PaletteTex, float2((index + 0.5) / 256.0, _PaletteRow));
            }
            ENDCG
        }
    }
    Fallback Off
}
