//--------------------------------------------------------------------------------------
// dds.h
//
// This header defines constants and structures that are useful when parsing 
// DDS files.  DDS files were originally designed to use several structures
// and constants that are native to DirectDraw and are defined in ddraw.h,
// such as DDSURFACEDESC2 and DDSCAPS2.  This file defines similar 
// (compatible) constants and structures so that one can use DDS files 
// without needing to include ddraw.h.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
//--------------------------------------------------------------------------------------

#pragma once

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#else
#include <dxgiformat.h>
#endif

// VS 2010's stdint.h conflicts with intsafe.h
#pragma warning(push)
#pragma warning(disable : 4005)
#include <stdint.h>
#pragma warning(pop)

namespace DirectX
{

#pragma pack(push,1)

const uint32_t DDS_MAGIC = 0x20534444; // "DDS "

struct DDS_PIXELFORMAT
{
    uint32_t    dwSize;
    uint32_t    dwFlags;
    uint32_t    dwFourCC;
    uint32_t    dwRGBBitCount;
    uint32_t    dwRBitMask;
    uint32_t    dwGBitMask;
    uint32_t    dwBBitMask;
    uint32_t    dwABitMask;
};

#define DDS_FOURCC      0x00000004  // DDPF_FOURCC
#define DDS_RGB         0x00000040  // DDPF_RGB
#define DDS_RGBA        0x00000041  // DDPF_RGB | DDPF_ALPHAPIXELS
#define DDS_LUMINANCE   0x00020000  // DDPF_LUMINANCE
#define DDS_LUMINANCEA  0x00020001  // DDPF_LUMINANCE | DDPF_ALPHAPIXELS
#define DDS_ALPHA       0x00000002  // DDPF_ALPHA
#define DDS_PAL8        0x00000020  // DDPF_PALETTEINDEXED8

#ifndef MAKEFOURCC
    #define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |       \
                ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))
