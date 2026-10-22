#pragma once
#include "Core_Defines.h"
#include "Level.h"


NS_BEGIN(GameCore)

class CLevel_Editor final : public CLevel
{
private :
	CLevel_Editor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HWND hWnd);
	virtual ~CLevel_Editor() = default;

public:
	virtual HRESULT			Initialize() override;
	virtual void			Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;

public:
	//HRESULT					Ready_Lights();	
	HRESULT					Rendy_Layer_Sky(const _wstring& strLayerTag);
	HRESULT					Ready_Layer_Camera(const _wstring& strLayerTag);

private :
	_bool					m_bIsFirst = { true };

public:
	static CLevel_Editor*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HWND hWnd);
	virtual void			Free() override;
};

NS_END