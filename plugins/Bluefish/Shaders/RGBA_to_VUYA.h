#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 9.30.9200.16384
//
//
///
// Buffer Definitions: 
//
// cbuffer controls
// {
//
//   float InputWidth;                  // Offset:    0 Size:     4 [unused]
//      = 0x44f00000 
//   float InputHeight;                 // Offset:    4 Size:     4 [unused]
//      = 0x44870000 
//   float OutputWidth;                 // Offset:    8 Size:     4 [unused]
//      = 0x44f00000 
//   float OutputHeight;                // Offset:   12 Size:     4 [unused]
//      = 0x44870000 
//   float OnePixelX;                   // Offset:   16 Size:     4 [unused]
//      = 0x3a088889 
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// s0                                sampler      NA          NA    0        1
// tex0                              texture  float4          2d    0        1
// controls                          cbuffer      NA          NA    0        1
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Position              0   xyzw        0      POS   float       
// TEXCOORD                 0   xy          1     NONE   float   xy  
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Target                0   xyzw        0   TARGET   float   xyzw
//
ps_5_0
dcl_globalFlags refactoringAllowed
dcl_constantbuffer cb0[1], immediateIndexed
dcl_sampler s0, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_input_ps linear v1.xy
dcl_output o0.xyzw
dcl_temps 2
sample_l_indexable(texture2d)(float,float,float,float) r0.xyzw, v1.xyxx, t0.xyzw, s0, l(0.000000)
dp3 r1.x, r0.xyzx, l(0.256816, 0.504154, 0.097914, 0.000000)
add_sat o0.z, r1.x, l(0.062500)
dp3 r1.x, r0.xyzx, l(-0.148246, -0.291020, 0.439266, 0.000000)
add_sat o0.y, r1.x, l(0.500000)
dp3 r0.x, r0.xyzx, l(0.439271, -0.367833, -0.071438, 0.000000)
mov o0.w, r0.w
add_sat o0.x, r0.x, l(0.500000)
ret 
// Approximately 9 instruction slots used
#endif