#endif /* defined(MAKEFOURCC) */

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_DXT1 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','1'), 0, 0, 0, 0, 0 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_DXT2 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','2'), 0, 0, 0, 0, 0 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_DXT3 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','3'), 0, 0, 0, 0, 0 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_DXT4 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','4'), 0, 0, 0, 0, 0 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_DXT5 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','5'), 0, 0, 0, 0, 0 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_BC4_UNORM =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('B','C','4','U'), 0, 0, 0, 0, 0 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_BC4_SNORM =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('B','C','4','S'), 0, 0, 0, 0, 0 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_BC5_UNORM =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('B','C','5','U'), 0, 0, 0, 0, 0 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_BC5_SNORM =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('B','C','5','S'), 0, 0, 0, 0, 0 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_R8G8_B8G8 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('R','G','B','G'), 0, 0, 0, 0, 0 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_G8R8_G8B8 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('G','R','G','B'), 0, 0, 0, 0, 0 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_YUY2 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('Y','U','Y','2'), 0, 0, 0, 0, 0 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_A8R8G8B8 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGBA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_X8R8G8B8 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGB,  0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_A8B8G8R8 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGBA, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_X8B8G8R8 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGB,  0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_G16R16 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGB,  0, 32, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_R5G6B5 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGB, 0, 16, 0x0000f800, 0x000007e0, 0x0000001f, 0x00000000 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_A1R5G5B5 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGBA, 0, 16, 0x00007c00, 0x000003e0, 0x0000001f, 0x00008000 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_A4R4G4B4 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGBA, 0, 16, 0x00000f00, 0x000000f0, 0x0000000f, 0x0000f000 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_R8G8B8 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGB, 0, 24, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_L8 =
    { sizeof(DDS_PIXELFORMAT), DDS_LUMINANCE, 0,  8, 0xff, 0x00, 0x00, 0x00 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_L16 =
    { sizeof(DDS_PIXELFORMAT), DDS_LUMINANCE, 0, 16, 0xffff, 0x0000, 0x0000, 0x0000 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_A8L8 =
    { sizeof(DDS_PIXELFORMAT), DDS_LUMINANCEA, 0, 16, 0x00ff, 0x0000, 0x0000, 0xff00 };

extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_A8 =
    { sizeof(DDS_PIXELFORMAT), DDS_ALPHA, 0, 8, 0x00, 0x00, 0x00, 0xff };

// D3DFMT_A2R10G10B10/D3DFMT_A2B10G10R10 should be written using DX10 extension to avoid D3DX 10:10:10:2 reversal issue

// This indicates the DDS_HEADER_DXT10 extension is present (the format is in dxgiFormat)
extern __declspec(selectany) const DDS_PIXELFORMAT DDSPF_DX10 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','1','0'), 0, 0, 0, 0, 0 };

#define DDS_HEADER_FLAGS_TEXTURE        0x00001007  // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT 
#define DDS_HEADER_FLAGS_MIPMAP         0x00020000  // DDSD_MIPMAPCOUNT
#define DDS_HEADER_FLAGS_VOLUME         0x00800000  // DDSD_DEPTH
#define DDS_HEADER_FLAGS_PITCH          0x00000008  // DDSD_PITCH
#define DDS_HEADER_FLAGS_LINEARSIZE     0x00080000  // DDSD_LINEARSIZE

#define DDS_HEIGHT 0x00000002 // DDSD_HEIGHT
#define DDS_WIDTH  0x00000004 // DDSD_WIDTH

#define DDS_SURFACE_FLAGS_TEXTURE 0x00001000 // DDSCAPS_TEXTURE
#define DDS_SURFACE_FLAGS_MIPMAP  0x00400008 // DDSCAPS_COMPLEX | DDSCAPS_MIPMAP
#define DDS_SURFACE_FLAGS_CUBEMAP 0x00000008 // DDSCAPS_COMPLEX

#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES ( DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
                               DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
                               DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ )

#define DDS_CUBEMAP 0x00000200 // DDSCAPS2_CUBEMAP

#define DDS_FLAGS_VOLUME 0x00200000 // DDSCAPS2_VOLUME

// Subset here matches D3D10_RESOURCE_DIMENSION and D3D11_RESOURCE_DIMENSION
enum DDS_RESOURCE_DIMENSION
{
    DDS_DIMENSION_TEXTURE1D	= 2,
    DDS_DIMENSION_TEXTURE2D	= 3,
    DDS_DIMENSION_TEXTURE3D	= 4,
};

// Subset here matches D3D10_RESOURCE_MISC_FLAG and D3D11_RESOURCE_MISC_FLAG
enum DDS_RESOURCE_MISC_FLAG
{
    DDS_RESOURCE_MISC_TEXTURECUBE = 0x4L,
};

enum DDS_MISC_FLAGS2
{
    DDS_MISC_FLAGS2_ALPHA_MODE_MASK = 0x7L,
};

enum DDS_ALPHA_MODE
{
    DDS_ALPHA_MODE_UNKNOWN       = 0,
    DDS_ALPHA_MODE_STRAIGHT      = 1,
    DDS_ALPHA_MODE_PREMULTIPLIED = 2,
    DDS_ALPHA_MODE_OPAQUE        = 3,
    DDS_ALPHA_MODE_CUSTOM        = 4,
};

struct DDS_HEADER
{
    uint32_t    dwSize;
    uint32_t    dwFlags;
    uint32_t    dwHeight;
    uint32_t    dwWidth;
    uint32_t    dwPitchOrLinearSize;
    uint32_t    dwDepth; // only if DDS_HEADER_FLAGS_VOLUME is set in dwFlags
    uint32_t    dwMipMapCount;
    uint32_t    dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32_t    dwCaps;
    uint32_t    dwCaps2;
    uint32_t    dwCaps3;
    uint32_t    dwCaps4;
    uint32_t    dwReserved2;
};

struct DDS_HEADER_DXT10
{
    DXGI_FORMAT dxgiFormat;
    uint32_t    resourceDimension;
    uint32_t    miscFlag; // see DDS_RESOURCE_MISC_FLAG
    uint32_t    arraySize;
    uint32_t    miscFlags2; // see DDS_MISC_FLAGS2
};

#pragma pack(pop)

static_assert( sizeof(DDS_HEADER) == 124, "DDS Header size mismatch" );
static_assert( sizeof(DDS_HEADER_DXT10) == 20, "DDS DX10 Extended Header size mismatch");



//-------------------------------------------------------------------------------------
// Legacy format mapping table (used for DDS files without 'DX10' extended header)
//-------------------------------------------------------------------------------------
enum CONVERSION_FLAGS
{
    CONV_FLAGS_NONE     = 0x0,
    CONV_FLAGS_EXPAND   = 0x1,      // Conversion requires expanded pixel size
    CONV_FLAGS_NOALPHA  = 0x2,      // Conversion requires setting alpha to known value
    CONV_FLAGS_SWIZZLE  = 0x4,      // BGR/RGB order swizzling required
    CONV_FLAGS_PAL8     = 0x8,      // Has an 8-bit palette
    CONV_FLAGS_888      = 0x10,     // Source is an 8:8:8 (24bpp) format
    CONV_FLAGS_565      = 0x20,     // Source is a 5:6:5 (16bpp) format
    CONV_FLAGS_5551     = 0x40,     // Source is a 5:5:5:1 (16bpp) format
    CONV_FLAGS_4444     = 0x80,     // Source is a 4:4:4:4 (16bpp) format
    CONV_FLAGS_44       = 0x100,    // Source is a 4:4 (8bpp) format
    CONV_FLAGS_332      = 0x200,    // Source is a 3:3:2 (8bpp) format
    CONV_FLAGS_8332     = 0x400,    // Source is a 8:3:3:2 (16bpp) format
    CONV_FLAGS_A8P8     = 0x800,    // Has an 8-bit palette with an alpha channel
    CONV_FLAGS_DX10     = 0x10000,  // Has the 'DX10' extension header
    CONV_FLAGS_PMALPHA  = 0x20000,  // Contains premultiplied alpha data
    CONV_FLAGS_L8       = 0x40000,  // Source is a 8 luminance format 
    CONV_FLAGS_L16      = 0x80000,  // Source is a 16 luminance format 
    CONV_FLAGS_A8L8     = 0x100000, // Source is a 8:8 luminance format 
};


enum DDS_FLAGS
{
	DDS_FLAGS_NONE                  = 0x0,

	DDS_FLAGS_LEGACY_DWORD          = 0x1,
		// Assume pitch is DWORD aligned instead of BYTE aligned (used by some legacy DDS files)

	DDS_FLAGS_NO_LEGACY_EXPANSION   = 0x2,
		// Do not implicitly convert legacy formats that result in larger pixel sizes (24 bpp, 3:3:2, A8L8, A4L4, P8, A8P8) 

	DDS_FLAGS_NO_R10B10G10A2_FIXUP  = 0x4,
		// Do not use work-around for long-standing D3DX DDS file format issue which reversed the 10:10:10:2 color order masks

	DDS_FLAGS_FORCE_RGB             = 0x8,
		// Convert DXGI 1.1 BGR formats to DXGI_FORMAT_R8G8B8A8_UNORM to avoid use of optional WDDM 1.1 formats

	DDS_FLAGS_NO_16BPP              = 0x10,
		// Conversions avoid use of 565, 5551, and 4444 formats and instead expand to 8888 to avoid use of optional WDDM 1.2 formats

	DDS_FLAGS_EXPAND_LUMINANCE      = 0x20,
		// When loading legacy luminance formats expand replicating the color channels rather than leaving them packed (L8, L16, A8L8)

	DDS_FLAGS_FORCE_DX10_EXT        = 0x10000,
		// Always use the 'DX10' header extension for DDS writer (i.e. don't try to write DX9 compatible DDS files)

	DDS_FLAGS_FORCE_DX10_EXT_MISC2  = 0x20000,
		// DDS_FLAGS_FORCE_DX10_EXT including miscFlags2 information (result may not be compatible with D3DX10 or D3DX11)
};

enum TEX_DIMENSION
	// Subset here matches D3D10_RESOURCE_DIMENSION and D3D11_RESOURCE_DIMENSION
{
	TEX_DIMENSION_TEXTURE1D    = 2,
	TEX_DIMENSION_TEXTURE2D    = 3,
	TEX_DIMENSION_TEXTURE3D    = 4,
};


enum TEX_ALPHA_MODE
	// Matches DDS_ALPHA_MODE, encoded in MISC_FLAGS2
{
	TEX_ALPHA_MODE_UNKNOWN       = 0,
	TEX_ALPHA_MODE_STRAIGHT      = 1,
	TEX_ALPHA_MODE_PREMULTIPLIED = 2,
	TEX_ALPHA_MODE_OPAQUE        = 3,
	TEX_ALPHA_MODE_CUSTOM        = 4,
};

struct TexMetadata
{
    size_t          width;
    size_t          height;     // Should be 1 for 1D textures
    size_t          depth;      // Should be 1 for 1D or 2D textures
    size_t          arraySize;  // For cubemap, this is a multiple of 6
    size_t          mipLevels;
    uint32_t        miscFlags;
    uint32_t        miscFlags2;
    DXGI_FORMAT     format;
    TEX_DIMENSION   dimension;
	size_t			bytesData;
	size_t			bytesHeader;
	size_t			rowPitch;

    size_t __cdecl ComputeIndex( _In_ size_t mip, _In_ size_t item, _In_ size_t slice ) const;
        // Returns size_t(-1) to indicate an out-of-range error

    bool __cdecl IsCubemap() const;
        // Helper for miscFlags

    bool __cdecl IsPMAlpha() const;
    void __cdecl SetAlphaMode( TEX_ALPHA_MODE mode );
        // Helpers for miscFlags2

    bool __cdecl IsVolumemap() const;
        // Helper for dimension
};

HRESULT GetMetadataFromDDSFile( const char * szFile, DWORD flags, TexMetadata& metadata );

}; // namespace
