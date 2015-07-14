#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 6.3.9600.16384
//
//
// Buffer Definitions: 
//
// cbuffer controls
// {
//
//   float InputWidth;                  // Offset:    0 Size:     4 [unused]
//      = 0x44f00000 
//   float InputHeight;                 // Offset:    4 Size:     4 [unused]
//      = 0x44870000 
//   float OutputWidth;                 // Offset:    8 Size:     4
//      = 0x44f00000 
//   float OutputHeight;                // Offset:   12 Size:     4
//      = 0x44870000 
//   float OnePixelX;                   // Offset:   16 Size:     4 [unused]
//      = 0x3a088889 
//   float YOrigin;                     // Offset:   20 Size:     4 [unused]
//      = 0x00000000 
//   float YCoordinateSign;             // Offset:   24 Size:     4 [unused]
//      = 0x3f800000 
//   bool RequiresSwap;                 // Offset:   28 Size:     4
//      = 0x00000000 
//   float RowPadding;                  // Offset:   32 Size:     4 [unused]
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim Slot Elements
// ------------------------------ ---------- ------- ----------- ---- --------
// tex0                              texture    uint          2d    0        1
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
dcl_constantbuffer cb0[2], immediateIndexed
dcl_resource_texture2d (uint,uint,uint,uint) t0
dcl_input_ps linear v1.xy
dcl_output o0.xyzw
dcl_temps 4
mul r0.xy, v1.xyxx, cb0[0].zwzz
ftoi r0.xy, r0.xyxx
mov r0.zw, l(0,0,0,0)
ld_indexable(texture2d)(uint,uint,uint,uint) r0.x, r0.xyzw, t0.xyzw
ubfe r0.yz, l(0, 10, 10, 0), l(0, 2, 12, 0), r0.xxxx
utof r0.yz, r0.yyzy
mul r1.xy, r0.yzyy, l(0.000978, 0.000978, 0.000000, 0.000000)
ushr r2.xyzw, r0.xxxx, l(8, 16, 24, 22)
bfi r0.yz, l(0, 8, 8, 0), l(0, 16, 8, 0), r2.xxyx, l(0, 0, 0, 0)
bfi r0.x, l(8), l(24), r0.x, r0.y
iadd r0.y, r0.x, r0.z
ushr r0.x, r0.x, l(22)
utof r0.x, r0.x
mul r3.x, r0.x, l(0.000978)
iadd r0.x, r0.y, r2.z
ubfe r0.xy, l(10, 10, 0, 0), l(2, 12, 0, 0), r0.xyxx
utof r0.xy, r0.xyxx
mul r3.yz, r0.yyxy, l(0.000000, 0.000978, 0.000978, 0.000000)
utof r0.y, r2.w
mul r1.z, r0.y, l(0.000978)
mov r3.w, l(1.000000)
mov r1.w, l(1.000000)
movc o0.xyzw, cb0[1].wwww, r3.xyzw, r1.xyzw
ret 
// Approximately 24 instruction slots used
#endif

