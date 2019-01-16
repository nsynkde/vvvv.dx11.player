#pragma once
#include <d3d11.h>
#include <string>
#include <vector>

class ImageFormat {
public:

  enum PixelFormat { ARGB, RGB, RGBA_PADDED, CbYCr, DX11_NATIVE };
  enum CopyType { DiskToGpu, DiskToRam };

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
  size_t bytes_per_pixel_in;
  CopyType copytype;
  size_t remainder;

  ImageFormat(const std::string &imageFile);

  static bool IsRGBA(DXGI_FORMAT format);
  static bool IsRGBA32(DXGI_FORMAT format);
  static bool IsBGRA(DXGI_FORMAT format);
  static bool IsBGRX(DXGI_FORMAT format);
  static bool IsR10G10B10A2(DXGI_FORMAT format);
  static bool IsBC(DXGI_FORMAT format);
  bool IsBC();
};

bool operator!=(const ImageFormat &format1, const ImageFormat &format2);
bool operator==(const ImageFormat &format1, const ImageFormat &format2);
