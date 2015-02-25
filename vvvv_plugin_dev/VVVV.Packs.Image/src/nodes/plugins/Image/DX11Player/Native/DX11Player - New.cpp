// Native.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "DX11Player.h"
#include "tinydir.h"
#include <comdef.h>
#include <sstream>
#include <iostream>
#include <math.h>
#include "Offset.h"
#include "Timer.h"
#include <memory>
#include "DDS.h"
#include <assert.h>

using namespace DirectX;

static const int HEADER_SIZE_BYTES = 128;
//static const int TEXTURE_WIDTH = 3840;
//static const int TEXTURE_HEIGHT = 2160;
//static const int TEXTURE_SIZE_BYTES = TEXTURE_WIDTH * TEXTURE_HEIGHT * 4;
//static const int BUFFER_SIZE_BYTES = TEXTURE_SIZE_BYTES + HEADER_SIZE_BYTES;
static const int RING_BUFFER_SIZE = 8;
static const int OUTPUT_BUFFER_SIZE = 4;


enum TEX_DIMENSION
    // Subset here matches D3D10_RESOURCE_DIMENSION and D3D11_RESOURCE_DIMENSION
{
    TEX_DIMENSION_TEXTURE1D    = 2,
    TEX_DIMENSION_TEXTURE2D    = 3,
    TEX_DIMENSION_TEXTURE3D    = 4,
};

enum TEX_MISC_FLAG
    // Subset here matches D3D10_RESOURCE_MISC_FLAG and D3D11_RESOURCE_MISC_FLAG
{
    TEX_MISC_TEXTURECUBE = 0x4L,
};

enum TEX_MISC_FLAG2
{
    TEX_MISC2_ALPHA_MODE_MASK = 0x7L,
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

    size_t __cdecl ComputeIndex( _In_ size_t mip, _In_ size_t item, _In_ size_t slice ) const;
        // Returns size_t(-1) to indicate an out-of-range error

    bool __cdecl IsCubemap() const { return (miscFlags & TEX_MISC_TEXTURECUBE) != 0; }
        // Helper for miscFlags

    bool __cdecl IsPMAlpha() const { return ((miscFlags2 & TEX_MISC2_ALPHA_MODE_MASK) == TEX_ALPHA_MODE_PREMULTIPLIED) != 0; }
    void __cdecl SetAlphaMode( TEX_ALPHA_MODE mode ) { miscFlags2 = (miscFlags2 & ~TEX_MISC2_ALPHA_MODE_MASK) | static_cast<uint32_t>(mode); }
        // Helpers for miscFlags2

