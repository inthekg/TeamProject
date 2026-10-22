#pragma once
#include "Core_Defines.h"
#include "PartObject.h"

NS_BEGIN(Engine)
class CModel;
NS_END

NS_BEGIN(GameCore)

class CAIObject;

class GAMECORE_DLL CMonster_Attach final : public CPartObject
{
public:
	typedef struct tagMonsterAttachDesc : public CPartObject::PARTOBJECT_DESC
	{
		CAIObject*			pOwner = { nullptr };
		_wstring			strModelTag = {};
		string				strSocketBoneName = {};
		_uint				iLevelID = {};
		_bool				bRender = { true };
		_bool				bShadow = { true };
	} ATTACH_DESC;

private:
	CMonster_Attach(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMonster_Attach(const CMonster_Attach& Prototype);
	virtual ~CMonster_Attach() = default;

public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Priority_Update(_float fTimeDelta) override;
	virtual void				Update(_float fTimeDelta) override;
	virtual void				Physical_Update(_float fFixedTime) override;
	virtual void				Late_Update(_float fTimeDelta) override;
	virtual HRESULT				Render() override;
	virtual HRESULT				Render_Shadow() override;

public:
	CAIObject* Get_Owner() const { return m_pOwner; }
	CModel* Get_Model() const { return m_pModelCom; }
	void						Set_Render(_bool bRender) { m_bRender = bRender; }
	void						Set_Shadow(_bool bShadow) { m_bShadow = bShadow; }

private:
	HRESULT						Ready_Components(void* pArg);

private:
	CAIObject* m_pOwner = { nullptr };
	CModel* m_pModelCom = { nullptr };
	const _float4x4* m_pSocketBoneMatrix = { nullptr };

	_bool						m_bRender = { true };
	_bool						m_bShadow = { true };

public:
	static CMonster_Attach* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void				Free() override;
};

NS_END
