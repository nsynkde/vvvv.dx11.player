
#pragma once
#include <d3d11.h>
#include <vector>
#include <memory>
#include <map>
#include <mutex>
#include "ImageSequence.h"

class Frame;

struct Format{
	int w;
	int h;
	ImageSequence::Format format;
	DXGI_FORMAT in_format;
	DXGI_FORMAT out_format;
	int depth;
	int out_w;
	bool vflip;
	bool byteswap;
};

class Context{
public:
	Context(const Format & format);
	~Context();
	Format GetFormat() const;
	std::shared_ptr<Frame> GetFrame();
	ID3D11DeviceContext * GetDX11Context();

	void CopyFrameToOutTexture(std::shared_ptr<Frame> frame);
	HANDLE GetSharedHandle();
	HRESULT CreateStagingTexture(ID3D11Texture2D ** texture);
	void Clear();
private:
	ID3D11Device * m_Device;
	ID3D11DeviceContext * m_Context;
	std::vector<ID3D11Texture2D *> m_CopyTextureIn;
	std::vector<ID3D11ShaderResourceView *> m_ShaderResourceViews;
    ID3D11Texture2D* m_BackBuffer;
	ID3D11RenderTargetView*  m_RenderTargetView;
    ID3D11Texture2D* m_TextureBack;
	std::vector<std::shared_ptr<Frame>> m_Frames;
	std::map<ID3D11Texture2D*,HANDLE> m_SharedHandles;
	int m_CurrentOutFront;
	D3D11_BOX m_CopyBox;
	Format m_Format;
	friend void ReleaseContext(Context * context);
	friend void ReleaseFrame(Frame * frame);
	std::mutex mutex;
};

class Pool
{
public:

	static Pool & GetInstance();

	std::shared_ptr<Context> AquireContext(const Format & format);

private:
	friend void ReleaseContext(Context * context);
	Pool(void){};
	std::vector<std::shared_ptr<Context>> contexts;
	std::mutex mutex;
};

bool operator!=(const Format & format1, const Format & format2);