    bool __cdecl IsVolumemap() const { return (dimension == TEX_DIMENSION_TEXTURE3D); }
        // Helper for dimension
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

struct handle_closer { void operator()(HANDLE h) { assert(h != INVALID_HANDLE_VALUE); if (h) CloseHandle(h); } };

typedef public std::unique_ptr<void, handle_closer> ScopedHandle;

inline HANDLE safe_handle( HANDLE h ) { return (h == INVALID_HANDLE_VALUE) ? 0 : h; }


struct LegacyDDS
{
    DXGI_FORMAT     format;
    DWORD           convFlags;
    DDS_PIXELFORMAT ddpf;
};

const LegacyDDS g_LegacyDDSMap[] = 
{
    { DXGI_FORMAT_BC1_UNORM,          CONV_FLAGS_NONE,        DDSPF_DXT1 }, // D3DFMT_DXT1
    { DXGI_FORMAT_BC2_UNORM,          CONV_FLAGS_NONE,        DDSPF_DXT3 }, // D3DFMT_DXT3
    { DXGI_FORMAT_BC3_UNORM,          CONV_FLAGS_NONE,        DDSPF_DXT5 }, // D3DFMT_DXT5

    { DXGI_FORMAT_BC2_UNORM,          CONV_FLAGS_PMALPHA,     DDSPF_DXT2 }, // D3DFMT_DXT2
    { DXGI_FORMAT_BC3_UNORM,          CONV_FLAGS_PMALPHA,     DDSPF_DXT4 }, // D3DFMT_DXT4

    { DXGI_FORMAT_BC4_UNORM,          CONV_FLAGS_NONE,        DDSPF_BC4_UNORM },
    { DXGI_FORMAT_BC4_SNORM,          CONV_FLAGS_NONE,        DDSPF_BC4_SNORM },
    { DXGI_FORMAT_BC5_UNORM,          CONV_FLAGS_NONE,        DDSPF_BC5_UNORM },
    { DXGI_FORMAT_BC5_SNORM,          CONV_FLAGS_NONE,        DDSPF_BC5_SNORM },

    { DXGI_FORMAT_BC4_UNORM,          CONV_FLAGS_NONE,        { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC( 'A', 'T', 'I', '1' ), 0, 0, 0, 0, 0 } },
    { DXGI_FORMAT_BC5_UNORM,          CONV_FLAGS_NONE,        { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC( 'A', 'T', 'I', '2' ), 0, 0, 0, 0, 0 } },

    { DXGI_FORMAT_R8G8_B8G8_UNORM,    CONV_FLAGS_NONE,        DDSPF_R8G8_B8G8 }, // D3DFMT_R8G8_B8G8
    { DXGI_FORMAT_G8R8_G8B8_UNORM,    CONV_FLAGS_NONE,        DDSPF_G8R8_G8B8 }, // D3DFMT_G8R8_G8B8

    { DXGI_FORMAT_B8G8R8A8_UNORM,     CONV_FLAGS_NONE,        DDSPF_A8R8G8B8 }, // D3DFMT_A8R8G8B8 (uses DXGI 1.1 format)
    { DXGI_FORMAT_B8G8R8X8_UNORM,     CONV_FLAGS_NONE,        DDSPF_X8R8G8B8 }, // D3DFMT_X8R8G8B8 (uses DXGI 1.1 format)
    { DXGI_FORMAT_R8G8B8A8_UNORM,     CONV_FLAGS_NONE,        DDSPF_A8B8G8R8 }, // D3DFMT_A8B8G8R8
    { DXGI_FORMAT_R8G8B8A8_UNORM,     CONV_FLAGS_NOALPHA,     DDSPF_X8B8G8R8 }, // D3DFMT_X8B8G8R8
    { DXGI_FORMAT_R16G16_UNORM,       CONV_FLAGS_NONE,        DDSPF_G16R16   }, // D3DFMT_G16R16

    { DXGI_FORMAT_R10G10B10A2_UNORM,  CONV_FLAGS_SWIZZLE,     { sizeof(DDS_PIXELFORMAT), DDS_RGB,       0, 32, 0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000 } }, // D3DFMT_A2R10G10B10 (D3DX reversal issue workaround)
    { DXGI_FORMAT_R10G10B10A2_UNORM,  CONV_FLAGS_NONE,        { sizeof(DDS_PIXELFORMAT), DDS_RGB,       0, 32, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000 } }, // D3DFMT_A2B10G10R10 (D3DX reversal issue workaround)

    { DXGI_FORMAT_R8G8B8A8_UNORM,     CONV_FLAGS_EXPAND
                                      | CONV_FLAGS_NOALPHA
                                      | CONV_FLAGS_888,       DDSPF_R8G8B8 }, // D3DFMT_R8G8B8

    { DXGI_FORMAT_B5G6R5_UNORM,       CONV_FLAGS_565,         DDSPF_R5G6B5 }, // D3DFMT_R5G6B5
    { DXGI_FORMAT_B5G5R5A1_UNORM,     CONV_FLAGS_5551,        DDSPF_A1R5G5B5 }, // D3DFMT_A1R5G5B5
    { DXGI_FORMAT_B5G5R5A1_UNORM,     CONV_FLAGS_5551
                                      | CONV_FLAGS_NOALPHA,   { sizeof(DDS_PIXELFORMAT), DDS_RGB,       0, 16, 0x7c00,     0x03e0,     0x001f,     0x0000     } }, // D3DFMT_X1R5G5B5
     
    { DXGI_FORMAT_R8G8B8A8_UNORM,     CONV_FLAGS_EXPAND
                                      | CONV_FLAGS_8332,      { sizeof(DDS_PIXELFORMAT), DDS_RGB,       0, 16, 0x00e0,     0x001c,     0x0003,     0xff00     } }, // D3DFMT_A8R3G3B2
    { DXGI_FORMAT_B5G6R5_UNORM,       CONV_FLAGS_EXPAND
                                      | CONV_FLAGS_332,       { sizeof(DDS_PIXELFORMAT), DDS_RGB,       0,  8, 0xe0,       0x1c,       0x03,       0x00       } }, // D3DFMT_R3G3B2
  
    { DXGI_FORMAT_R8_UNORM,           CONV_FLAGS_NONE,        DDSPF_L8   }, // D3DFMT_L8
    { DXGI_FORMAT_R16_UNORM,          CONV_FLAGS_NONE,        DDSPF_L16  }, // D3DFMT_L16
    { DXGI_FORMAT_R8G8_UNORM,         CONV_FLAGS_NONE,        DDSPF_A8L8 }, // D3DFMT_A8L8

    { DXGI_FORMAT_A8_UNORM,           CONV_FLAGS_NONE,        DDSPF_A8   }, // D3DFMT_A8

    { DXGI_FORMAT_R16G16B16A16_UNORM, CONV_FLAGS_NONE,        { sizeof(DDS_PIXELFORMAT), DDS_FOURCC,   36,  0, 0,          0,          0,          0          } }, // D3DFMT_A16B16G16R16
    { DXGI_FORMAT_R16G16B16A16_SNORM, CONV_FLAGS_NONE,        { sizeof(DDS_PIXELFORMAT), DDS_FOURCC,  110,  0, 0,          0,          0,          0          } }, // D3DFMT_Q16W16V16U16
    { DXGI_FORMAT_R16_FLOAT,          CONV_FLAGS_NONE,        { sizeof(DDS_PIXELFORMAT), DDS_FOURCC,  111,  0, 0,          0,          0,          0          } }, // D3DFMT_R16F
    { DXGI_FORMAT_R16G16_FLOAT,       CONV_FLAGS_NONE,        { sizeof(DDS_PIXELFORMAT), DDS_FOURCC,  112,  0, 0,          0,          0,          0          } }, // D3DFMT_G16R16F
    { DXGI_FORMAT_R16G16B16A16_FLOAT, CONV_FLAGS_NONE,        { sizeof(DDS_PIXELFORMAT), DDS_FOURCC,  113,  0, 0,          0,          0,          0          } }, // D3DFMT_A16B16G16R16F
    { DXGI_FORMAT_R32_FLOAT,          CONV_FLAGS_NONE,        { sizeof(DDS_PIXELFORMAT), DDS_FOURCC,  114,  0, 0,          0,          0,          0          } }, // D3DFMT_R32F
    { DXGI_FORMAT_R32G32_FLOAT,       CONV_FLAGS_NONE,        { sizeof(DDS_PIXELFORMAT), DDS_FOURCC,  115,  0, 0,          0,          0,          0          } }, // D3DFMT_G32R32F
    { DXGI_FORMAT_R32G32B32A32_FLOAT, CONV_FLAGS_NONE,        { sizeof(DDS_PIXELFORMAT), DDS_FOURCC,  116,  0, 0,          0,          0,          0          } }, // D3DFMT_A32B32G32R32F

    { DXGI_FORMAT_R32_FLOAT,          CONV_FLAGS_NONE,        { sizeof(DDS_PIXELFORMAT), DDS_RGB,       0, 32, 0xffffffff, 0x00000000, 0x00000000, 0x00000000 } }, // D3DFMT_R32F (D3DX uses FourCC 114 instead)

    { DXGI_FORMAT_R8G8B8A8_UNORM,     CONV_FLAGS_EXPAND
                                      | CONV_FLAGS_PAL8
                                      | CONV_FLAGS_A8P8,      { sizeof(DDS_PIXELFORMAT), DDS_PAL8,      0, 16, 0,          0,          0,          0         } }, // D3DFMT_A8P8
    { DXGI_FORMAT_R8G8B8A8_UNORM,     CONV_FLAGS_EXPAND
                                      | CONV_FLAGS_PAL8,      { sizeof(DDS_PIXELFORMAT), DDS_PAL8,      0,  8, 0,          0,          0,          0         } }, // D3DFMT_P8

    { DXGI_FORMAT_B4G4R4A4_UNORM,     CONV_FLAGS_4444,        DDSPF_A4R4G4B4 }, // D3DFMT_A4R4G4B4 (uses DXGI 1.2 format)
    { DXGI_FORMAT_B4G4R4A4_UNORM,     CONV_FLAGS_NOALPHA
                                      | CONV_FLAGS_4444,      { sizeof(DDS_PIXELFORMAT), DDS_RGB,       0, 16, 0x0f00,     0x00f0,     0x000f,     0x0000     } }, // D3DFMT_X4R4G4B4 (uses DXGI 1.2 format)
    { DXGI_FORMAT_B4G4R4A4_UNORM,     CONV_FLAGS_EXPAND
                                      | CONV_FLAGS_44,        { sizeof(DDS_PIXELFORMAT), DDS_LUMINANCE, 0,  8, 0x0f,       0x00,       0x00,       0xf0       } }, // D3DFMT_A4L4 (uses DXGI 1.2 format)

    { DXGI_FORMAT_YUY2,               CONV_FLAGS_NONE,        DDSPF_YUY2 }, // D3DFMT_YUY2 (uses DXGI 1.2 format)
    { DXGI_FORMAT_YUY2,               CONV_FLAGS_SWIZZLE,     { sizeof(DDS_PIXELFORMAT), DDS_FOURCC,    MAKEFOURCC('U','Y','V','Y'), 0, 0, 0, 0, 0            } }, // D3DFMT_UYVY (uses DXGI 1.2 format)
};


// Note that many common DDS reader/writers (including D3DX) swap the
// the RED/BLUE masks for 10:10:10:2 formats. We assumme
// below that the 'backwards' header mask is being used since it is most
// likely written by D3DX. The more robust solution is to use the 'DX10'
// header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

// We do not support the following legacy Direct3D 9 formats:
//      BumpDuDv D3DFMT_V8U8, D3DFMT_Q8W8V8U8, D3DFMT_V16U16, D3DFMT_A2W10V10U10
//      BumpLuminance D3DFMT_L6V5U5, D3DFMT_X8L8V8U8
//      FourCC 117 D3DFMT_CxV8U8
//      ZBuffer D3DFMT_D16_LOCKABLE
//      FourCC 82 D3DFMT_D32F_LOCKABLE

static DXGI_FORMAT _GetDXGIFormat( const DDS_PIXELFORMAT& ddpf, DWORD flags, _Inout_ DWORD& convFlags )
{
    const size_t MAP_SIZE = sizeof(g_LegacyDDSMap) / sizeof(LegacyDDS);
    size_t index = 0;
    for( index = 0; index < MAP_SIZE; ++index )
    {
        const LegacyDDS* entry = &g_LegacyDDSMap[index];

        if ( ddpf.dwFlags & entry->ddpf.dwFlags )
        {
            if ( entry->ddpf.dwFlags & DDS_FOURCC )
            {
                if ( ddpf.dwFourCC == entry->ddpf.dwFourCC )
                    break;
            }
            else if ( entry->ddpf.dwFlags & DDS_PAL8 )
            {
                if (  ddpf.dwRGBBitCount == entry->ddpf.dwRGBBitCount )
                    break;
            }
            else if ( ddpf.dwRGBBitCount == entry->ddpf.dwRGBBitCount )
            {
                // RGB, RGBA, ALPHA, LUMINANCE
                if ( ddpf.dwRBitMask == entry->ddpf.dwRBitMask
                     && ddpf.dwGBitMask == entry->ddpf.dwGBitMask
                     && ddpf.dwBBitMask == entry->ddpf.dwBBitMask
                     && ddpf.dwABitMask == entry->ddpf.dwABitMask )
                    break;
            }
        }
    }

    if ( index >= MAP_SIZE )
        return DXGI_FORMAT_UNKNOWN;

    DWORD cflags = g_LegacyDDSMap[index].convFlags;
    DXGI_FORMAT format = g_LegacyDDSMap[index].format;

    if ( (cflags & CONV_FLAGS_EXPAND) && (flags & DDS_FLAGS_NO_LEGACY_EXPANSION) )
        return DXGI_FORMAT_UNKNOWN;

    if ( (format == DXGI_FORMAT_R10G10B10A2_UNORM) && (flags & DDS_FLAGS_NO_R10B10G10A2_FIXUP) )
    {
        cflags ^= CONV_FLAGS_SWIZZLE;
    }

    convFlags = cflags;

    return format;
}

_Use_decl_annotations_
inline bool __cdecl IsValid( DXGI_FORMAT fmt )
{
    return ( static_cast<size_t>(fmt) >= 1 && static_cast<size_t>(fmt) <= 120 );
}


_Use_decl_annotations_
inline bool __cdecl IsPalettized(DXGI_FORMAT fmt)
{
    switch( fmt )
    {
    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
    case DXGI_FORMAT_A8P8:
        return true;

    default:
        return false;
    }
}

static HRESULT _DecodeDDSHeader( _In_reads_bytes_(size) LPCVOID pSource, size_t size, DWORD flags, _Out_ TexMetadata& metadata,
                                 _Inout_ DWORD& convFlags )
{
    if ( !pSource )
        return E_INVALIDARG;

    memset( &metadata, 0, sizeof(TexMetadata) );

    if ( size < (sizeof(DDS_HEADER) + sizeof(uint32_t)) )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
    }

