#pragma once
#include "Base.h"
#include "Core_Defines.h"

NS_BEGIN(Engine)
class CGameInstance;
class CMasterModel;
NS_END

NS_BEGIN(GameCore)
class CLegionArm;
class CGrappleGun;
class CChargeGun;
class CPlayer;

class GAMECORE_DLL CLegionArm_Manager final :public CBase
{
public:
	typedef struct tagLegionArmMgrDesc
	{
		CMasterModel*		pMasterModel = { nullptr };
		const _float4x4*	pParentMatrix = { nullptr };
		CPlayer*			pOwner = { nullptr };
		_uint				iLevelIndex = { 0 };
	} LEGIONARM_MGR_DESC;

private:
	CLegionArm_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CLegionArm_Manager() = default;
public:
	HRESULT					Initialize(const LEGIONARM_MGR_DESC& Desc);
	void					Update(_float fTimeDelta);
	HRESULT					Render();
	HRESULT					Render_Shadow();
	//본 행렬 갱신 후 호출. 최종 본 위치가 필요한 처리용 
	void					Post_BoneUpdate(_float fTimeDelta);

public:
	void					On_AnimEvent(LegionArmEvent eEvent);
	LegionArmType			Get_CurrentArmType() const { return m_eCurrent; }
	CLegionArm*				Get_CurrentArm() const { return m_pArms[ETOUI(m_eCurrent)]; }
	void					Set_Dissolve(_float fAmount, const _float4& vColor);

public:
	CGrappleGun*			Get_GrappleGun() const;
	CChargeGun*				Get_ChargeGun() const;
	_bool					Is_Arm_Upgraded(LegionArmType eType) const;
public:
	void					Change_Arm(LegionArmType eType);
	void					Use_CurrentArm();
	void					Use_Upgrade_CurrentArm();
	_bool					Upgrade_Arm(LegionArmType eType);

	void					Cancel_CurrentArm();
private:
	CLegionArm*				m_pArms[ETOUI(LegionArmType::End)] = {};
	LegionArmType			m_eCurrent = { LegionArmType::ProtoArm };

	ID3D11Device*			m_pDevice = { nullptr };
	ID3D11DeviceContext*	m_pContext = { nullptr };
	CGameInstance*			m_pGameInstance = { nullptr };
public:
	static CLegionArm_Manager* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const LEGIONARM_MGR_DESC& Desc);
	virtual void			Free() override;

};

NS_END