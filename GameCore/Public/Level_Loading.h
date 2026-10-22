#pragma once
#include "Core_Defines.h"
#include "Level.h"
#include "UI_RuntimeData.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CLevel_Loading final : public CLevel
{
private:
	CLevel_Loading(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HWND hWnd);
	virtual ~CLevel_Loading() = default;

public:
	virtual HRESULT Initialize(LEVEL eNextLevelID);
	virtual void	Update(_float fTimeDelta) override;
	virtual void	Late_Update(_float fTimeDelta) override;
	virtual HRESULT Render() override;

private:
	HRESULT			Ready_Layer_BackGround(const _wstring& strLayerTag);

private:
	LEVEL			m_eNextLevelID = { LEVEL::END };
	class CLoader*	m_pLoader = { nullptr };
	UI_HANDLE       m_hLoadingUI = {};

public:
	static CLevel_Loading* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eNextLevelID, HWND hWnd);
	virtual void Free() override;
};

NS_END