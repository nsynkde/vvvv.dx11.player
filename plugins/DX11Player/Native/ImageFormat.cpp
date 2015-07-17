#include "stdafx.h"
#include "ImageFormat.h"
#include <sstream>
#include <fstream>
#include <bitset>
#include <shlwapi.h>
#include <algorithm>
#include "tinydir.h"
#include "DDS.h"
#include "libdpx\DPXHeader.h"
#include "libdpx\DPXStream.h"
#include "libdpx\DPX.h"
#include "libdpx\ElementReadStream.h"
#include "TGA.h"


BOOL szWildMatch3(const char * pat, const char * str) {
   switch (*pat) {
      case '\0':
         return !*str;
      case '*' :
         return szWildMatch3(pat+1, str) || *str && szWildMatch3(pat, str+1);
      case '?' :
         return *str && (*str != '.') && szWildMatch3(pat+1, str+1);
      default  :
		  return (::toupper(*str) == ::toupper(*pat)) &&
                 szWildMatch3(pat+1, str+1);
   } /* endswitch */
}

static DWORD NextMultiple(DWORD in, DWORD multiple){
	if(in % multiple != 0){
		return in + (multiple - in % multiple);
	}else{
		return in;
	}
}


ImageFormat::ImageFormat(const std::string & imageFile)
:w(0)
,h(0)
,pixel_format(DX11_NATIVE)
,in_format(DXGI_FORMAT_UNKNOWN)
,out_format(DXGI_FORMAT_UNKNOWN)
,depth(0)
,out_w(0)
,vflip(false)
,byteswap(false)
,row_padding(0)
,row_pitch(0)
,data_offset(0)
,bytes_data(0)
,bytes_per_pixel_in(4)
,copytype(DiskToGpu)
, remainder(0)
{	
	if(!PathFileExistsA(imageFile.c_str())){
		std::stringstream str;
		str << "File for format " << imageFile << " doesn't exist\n";
		throw std::exception(str.str().c_str());
	}

	// find files extension, 
	// TODO: remove files with different extension
	auto cextension = PathFindExtensionA(imageFile.c_str());
	std::string extension = cextension?cextension:"";
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

	// parse first image header and assume all have the same format
	// TODO: tex format breaks encapsulation. only set m_TexFormat if
	// input format is DX11_NATIVE else the input format depends on 
	// the shader and the out format is RGBA + depth (keep m_TexOutFormat??)
	if (extension == ".dds"){
		DirectX::TexMetadata mdata;
		DirectX::GetMetadataFromDDSFile(imageFile.c_str(), DirectX::DDS_FLAGS_NONE, mdata);
		in_format = mdata.format;
		out_format = DXGI_FORMAT_R8G8B8A8_UNORM;
		size_t blockSize;
		if (in_format == DXGI_FORMAT_BC1_UNORM || in_format == DXGI_FORMAT_BC1_UNORM_SRGB ||
			in_format == DXGI_FORMAT_BC4_UNORM || in_format == DXGI_FORMAT_BC4_SNORM) {
			h = NextMultiple(mdata.height, 4);
			w = NextMultiple(mdata.width, 4);
			blockSize = 8;
			bytes_per_pixel_in = 2;
			out_w = w;
		}else if (in_format == DXGI_FORMAT_BC2_UNORM || in_format == DXGI_FORMAT_BC2_UNORM_SRGB ||
			in_format == DXGI_FORMAT_BC3_UNORM || in_format == DXGI_FORMAT_BC3_UNORM_SRGB ||
			in_format == DXGI_FORMAT_BC5_UNORM || in_format == DXGI_FORMAT_BC5_SNORM ||
			in_format == DXGI_FORMAT_BC6H_SF16 || in_format == DXGI_FORMAT_BC6H_UF16 ||
			in_format == DXGI_FORMAT_BC7_UNORM || in_format == DXGI_FORMAT_BC7_UNORM_SRGB){
			h = NextMultiple(mdata.height, 4);
			w = NextMultiple(mdata.width, 4);
			blockSize = 16;
			bytes_per_pixel_in = 4;
			out_w = w;
		}
		if (mdata.width > 0)
		{
			row_pitch = std::max<size_t>(1, (mdata.width + 3) / 4) * blockSize; 
		}
		data_offset = mdata.bytesHeader;
		bytes_data = mdata.bytesData;
		row_padding = 0;
		pixel_format = DX11_NATIVE;
		if (w != mdata.width){
			copytype = DiskToRam;
		}
		OutputDebugStringA(("width: " + std::to_string(mdata.width) + " pitch " + std::to_string(mdata.rowPitch) + "\n").c_str());
	}else if(extension == ".tga"){
		TGA_HEADER header;
		std::fstream tgafile(imageFile, std::fstream::in | std::fstream::binary);
		tgafile.read((char*)&header, sizeof(TGA_HEADER));
		tgafile.close();
		if(header.datatypecode!=2){
			std::stringstream str;
			str << "tga format " << (int)header.datatypecode << " not suported, only RGB(A) truecolor supported\n";
			throw std::exception(str.str().c_str());
		}

		vflip = !(header.imagedescriptor & 32);
		auto alphaDepth = header.imagedescriptor & 15;
		OutputDebugStringA(std::to_string(header.bitsperpixel).c_str());
		out_w = header.width;
		h = header.height;
		data_offset = sizeof(TGA_HEADER) + header.idlength;
		std::stringstream str;
		str << "TGA " << std::endl;
		str << "idlength " << (int)header.idlength << std::endl;
		str << "colourmaptyp e" << (int)header.colourmaptype << std::endl;
		str << "datatypecode " << (int)header.datatypecode << std::endl;
		str << "colourmaporigin " << (int)header.colourmaporigin << std::endl;
		str << "colourmaplength " << (int)header.colourmaplength << std::endl;
		str << "colourmapdepth " << (int)header.colourmapdepth << std::endl;
		str << "x_origin " << (int)header.x_origin << std::endl;
		str << "y_origin " << (int)header.y_origin << std::endl;
		str << "width " << (int)header.width << std::endl;
		str << "height " << (int)header.height << std::endl;
		str << "bitsperpixel " << (int)header.bitsperpixel << std::endl;
		str << "imagedescriptor " << (int)header.imagedescriptor << std::endl;
		OutputDebugStringA(str.str().c_str());
		switch(header.bitsperpixel){
			case 24:
				if (header.width * 3 % 4){
					in_format = DXGI_FORMAT_R8_UNORM;
					out_format = DXGI_FORMAT_R8G8B8A8_UNORM;
					w = header.width * 3;
					row_pitch = header.width * 3;
					pixel_format = RGB;
					byteswap = true;
				} else {
					in_format = DXGI_FORMAT_R8G8B8A8_UNORM;
					out_format = DXGI_FORMAT_R8G8B8A8_UNORM;
					w = header.width * 3 /4;
					row_pitch = header.width * 3;
					pixel_format = RGB;
					byteswap = true;
				}
				bytes_per_pixel_in = 4;
				OutputDebugStringA("\nrgb\n");
			break;
			case 32:
				in_format = DXGI_FORMAT_B8G8R8A8_UNORM;
				out_format = in_format;
				w = header.width;
				row_pitch = header.width * 4;
				pixel_format = DX11_NATIVE;
				OutputDebugStringA("\nrgba\n");
			break;
			default:{	
				std::stringstream str;
				str << "tga format with bitsperpixel " << header.bitsperpixel << " not suported, only RGB(A) truecolor supported\n";
				throw std::exception(str.str().c_str());
			}
		}
		bytes_data = row_pitch * header.height;
		depth = 8;
	}else if(extension == ".dpx"){
		dpx::Header header;
		InStream stream;
		stream.Open(imageFile.c_str());
		header.Read(&stream);
		out_w = header.Width();
		h = header.Height();
		auto dpx_descriptor = header.ImageDescriptor(0);
		byteswap = header.DatumSwap(0);
		depth = header.BitDepth(0);
		std::stringstream str;
		str << "dpx with " << header.numberOfElements << " elements and format " << dpx_descriptor << " " << (int)header.BitDepth(0) << "bits packed with " << header.ImagePacking(0) << std::endl;
		str << " signed: " << header.DataSign(0)  << std::endl;
		str << " colorimetric: " << header.Colorimetric(0)  << std::endl;
		str << " low data: " << header.LowData(0)  << std::endl;
		str << " low quantity: " << header.LowQuantity(0)  << std::endl;
		str << " high data: " << header.HighData(0)  << std::endl;
		str << " high quantity: " << header.HighQuantity(0)  << std::endl;
		str << " endian swap? " << header.RequiresByteSwap() << std::endl;
		str << " white " << header.WhiteLevel() << std::endl;
		str << " black " << header.BlackLevel() << std::endl;
		str << " black gain " << header.BlackGain() << std::endl;
		str << " gamma " << header.Gamma() << std::endl;
		str << " width " << header.Width() << std::endl;
		str << " height " << header.Height() << std::endl;
		str << " xOffset " << header.xOffset << std::endl;
		str << " xOriginalSize " << header.xOriginalSize << std::endl;
		str << " xScannedSize " << header.xScannedSize << std::endl;
		str << " sign " << header.DataSign(0) << std::endl;
		
		str << " first 8 bytes: "  << std::endl;
		uint64_t buffer;
		dpx::ElementReadStream element(&stream);
		element.Read(header,0,0,&buffer,8);
		const char * data = (const char*) &buffer;
		for(int i=0;i<8;i++){
			str << std::bitset<8>(data[i]) << std::endl;
		}
		OutputDebugStringA(str.str().c_str());
		switch(dpx_descriptor){
		case dpx::Descriptor::kAlpha:
			switch(depth){
			case 8:
				in_format = DXGI_FORMAT_A8_UNORM;
				break;
			case 10:
			default:
				throw std::exception("Alpha only 10bits not supported");
				break;
			}
			pixel_format = DX11_NATIVE;
			row_pitch = out_w;
			out_format = in_format;
			w = header.Width();
			break;
		case dpx::Descriptor::kBlue:
		case dpx::Descriptor::kRed:
		case dpx::Descriptor::kGreen:
		case dpx::Descriptor::kLuma:
		case dpx::Descriptor::kDepth:
			switch(depth){
			case 8:
				in_format = DXGI_FORMAT_R8_UNORM;
				break;
			case 10:
				throw std::exception("One channel only 10bits not supported");
				break;
			case 16:
				in_format = DXGI_FORMAT_R16_UNORM;
				break;
			case 32:
				in_format = DXGI_FORMAT_R32_FLOAT;
				break;
			}
			w = header.Width();
			pixel_format = DX11_NATIVE;
			out_format = in_format;
			row_pitch = out_w;
			break;
		case dpx::Descriptor::kRGB:
			switch(depth){
			case 8:
				if (header.Width() * 3 % 4){
					OutputDebugStringA(("8bit dpx rgb on r8 " + std::to_string(header.Width()) + "\n").c_str());
					in_format = DXGI_FORMAT_R8_UNORM;
					out_format = DXGI_FORMAT_R8G8B8A8_UNORM;
					remainder = header.Width() % 4;
					w = header.Width() * 3 + remainder;
					row_pitch = header.Width() * 3 + remainder;
					pixel_format = RGB;
					byteswap = !byteswap;
				} else {
					OutputDebugStringA("8bit dpx rgb on rgba8\n");
					in_format = DXGI_FORMAT_R8G8B8A8_UNORM;
					out_format = DXGI_FORMAT_R8G8B8A8_UNORM;
					w = header.Width() * 3 / 4;
					row_pitch = header.Width() * 3;
					pixel_format = RGB;
					byteswap = !byteswap;
				}
				break;
			case 10:
				if(header.ImagePacking(0)==1){
					in_format = DXGI_FORMAT_R32_UINT;
					out_format = DXGI_FORMAT_R10G10B10A2_UNORM;
					row_pitch = out_w * 4;
					w = header.Width();
					pixel_format = ARGB;
				}else{
					throw std::exception("RGB 10bits without packing to 10/10/10/2 not supported");
				}
				break;
			case 16:
				in_format = DXGI_FORMAT_R16G16B16A16_UNORM;
				out_format = in_format;
				row_pitch = out_w * 3 * 2;
				w = header.Width() * 3 / 4;
				pixel_format = RGB;
				byteswap = !byteswap;
				break;
			case 32:
				if (header.Width() * 3 * 4 % 32){
					in_format = DXGI_FORMAT_R32G32B32A32_FLOAT;
					out_format = in_format;
					row_pitch = out_w * 3 * 4;
					w = header.Width() * 3 / 4;
					pixel_format = RGB;
				} else {
					in_format = DXGI_FORMAT_R32G32B32_FLOAT;
					out_format = in_format;
					row_pitch = out_w * 3 * 4;
					w = header.Width();
					pixel_format = DX11_NATIVE;
				}
				break;
			}
			break;
		case dpx::Descriptor::kRGBA:
			switch(depth){
			case 8:
				in_format = DXGI_FORMAT_R8G8B8A8_UNORM;
				row_pitch = out_w * 4;
				break;
			case 10:
				in_format = DXGI_FORMAT_R10G10B10A2_UNORM;
				row_pitch = out_w * 4;
				break;
			case 16:
				in_format = DXGI_FORMAT_R16G16B16A16_UNORM;
				row_pitch = out_w * 4 * 2;
				break;
			case 32:
				in_format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				row_pitch = out_w * 4 * 4;
				break;
			}
			w = header.Width();
			out_format = in_format;
			pixel_format = DX11_NATIVE;
			break;
		case dpx::Descriptor::kCbYCr:
			switch(depth){
			case 8:
				in_format = DXGI_FORMAT_R8G8B8A8_UNORM;
				out_format = in_format;
				row_pitch = out_w * 3;
				w = header.Width() * 3 / 4;
				break;
			case 10:
				in_format = DXGI_FORMAT_R32_UINT;
				out_format = DXGI_FORMAT_R10G10B10A2_UNORM;
				row_pitch = out_w * 4;
				w = header.Width();
				break;
			case 16:
				in_format = DXGI_FORMAT_R16G16B16A16_UNORM;
				out_format = DXGI_FORMAT_R16G16B16A16_UNORM;
				row_pitch = out_w * 3 * 2;
				w = header.Width() * 3 / 4;
				break;
			default:
				str << " bitdepth not supported" << std::endl;
				throw std::exception(str.str().c_str());
				break;
			}
			pixel_format = CbYCr;
			break;
		case dpx::Descriptor::kCbYACrYA:
		case dpx::Descriptor::kCbYCrA:
		case dpx::Descriptor::kCbYCrY:
		case dpx::Descriptor::kABGR:
		default:
			str << " not supported" << std::endl;
			throw std::exception(str.str().c_str());
			break;

		}
		data_offset = header.DataOffset(0);
		bytes_data = row_pitch * h;
	}else{
		throw std::exception(("format " + extension + " not suported").c_str());
	}
		
	//size_t numbytesfile = bytes_data + data_offset;
	/*std::stringstream ss;
	ss << "loading " << m_ImageFiles.size() << " images from " << directory << " " << out_w << "x" << h << " " << pixel_format << " with " << bytes_data << " bytes per file and " << data_offset << " data_offset, row_pitch: " << row_pitch << " input tex format " << in_format << " and output tex format " << out_format << std::endl;
	OutputDebugStringA( ss.str().c_str() ); */

}

