#pragma once
#include "Core_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CShader;
class CModel;
class CAnimationPlayer;
NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CFacial_Test : public CGameObject
{
protected:
	CFacial_Test(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CFacial_Test(const CFacial_Test& Prototype);
	virtual ~CFacial_Test() = default;

public:
	virtual HRESULT         Initialize_Prototype() override;
	virtual HRESULT         Initialize(void* pArg) override;
	virtual void            Priority_Update(_float fTimeDelta) override;
	virtual void            Update(_float fTimeDelta) override;
	virtual void            Physical_Update(_float fFixedTime) override;
	virtual void            Late_Update(_float fTimeDelta) override;
	virtual HRESULT         Render();

protected:
	HRESULT					Ready_Components();

protected:
	CShader* m_pShaderCom = { nullptr };
	CModel* m_pModelCom = { nullptr };

	//TEST용
	unordered_map<_uint64, CAnimationPlayer*> m_AnimationPlayers;
	_uint					m_iMaxAnimation = {};
	CAnimationPlayer* m_pCurrentAnimationPlayer = { nullptr };
	POSE 					m_CurrentPose;
public:
	static CFacial_Test* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg);
	virtual void            Free();

};

NS_END