    // DDS files always start with the same magic number ("DDS ")
    uint32_t dwMagicNumber = *reinterpret_cast<const uint32_t*>(pSource);
    if ( dwMagicNumber != DDS_MAGIC )
    {
        return E_FAIL;
    }

    auto pHeader = reinterpret_cast<const DDS_HEADER*>( (const uint8_t*)pSource + sizeof( uint32_t ) );

    // Verify header to validate DDS file
    if ( pHeader->dwSize != sizeof(DDS_HEADER)
         || pHeader->ddspf.dwSize != sizeof(DDS_PIXELFORMAT) )
    {
        return E_FAIL;
    }

    metadata.mipLevels = pHeader->dwMipMapCount;
    if ( metadata.mipLevels == 0 )
        metadata.mipLevels = 1;

    // Check for DX10 extension
    if ( (pHeader->ddspf.dwFlags & DDS_FOURCC)
         && (MAKEFOURCC( 'D', 'X', '1', '0' ) == pHeader->ddspf.dwFourCC) )
    {
        // Buffer must be big enough for both headers and magic value
        if ( size < ( sizeof(DDS_HEADER) + sizeof(uint32_t) + sizeof(DDS_HEADER_DXT10) ) )
        {
            return E_FAIL;
        }

        auto d3d10ext = reinterpret_cast<const DDS_HEADER_DXT10*>( (const uint8_t*)pSource + sizeof( uint32_t ) + sizeof(DDS_HEADER) );
        convFlags |= CONV_FLAGS_DX10;

        metadata.arraySize = d3d10ext->arraySize;
        if ( metadata.arraySize == 0 )
        {
            return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
        }

        metadata.format = d3d10ext->dxgiFormat;
        if ( !IsValid( metadata.format ) || IsPalettized( metadata.format ) )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
        }

        static_assert( TEX_MISC_TEXTURECUBE == DDS_RESOURCE_MISC_TEXTURECUBE, "DDS header mismatch");

        metadata.miscFlags = d3d10ext->miscFlag & ~TEX_MISC_TEXTURECUBE;

