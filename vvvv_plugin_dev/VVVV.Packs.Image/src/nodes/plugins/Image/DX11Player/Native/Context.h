#pragma once
#include <d3d11.h>
#include "ImageSequence.h"
#include <mutex>
#include <memory>
#include <map>

class Frame;

struct Format{
	size_t w;
	size_t h;
	ImageSequence::Format format;
	DXGI_FORMAT in_format;
	DXGI_FORMAT out_format;
	size_t depth;
	size_t out_w;
	bool vflip;
	bool byteswap;
	size_t row_padding;
};

class Context{
public:
	Context(const Format & format);
	~Context();
	Format GetFormat() const;
	std::shared_ptr<Frame> GetFrame();
	ID3D11DeviceContext * GetDX11Context();
	void CopyFrameToOutTexture(Frame * frame);
	HRESULT CreateStagingTexture(ID3D11Texture2D ** texture);
	HRESULT CreateRenderTexture(ID3D11Texture2D ** texture);
	void Clear();
private:
	D3D11_TEXTURE2D_DESC m_RenderTextureDescription;
	ID3D11Device * m_Device;
	ID3D11DeviceContext * m_Context;
	ID3D11Texture2D * m_CopyTextureIn;
	ID3D11ShaderResourceView * m_ShaderResourceView;
    ID3D11Texture2D* m_BackBuffer;
	ID3D11RenderTargetView*  m_RenderTargetView;
	std::vector<std::shared_ptr<Frame>> m_Frames;
	D3D11_BOX m_CopyBox;
	Format m_Format;
	friend void ReleaseContext(Context * context);
	friend void ReleaseFrame(Frame * frame);
	std::mutex mutex;
};

bool operator!=(const Format & format1, const Format & format2);
