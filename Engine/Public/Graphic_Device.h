#pragma once

#include "Base.h"

NS_BEGIN(Engine)


class CGraphic_Device final : public CBase
{		
private:
	CGraphic_Device();
	virtual ~CGraphic_Device() = default;

public:
	/* 그래픽 디바이스의 초기화. */
	/* 장치객체를 생성한다. */
	HRESULT Initialize(HWND hWnd, WINMODE isWindowed, _uint iWinSizeX, _uint iWinSizeY,
		_Inout_ ID3D11Device** ppDevice, _Inout_ ID3D11DeviceContext** ppContext);
	
	/* 백버퍼를 지운다. */
	HRESULT Clear_BackBuffer_View(const _float4* pClearColor);

	/* 깊이버퍼 + 스텐실버퍼를 지운다. */
	HRESULT Clear_DepthStencil_View();

	/* 후면 버퍼를 전면버퍼로 교체한다.(백버퍼를 화면에 직접 보여준다.) */
	HRESULT Present();

public:
	HRESULT Resize(_uint iNewWidth, _uint iNewHeight);

private:	

	ID3D11Device*				m_pDevice			= { nullptr };
	ID3D11DeviceContext*		m_pDeviceContext	= { nullptr };

	/* 후면버퍼와 전면버퍼를 교체해가면서 화면에 보여주는 역할 */
	IDXGISwapChain*				m_pSwapChain		= { nullptr };

	ID3D11RenderTargetView*		m_pBackBufferRTV	= { nullptr };	
	ID3D11DepthStencilView*		m_pDepthStencilView = { nullptr };

	HWND						m_hWnd				= { nullptr };

private:
	HRESULT						Ready_SwapChain(HWND hWnd, WINMODE isWindowed, _uint iWinCX, _uint iWinCY);
	HRESULT						Ready_BackBufferRenderTargetView();	
	HRESULT						Ready_DepthStencilView(_uint iWinCX, _uint iWinCY);

public:
	static CGraphic_Device*		Create(_In_ HWND hWnd, WINMODE isWindowed, _uint iWinSizeX, _uint iWinSizeY, _Out_ ID3D11Device** ppDevice, _Out_ ID3D11DeviceContext** ppDeviceContextOut);
	virtual void				Free() override;
};

NS_END