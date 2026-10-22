#include "Graphic_Device.h"

CGraphic_Device::CGraphic_Device()
{
}

HRESULT CGraphic_Device::Initialize(HWND hWnd, WINMODE isWindowed, _uint iViewportWidth, _uint iViewportHeight, _Inout_ ID3D11Device** ppDevice, _Inout_ ID3D11DeviceContext** ppContext)
{
	_uint		iFlag = 0;
	m_hWnd = hWnd;

#ifdef _DEBUG
	iFlag = D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL			FeatureLV;
	if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, iFlag, nullptr, 0, D3D11_SDK_VERSION, &m_pDevice, &FeatureLV, &m_pDeviceContext)))
		return E_FAIL;

#ifdef _DEBUG
	{
		ID3D11InfoQueue* pInfoQueue = nullptr;
		if (SUCCEEDED(m_pDevice->QueryInterface(__uuidof(ID3D11InfoQueue), reinterpret_cast<void**>(&pInfoQueue))))
		{
			pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			// pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
			pInfoQueue->Release();
		}
	}
#endif

	if (FAILED(Ready_SwapChain(hWnd, isWindowed, iViewportWidth, iViewportHeight)))
		return E_FAIL;

	if (FAILED(Ready_BackBufferRenderTargetView()))
		return E_FAIL;	

	if (FAILED(Ready_DepthStencilView(iViewportWidth, iViewportHeight)))
		return E_FAIL;

	ID3D11RenderTargetView* pRTVs[] = {
	m_pBackBufferRTV,
	};

	m_pDeviceContext->OMSetRenderTargets(1, pRTVs, m_pDepthStencilView);

	D3D11_VIEWPORT			ViewPortDesc;
	ZeroMemory(&ViewPortDesc, sizeof(D3D11_VIEWPORT));
	ViewPortDesc.TopLeftX = 0;
	ViewPortDesc.TopLeftY = 0;
	ViewPortDesc.Width = (_float)iViewportWidth;
	ViewPortDesc.Height = (_float)iViewportHeight;
	ViewPortDesc.MinDepth = 0.f;
	ViewPortDesc.MaxDepth = 1.f;

	m_pDeviceContext->RSSetViewports(1, &ViewPortDesc);

	*ppDevice = m_pDevice;
	*ppContext = m_pDeviceContext;

	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pDeviceContext);

	return S_OK;
}


HRESULT CGraphic_Device::Clear_BackBuffer_View(const _float4* pClearColor)
{
	if (m_pDeviceContext == nullptr) return E_FAIL;
	m_pDeviceContext->ClearRenderTargetView(m_pBackBufferRTV, reinterpret_cast<const float*>(pClearColor));      
	return S_OK;
}

HRESULT CGraphic_Device::Clear_DepthStencil_View()
{
	if (m_pDeviceContext == nullptr) return E_FAIL;

	_uint iFlag = D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;
	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, iFlag, 1.f, 0);
	return S_OK;
}

HRESULT CGraphic_Device::Present()
{
	if (nullptr == m_pSwapChain) return E_FAIL;
	
	return m_pSwapChain->Present(1, 0);
}

HRESULT CGraphic_Device::Resize(_uint iNewWidth, _uint iNewHeight)
{
	if (m_pDeviceContext == nullptr || m_pDevice == nullptr || m_pSwapChain == nullptr) return E_FAIL;
	
	m_pDeviceContext->ClearState();
	m_pDeviceContext->Flush();

	Safe_Release(m_pDepthStencilView);
	Safe_Release(m_pBackBufferRTV);

	if (FAILED(m_pSwapChain->ResizeBuffers(0, iNewWidth, iNewHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0)))
	{
		MSG_BOX("Graphic Device : Resize Buffer Failed");
		return E_FAIL;
	}
	
	//Create BackBuffer

	ID3D11Texture2D* pTempBackBuffer = nullptr;
	if (FAILED(m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pTempBackBuffer))))
	{
		MSG_BOX("Graphic Device : Get Back Buffer Failed");
		return E_FAIL;
	}

	if (FAILED(m_pDevice->CreateRenderTargetView(pTempBackBuffer, nullptr, &m_pBackBufferRTV)))
	{
		MSG_BOX("Graphic Device : Create Resizing RTV Failed");
		return E_FAIL;
	}

	Safe_Release(pTempBackBuffer);

	if (FAILED(Ready_DepthStencilView(iNewWidth, iNewHeight)))
	{
		MSG_BOX("Graphic Device : Ready DepthStencilView Failed ");
		return E_FAIL;
	}

	ID3D11RenderTargetView* pRTVs[] = { m_pBackBufferRTV, };

	m_pDeviceContext->OMSetRenderTargets(1, pRTVs, m_pDepthStencilView);

	D3D11_VIEWPORT			ViewPortDesc;
	ZeroMemory(&ViewPortDesc, sizeof(D3D11_VIEWPORT));
	ViewPortDesc.TopLeftX = 0;
	ViewPortDesc.TopLeftY = 0;
	ViewPortDesc.Width = static_cast<_float>(iNewWidth);
	ViewPortDesc.Height = static_cast<_float>(iNewHeight);
	ViewPortDesc.MinDepth = 0.f;
	ViewPortDesc.MaxDepth = 1.f;

	m_pDeviceContext->RSSetViewports(1, &ViewPortDesc);


	RECT     rcWindow = { 0, 0, (LONG)iNewWidth, (LONG)iNewHeight };

	AdjustWindowRect(&rcWindow, WS_OVERLAPPEDWINDOW, FALSE);
	SetWindowPos(m_hWnd, nullptr, 0, 0, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top, 0);
	return S_OK;
}

