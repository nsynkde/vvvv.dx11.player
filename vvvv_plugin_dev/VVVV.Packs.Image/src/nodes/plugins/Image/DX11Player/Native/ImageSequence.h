#pragma once
#include <string>
#include <vector>
#include <d3d11.h>

class ImageSequence
{
public:
	ImageSequence(const std::string & directory);
	~ImageSequence(void);
	
	enum Format{
		ARGB,
		BGR,
		CbYCr,
		RGB,
		DX11_NATIVE
	};

	const std::string & Directory() const;
	const std::string & Image(const size_t & position) const;
	size_t NumImages() const;
	size_t Width() const;
	size_t Height() const;
	size_t InputWidth() const;
	size_t RowPitch() const;
	DXGI_FORMAT TextureFormat() const;
	DXGI_FORMAT TextureOutFormat() const;
	Format InputFormat() const;
	size_t InputDepth() const;
	size_t DataOffset() const;
	size_t BytesData() const;
	bool RequiresByteSwap() const;
	bool RequiresVFlip() const;
private:
	std::string m_Directory;
	std::vector<std::string> m_ImageFiles;
	size_t m_Width;
	size_t m_Height;
	size_t m_InputWidth;
	size_t m_RowPitch;
	DXGI_FORMAT m_TextureFormat;
	DXGI_FORMAT m_TextureOutFormat;
	Format m_InputFormat;
	size_t m_InputDepth;
	size_t m_DataOffset;
	size_t m_BytesData;
	bool m_RequiresByteSwap;
	bool m_RequiresVFlip;
};