        switch ( d3d10ext->resourceDimension )
        {
        case DDS_DIMENSION_TEXTURE1D:

            // D3DX writes 1D textures with a fixed Height of 1
            if ( (pHeader->dwFlags & DDS_HEIGHT) && pHeader->dwHeight != 1 )
            {
                return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            }

            metadata.width = pHeader->dwWidth;
            metadata.height = 1;
            metadata.depth = 1;
            metadata.dimension = TEX_DIMENSION_TEXTURE1D;
            break;

        case DDS_DIMENSION_TEXTURE2D:
            if ( d3d10ext->miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE )
            {
                metadata.miscFlags |= TEX_MISC_TEXTURECUBE;
                metadata.arraySize *= 6;
            }

            metadata.width = pHeader->dwWidth;
            metadata.height = pHeader->dwHeight;
            metadata.depth = 1;
            metadata.dimension = TEX_DIMENSION_TEXTURE2D;
            break;

        case DDS_DIMENSION_TEXTURE3D:
            if ( !(pHeader->dwFlags & DDS_HEADER_FLAGS_VOLUME) )
            {
                return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            }

            if ( metadata.arraySize > 1 )
                return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );

            metadata.width = pHeader->dwWidth;
            metadata.height = pHeader->dwHeight;
            metadata.depth = pHeader->dwDepth;
            metadata.dimension = TEX_DIMENSION_TEXTURE3D;
            break;

        default:
            return HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
        }

        static_assert( TEX_MISC2_ALPHA_MODE_MASK == DDS_MISC_FLAGS2_ALPHA_MODE_MASK, "DDS header mismatch");

        static_assert( TEX_ALPHA_MODE_UNKNOWN == DDS_ALPHA_MODE_UNKNOWN, "DDS header mismatch");
        static_assert( TEX_ALPHA_MODE_STRAIGHT == DDS_ALPHA_MODE_STRAIGHT, "DDS header mismatch");
        static_assert( TEX_ALPHA_MODE_PREMULTIPLIED == DDS_ALPHA_MODE_PREMULTIPLIED, "DDS header mismatch");
        static_assert( TEX_ALPHA_MODE_OPAQUE == DDS_ALPHA_MODE_OPAQUE, "DDS header mismatch");
        static_assert( TEX_ALPHA_MODE_CUSTOM == DDS_ALPHA_MODE_CUSTOM, "DDS header mismatch");

        metadata.miscFlags2 = d3d10ext->miscFlags2;
    }
    else
    {
        metadata.arraySize = 1;

        if ( pHeader->dwFlags & DDS_HEADER_FLAGS_VOLUME )
        {
            metadata.width = pHeader->dwWidth;
            metadata.height = pHeader->dwHeight;
            metadata.depth = pHeader->dwDepth;
            metadata.dimension = TEX_DIMENSION_TEXTURE3D;
        }
        else 
        {
            if ( pHeader->dwCaps2 & DDS_CUBEMAP )
            {
               // We require all six faces to be defined
               if ( (pHeader->dwCaps2 & DDS_CUBEMAP_ALLFACES ) != DDS_CUBEMAP_ALLFACES )
                   return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );

                metadata.arraySize = 6;
                metadata.miscFlags |= TEX_MISC_TEXTURECUBE;
            }

            metadata.width = pHeader->dwWidth;
            metadata.height = pHeader->dwHeight;
            metadata.depth = 1;
            metadata.dimension = TEX_DIMENSION_TEXTURE2D;

            // Note there's no way for a legacy Direct3D 9 DDS to express a '1D' texture
        }

        metadata.format = _GetDXGIFormat( pHeader->ddspf, flags, convFlags );

        if ( metadata.format == DXGI_FORMAT_UNKNOWN )
            return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );

        if ( convFlags & CONV_FLAGS_PMALPHA )
            metadata.miscFlags2 |= TEX_ALPHA_MODE_PREMULTIPLIED;

        // Special flag for handling LUMINANCE legacy formats
        if ( flags & DDS_FLAGS_EXPAND_LUMINANCE )
        {
            switch ( metadata.format )
            {
            case DXGI_FORMAT_R8_UNORM:
                metadata.format = DXGI_FORMAT_R8G8B8A8_UNORM;
                convFlags |= CONV_FLAGS_L8 | CONV_FLAGS_EXPAND;
                break;

            case DXGI_FORMAT_R8G8_UNORM:
                metadata.format = DXGI_FORMAT_R8G8B8A8_UNORM;
                convFlags |= CONV_FLAGS_A8L8 | CONV_FLAGS_EXPAND;
                break;

            case DXGI_FORMAT_R16_UNORM:
                metadata.format = DXGI_FORMAT_R16G16B16A16_UNORM;
                convFlags |= CONV_FLAGS_L16 | CONV_FLAGS_EXPAND;
                break;
            }
        }
    }

    // Special flag for handling BGR DXGI 1.1 formats
    if (flags & DDS_FLAGS_FORCE_RGB)
    {
        switch ( metadata.format )
        {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            metadata.format = DXGI_FORMAT_R8G8B8A8_UNORM;
            convFlags |= CONV_FLAGS_SWIZZLE;
            break;

        case DXGI_FORMAT_B8G8R8X8_UNORM:
            metadata.format = DXGI_FORMAT_R8G8B8A8_UNORM;
            convFlags |= CONV_FLAGS_SWIZZLE | CONV_FLAGS_NOALPHA;
            break;

        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            metadata.format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
            convFlags |= CONV_FLAGS_SWIZZLE;
            break;

        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            metadata.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            convFlags |= CONV_FLAGS_SWIZZLE;
            break;

        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            metadata.format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
            convFlags |= CONV_FLAGS_SWIZZLE | CONV_FLAGS_NOALPHA;
            break;

        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            metadata.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            convFlags |= CONV_FLAGS_SWIZZLE | CONV_FLAGS_NOALPHA;
            break;
        }
    }

    // Special flag for handling 16bpp formats
    if (flags & DDS_FLAGS_NO_16BPP)
    {
        switch ( metadata.format )
        {
        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            metadata.format = DXGI_FORMAT_R8G8B8A8_UNORM;
            convFlags |= CONV_FLAGS_EXPAND;
            if ( metadata.format == DXGI_FORMAT_B5G6R5_UNORM )
                convFlags |= CONV_FLAGS_NOALPHA;
        }
    }

    return S_OK;
}