bool operator!=(const ImageFormat & format1, const ImageFormat & format2){
	return format1.w != format2.w || format1.h != format2.h || format1.pixel_format!=format2.pixel_format || format1.in_format!=format2.in_format || format1.out_format!=format2.out_format || format1.depth!=format2.depth || format1.out_w != format2.out_w || format1.vflip!=format2.vflip || format1.byteswap!=format2.byteswap;
}

bool ImageFormat::IsRGBA(DXGI_FORMAT format)
{
	return format == DXGI_FORMAT_R16G16B16A16_FLOAT ||
		format == DXGI_FORMAT_R16G16B16A16_SINT ||
		format == DXGI_FORMAT_R16G16B16A16_SNORM ||
		format == DXGI_FORMAT_R16G16B16A16_TYPELESS ||
		format == DXGI_FORMAT_R16G16B16A16_UINT ||
		format == DXGI_FORMAT_R16G16B16A16_UNORM ||
		format == DXGI_FORMAT_R32G32B32A32_FLOAT ||
		format == DXGI_FORMAT_R32G32B32A32_SINT ||
		format == DXGI_FORMAT_R32G32B32A32_TYPELESS ||
		format == DXGI_FORMAT_R32G32B32A32_UINT ||
		format == DXGI_FORMAT_R8G8B8A8_SINT ||
		format == DXGI_FORMAT_R8G8B8A8_SNORM ||
		format == DXGI_FORMAT_R8G8B8A8_TYPELESS ||
		format == DXGI_FORMAT_R8G8B8A8_UINT ||
		format == DXGI_FORMAT_R8G8B8A8_UNORM ||
		format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
}

