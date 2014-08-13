
#include <d3d11.h>
#include <array>
#include <vector>

class ReadTextureDX11
{
public:
	ReadTextureDX11(HANDLE tex_handle, int outFormat);
	~ReadTextureDX11();
	
	void * ReadBack();

private:
	ID3D11Device * m_Device;
	std::vector<ID3D11RenderTargetView*>  m_RenderTargetView;
    std::vector<ID3D11Texture2D*> m_BackBuffer;
    ID3D11Texture2D* m_SharedTexture;
    std::vector<ID3D11Texture2D*> m_TextureBack;
	std::vector<D3D11_MAPPED_SUBRESOURCE> m_ReadBackData;
	ID3D11DeviceContext * m_Context;
    int m_CurrentBack;
    int m_CurrentFront;
    int m_RendererOutput;
    bool m_DirectCopy;

};
