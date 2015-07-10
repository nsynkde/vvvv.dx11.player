#pragma once
#include <string>
#include <vector>
#include <d3d11.h>

class ImageFormat
{
public:
	~ImageFormat(void);
	
	enum PixelFormat{
		ARGB,
		BGR,
		CbYCr,
		RGB,
		RGBA_PADDED,
		DX11_NATIVE
	};

	struct Format{
		size_t w;
		size_t h;
		PixelFormat pixel_format;
		DXGI_FORMAT in_format;
		DXGI_FORMAT out_format;
		size_t depth;
		size_t out_w;
		bool vflip;
		bool byteswap;
		size_t row_padding;
		size_t row_pitch;
		size_t data_offset;
		size_t bytes_data;
	};

	static Format FormatFor(const std::string & imageFile);
private:
	ImageFormat();
};


bool operator!=(const ImageFormat::Format & format1, const ImageFormat::Format & format2);
