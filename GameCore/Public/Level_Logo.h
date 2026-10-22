#pragma once
#include "Core_Defines.h"
#include "Level.h"
#include "IListener.h"
#include "UI_RuntimeData.h"

NS_BEGIN(GameCore)
class CChapterMapManager;

class GAMECORE_DLL CLevel_Logo final : public CLevel, public IListener
{
private:
	CLevel_Logo(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HWND hWnd);
	virtual ~CLevel_Logo() = default;

public:
	virtual HRESULT Initialize(LEVEL eNextLevelID);
	virtual void    Update(_float fTimeDelta) override;
	virtual void    Late_Update(_float fTimeDelta) override;
	virtual HRESULT Render() override;
	virtual void	OnEvent(EVENT_TYPE eType, Event_Desc* pData) override;

public:
	virtual void	Level_Exit() override;

private:
	HRESULT			Ready_UI();
	void			Request_StartGame();
	HRESULT         Ready_Camera();
	HRESULT         Ready_Layer_Sky(const _wstring& strLayerTag);
	HRESULT         Ready_LogoPrototypes();
	HRESULT         Ready_LogoChapter();

private:
	CChapterMapManager* m_pChapterMapManager = { nullptr };

private:
	UI_HANDLE		m_hTitleUI = {};
	LEVEL			m_eNextLevelID = { LEVEL::END };
	_bool			m_bTransitionRequested = { false };

public:
	static CLevel_Logo* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eNextLevelID, HWND hWnd);
	virtual void Free() override;
};

NS_END