HRESULT GetMetadataFromDDSFile( const char * szFile, DWORD flags, TexMetadata& metadata )
{
    if ( !szFile )
        return E_INVALIDARG;

    ScopedHandle hFile( safe_handle( CreateFileA( szFile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
                                                  FILE_FLAG_SEQUENTIAL_SCAN, 0 ) ) );
    if ( !hFile )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    // Get the file size
    LARGE_INTEGER fileSize = {0};

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
    FILE_STANDARD_INFO fileInfo;
    if ( !GetFileInformationByHandleEx( hFile.get(), FileStandardInfo, &fileInfo, sizeof(fileInfo) ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    fileSize = fileInfo.EndOfFile;
#else
    if ( !GetFileSizeEx( hFile.get(), &fileSize ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
#endif

    // File is too big for 32-bit allocation, so reject read (4 GB should be plenty large enough for a valid DDS file)
    if ( fileSize.HighPart > 0 )
    {
        return HRESULT_FROM_WIN32( ERROR_FILE_TOO_LARGE );
    }

    // Need at least enough data to fill the standard header and magic number to be a valid DDS
    if ( fileSize.LowPart < ( sizeof(DDS_HEADER) + sizeof(uint32_t) ) )
    {
        return E_FAIL;
    }

    // Read the header in (including extended header if present)
    const size_t MAX_HEADER_SIZE = sizeof(uint32_t) + sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10);
    uint8_t header[MAX_HEADER_SIZE];

    DWORD bytesRead = 0;
    if ( !ReadFile( hFile.get(), header, MAX_HEADER_SIZE, &bytesRead, 0 ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    DWORD convFlags = 0;
    HRESULT hr = _DecodeDDSHeader( header, bytesRead, flags, metadata, convFlags );

	if ( FAILED(hr) )
        return hr;

    DWORD offset = MAX_HEADER_SIZE;

    if ( !(convFlags & CONV_FLAGS_DX10) )
    {
        // Must reset file position since we read more than the standard header above
        LARGE_INTEGER filePos = { sizeof(uint32_t) + sizeof(DDS_HEADER), 0};
        if ( !SetFilePointerEx( hFile.get(), filePos, 0, FILE_BEGIN ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

        offset = sizeof(uint32_t) + sizeof(DDS_HEADER);
    }

    std::unique_ptr<uint32_t[]> pal8;
    if ( convFlags & CONV_FLAGS_PAL8 )
    {
        pal8.reset( new (std::nothrow) uint32_t[256] );
        if ( !pal8 )
        {
            return E_OUTOFMEMORY;
        }

        if ( !ReadFile( hFile.get(), pal8.get(), 256 * sizeof(uint32_t), &bytesRead, 0 ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

        if ( bytesRead != (256 * sizeof(uint32_t)) )
        {
            return E_FAIL;
        }

        offset += ( 256 * sizeof(uint32_t) );
    }

    DWORD remaining = fileSize.LowPart - offset;
    if ( remaining == 0 )
        return E_FAIL;

	std::stringstream str;
	str << "file size " <<  fileSize.LowPart <<  " - " << offset << " = " << remaining << std::endl;
	OutputDebugStringA(str.str().c_str());

	metadata.bytesData = remaining;
}

static bool IsFrameLate(const HighResClock::time_point & presentationTime, int fps_video, HighResClock::time_point now, HighResClock::duration max_lateness){
	auto duration = std::chrono::nanoseconds(1000000000/fps_video);
	return presentationTime+duration+max_lateness<now;
}

static bool IsFrameReady(const HighResClock::time_point & presentationTime, int fps_video, int fps_thread, HighResClock::time_point now, HighResClock::duration max_lateness){
	auto duration_video = std::chrono::nanoseconds(1000000000/fps_video);
	auto duration_thread = std::chrono::nanoseconds(1000000000/fps_thread);
	return presentationTime<=now+duration_thread && now<=presentationTime+duration_video+max_lateness;
}


DX11Player::DX11Player(ID3D11Device * device, const std::string & directory)
	:m_Device(device)
	,m_Context(nullptr)
	,m_CopyTextureIn(RING_BUFFER_SIZE)
	,m_ShaderResourceView(RING_BUFFER_SIZE)
	,m_UAVs(RING_BUFFER_SIZE)
	,m_OutputTextures(RING_BUFFER_SIZE)
	,m_UploadBuffers(RING_BUFFER_SIZE)
	,m_Overlaps(RING_BUFFER_SIZE)
	,m_WaitEvents(RING_BUFFER_SIZE)
	,m_FileHandles(RING_BUFFER_SIZE)
	,m_MappedBuffers(RING_BUFFER_SIZE)
	,m_UploaderThreadRunning(false)
	,m_WaiterThreadRunning(false)
	,m_CurrentFrame(0)
	,m_CurrentOutFront(RING_BUFFER_SIZE/2)
	,m_CurrentOutBack(0)
	,m_DroppedFrames(0)
	,m_Fps(60)
{
	HRESULT hr;

	

	// list all files in folder
	// TODO: filter by extension
	tinydir_dir dir;
	tinydir_open_sorted(&dir, directory.c_str());
	std::stringstream str;
	str << "reading folder " <<  directory <<  " with " << dir.n_files << " files" << std::endl;
	OutputDebugStringA( str.str().c_str());
	for (int i = 0; i < dir.n_files; i++)
	{

		tinydir_file file;
		tinydir_readfile_n(&dir, &file, i);

		if (!file.is_dir)
		{
			m_ImageFiles.push_back(directory+"\\"+std::string(file.name));
		}
	}
	tinydir_close(&dir); 

	// if no files finish
	if(m_ImageFiles.empty()){
		throw std::exception(("no images found in " + directory).c_str());
	}

	// parse first image header and assume all have the same format
	auto data = std::unique_ptr<uint8_t[]>();
	DDS_HEADER * header;
	uint8_t * dataptr;
	size_t size;
    TexMetadata mdata;
	GetMetadataFromDDSFile(m_ImageFiles[0].c_str(),DDS_FLAGS_NONE, mdata);
	m_Width = mdata.width;
	m_Height = mdata.height;
	auto format = mdata.format;
	size_t numbytesdata = mdata.bytesData;
	size_t numbytesfile = numbytesdata + HEADER_SIZE_BYTES;
	std::stringstream ss;
	ss << "loading " << m_ImageFiles.size() << " images from " << directory << " " << m_Width << "x" << m_Height << " " << format << " with " << numbytesfile << " bytes per file" << std::endl;
	OutputDebugStringA( ss.str().c_str() ); 


	// create dx device in non passed or get the 
	// immediate context of the one passed by parameter
	if(device==nullptr){
		OutputDebugString( L"creating device\n" );
		D3D_FEATURE_LEVEL level;
		hr = D3D11CreateDevice(nullptr,
			D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_SINGLETHREADED,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&m_Device,&level,
			&m_Context);

		if(FAILED(hr)){
			auto lpBuf = new char[1024];
			ZeroMemory(lpBuf,sizeof(lpBuf));
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, hr, 0, lpBuf, 0, NULL);
			throw std::exception((std::string("Coudln't create device: ") + lpBuf).c_str());
		}
	}else{
		device->GetImmediateContext(&m_Context);
	}

	// init mapped buffers to 0
	OutputDebugString( L"zeroing read back data\n" );
	for(auto & map: m_MappedBuffers){
		ZeroMemory(&map,sizeof(D3D11_MAPPED_SUBRESOURCE));
	}


	// create output textures, the ones shared with the application
	OutputDebugString( L"creating render textures\n" );
	D3D11_TEXTURE2D_DESC textureDescription;
	textureDescription.MipLevels = 1;
	textureDescription.ArraySize = 1;
	textureDescription.SampleDesc.Count = 1;
	textureDescription.SampleDesc.Quality = 0;
	textureDescription.Usage = D3D11_USAGE_DEFAULT;
	textureDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	textureDescription.CPUAccessFlags = 0;
	if(device==nullptr){
		textureDescription.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	}else{
		textureDescription.MiscFlags = 0;
	}
	textureDescription.Width = m_Width;
	textureDescription.Height = m_Height;
	textureDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	for(size_t i=0;i<m_OutputTextures.size();i++){
		hr = m_Device->CreateTexture2D(&textureDescription,nullptr,&m_OutputTextures[i]);
		if(FAILED(hr)){
			throw std::exception("Coudln't create render texture");
		} 

		if(!device){
			// if using our own device, create a shared handle for each texture
			OutputDebugString( L"getting shared texture handle\n" );
			IDXGIResource* pTempResource(NULL);
			HANDLE sharedHandle;
			hr = m_OutputTextures[i]->QueryInterface( __uuidof(IDXGIResource), (void**)&pTempResource );
			if(FAILED(hr)){
				throw std::exception("Coudln't query interface");
			}
			hr = pTempResource->GetSharedHandle(&sharedHandle);
			if(FAILED(hr)){
				throw std::exception("Coudln't get shared handle");
			}
			m_SharedHandles[m_OutputTextures[i]] = sharedHandle;
			pTempResource->Release();
		}
	}
	

	// create the copy textures, used to copy from the upload
	// buffers using the compute shader to jump the offset and then
	// to the output textures
	OutputDebugString( L"creating staging textures\n" );
	D3D11_TEXTURE2D_DESC textureDescriptionCopy;
	textureDescriptionCopy.MipLevels = 1;
	textureDescriptionCopy.ArraySize = 1;
	textureDescriptionCopy.SampleDesc.Count = 1;
	textureDescriptionCopy.SampleDesc.Quality = 0;
	textureDescriptionCopy.Usage = D3D11_USAGE_DEFAULT;
	textureDescriptionCopy.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDescriptionCopy.CPUAccessFlags = 0;
	textureDescriptionCopy.MiscFlags = 0;
	textureDescriptionCopy.Width = m_Width;
	textureDescriptionCopy.Height = m_Height;
	textureDescriptionCopy.Format = format;
	for(size_t i=0;i<m_CopyTextureIn.size();i++){
		hr = m_Device->CreateTexture2D(&textureDescriptionCopy,nullptr,&m_CopyTextureIn[i]);
	}
	

	// create the upload buffers, we upload directly to these from the disk
	for(size_t i = 0; i < m_UploadBuffers.size(); i++){
		hr = CreateStagingTexture(m_Width,m_Height,format,&m_UploadBuffers[i]);
		if(FAILED(hr)){
			_com_error error(hr);
			auto msg = error.ErrorMessage();

			auto lpBuf = new char[1024];
			ZeroMemory(lpBuf,sizeof(lpBuf));
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), lpBuf, 0, NULL);
			throw std::exception((std::string("Coudln't create staging texture") + lpBuf).c_str());
		}
	}

	// compute shader to copy from upload buffers to textures jumping the offset
	// also should convert format?
	ID3D11ComputeShader * shader;
	m_Device->CreateComputeShader(Offset,sizeof(Offset),nullptr,&shader);
	if(FAILED(hr)){
		throw std::exception("Coudln't create compute shader");
	}
	m_Context->CSSetShader(shader,nullptr,0);
	
	// TODO: is this needed?
	for(size_t i=0;i<m_CopyTextureIn.size();i++){
		hr = m_Device->CreateShaderResourceView(m_CopyTextureIn[i],nullptr,&m_ShaderResourceView[i]);
		if(FAILED(hr)){
			throw std::exception("Coudln't create shader resource view");
		}
	}
	m_Context->CSSetShaderResources(0,1,&m_ShaderResourceView[0]);

	// TODO: is this needed?
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Texture2D.MipSlice = 0;
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	for(size_t i=0;i<m_UAVs.size();i++){
		m_Device->CreateUnorderedAccessView(m_OutputTextures[i],&uavDesc,&m_UAVs[i]);
	}
	m_Context->CSSetUnorderedAccessViews(0,1,&m_UAVs[m_CurrentOutFront],0);

	// shader constants
	_declspec(align(16))
	struct PS_CONSTANT_BUFFER
	{
		int InputWidth;
		int Offset;
	};

	PS_CONSTANT_BUFFER constantData;
	constantData.InputWidth = m_Width;
	constantData.Offset = HEADER_SIZE_BYTES/4;

	D3D11_BUFFER_DESC shaderVarsDescription;
	shaderVarsDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	shaderVarsDescription.ByteWidth = sizeof(PS_CONSTANT_BUFFER);
	shaderVarsDescription.CPUAccessFlags = 0;
	shaderVarsDescription.MiscFlags = 0;
	shaderVarsDescription.StructureByteStride = 0;
	shaderVarsDescription.Usage = D3D11_USAGE_DEFAULT;
		
	D3D11_SUBRESOURCE_DATA bufferData;
	bufferData.pSysMem = &constantData;
	bufferData.SysMemPitch = 0;
	bufferData.SysMemSlicePitch = 0;

	ID3D11Buffer * varsBuffer;
	hr = m_Device->CreateBuffer(&shaderVarsDescription,&bufferData,&varsBuffer);
	if(FAILED(hr)){
		throw std::exception("Coudln't create constant buffer");
	}
	OutputDebugString( L"Created constants buffer\n" );
        
	m_Context->CSSetConstantBuffers(0,1,&varsBuffer);
	OutputDebugString( L"Set constants buffer\n" );
	varsBuffer->Release();
	
	// map all the upload buffers and send the adddresses to the uploader thread
	for(int i=0;i<RING_BUFFER_SIZE;i++){
		Frame nextFrame;
		nextFrame.idx = i;
		m_Context->Map(m_UploadBuffers[i],0,D3D11_MAP_WRITE,0,&m_MappedBuffers[i]);
		m_ReadyToUpload.send(nextFrame);
		m_WaitEvents[i] = CreateEventW(0,false,false,0);
		ZeroMemory(&m_Overlaps[i],sizeof(OVERLAPPED));
	}

	// uploader thread: reads async from disk to mapped GPU memory
	// and sends an event to wait on to the waiter thread
	OutputDebugString( L"creating upload thread\n" );
	m_UploaderThreadRunning = true;
	auto running = &m_UploaderThreadRunning;
	auto currentFrame = &m_CurrentFrame;
	auto imageFiles = &m_ImageFiles;
	auto readyToUpload = &m_ReadyToUpload;
	auto readyToWait = &m_ReadyToWait;
	auto readyToRender = &m_ReadyToRender;
	auto readyToRate = &m_ReadyToRate;
	auto mappedBuffers = &m_MappedBuffers;
	auto overlaps = &m_Overlaps;
	auto waitEvents = &m_WaitEvents;
	auto fileHandlers = &m_FileHandles;
	auto fps = &m_Fps;
	m_UploaderThread = std::thread([numbytesdata,running,currentFrame,imageFiles,readyToUpload,readyToWait,readyToRender,mappedBuffers,overlaps,waitEvents,fileHandlers,fps]{
		auto nextFrameTime = HighResClock::now();
		while(*running){
			Frame nextFrame;
			readyToUpload->recv(nextFrame);
			auto ptr = (char*)mappedBuffers->at(nextFrame.idx).pData;
			if(ptr){
				auto file = CreateFileA(imageFiles->at(*currentFrame).c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,NULL);
				if (file != INVALID_HANDLE_VALUE) {
					ZeroMemory(&overlaps->at(nextFrame.idx),sizeof(OVERLAPPED));
					overlaps->at(nextFrame.idx).hEvent = waitEvents->at(nextFrame.idx);
					fileHandlers->at(nextFrame.idx) = file;
					DWORD bytesRead=0;
					DWORD totalBytesRead=0;
					DWORD totalToRead = numbytesdata;
					//SetFilePointer(file,HEADER_SIZE_BYTES,nullptr,FILE_BEGIN);
					//while(totalBytesRead<totalToRead){
						ReadFile(file,ptr+totalBytesRead,totalToRead - totalBytesRead,&bytesRead,&overlaps->at(nextFrame.idx));
						//totalBytesRead += bytesRead;
					//}
					nextFrameTime += std::chrono::nanoseconds((uint64_t)floor(1000000000/double(*fps)+0.5));
					nextFrame.presentationTime = nextFrameTime;
					readyToWait->send(nextFrame);
				}

				(*currentFrame)+=1;
				(*currentFrame)%=imageFiles->size();
			}
		} 
	});
	
	// waiter thread: waits for the transfer from disk to GPU mem to finish
	// and sends the frame num to the main thread
	OutputDebugString( L"creating waiter thread\n" );
	m_WaiterThreadRunning = true;
	running = &m_WaiterThreadRunning;
	auto context = m_Context;
	auto copyTextureIn = &m_CopyTextureIn;
	auto uploadBuffers = &m_UploadBuffers;
	auto uavs = &m_UAVs;
	auto outputTextures = &m_OutputTextures;
	auto currentBack = &m_CurrentOutBack;
	m_WaiterThread = std::thread([running,readyToWait,readyToRate,readyToRender,waitEvents,fileHandlers,context,copyTextureIn,uploadBuffers,uavs,mappedBuffers,outputTextures,currentBack,fps]{
		auto start = HighResClock::now();
		while(*running){
			Frame nextFrame;
			readyToWait->recv(nextFrame);
			auto waitEvent = waitEvents->at(nextFrame.idx);
			WaitForSingleObject(waitEvent,INFINITE);
			readyToRender->send(nextFrame);
			CloseHandle(fileHandlers->at(nextFrame.idx));
			/*timer.wait_next();
			auto now = HighResClock::now();
			std::stringstream str;
			str << std::chrono::duration_cast<std::chrono::nanoseconds>(now-start).count();
			OutputDebugStringA(str.str().c_str());
			start = now;*/
			/*context->Unmap(uploadBuffers->at(nextBuffer), 0);
			context->CopyResource(copyTextureIn->at(0),uploadBuffers->at(nextBuffer));
			context->Dispatch(TEXTURE_WIDTH/32,TEXTURE_HEIGHT/30,1);
			context->CopyResource(outputTextures->at(nextBuffer),copyTextureOut->at(0));
			context->Map(uploadBuffers->at(nextBuffer),0,D3D11_MAP_WRITE,0,&mappedBuffers->at(nextBuffer));*/
		}
	});

	/*m_RateThread = std::thread([fps,running,readyToRate,readyToRender,readyToUpload]{
		Frame nextFrame;
		auto max_lateness = std::chrono::milliseconds(2);
		while(*running){
			auto now = HighResClock::now();
			
			bool ready = false;
			bool late = true;
			if(nextFrame.idx != -1){
				ready = IsFrameReady(nextFrame.presentationTime,*fps,(*fps)*4,now,max_lateness);
				if(ready){
					readyToRender->send(nextFrame);
					nextFrame.idx = -1;
				}else{
					late = IsFrameLate(nextFrame.presentationTime,*fps,now,max_lateness);
					if(late){
						readyToUpload->send(nextFrame);
						nextFrame.idx = -1;
					}
				}
			}

			while (!ready && late){
				readyToRate->recv(nextFrame);
				ready = IsFrameReady(nextFrame.presentationTime,*fps,(*fps)*4,now,max_lateness);
				if(ready){
					readyToRender->send(nextFrame);
					nextFrame.idx = -1;
				}else{
					late = IsFrameLate(nextFrame.presentationTime,*fps,now,max_lateness);
					if(late){
						readyToUpload->send(nextFrame);
						nextFrame.idx = -1;
					}
				}
			}

			auto sleep_for = std::chrono::nanoseconds(1000000000/ *fps / 4);
			std::this_thread::sleep_for(sleep_for);

		}
	});*/

	m_NextRenderFrame.idx = -1;
}

HRESULT DX11Player::CreateStagingTexture(int Width, int Height, DXGI_FORMAT Format, ID3D11Texture2D ** texture){
	D3D11_TEXTURE2D_DESC textureUploadDescription;
    textureUploadDescription.MipLevels = 1;
    textureUploadDescription.ArraySize = 1;
	textureUploadDescription.SampleDesc.Count = 1;
	textureUploadDescription.SampleDesc.Quality = 0;
	textureUploadDescription.Usage = D3D11_USAGE_STAGING;
    textureUploadDescription.BindFlags = 0;
	textureUploadDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    textureUploadDescription.MiscFlags = 0;
	textureUploadDescription.Width = Width;
	textureUploadDescription.Height = Height;
	textureUploadDescription.Format = Format;
	return m_Device->CreateTexture2D(&textureUploadDescription,nullptr,texture);
}

HRESULT DX11Player::CreateStagingBuffer(size_t bytes, ID3D11Buffer ** buffer){
	D3D11_BUFFER_DESC dynamicBufferDescription;
	dynamicBufferDescription.BindFlags = 0;
	dynamicBufferDescription.ByteWidth = bytes;
	dynamicBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	dynamicBufferDescription.MiscFlags = 0;
	dynamicBufferDescription.StructureByteStride = 0;
	dynamicBufferDescription.Usage = D3D11_USAGE_STAGING;
	OutputDebugString( L"creating staging buffers" );
	return m_Device->CreateBuffer(&dynamicBufferDescription,nullptr,buffer);
}

HRESULT DX11Player::CreateCopyBufferIn(size_t bytes, ID3D11Buffer ** buffer){
	D3D11_BUFFER_DESC dynamicBufferDescription;
	dynamicBufferDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	dynamicBufferDescription.ByteWidth = bytes;
	dynamicBufferDescription.CPUAccessFlags = 0;
	dynamicBufferDescription.MiscFlags = 0;
	dynamicBufferDescription.StructureByteStride = 0;
	dynamicBufferDescription.Usage = D3D11_USAGE_DEFAULT;
	OutputDebugString( L"creating staging buffers" );
	return m_Device->CreateBuffer(&dynamicBufferDescription,nullptr,buffer);
}

HRESULT DX11Player::CreateCopyBufferOut(size_t bytes, ID3D11Buffer ** buffer){
	D3D11_BUFFER_DESC dynamicBufferDescription;
	dynamicBufferDescription.BindFlags = 0;
	dynamicBufferDescription.ByteWidth = bytes;
	dynamicBufferDescription.CPUAccessFlags = 0;
	dynamicBufferDescription.MiscFlags = 0;
	dynamicBufferDescription.StructureByteStride = 0;
	dynamicBufferDescription.Usage = D3D11_USAGE_DEFAULT;
	OutputDebugString( L"creating staging buffers" );
	return m_Device->CreateBuffer(&dynamicBufferDescription,nullptr,buffer);
}

void DX11Player::OnRender(){
	auto now = HighResClock::now();
	auto max_lateness = std::chrono::milliseconds(2);
	std::vector<Frame> consumedBuffers;
	bool ready = false;
	bool late = true;
	if(m_NextRenderFrame.idx != -1){
		ready = IsFrameReady(m_NextRenderFrame.presentationTime,m_Fps,60,now,max_lateness);
		late = IsFrameLate(m_NextRenderFrame.presentationTime,m_Fps,now,max_lateness);
		if(!ready && !late){
			return;
		}else{
			consumedBuffers.push_back(m_NextRenderFrame);
			m_NextRenderFrame.idx = -1;
		}
	}

	while (late && m_ReadyToRender.try_recv(m_NextRenderFrame)){
		ready = IsFrameReady(m_NextRenderFrame.presentationTime,m_Fps,60,now,max_lateness);
		if(ready){
			consumedBuffers.push_back(m_NextRenderFrame);
			m_NextRenderFrame.idx = -1;
			break;
		}else{
			late = IsFrameLate(m_NextRenderFrame.presentationTime,m_Fps,now,max_lateness);
			if(late){
				consumedBuffers.push_back(m_NextRenderFrame);
				m_NextRenderFrame.idx = -1;
			}
		}
	}
	/*std::vector<Frame> consumedBuffers;
	while(m_ReadyToRender.try_recv(m_NextRenderFrame)){
		consumedBuffers.push_back(m_NextRenderFrame);
	}*/
	if(!consumedBuffers.empty() && ready){
		auto nextFrame = consumedBuffers.back();
		m_Context->Unmap(m_UploadBuffers[nextFrame.idx], 0);

		m_Context->CopyResource(m_CopyTextureIn[0],m_UploadBuffers[nextFrame.idx]);
		m_Context->Dispatch(m_Width/32,m_Height/30,1);
		
		m_Context->Map(m_UploadBuffers[nextFrame.idx],0,D3D11_MAP_WRITE,0,&m_MappedBuffers[nextFrame.idx]);
		m_DroppedFrames+=int(consumedBuffers.size())-1;
	}

	for(auto buffer: consumedBuffers){
		m_ReadyToUpload.send(buffer);
	}
}


HANDLE DX11Player::GetSharedHandle(){
	return m_SharedHandles[m_OutputTextures[m_CurrentOutFront]];
}
	
ID3D11Texture2D * DX11Player::GetTexturePointer(){
	return m_OutputTextures[m_CurrentOutFront];
}
	
ID3D11Texture2D * DX11Player::GetRenderTexturePointer(){
	/*if(m_NextRenderBuffer!=-1){
		return m_UploadBuffers[m_NextRenderBuffer];
	}else{*/
		return nullptr;
	//}
}


int DX11Player::GetUploadBufferSize()
{
	return m_ReadyToUpload.size();
}

int DX11Player::GetWaitBufferSize()
{
	return m_ReadyToWait.size();
}

int DX11Player::GetRenderBufferSize()
{
	return m_ReadyToRender.size();
}

int DX11Player::GetDroppedFrames(){
	return m_DroppedFrames;
}

void DX11Player::SetFPS(int fps){
	m_Fps = fps;
}

extern "C"{
	NATIVE_API DX11HANDLE DX11Player_Create(DX11HANDLE device, const char * directory)
	{
		try{
			return new DX11Player(static_cast<ID3D11Device*>(device),directory);
		}catch(std::exception & e){
			OutputDebugStringA( e.what() );
			return nullptr;
		}
	}

	NATIVE_API void DX11Player_Destroy(DX11HANDLE player)
	{
		delete static_cast<DX11Player*>(player);
	}

	NATIVE_API void DX11Player_OnRender(DX11HANDLE player)
	{
		static_cast<DX11Player*>(player)->OnRender();
	}

	NATIVE_API HANDLE DX11Player_GetSharedHandle(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetSharedHandle();
	}

	NATIVE_API DX11HANDLE DX11Player_GetTexturePointer(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetTexturePointer();
	}
	
	NATIVE_API DX11HANDLE DX11Player_GetRenderTexturePointer(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetRenderTexturePointer();
	}

	NATIVE_API int DX11Player_GetUploadBufferSize(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetUploadBufferSize();
	}

	NATIVE_API int DX11Player_GetWaitBufferSize(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetWaitBufferSize();
	}

	NATIVE_API int DX11Player_GetRenderBufferSize(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetRenderBufferSize();
	}

	NATIVE_API int DX11Player_GetDroppedFrames(DX11HANDLE player)
	{
		return static_cast<DX11Player*>(player)->GetDroppedFrames();
	}
	
	NATIVE_API void DX11Player_SetFPS(DX11HANDLE player, int fps)
	{
		static_cast<DX11Player*>(player)->SetFPS(fps);
	}

}
