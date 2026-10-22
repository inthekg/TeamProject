#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL CLevel abstract : public CBase
{
protected:
	CLevel(ID3D11Device * pDevice, ID3D11DeviceContext * pContext, HWND hWnd);
	virtual ~CLevel() = default;

public:
	virtual HRESULT			Initialize();
	virtual void			Update(_float fTimeDelta);
	virtual void			Late_Update(_float fTimeDelta);
	virtual HRESULT			Render();

public:
	virtual void Level_Enter() {}
	virtual void Level_Exit() {}

protected:
	ID3D11Device*			m_pDevice		= { nullptr };
	ID3D11DeviceContext*	m_pContext		= { nullptr };
	class CGameInstance*	m_pGameInstance = { nullptr };

	HWND					m_hWnd			= { nullptr };

protected:
	_uint					m_iFrameCount	= {};
	_uint					m_iFPS			= {};
	_float					m_fTimeAcc		= {};
public:
	virtual void			Free() override;
};

NS_END