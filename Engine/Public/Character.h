#pragma once
#include "ContainerObject.h"

NS_BEGIN(Engine)
class ENGINE_DLL CCharacter abstract : public CContainerObject
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CCharacter)
#endif // _DEBUG

protected:
	CCharacter(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CCharacter(const CCharacter& Prototype);
	virtual ~CCharacter() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Physical_Update(_float fFixedTime) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;
	
public:
	virtual void			Set_InActive() override;
public:
	class CModel*			Get_Model() { return m_pModelCom; }
	HRESULT					Get_BoneWorldMatrix(_uint iBoneIndex, _matrix& OutWorldMatrix);
	const _float4x4*		Get_Bone_CombinedMatPtr(_uint iBoneIndex);
	const _float4x4*		Get_Bone_CombinedMatPtr(const _char* pBoneName);

	string 					Get_RightHandBoneName() const { return m_strRightHand_BoneName; }
	void					Set_RightHandBoneName(const string& strBoneName) { m_strRightHand_BoneName = strBoneName; }

	ACTOR_TYPE				Get_ActorType() const { return m_eActorType; }	// 추가
	string 					Get_LeftHandBoneName() const { return m_strLeftHand_BoneName; }
	void					Set_LeftHandBoneName(const string& strBoneName) { m_strLeftHand_BoneName = strBoneName; }

	_float					Get_GravityWeight() const { return m_fGravityWeight; }
	void					Set_GravityWeight(_float fWeight) { m_fGravityWeight = fWeight; }

	/* Lock & Head Bone */
	HRESULT					Get_LockOnWorldMatrix(_matrix& OutWorld);
	_bool					Get_LockOnWorldPosition(_vector& OutPos);
	HRESULT					Get_HeadWorldMatrix(_matrix& OutWorld);
	_bool					Get_HeadWorldPosition(_vector& OutPos);

protected:
	//하위에서 Create하기 
	class CModel*			m_pModelCom = { nullptr };
	string					m_strRightHand_BoneName = {};
	string					m_strLeftHand_BoneName = {};
	ACTOR_TYPE 				m_eActorType = { ACTOR_TYPE::END };
	_float					m_fGravityWeight = { 1.f };
public:
	virtual CGameObject*	Clone(void* pArg) PURE;
	virtual void			Free();
};

NS_END