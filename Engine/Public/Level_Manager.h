#pragma once

#include "Base.h"


NS_BEGIN(Engine)

class CLevel_Manager final : public CBase
{
private:
	CLevel_Manager(HWND hWnd);
	virtual ~CLevel_Manager() = default;

public :
	_int					Get_CurrentLevelIndex() const { return m_iCurrentLevelIndex; }

public:
	void					Update(_float fTimeDelta);
	void					Late_Update(_float fTimeDelta);
	HRESULT					Render();
	HRESULT					Request_ChangeLevel(_int iNewLevelIndex, class CLevel* pNewLevel);

	_bool					Is_Changing() const { return m_bChangeFlag; }
	class CLevel*			Get_CurrentLevel() const { return m_pCurrentLevel; }
private:
	HRESULT					Change_Level();

private:
	class CGameInstance*	m_pGameInstance			= { nullptr };

	class CLevel*			m_pCurrentLevel			= { nullptr };
	_int					m_iCurrentLevelIndex	= { -1 };

	class CLevel*			m_pNextLevel			= { nullptr };
	_int					m_iNextLevelIndex		= { -1 };

	_bool					m_bChangeFlag			= {};

	HWND					m_hWnd;
public:
	static CLevel_Manager*	Create(HWND hWnd);
	virtual void			Free() override;
};

NS_END