const BYTE A2R10G10B10_to_R10G10B10A2[] =
{
     68,  88,  66,  67,  88, 166, 
    220, 153, 200,  11, 224,   1, 
     39, 202, 136, 184, 171, 183, 
      3, 151,   1,   0,   0,   0, 
    228,   7,   0,   0,   5,   0, 
      0,   0,  52,   0,   0,   0, 
     84,   3,   0,   0, 172,   3, 
      0,   0, 224,   3,   0,   0, 
     72,   7,   0,   0,  82,  68, 
     69,  70,  24,   3,   0,   0, 
      1,   0,   0,   0, 140,   0, 
      0,   0,   2,   0,   0,   0, 
     60,   0,   0,   0,   0,   5, 
    255, 255,   0,   1,   0,   0, 
    227,   2,   0,   0,  82,  68, 
     49,  49,  60,   0,   0,   0, 
     24,   0,   0,   0,  32,   0, 
      0,   0,  40,   0,   0,   0, 
     36,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
    124,   0,   0,   0,   2,   0, 
      0,   0,   4,   0,   0,   0, 
      4,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0, 129,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      1,   0,   0,   0, 116, 101, 
    120,  48,   0,  99, 111, 110, 
    116, 114, 111, 108, 115,   0, 
    171, 171, 129,   0,   0,   0, 
      9,   0,   0,   0, 164,   0, 
      0,   0,  48,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  12,   2,   0,   0, 
      0,   0,   0,   0,   4,   0, 
      0,   0,   0,   0,   0,   0, 
     32,   2,   0,   0,  68,   2, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
     72,   2,   0,   0,   4,   0, 
      0,   0,   4,   0,   0,   0, 
      0,   0,   0,   0,  32,   2, 
      0,   0,  84,   2,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,  88,   2, 
      0,   0,   8,   0,   0,   0, 
      4,   0,   0,   0,   2,   0, 
      0,   0,  32,   2,   0,   0, 
     68,   2,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 100,   2,   0,   0, 
     12,   0,   0,   0,   4,   0, 
      0,   0,   2,   0,   0,   0, 
     32,   2,   0,   0,  84,   2, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    113,   2,   0,   0,  16,   0, 
      0,   0,   4,   0,   0,   0, 
      0,   0,   0,   0,  32,   2, 
      0,   0, 124,   2,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 128,   2, 
      0,   0,  20,   0,   0,   0, 
      4,   0,   0,   0,   0,   0, 
      0,   0,  32,   2,   0,   0, 
    136,   2,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 140,   2,   0,   0, 
     24,   0,   0,   0,   4,   0, 
      0,   0,   0,   0,   0,   0, 
     32,   2,   0,   0, 156,   2, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    160,   2,   0,   0,  28,   0, 
      0,   0,   4,   0,   0,   0, 
      2,   0,   0,   0, 180,   2, 
      0,   0, 136,   2,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0, 216,   2, 
      0,   0,  32,   0,   0,   0, 
      4,   0,   0,   0,   0,   0, 
      0,   0,  32,   2,   0,   0, 
      0,   0,   0,   0, 255, 255, 
    255, 255,   0,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,  73, 110, 112, 117, 
    116,  87, 105, 100, 116, 104, 
      0, 102, 108, 111,  97, 116, 
      0, 171, 171, 171,   0,   0, 
      3,   0,   1,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     23,   2,   0,   0,   0,   0, 
    240,  68,  73, 110, 112, 117, 
    116,  72, 101, 105, 103, 104, 
    116,   0,   0,   0, 135,  68, 
     79, 117, 116, 112, 117, 116, 
     87, 105, 100, 116, 104,   0, 
     79, 117, 116, 112, 117, 116, 
     72, 101, 105, 103, 104, 116, 
      0,  79, 110, 101,  80, 105, 
    120, 101, 108,  88,   0, 171, 
    137, 136,   8,  58,  89,  79, 
    114, 105, 103, 105, 110,   0, 
      0,   0,   0,   0,  89,  67, 
    111, 111, 114, 100, 105, 110, 
     97, 116, 101,  83, 105, 103, 
    110,   0,   0,   0, 128,  63, 
     82, 101, 113, 117, 105, 114, 
    101, 115,  83, 119,  97, 112, 
      0,  98, 111, 111, 108,   0, 
    171, 171,   0,   0,   1,   0, 
      1,   0,   1,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0, 173,   2, 
      0,   0,  82, 111, 119,  80, 
     97, 100, 100, 105, 110, 103, 
      0,  77, 105,  99, 114, 111, 
    115, 111, 102, 116,  32,  40, 
     82,  41,  32,  72,  76,  83, 
     76,  32,  83, 104,  97, 100, 
    101, 114,  32,  67, 111, 109, 
    112, 105, 108, 101, 114,  32, 
     54,  46,  51,  46,  57,  54, 
     48,  48,  46,  49,  54,  51, 
     56,  52,   0, 171, 171, 171, 
     73,  83,  71,  78,  80,   0, 
      0,   0,   2,   0,   0,   0, 
      8,   0,   0,   0,  56,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  68,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
      3,   3,   0,   0,  83,  86, 
     95,  80, 111, 115, 105, 116, 
    105, 111, 110,   0,  84,  69, 
     88,  67,  79,  79,  82,  68, 
      0, 171, 171, 171,  79,  83, 
     71,  78,  44,   0,   0,   0, 
      1,   0,   0,   0,   8,   0, 
      0,   0,  32,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,   0, 
      0,   0,  83,  86,  95,  84, 
     97, 114, 103, 101, 116,   0, 
    171, 171,  83,  72,  69,  88, 
     96,   3,   0,   0,  80,   0, 
      0,   0, 216,   0,   0,   0, 
    106,   8,   0,   1,  89,   0, 
      0,   4,  70, 142,  32,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   0,   0, 
      0,   0,  68,  68,   0,   0, 
     98,  16,   0,   3,  50,  16, 
     16,   0,   1,   0,   0,   0, 
    101,   0,   0,   3, 242,  32, 
     16,   0,   0,   0,   0,   0, 
    104,   0,   0,   2,   4,   0, 
      0,   0,  56,   0,   0,   8, 
     50,   0,  16,   0,   0,   0, 
      0,   0,  70,  16,  16,   0, 
      1,   0,   0,   0, 230, 138, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  27,   0, 
      0,   5,  50,   0,  16,   0, 
      0,   0,   0,   0,  70,   0, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   8, 194,   0, 
     16,   0,   0,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  45,   0,   0, 137, 
    194,   0,   0, 128,   3,  17, 
     17,   0,  18,   0,  16,   0, 
      0,   0,   0,   0,  70,  14, 
     16,   0,   0,   0,   0,   0, 
     70, 126,  16,   0,   0,   0, 
      0,   0, 138,   0,   0,  15, 
     98,   0,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
      0,   0,   0,   0,  10,   0, 
      0,   0,  10,   0,   0,   0, 
      0,   0,   0,   0,   2,  64, 
      0,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,  12,   0, 
      0,   0,   0,   0,   0,   0, 
      6,   0,  16,   0,   0,   0, 
      0,   0,  86,   0,   0,   5, 
     98,   0,  16,   0,   0,   0, 
      0,   0,  86,   6,  16,   0, 
      0,   0,   0,   0,  56,   0, 
      0,  10,  50,   0,  16,   0, 
      1,   0,   0,   0, 150,   5, 
     16,   0,   0,   0,   0,   0, 
      2,  64,   0,   0,   8,  32, 
    128,  58,   8,  32, 128,  58, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  85,   0,   0,  10, 
    242,   0,  16,   0,   2,   0, 
      0,   0,   6,   0,  16,   0, 
      0,   0,   0,   0,   2,  64, 
      0,   0,   8,   0,   0,   0, 
     16,   0,   0,   0,  24,   0, 
      0,   0,  22,   0,   0,   0, 
    140,   0,   0,  20,  98,   0, 
     16,   0,   0,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,   8,   0,   0,   0, 
      8,   0,   0,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
      0,   0,   0,   0,  16,   0, 
      0,   0,   8,   0,   0,   0, 
      0,   0,   0,   0,   6,   1, 
     16,   0,   2,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0, 140,   0,   0,  11, 
     18,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      8,   0,   0,   0,   1,  64, 
      0,   0,  24,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,  26,   0,  16,   0, 
      0,   0,   0,   0,  30,   0, 
      0,   7,  34,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
     42,   0,  16,   0,   0,   0, 
      0,   0,  85,   0,   0,   7, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,  22,   0,   0,   0, 
     86,   0,   0,   5,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,  56,   0,   0,   7, 
     18,   0,  16,   0,   3,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   8,  32, 128,  58, 
     30,   0,   0,   7,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     26,   0,  16,   0,   0,   0, 
      0,   0,  42,   0,  16,   0, 
      2,   0,   0,   0, 138,   0, 
      0,  15,  50,   0,  16,   0, 
      0,   0,   0,   0,   2,  64, 
      0,   0,  10,   0,   0,   0, 
     10,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      2,  64,   0,   0,   2,   0, 
      0,   0,  12,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,  70,   0,  16,   0, 
      0,   0,   0,   0,  86,   0, 
      0,   5,  50,   0,  16,   0, 
      0,   0,   0,   0,  70,   0, 
     16,   0,   0,   0,   0,   0, 
     56,   0,   0,  10,  98,   0, 
     16,   0,   3,   0,   0,   0, 
     86,   4,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
      0,   0,   0,   0,   8,  32, 
    128,  58,   8,  32, 128,  58, 
      0,   0,   0,   0,  86,   0, 
      0,   5,  34,   0,  16,   0, 
      0,   0,   0,   0,  58,   0, 
     16,   0,   2,   0,   0,   0, 
     56,   0,   0,   7,  66,   0, 
     16,   0,   1,   0,   0,   0, 
     26,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      8,  32, 128,  58,  54,   0, 
      0,   5, 130,   0,  16,   0, 
      3,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     54,   0,   0,   5, 130,   0, 
     16,   0,   1,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  55,   0,   0,  10, 
    242,  32,  16,   0,   0,   0, 
      0,   0, 246, 143,  32,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,  70,  14,  16,   0, 
      3,   0,   0,   0,  70,  14, 
     16,   0,   1,   0,   0,   0, 
     62,   0,   0,   1,  83,  84, 
     65,  84, 148,   0,   0,   0, 
     24,   0,   0,   0,   4,   0, 
      0,   0,   0,   0,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   2,   0,   0,   0, 
      2,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   1,   0,   0,   0, 
      5,   0,   0,   0,   0,   0, 
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