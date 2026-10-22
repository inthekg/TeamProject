#pragma once

#include "Core_Defines.h"
#include "GameObject.h"

#ifdef _DEBUG

NS_BEGIN(Engine)
class CShader;
class CGameInstance;
NS_END

NS_BEGIN(GameCore)

class CChapterMapManager;
class CNavigationDebugLineBuffer;

class CNavigationDebugRenderer final : public Engine::CGameObject
{
private:
	CNavigationDebugRenderer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CNavigationDebugRenderer() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual void Late_Update(_float fTimeDelta) override;
	virtual HRESULT Render() override;
	virtual Engine::CGameObject* Clone(void* pArg) override { return nullptr; }

public:
	void Toggle_Enable() { m_bEnable = false == m_bEnable; }
	void Set_Enable(_bool bEnable) { m_bEnable = bEnable; }
	_bool Is_Enable() const { return m_bEnable; }
	HRESULT Update_DebugLines(CChapterMapManager* pChapterMapManager);
	HRESULT Set_Lines(const vector<Engine::VTXCOLOR>& Lines);

private:
	HRESULT Ready_Components();

private:
	CNavigationDebugLineBuffer* m_pVIBufferCom = { nullptr };
	Engine::CShader* m_pShaderCom = { nullptr };
	Engine::CGameInstance* m_pGameInstance = { nullptr };
	_bool m_bEnable = { false };

public:
	static CNavigationDebugRenderer* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void Free() override;
};

NS_END

#endif