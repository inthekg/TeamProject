#pragma once

#include "Client_Defines.h"
#include "Base.h"

NS_BEGIN(Engine)
class CGameInstance;
NS_END


NS_BEGIN(Client)

class CMainApp final : public CBase
{
private:
	CMainApp();
	virtual ~CMainApp() = default;

public:
	HRESULT					Initialize();
	void					Update(_float fTimeDelta);
	HRESULT					Render();


private:
	ID3D11Device*			m_pDevice = { nullptr };
	ID3D11DeviceContext*	m_pContext = { nullptr };
	CGameInstance*			m_pGameInstance = { nullptr };



private:
	//HRESULT				Ready_Fonts();
	HRESULT					Ready_Prototype_For_Static();
	HRESULT					Start_Level(LEVEL eStartLevelID);



public:
	static CMainApp*		Create();
	virtual void			Free() override;
};

NS_END