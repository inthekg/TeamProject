#include "LegionArm_Manager.h"
#include "GameInstance.h"
#include "LegionArm.h"
#include "BaseArm.h"
#include "GrappleGun.h"
#include "ChargeGun.h"
#include "ItemTable.h"

CLegionArm_Manager::CLegionArm_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice{ pDevice }, m_pContext{ pContext }, m_pGameInstance{ CGameInstance::Get_Instance() }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

HRESULT CLegionArm_Manager::Initialize(const LEGIONARM_MGR_DESC& Desc)
{
    CLegionArm::LEGIONARM_DESC Arm_Desc = {};
	Arm_Desc.pMasterModel = Desc.pMasterModel;
	Arm_Desc.iLevelIndex = Desc.iLevelIndex;
	Arm_Desc.pOwner = Desc.pOwner;
    Arm_Desc.pParentMatrix = Desc.pParentMatrix;

    /* Base Arm (ProtoArm) */
    m_pArms[ETOUI(LegionArmType::ProtoArm)] = CBaseArm::Create(m_pDevice, m_pContext, &Arm_Desc);
    if (nullptr == m_pArms[ETOUI(LegionArmType::ProtoArm)])
        return E_FAIL;

    /* GrappleGun */
    m_pArms[ETOUI(LegionArmType::Grapple)] = CGrappleGun::Create(m_pDevice, m_pContext, &Arm_Desc);
    if (nullptr == m_pArms[ETOUI(LegionArmType::Grapple)])
        return E_FAIL;

    /* Charget Gun */
    m_pArms[ETOUI(LegionArmType::ChargeGun)] = CChargeGun::Create(m_pDevice, m_pContext, &Arm_Desc);
    if (nullptr == m_pArms[ETOUI(LegionArmType::ChargeGun)])
        return E_FAIL;

    for(_uint i=0; i< ETOUI(LegionArmType::End); ++i)
    {
        if (m_pArms[i] == nullptr) continue;

        const ITEM_DATA* pData = CItemTable::Find_ByArmType(static_cast<LegionArmType>(i));
        if (pData)
            m_pArms[i]->Set_ItemID(pData->iItemID);
	}

    m_eCurrent = LegionArmType::ProtoArm;

    return S_OK;
}

void CLegionArm_Manager::Update(_float fTimeDelta)
{
    CLegionArm* pArm = Get_CurrentArm();
    if (pArm)
        pArm->Update(fTimeDelta);
}

HRESULT CLegionArm_Manager::Render()
{
    CLegionArm* pArm = Get_CurrentArm();

    if(pArm == nullptr)
		return E_FAIL;
    
    pArm->Render();
    return S_OK;
}

HRESULT CLegionArm_Manager::Render_Shadow()
{
    CLegionArm* pArm = Get_CurrentArm();
    if (pArm == nullptr)
        return E_FAIL;

    pArm->Render_Shadow();
    return S_OK;
}

void CLegionArm_Manager::Post_BoneUpdate(_float fTimeDelta)
{
    CLegionArm* pArm = Get_CurrentArm();
    if (pArm)
        pArm->Post_BoneUpdate(fTimeDelta);
}

void CLegionArm_Manager::On_AnimEvent(LegionArmEvent eEvent)
{
	CLegionArm* pCurrentArm = Get_CurrentArm();
	if (pCurrentArm) pCurrentArm->On_AnimEvent(eEvent);
}

void CLegionArm_Manager::Set_Dissolve(_float fAmount, const _float4& vColor)
{
    if (CLegionArm* pArm = Get_CurrentArm())
        pArm->Set_Dissolve(fAmount, vColor);
}

CGrappleGun* CLegionArm_Manager::Get_GrappleGun() const
{
	return m_pArms[ETOUI(LegionArmType::Grapple)] ? static_cast<CGrappleGun*>(m_pArms[ETOUI(LegionArmType::Grapple)]) : nullptr;
}

CChargeGun* CLegionArm_Manager::Get_ChargeGun() const
{
    return m_pArms[ETOUI(LegionArmType::ChargeGun)] ? static_cast<CChargeGun*>(m_pArms[ETOUI(LegionArmType::ChargeGun)]) : nullptr;
}

_bool CLegionArm_Manager::Is_Arm_Upgraded(LegionArmType eType) const
{
	if (eType >= LegionArmType::End) return false;

    if (m_pArms[ETOUI(eType)]) 
        return m_pArms[ETOUI(eType)]->Is_Upgrade();
    return false;
}

void CLegionArm_Manager::Change_Arm(LegionArmType eType)
{
	if (eType >= LegionArmType::End || eType == m_eCurrent) return;
	if (m_pArms[ETOUI(eType)] == nullptr) return;    

    CLegionArm* pCurrentArm = Get_CurrentArm();
    //사용중이면 교체 X 
    if (pCurrentArm && pCurrentArm->Is_Used()) return; 

	m_eCurrent = eType;
}

void CLegionArm_Manager::Use_CurrentArm()
{
    CLegionArm* pCurrentArm = Get_CurrentArm();
    if (pCurrentArm) pCurrentArm->Use();
}

void CLegionArm_Manager::Use_Upgrade_CurrentArm()
{
    CLegionArm* pCurrentArm = Get_CurrentArm();
    if (pCurrentArm)
    {
        if (pCurrentArm->Is_Upgrade())
            pCurrentArm->Use_Upgrade();
        else pCurrentArm->Use(); // 강화안됐으면 일반버전 
    }
}

_bool CLegionArm_Manager::Upgrade_Arm(LegionArmType eType)
{
    if (eType >= LegionArmType::End || m_pArms[ETOUI(eType)] == nullptr || m_pArms[ETOUI(eType)]->Is_Upgrade()) return false;
    m_pArms[ETOUI(eType)]->Upgrade();
    return true;
}

void CLegionArm_Manager::Cancel_CurrentArm()
{
    CLegionArm* pCurrentArm = Get_CurrentArm();
	if (pCurrentArm) pCurrentArm->Cancel();
}

CLegionArm_Manager* CLegionArm_Manager::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const LEGIONARM_MGR_DESC& Desc)
{
	CLegionArm_Manager* pInstance = new CLegionArm_Manager(pDevice, pContext);
    if(FAILED(pInstance->Initialize(Desc)))
    {
        MSG_BOX("Failed to Created : CLegionArm_Manager");
        Safe_Release(pInstance);
	}
	return pInstance;
}


void CLegionArm_Manager::Free()
{
    for(_uint i=0; i < ETOUI(LegionArmType::End); ++i)
    {
        Safe_Release(m_pArms[i]);
	}
    Safe_Release(m_pContext);
    Safe_Release(m_pDevice);
    __super::Free();
}
