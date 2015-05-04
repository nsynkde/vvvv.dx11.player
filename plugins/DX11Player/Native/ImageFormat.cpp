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

ImageFormat::FileInfo ImageFormat::FormatFor(const std::string & imageFile)
{	
	if(!PathFileExistsA(imageFile.c_str())){
		std::stringstream str;
		str << "File for format " << imageFile << " doesn't exist\n";
		throw std::exception(str.str().c_str());
	}

	FileInfo fileInfo;
	// find files extension, 
	// TODO: remove files with different extension
	auto cextension = PathFindExtensionA(imageFile.c_str());
	std::string extension = cextension?cextension:"";
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	ZeroMemory(&fileInfo,sizeof(FileInfo));

	// parse first image header and assume all have the same format
	// TODO: tex format breaks encapsulation. only set m_TexFormat if
	// input format is DX11_NATIVE else the input format depends on 
	// the shader and the out format is RGBA + depth (keep m_TexOutFormat??)
	if(extension == ".dds"){
		DirectX::TexMetadata mdata;
		DirectX::GetMetadataFromDDSFile(imageFile.c_str(),DirectX::DDS_FLAGS_NONE, mdata);
		fileInfo.format.w = mdata.width;
		fileInfo.format.out_w = mdata.width;
		fileInfo.format.h = mdata.height;
		fileInfo.format.in_format = mdata.format;
		fileInfo.format.out_format = fileInfo.format.in_format;
		fileInfo.data_offset = mdata.bytesHeader;
		fileInfo.bytes_data = mdata.bytesData;
		fileInfo.row_pitch = mdata.rowPitch;
		fileInfo.format.pixel_format = DX11_NATIVE;
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

		fileInfo.format.vflip = !(header.imagedescriptor & 32);
		auto alphaDepth = header.imagedescriptor & 15;
		fileInfo.format.out_w = header.width;
		fileInfo.format.h = header.height;
		fileInfo.data_offset = sizeof(TGA_HEADER) + header.idlength;
		/*std::stringstream str;
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
		OutputDebugStringA(str.str().c_str());*/
		switch(alphaDepth){
			case 0:
				fileInfo.format.in_format = DXGI_FORMAT_R8G8B8A8_UNORM;
				fileInfo.format.out_format = fileInfo.format.in_format;
				fileInfo.format.w = header.width*3/4;
				fileInfo.format.row_padding = NextMultiple(fileInfo.format.w,32) - fileInfo.format.w;
				/*if(header.width*3.0/4.0>fileInfo.format.w){
					std::stringstream str;
					str << "Can't convert image size to RGBA. Row size in bytes should be % 3/4. w = " << header.width << " w*3/4 = " << fileInfo.format.w << " w*3.0*4.0 = " << header.width*3.0 / 4.0;
					throw std::exception(str.str().c_str());
				}*/
				fileInfo.row_pitch = header.width * 3;
				fileInfo.format.pixel_format = BGR;
			break;
			case 8:
				fileInfo.format.in_format = DXGI_FORMAT_B8G8R8A8_UNORM;
				fileInfo.format.out_format = fileInfo.format.in_format;
				fileInfo.format.w = header.width;
				fileInfo.row_pitch = header.width * 4;
				fileInfo.format.pixel_format = DX11_NATIVE;
			break;
			default:{	
				std::stringstream str;
				str << "tga format with alpha depth " << alphaDepth << " not suported, only RGB(A) truecolor supported\n";
				throw std::exception(str.str().c_str());
			}
		}
		fileInfo.bytes_data = fileInfo.row_pitch * header.height;
		fileInfo.format.depth = 8;
	}else if(extension == ".dpx"){
		dpx::Header header;
		InStream stream;
		stream.Open(imageFile.c_str());
		header.Read(&stream);
		fileInfo.format.out_w = header.pixelsPerLine;
		fileInfo.format.h = header.linesPerElement * header.numberOfElements;
		auto dpx_descriptor = header.ImageDescriptor(0);
		fileInfo.format.byteswap = header.RequiresByteSwap();
		fileInfo.format.depth = header.BitDepth(0);
		/*std::stringstream str;
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
		str << " first 8 bytes: "  << std::endl;
		uint64_t buffer;
		dpx::ElementReadStream element(&stream);
		element.Read(header,0,0,&buffer,8);
		const char * data = (const char*) &buffer;
		for(int i=0;i<8;i++){
			str << std::bitset<8>(data[i]) << std::endl;
		}
		OutputDebugStringA(str.str().c_str());*/
		std::stringstream str;
		switch(dpx_descriptor){
		case dpx::Descriptor::kAlpha:
			switch(fileInfo.format.depth){
			case 8:
				fileInfo.format.in_format = DXGI_FORMAT_A8_UNORM;
				break;
			case 10:
			default:
				throw std::exception("Alpha only 10bits not supported");
				break;
			}
			fileInfo.format.pixel_format = DX11_NATIVE;
			fileInfo.row_pitch = fileInfo.format.out_w;
			fileInfo.format.out_format = fileInfo.format.in_format;
			fileInfo.format.w = header.pixelsPerLine;
			break;
		case dpx::Descriptor::kBlue:
		case dpx::Descriptor::kRed:
		case dpx::Descriptor::kGreen:
		case dpx::Descriptor::kLuma:
		case dpx::Descriptor::kDepth:
			switch(fileInfo.format.depth){
			case 8:
				fileInfo.format.in_format = DXGI_FORMAT_R8_UNORM;
				break;
			case 10:
				throw std::exception("One channel only 10bits not supported");
				break;
			case 16:
				fileInfo.format.in_format = DXGI_FORMAT_R16_UNORM;
				break;
			case 32:
				fileInfo.format.in_format = DXGI_FORMAT_R32_FLOAT;
				break;
			}
			fileInfo.format.w = header.pixelsPerLine;
			fileInfo.format.pixel_format = DX11_NATIVE;
			fileInfo.format.out_format = fileInfo.format.in_format;
			fileInfo.row_pitch = fileInfo.format.out_w;
			break;
		case dpx::Descriptor::kRGB:
			switch(fileInfo.format.depth){
			case 8:
				fileInfo.format.in_format = DXGI_FORMAT_R8G8B8A8_UNORM;
				fileInfo.format.out_format = fileInfo.format.in_format;
				fileInfo.row_pitch = fileInfo.format.out_w * 3;
				fileInfo.format.w = header.pixelsPerLine*3/4;
				fileInfo.format.row_padding = NextMultiple(fileInfo.format.w,8) - fileInfo.format.w;
				fileInfo.format.pixel_format = RGB;
				break;
			case 10:
				if(header.ImagePacking(0)==1){
					fileInfo.format.in_format = DXGI_FORMAT_R32_UINT;
					fileInfo.format.out_format = DXGI_FORMAT_R10G10B10A2_UNORM;
					fileInfo.row_pitch = fileInfo.format.out_w * 4;
					fileInfo.format.w = header.pixelsPerLine;
					fileInfo.format.pixel_format = ARGB;
				}else{
					throw std::exception("RGB 10bits without packing to 10/10/10/2 not supported");
				}
				break;
			case 16:
				fileInfo.format.in_format = DXGI_FORMAT_R16G16B16A16_UNORM;
				fileInfo.format.out_format = fileInfo.format.in_format;
				fileInfo.row_pitch = fileInfo.format.out_w * 3 * 2;
				fileInfo.format.w = header.pixelsPerLine*3/4;
				fileInfo.format.pixel_format = RGB;
				break;
			case 32:
				fileInfo.format.in_format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				fileInfo.format.out_format = fileInfo.format.in_format;
				fileInfo.row_pitch = fileInfo.format.out_w * 3 * 4;
				fileInfo.format.w = header.pixelsPerLine*3/4;
				fileInfo.format.pixel_format = RGB;
				break;
			}
			break;
		case dpx::Descriptor::kRGBA:
			switch(fileInfo.format.depth){
			case 8:
				fileInfo.format.in_format = DXGI_FORMAT_R8G8B8A8_UNORM;
				fileInfo.row_pitch = fileInfo.format.out_w * 4;
				break;
			case 10:
				fileInfo.format.in_format = DXGI_FORMAT_R10G10B10A2_UNORM;
				fileInfo.row_pitch = fileInfo.format.out_w * 4;
				break;
			case 16:
				fileInfo.format.in_format = DXGI_FORMAT_R16G16B16A16_UNORM;
				fileInfo.row_pitch = fileInfo.format.out_w * 4 * 2;
				break;
			case 32:
				fileInfo.format.in_format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				fileInfo.row_pitch = fileInfo.format.out_w * 4 * 4;
				break;
			}
			fileInfo.format.w = header.pixelsPerLine;
			fileInfo.format.out_format = fileInfo.format.in_format;
			fileInfo.format.pixel_format = DX11_NATIVE;
			break;
		case dpx::Descriptor::kCbYCr:
			switch(fileInfo.format.depth){
			case 8:
				fileInfo.format.in_format = DXGI_FORMAT_R8G8B8A8_UNORM;
				fileInfo.format.out_format = fileInfo.format.in_format;
				fileInfo.row_pitch = fileInfo.format.out_w * 3;
				fileInfo.format.w = header.pixelsPerLine*3/4;
				break;
			case 10:
				fileInfo.format.in_format = DXGI_FORMAT_R32_UINT;
				fileInfo.format.out_format = DXGI_FORMAT_R10G10B10A2_UNORM;
				fileInfo.row_pitch = fileInfo.format.out_w * 4;
				fileInfo.format.w = header.pixelsPerLine;
				break;
			case 16:
				fileInfo.format.in_format = DXGI_FORMAT_R16G16B16A16_UNORM;
				fileInfo.format.out_format = DXGI_FORMAT_R16G16B16A16_UNORM;
				fileInfo.row_pitch = fileInfo.format.out_w * 3 * 2;
				fileInfo.format.w = header.pixelsPerLine*3/4;
				break;
			default:
				str << " bitdepth not supported" << std::endl;
				throw std::exception(str.str().c_str());
				break;
			}
			fileInfo.format.pixel_format = CbYCr;
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
		fileInfo.data_offset = header.imageOffset;
		fileInfo.bytes_data = fileInfo.row_pitch * fileInfo.format.h;
	}else{
		throw std::exception(("format " + extension + " not suported").c_str());
	}
		
	//size_t numbytesfile = fileInfo.bytes_data + fileInfo.data_offset;
	/*std::stringstream ss;
	ss << "loading " << m_ImageFiles.size() << " images from " << directory << " " << fileInfo.format.out_w << "x" << fileInfo.format.h << " " << fileInfo.format.pixel_format << " with " << fileInfo.bytes_data << " bytes per file and " << fileInfo.data_offset << " fileInfo.data_offset, fileInfo.row_pitch: " << fileInfo.row_pitch << " input tex format " << fileInfo.format.in_format << " and output tex format " << fileInfo.format.out_format << std::endl;
	OutputDebugStringA( ss.str().c_str() ); */

	return fileInfo;
}

bool operator!=(const ImageFormat::Format & format1, const ImageFormat::Format & format2){
	return format1.w != format2.w || format1.h != format2.h || format1.pixel_format!=format2.pixel_format || format1.in_format!=format2.in_format || format1.out_format!=format2.out_format || format1.depth!=format2.depth || format1.out_w != format2.out_w || format1.vflip!=format2.vflip || format1.byteswap!=format2.byteswap;
}