const BYTE RGBA_to_VUYA[] =
{
     68,  88,  66,  67, 167, 217, 
    232,  44,  28, 169, 149,  29, 
      9, 156, 139,  22,  92, 144, 
    153,  29,   1,   0,   0,   0, 
     20,   5,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
    116,   2,   0,   0, 204,   2, 
      0,   0,   0,   3,   0,   0, 
    120,   4,   0,   0,  82,  68, 
     69,  70,  56,   2,   0,   0, 
      1,   0,   0,   0, 176,   0, 
      0,   0,   3,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0,   1,   0,   0, 
      4,   2,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
    156,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0, 159,   0,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     12,   0,   0,   0, 164,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   1,   0,   0,   0, 
    115,  48,   0, 116, 101, 120, 
     48,   0,  99, 111, 110, 116, 
    114, 111, 108, 115,   0, 171, 
    171, 171, 164,   0,   0,   0, 
      5,   0,   0,   0, 200,   0, 
      0,   0,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0, 144,   1,   0,   0, 
      0,   0,   0,   0,   4,   0, 
      0,   0,   0,   0,   0,   0, 
    164,   1,   0,   0, 200,   1, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    204,   1,   0,   0,   4,   0, 
      0,   0,   4,   0,   0,   0, 
      0,   0,   0,   0, 164,   1, 
      0,   0, 216,   1,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 220,   1, 
      0,   0,   8,   0,   0,   0, 
      4,   0,   0,   0,   0,   0, 
      0,   0, 164,   1,   0,   0, 
    200,   1,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 232,   1,   0,   0, 
     12,   0,   0,   0,   4,   0, 
      0,   0,   0,   0,   0,   0, 
    164,   1,   0,   0, 216,   1, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    245,   1,   0,   0,  16,   0, 
      0,   0,   4,   0,   0,   0, 
      0,   0,   0,   0, 164,   1, 
      0,   0,   0,   2,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,  73, 110, 
    112, 117, 116,  87, 105, 100, 
    116, 104,   0, 102, 108, 111, 
     97, 116,   0, 171, 171, 171, 
      0,   0,   3,   0,   1,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0, 155,   1,   0,   0, 
      0,   0, 240,  68,  73, 110, 
    112, 117, 116,  72, 101, 105, 
    103, 104, 116,   0,   0,   0, 
    135,  68,  79, 117, 116, 112, 
    117, 116,  87, 105, 100, 116, 
    104,   0,  79, 117, 116, 112, 
    117, 116,  72, 101, 105, 103, 
    104, 116,   0,  79, 110, 101, 
     80, 105, 120, 101, 108,  88, 
      0, 171, 137, 136,   8,  58, 
     77, 105,  99, 114, 111, 115, 
    111, 102, 116,  32,  40,  82, 
     41,  32,  72,  76,  83,  76, 
     32,  83, 104,  97, 100, 101, 
    114,  32,  67, 111, 109, 112, 
    105, 108, 101, 114,  32,  57, 
     46,  51,  48,  46,  57,  50, 
     48,  48,  46,  49,  54,  51, 
     56,  52,   0, 171,  73,  83, 
     71,  78,  80,   0,   0,   0, 
      2,   0,   0,   0,   8,   0, 
      0,   0,  56,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,   0, 
      0,   0,  68,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,   3,   3, 
      0,   0,  83,  86,  95,  80, 
    111, 115, 105, 116, 105, 111, 
    110,   0,  84,  69,  88,  67, 
     79,  79,  82,  68,   0, 171, 
    171, 171,  79,  83,  71,  78, 
     44,   0,   0,   0,   1,   0, 
      0,   0,   8,   0,   0,   0, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   0,   0,   0, 
     83,  86,  95,  84,  97, 114, 
    103, 101, 116,   0, 171, 171, 
     83,  72,  69,  88, 112,   1, 
      0,   0,  80,   0,   0,   0, 
     92,   0,   0,   0, 106,   8, 
      0,   1,  89,   0,   0,   4, 
     70, 142,  32,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     85,  85,   0,   0,  98,  16, 
      0,   3,  50,  16,  16,   0, 
      1,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0, 104,   0, 
      0,   2,   2,   0,   0,   0, 
     72,   0,   0, 141, 194,   0, 
      0, 128,  67,  85,  21,   0, 
    242,   0,  16,   0,   0,   0, 
      0,   0,  70,  16,  16,   0, 
      1,   0,   0,   0,  70, 126, 
     16,   0,   0,   0,   0,   0, 
      0,  96,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0,   0,  16,   0, 
      0,  10,  18,   0,  16,   0, 
      1,   0,   0,   0,  70,   2, 
     16,   0,   0,   0,   0,   0, 
      2,  64,   0,   0,  99, 125, 
    131,  62,  61,  16,   1,  63, 
    250, 134, 200,  61,   0,   0, 
      0,   0,   0,  32,   0,   7, 
     66,  32,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      1,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  61, 
     16,   0,   0,  10,  18,   0, 
     16,   0,   1,   0,   0,   0, 
     70,   2,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
    205, 205,  23, 190, 147,   0, 
    149, 190, 121, 231, 224,  62, 
      0,   0,   0,   0,   0,  32, 
      0,   7,  34,  32,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   1,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,  63,  16,   0,   0,  10, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  70,   2,  16,   0, 
      0,   0,   0,   0,   2,  64, 
      0,   0,  33, 232, 224,  62, 
    155,  84, 188, 190,  22,  78, 
    146, 189,   0,   0,   0,   0, 
     54,   0,   0,   5, 130,  32, 
     16,   0,   0,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,   0,  32,   0,   7, 
     18,  32,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,  63, 
     62,   0,   0,   1,  83,  84, 
     65,  84, 148,   0,   0,   0, 
      9,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,   6,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0
};
