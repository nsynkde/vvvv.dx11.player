

#include "stdafx.h"
#include <d3d11.h>
#include <memory>
#include <assert.h>
#include "DDS.h"
#include <algorithm>

namespace DirectX{

	enum TEX_MISC_FLAG
		// Subset here matches D3D10_RESOURCE_MISC_FLAG and D3D11_RESOURCE_MISC_FLAG
	{
		TEX_MISC_TEXTURECUBE = 0x4L,
	};

	enum TEX_MISC_FLAG2
	{
		TEX_MISC2_ALPHA_MODE_MASK = 0x7L,
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
	inline bool __cdecl IsCompressed(DXGI_FORMAT fmt)
	{
		switch ( fmt )
		{
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return true;

		default:
			return false;
		}
	}

	_Use_decl_annotations_
	inline bool __cdecl IsPacked(DXGI_FORMAT fmt)
	{
		switch( fmt )
		{
		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
		case DXGI_FORMAT_YUY2: // 4:2:2 8-bit
		case DXGI_FORMAT_Y210: // 4:2:2 10-bit
		case DXGI_FORMAT_Y216: // 4:2:2 16-bit
			return true;

		default:
			return false;
		}
	}

	_Use_decl_annotations_
	inline bool __cdecl IsPlanar(DXGI_FORMAT fmt)
	{
		switch ( static_cast<int>(fmt) )
		{
		case DXGI_FORMAT_NV12:      // 4:2:0 8-bit
		case DXGI_FORMAT_P010:      // 4:2:0 10-bit
		case DXGI_FORMAT_P016:      // 4:2:0 16-bit
		case DXGI_FORMAT_420_OPAQUE:// 4:2:0 8-bit
		case DXGI_FORMAT_NV11:      // 4:1:1 8-bit
			return true;

		case 118 /* DXGI_FORMAT_D16_UNORM_S8_UINT */:
		case 119 /* DXGI_FORMAT_R16_UNORM_X8_TYPELESS */:
		case 120 /* DXGI_FORMAT_X16_TYPELESS_G8_UINT */:
			// These are Xbox One platform specific types
			return true;

		default:
			return false;
		}
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

	enum CP_FLAGS
	{
		CP_FLAGS_NONE               = 0x0,      // Normal operation
		CP_FLAGS_LEGACY_DWORD       = 0x1,      // Assume pitch is DWORD aligned instead of BYTE aligned
		CP_FLAGS_PARAGRAPH          = 0x2,      // Assume pitch is 16-byte aligned instead of BYTE aligned
		CP_FLAGS_YMM                = 0x4,      // Assume pitch is 32-byte aligned instead of BYTE aligned
		CP_FLAGS_ZMM                = 0x8,      // Assume pitch is 64-byte aligned instead of BYTE aligned
		CP_FLAGS_PAGE4K             = 0x200,    // Assume pitch is 4096-byte aligned instead of BYTE aligned
		CP_FLAGS_24BPP              = 0x10000,  // Override with a legacy 24 bits-per-pixel format size
		CP_FLAGS_16BPP              = 0x20000,  // Override with a legacy 16 bits-per-pixel format size
		CP_FLAGS_8BPP               = 0x40000,  // Override with a legacy 8 bits-per-pixel format size
	};


	//=====================================================================================
	// DXGI Format Utilities
	//=====================================================================================

	//-------------------------------------------------------------------------------------
	// Returns bits-per-pixel for a given DXGI format, or 0 on failure
	//-------------------------------------------------------------------------------------
	_Use_decl_annotations_
	size_t BitsPerPixel( DXGI_FORMAT fmt )
	{
		switch( static_cast<int>(fmt) )
		{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return 128;

		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 96;

		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		case DXGI_FORMAT_Y416:
		case DXGI_FORMAT_Y210:
		case DXGI_FORMAT_Y216:
			return 64;

		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		case DXGI_FORMAT_AYUV:
		case DXGI_FORMAT_Y410:
		case DXGI_FORMAT_YUY2:
		case 116 /* DXGI_FORMAT_R10G10B10_7E3_A2_FLOAT */:
		case 117 /* DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT */:
			return 32;

		case DXGI_FORMAT_P010:
		case DXGI_FORMAT_P016:
		case 118 /* DXGI_FORMAT_D16_UNORM_S8_UINT */:
		case 119 /* DXGI_FORMAT_R16_UNORM_X8_TYPELESS */:
		case 120 /* DXGI_FORMAT_X16_TYPELESS_G8_UINT */:
			return 24;

		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_B5G6R5_UNORM:
		case DXGI_FORMAT_B5G5R5A1_UNORM:
		case DXGI_FORMAT_A8P8:
		case DXGI_FORMAT_B4G4R4A4_UNORM:
			return 16;

		case DXGI_FORMAT_NV12:
		case DXGI_FORMAT_420_OPAQUE:
		case DXGI_FORMAT_NV11:
			return 12;

		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM:
		case DXGI_FORMAT_AI44:
		case DXGI_FORMAT_IA44:
		case DXGI_FORMAT_P8:
			return 8;

		case DXGI_FORMAT_R1_UNORM:
			return 1;

		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			return 4;

		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return 8;

		default:
			return 0;
		}
	}

	//-------------------------------------------------------------------------------------
	// Computes the image row pitch in bytes, and the slice ptich (size in bytes of the image)
	// based on DXGI format, width, and height
	//-------------------------------------------------------------------------------------
	_Use_decl_annotations_
	void ComputePitch( DXGI_FORMAT fmt, size_t width, size_t height,
					   size_t& rowPitch, size_t& slicePitch, DWORD flags )
	{
		assert( IsValid(fmt) );

		if ( IsCompressed(fmt) )
		{
			size_t bpb = ( fmt == DXGI_FORMAT_BC1_TYPELESS
						 || fmt == DXGI_FORMAT_BC1_UNORM
						 || fmt == DXGI_FORMAT_BC1_UNORM_SRGB
						 || fmt == DXGI_FORMAT_BC4_TYPELESS
						 || fmt == DXGI_FORMAT_BC4_UNORM
						 || fmt == DXGI_FORMAT_BC4_SNORM) ? 8 : 16;
			size_t nbw = std::max<size_t>( 1, (width + 3) / 4 );
			size_t nbh = std::max<size_t>( 1, (height + 3) / 4 );
			rowPitch = nbw * bpb;

			slicePitch = rowPitch * nbh;
		}
		else if ( IsPacked(fmt) )
		{
			size_t bpe = ( fmt == DXGI_FORMAT_Y210 || fmt == DXGI_FORMAT_Y216 ) ? 8 : 4;
			rowPitch = ( ( width + 1 ) >> 1 ) * bpe;

			slicePitch = rowPitch * height;
		}
		else if ( fmt == DXGI_FORMAT_NV11 )
		{
			rowPitch = ( ( width + 3 ) >> 2 ) * 4;

			// Direct3D makes this simplifying assumption, although it is larger than the 4:1:1 data
			slicePitch = rowPitch * height * 2;
		}
		else if ( IsPlanar(fmt) )
		{
			size_t bpe = ( fmt == DXGI_FORMAT_P010 || fmt == DXGI_FORMAT_P016
						   || fmt == DXGI_FORMAT(118 /* DXGI_FORMAT_D16_UNORM_S8_UINT */)
						   || fmt == DXGI_FORMAT(119 /* DXGI_FORMAT_R16_UNORM_X8_TYPELESS */)
						   || fmt == DXGI_FORMAT(120 /* DXGI_FORMAT_X16_TYPELESS_G8_UINT */) ) ? 4 : 2;
			rowPitch = ( ( width + 1 ) >> 1 ) * bpe;

			slicePitch = rowPitch * ( height + ( ( height + 1 ) >> 1 ) );
		}
		else
		{
			size_t bpp;

			if ( flags & CP_FLAGS_24BPP )
				bpp = 24;
			else if ( flags & CP_FLAGS_16BPP )
				bpp = 16;
			else if ( flags & CP_FLAGS_8BPP )
				bpp = 8;
			else
				bpp = BitsPerPixel( fmt );

			if ( flags & ( CP_FLAGS_LEGACY_DWORD | CP_FLAGS_PARAGRAPH | CP_FLAGS_YMM | CP_FLAGS_ZMM | CP_FLAGS_PAGE4K ) )
			{
				if ( flags & CP_FLAGS_PAGE4K )
				{
					rowPitch = ( ( width * bpp + 32767 ) / 32768 ) * 4096;
					slicePitch = rowPitch * height;
				}
				else if ( flags & CP_FLAGS_ZMM )
				{
					rowPitch = ( ( width * bpp + 511 ) / 512 ) * 64;
					slicePitch = rowPitch * height;
				}
				else if ( flags & CP_FLAGS_YMM )
				{
					rowPitch = ( ( width * bpp + 255 ) / 256) * 32;
					slicePitch = rowPitch * height;
				}
				else if ( flags & CP_FLAGS_PARAGRAPH )
				{
					rowPitch = ( ( width * bpp + 127 ) / 128 ) * 16;
					slicePitch = rowPitch * height;
				}
				else // DWORD alignment
				{
					// Special computation for some incorrectly created DDS files based on
					// legacy DirectDraw assumptions about pitch alignment
					rowPitch = ( ( width * bpp + 31 ) / 32 ) * sizeof(uint32_t);
					slicePitch = rowPitch * height;
				}
			}
			else
			{
				// Default byte alignment
				rowPitch = ( width * bpp + 7 ) / 8;
				slicePitch = rowPitch * height;
			}
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
			offset = sizeof(uint32_t) + sizeof(DDS_HEADER);
		}

		if ( convFlags & CONV_FLAGS_PAL8 )
		{
			offset += ( 256 * sizeof(uint32_t) );
		}

		size_t rowPitch, slicePitch, _pixelSize;
		ComputePitch( metadata.format, metadata.width, metadata.height, rowPitch, slicePitch, flags );

		metadata.bytesData = slicePitch;
		metadata.bytesHeader = offset;
		metadata.rowPitch = metadata.width*BitsPerPixel(metadata.format)/8;
	}

	
    bool __cdecl TexMetadata::IsCubemap() const { return (miscFlags & TEX_MISC_TEXTURECUBE) != 0; }
        // Helper for miscFlags

    bool __cdecl TexMetadata::IsPMAlpha() const { return ((miscFlags2 & TEX_MISC2_ALPHA_MODE_MASK) == TEX_ALPHA_MODE_PREMULTIPLIED) != 0; }
    void __cdecl TexMetadata::SetAlphaMode( TEX_ALPHA_MODE mode ) { miscFlags2 = (miscFlags2 & ~TEX_MISC2_ALPHA_MODE_MASK) | static_cast<uint32_t>(mode); }
        // Helpers for miscFlags2

    bool __cdecl TexMetadata::IsVolumemap() const { return (dimension == TEX_DIMENSION_TEXTURE3D); }
}