HRESULT CGraphic_Device::Ready_SwapChain(HWND hWnd, WINMODE isWindowed, _uint iWinCX, _uint iWinCY)
{
	IDXGIDevice* pDevice = nullptr;
	if (FAILED(m_pDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDevice))) return E_FAIL;

	IDXGIAdapter* pAdapter = nullptr;
	if (FAILED(pDevice->GetAdapter(&pAdapter))) return E_FAIL;

	IDXGIFactory* pFactory = nullptr;
	if (FAILED(pAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&pFactory))) return E_FAIL;
	
	DXGI_SWAP_CHAIN_DESC		SwapChain;
	ZeroMemory(&SwapChain, sizeof(DXGI_SWAP_CHAIN_DESC));

	SwapChain.BufferDesc.Width = iWinCX;
	SwapChain.BufferDesc.Height = iWinCY;
	
	//픽셀 Format 
	SwapChain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChain.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	SwapChain.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	SwapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChain.BufferCount = 1;

	//모니터 주사율에 따라 설정 
	SwapChain.BufferDesc.RefreshRate.Numerator = 60;
	SwapChain.BufferDesc.RefreshRate.Denominator = 1;

	//멀티 샘플링. 후처리 호환성을 위해 비활성화 
	SwapChain.SampleDesc.Quality = 0;
	SwapChain.SampleDesc.Count = 1;

	SwapChain.OutputWindow = hWnd;
	SwapChain.Windowed = static_cast<BOOL>(isWindowed);
	SwapChain.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	if (FAILED(pFactory->CreateSwapChain(m_pDevice, &SwapChain, &m_pSwapChain)))
		return E_FAIL;

	Safe_Release(pFactory);
	Safe_Release(pAdapter);
	Safe_Release(pDevice);

	return S_OK;
}

HRESULT CGraphic_Device::Ready_BackBufferRenderTargetView()
{
	if (m_pDevice == nullptr || m_pSwapChain == nullptr) return E_FAIL;

	ID3D11Texture2D* pBackBufferTexture = nullptr;

	if (FAILED(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBufferTexture)))
		return E_FAIL;

	if (FAILED(m_pDevice->CreateRenderTargetView(pBackBufferTexture, nullptr, &m_pBackBufferRTV)))
		return E_FAIL;

	Safe_Release(pBackBufferTexture);

	return S_OK;
}

HRESULT CGraphic_Device::Ready_DepthStencilView(_uint iWinCX, _uint iWinCY)
{
	if (m_pDevice == nullptr) return E_FAIL;

	ID3D11Texture2D* pDepthStencilTexture = nullptr;

	D3D11_TEXTURE2D_DESC	TextureDesc;
	ZeroMemory(&TextureDesc, sizeof(D3D11_TEXTURE2D_DESC));

	TextureDesc.Width = iWinCX;
	TextureDesc.Height = iWinCY;
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	TextureDesc.SampleDesc.Quality = 0;
	TextureDesc.SampleDesc.Count = 1;

	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.MiscFlags = 0;

	if (FAILED(m_pDevice->CreateTexture2D(&TextureDesc, nullptr, &pDepthStencilTexture)))
		return E_FAIL;


	if (FAILED(m_pDevice->CreateDepthStencilView(pDepthStencilTexture, nullptr, &m_pDepthStencilView)))
		return E_FAIL;

	Safe_Release(pDepthStencilTexture);

	return S_OK;
}

CGraphic_Device* CGraphic_Device::Create(HWND hWnd, WINMODE isWindowed, _uint iViewportWidth, _uint iViewportHeight, ID3D11Device** ppDevice, ID3D11DeviceContext** ppDeviceContextOut)
{
	CGraphic_Device* pInstance = new CGraphic_Device();

	if (FAILED(pInstance->Initialize(hWnd, isWindowed, iViewportWidth, iViewportHeight, ppDevice, ppDeviceContextOut)))
	{
		MSG_BOX("Failed to Created : CGraphic_Device");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CGraphic_Device::Free()
{
	if (nullptr != m_pDeviceContext)
	{
		m_pDeviceContext->ClearState();
		m_pDeviceContext->Flush();
	}

	Safe_Release(m_pSwapChain);
	Safe_Release(m_pDepthStencilView);
	Safe_Release(m_pBackBufferRTV);
	Safe_Release(m_pDeviceContext);


#if defined(DEBUG) || defined(_DEBUG)
	ID3D11Debug* d3dDebug;
	HRESULT hr = m_pDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&d3dDebug));
	if (SUCCEEDED(hr))
	{
		OutputDebugStringW(L"----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- \r ");
		OutputDebugStringW(L"                                                                    D3D11 Live Object ref Count Checker \r ");
		OutputDebugStringW(L"----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- \r ");

		hr = d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);

		OutputDebugStringW(L"----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- \r ");
		OutputDebugStringW(L"                                                                    D3D11 Live Object ref Count Checker END \r ");
		OutputDebugStringW(L"----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- \r ");
	}
	if (d3dDebug != nullptr)            d3dDebug->Release();
#endif

	Safe_Release(m_pDevice);

}
