#include "stdafx.h"
#include "ImageSequence.h"
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





ImageSequence::ImageSequence(const std::string & directory)
:m_Directory(directory)
{
	// list all files in folder
	// TODO: filter by extension
	std::stringstream str;
	str << "trying to open folder " <<  directory << std::endl;
	OutputDebugStringA( str.str().c_str());
	tinydir_dir dir;
	tinydir_open_sorted(&dir, directory.c_str());
	str << "reading folder " <<  directory <<  " with " << dir.n_files << " files" << std::endl;
	OutputDebugStringA( str.str().c_str());
	for (size_t i = 0; i < dir.n_files; i++)
	{

		tinydir_file file;
		tinydir_readfile_n(&dir, &file, i);

		if (!file.is_dir)
		{
			m_ImageFiles.emplace_back(directory+"\\"+std::string(file.name));
		}
	}
	tinydir_close(&dir); 

	// if no files finish
	if(m_ImageFiles.empty()){
		throw std::exception(("no images found in " + directory).c_str());
	}

	// find files extension, 
	// TODO: remove files with different extension
	auto cextension = PathFindExtensionA(m_ImageFiles[0].c_str());
	std::string extension = cextension?cextension:"";
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	m_RowPitch = 0;
	m_DataOffset = 0;
	m_BytesData = 0;
	m_RequiresByteSwap = false;
	m_RequiresVFlip = false;

	// parse first image header and assume all have the same format
	// TODO: tex format breaks encapsulation. only set m_TexFormat if
	// input format is DX11_NATIVE else the input format depends on 
	// the shader and the out format is RGBA + depth (keep m_TexOutFormat??)
	if(extension == ".dds"){
		DirectX::TexMetadata mdata;
		DirectX::GetMetadataFromDDSFile(m_ImageFiles[0].c_str(),DirectX::DDS_FLAGS_NONE, mdata);
		m_InputWidth = mdata.width;
		m_Width = mdata.width;
		m_Height = mdata.height;
		m_TextureFormat = mdata.format;
		m_TextureOutFormat = m_TextureFormat;
		m_DataOffset = mdata.bytesHeader;
		m_BytesData = mdata.bytesData;
		m_RowPitch = mdata.rowPitch;
		m_InputFormat = DX11_NATIVE;
	}else if(extension == ".tga"){
		TGA_HEADER header;
		std::fstream tgafile(m_ImageFiles[0], std::fstream::in | std::fstream::binary);
		tgafile.read((char*)&header, sizeof(TGA_HEADER));
		tgafile.close();
		if(header.datatypecode!=2){
			std::stringstream str;
			str << "tga format " << (int)header.datatypecode << " not suported, only RGB(A) truecolor supported\n";
			throw std::exception(str.str().c_str());
		}

		m_RequiresVFlip = !(header.imagedescriptor & 32);
		auto alphaDepth = header.imagedescriptor & 15;
		m_Width = header.width;
		m_Height = header.height;
		m_DataOffset = sizeof(TGA_HEADER) + header.idlength;
		std::stringstream str;
		str << "TGA with x,y origin: " << header.x_origin << ", " << header.y_origin << " descriptor " << (int)header.imagedescriptor <<  " alpha: " << alphaDepth << std::endl;
		OutputDebugStringA(str.str().c_str());
		switch(alphaDepth){
			case 0:
				m_TextureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
				m_TextureOutFormat = m_TextureFormat;
				m_InputWidth = header.width*3/4;
				m_RowPitch = header.width * 3;
				m_InputFormat = BGR;
			break;
			case 8:
				m_TextureFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
				m_TextureOutFormat = m_TextureFormat;
				m_InputWidth = header.width;
				m_RowPitch = header.width * 4;
				m_InputFormat = DX11_NATIVE;
			break;
			default:{	
				std::stringstream str;
				str << "tga format with alpha depth " << alphaDepth << " not suported, only RGB(A) truecolor supported\n";
				throw std::exception(str.str().c_str());
			}
		}
		m_BytesData = m_RowPitch * header.height;
		m_InputDepth = 8;
	}else if(extension == ".dpx"){
		dpx::Header header;
		InStream stream;
		stream.Open(m_ImageFiles[0].c_str());
		header.Read(&stream);
		m_Width = header.pixelsPerLine;
		m_Height = header.linesPerElement * header.numberOfElements;
		auto dpx_descriptor = header.ImageDescriptor(0);
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
		m_RequiresByteSwap = header.RequiresByteSwap();
		str << " first 8 bytes: "  << std::endl;
		uint64_t buffer;
		dpx::ElementReadStream element(&stream);
		element.Read(header,0,0,&buffer,8);
		const char * data = (const char*) &buffer;
		for(int i=0;i<8;i++){
			str << std::bitset<8>(data[i]) << std::endl;
		}
		m_InputDepth = header.BitDepth(0);

		OutputDebugStringA(str.str().c_str());
		switch(dpx_descriptor){
		case dpx::Descriptor::kAlpha:
			switch(m_InputDepth){
			case 8:
				m_TextureFormat = DXGI_FORMAT_A8_UNORM;
				break;
			case 10:
			default:
				throw std::exception("Alpha only 10bits not supported");
				break;
			}
			m_InputFormat = DX11_NATIVE;
			m_RowPitch = m_Width;
			m_TextureOutFormat = m_TextureFormat;
			m_InputWidth = header.pixelsPerLine;
			break;
		case dpx::Descriptor::kBlue:
		case dpx::Descriptor::kRed:
		case dpx::Descriptor::kGreen:
		case dpx::Descriptor::kLuma:
		case dpx::Descriptor::kDepth:
			switch(m_InputDepth){
			case 8:
				m_TextureFormat = DXGI_FORMAT_R8_UNORM;
				break;
			case 10:
				throw std::exception("One channel only 10bits not supported");
				break;
			case 16:
				m_TextureFormat = DXGI_FORMAT_R16_UNORM;
				break;
			case 32:
				m_TextureFormat = DXGI_FORMAT_R32_FLOAT;
				break;
			}
			m_InputWidth = header.pixelsPerLine;
			m_InputFormat = DX11_NATIVE;
			m_TextureOutFormat = m_TextureFormat;
			m_RowPitch = m_Width;
			break;
		case dpx::Descriptor::kRGB:
			switch(m_InputDepth){
			case 8:
				m_TextureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
				m_TextureOutFormat = m_TextureFormat;
				m_RowPitch = m_Width * 3;
				m_InputWidth = header.pixelsPerLine*3/4;
				break;
			case 10:
				if(header.ImagePacking(0)==1){
					m_TextureFormat = DXGI_FORMAT_R32_UINT;
					m_TextureOutFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
					m_RowPitch = m_Width * 4;
					m_InputWidth = header.pixelsPerLine;
				}else{
					throw std::exception("RGB 10bits without packing to 10/10/10/2 not supported");
				}
				break;
			case 16:
				m_TextureFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
				m_TextureOutFormat = m_TextureFormat;
				m_RowPitch = m_Width * 3 * 2;
				m_InputWidth = header.pixelsPerLine*3/4;
				break;
			case 32:
				m_TextureFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
				m_TextureOutFormat = m_TextureFormat;
				m_RowPitch = m_Width * 3 * 4;
				m_InputWidth = header.pixelsPerLine*3/4;
				break;
			}
			m_InputFormat = RGB;
			break;
		case dpx::Descriptor::kRGBA:
			switch(m_InputDepth){
			case 8:
				m_TextureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
				m_RowPitch = m_Width * 4;
				break;
			case 10:
				m_TextureFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
				m_RowPitch = m_Width * 4;
				break;
			case 16:
				m_TextureFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
				m_RowPitch = m_Width * 4 * 2;
				break;
			case 32:
				m_TextureFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
				m_RowPitch = m_Width * 4 * 4;
				break;
			}
			m_InputWidth = header.pixelsPerLine;
			m_TextureOutFormat = m_TextureFormat;
			m_InputFormat = DX11_NATIVE;
			break;
		case dpx::Descriptor::kCbYCr:
			switch(m_InputDepth){
			case 8:
				m_TextureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
				m_TextureOutFormat = m_TextureFormat;
				m_RowPitch = m_Width * 3;
				m_InputWidth = header.pixelsPerLine*3/4;
				break;
			case 10:
				m_TextureFormat = DXGI_FORMAT_R32_UINT;
				m_TextureOutFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
				m_RowPitch = m_Width * 4;
				m_InputWidth = header.pixelsPerLine;
				break;
			case 16:
				m_TextureFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
				m_TextureOutFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
				m_RowPitch = m_Width * 3 * 2;
				m_InputWidth = header.pixelsPerLine*3/4;
				break;
			default:
				str << " bitdepth not supported" << std::endl;
				throw std::exception(str.str().c_str());
				break;
			}
			m_InputFormat = CbYCr;
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
		m_DataOffset = header.imageOffset;
		m_BytesData = m_RowPitch * m_Height;
	}else{
		throw std::exception(("format " + extension + " not suported").c_str());
	}
		
	//size_t numbytesfile = m_BytesData + m_DataOffset;
	std::stringstream ss;
	ss << "loading " << m_ImageFiles.size() << " images from " << directory << " " << m_Width << "x" << m_Height << " " << m_InputFormat << " with " << m_BytesData << " bytes per file and " << m_DataOffset << " m_DataOffset, m_RowPitch: " << m_RowPitch << " input tex format " << m_TextureFormat << " and output tex format " << m_TextureOutFormat << std::endl;
	OutputDebugStringA( ss.str().c_str() ); 
}


ImageSequence::~ImageSequence(void)
{
	m_ImageFiles.clear();
}

const std::string & ImageSequence::Directory() const{
	return m_Directory;
}

const std::string & ImageSequence::Image(const size_t & position) const
{
	return m_ImageFiles[position];
}

size_t ImageSequence::NumImages() const{
	return m_ImageFiles.size();
}

size_t ImageSequence::Width() const
{
	return m_Width;
}

size_t ImageSequence::Height() const
{
	return m_Height;
}

size_t ImageSequence::InputWidth() const{
	return m_InputWidth;
}

size_t ImageSequence::RowPitch() const
{
	return m_RowPitch;
}

DXGI_FORMAT ImageSequence::TextureFormat() const
{
	return m_TextureFormat;
}

DXGI_FORMAT ImageSequence::TextureOutFormat() const
{
	return m_TextureOutFormat;
}

ImageSequence::Format ImageSequence::InputFormat() const
{
	return m_InputFormat;
}

size_t ImageSequence::InputDepth() const
{
	return m_InputDepth;
}

size_t ImageSequence::DataOffset() const
{
	return m_DataOffset;
}

size_t ImageSequence::BytesData() const
{
	return m_BytesData;
}

bool ImageSequence::RequiresByteSwap() const
{
	return m_RequiresByteSwap;
}

bool ImageSequence::RequiresVFlip() const
{
	return m_RequiresVFlip;
}