bool ImageFormat::IsRGBA32(DXGI_FORMAT format)
{
	return format == DXGI_FORMAT_R8G8B8A8_SINT ||
		format == DXGI_FORMAT_R8G8B8A8_SNORM ||
		format == DXGI_FORMAT_R8G8B8A8_TYPELESS ||
		format == DXGI_FORMAT_R8G8B8A8_UINT ||
		format == DXGI_FORMAT_R8G8B8A8_UNORM ||
		format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

}

bool ImageFormat::IsBGRA(DXGI_FORMAT format)
{
	return format == DXGI_FORMAT_B8G8R8A8_TYPELESS ||
		format == DXGI_FORMAT_B8G8R8A8_UNORM ||
		format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
}

bool ImageFormat::IsBGRX(DXGI_FORMAT format)
{
	return format == DXGI_FORMAT_B8G8R8X8_TYPELESS ||
		format == DXGI_FORMAT_B8G8R8X8_UNORM ||
		format == DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
}

bool ImageFormat::IsR10G10B10A2(DXGI_FORMAT format)
{
	return format == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM ||
		format == DXGI_FORMAT_R10G10B10A2_TYPELESS ||
		format == DXGI_FORMAT_R10G10B10A2_UINT ||
		format == DXGI_FORMAT_R10G10B10A2_UNORM;
}

bool ImageFormat::IsBC(DXGI_FORMAT format){

	return format == DXGI_FORMAT_BC1_UNORM || format == DXGI_FORMAT_BC1_UNORM_SRGB ||
		format == DXGI_FORMAT_BC4_UNORM || format == DXGI_FORMAT_BC4_SNORM ||
		format == DXGI_FORMAT_BC2_UNORM || format == DXGI_FORMAT_BC2_UNORM_SRGB ||
		format == DXGI_FORMAT_BC3_UNORM || format == DXGI_FORMAT_BC3_UNORM_SRGB ||
		format == DXGI_FORMAT_BC5_UNORM || format == DXGI_FORMAT_BC5_SNORM ||
		format == DXGI_FORMAT_BC6H_SF16 || format == DXGI_FORMAT_BC6H_UF16 ||
		format == DXGI_FORMAT_BC7_UNORM || format == DXGI_FORMAT_BC7_UNORM_SRGB;
}

bool ImageFormat::IsBC(){
	return ImageFormat::IsBC(in_format);
}
