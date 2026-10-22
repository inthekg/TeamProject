#include "Weapon_Melee.h"
#include "GameInstance.h"
#include "Model.h"
#include "AIObject.h"
#include "HitUtil.h"
#include "Effect.h"

#ifdef _DEBUG
#include "DebugCapsule.h"
BEGIN_REFLECTION(CWeapon_Melee, CWeapon_Monster)
ADD_PROPERTY_SETTER(CWeapon_Melee, m_fSweepRadius, Float, Set_SweepRadius, _float)
ADD_PROPERTY_SETTER(CWeapon_Melee, m_fSweepHalfHeight, Float, Set_SweepHeight, _float)
END_REFLECTION(CWeapon_Melee)
#endif // _DEBUG



CWeapon_Melee::CWeapon_Melee(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CWeapon_Monster{ pDevice, pContext }
{
}

CWeapon_Melee::CWeapon_Melee(const CWeapon_Melee& Prototype)
    : CWeapon_Monster{ Prototype }
{
}

HRESULT CWeapon_Melee::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CWeapon_Melee::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    auto pDesc = static_cast<MELEE_DESC*>(pArg);
    m_fSweepRadius = pDesc->fSweepRadius;
    m_fSweepHalfHeight = pDesc->fSweepHalfHeight;
    m_bTrailUse = pDesc->bTrailUse;
    if (false == pDesc->strTrailTopBoneName.empty())
        m_iTrailTopBoneIndex = m_pModelCom->Get_BoneIndex(pDesc->strTrailTopBoneName.c_str());
    if (false == pDesc->strTrailBottomBoneName.empty())
        m_iTrailBottomBoneIndex = m_pModelCom->Get_BoneIndex(pDesc->strTrailBottomBoneName.c_str());

    m_strTrailTop = pDesc->strTrailTopBoneName;
    m_strTrailBot = pDesc->strTrailBottomBoneName;

    if (false == pDesc->strTrailFxTag.empty())
        m_strTrailFx = pDesc->strTrailFxTag;

#ifdef _DEBUG
    Rebuild_DebugCapsule();
#endif
    return S_OK;
}

void CWeapon_Melee::Priority_Update(_float fTimeDelta)
{
}

void CWeapon_Melee::Update(_float fTimeDelta)
{
}

void CWeapon_Melee::Physical_Update(_float fFixedTime)
{
}

void CWeapon_Melee::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);

#ifdef _DEBUG
    if (true == m_pGameInstance->Get_DebugFlag())
    {
        if (m_fSweepRadius != m_fLastDbgRadius || m_fSweepHalfHeight != m_fLastDbgHalfHeight)
            Rebuild_DebugCapsule();

        _matrix BladeTop, BladeBot;
        if (nullptr != m_pDebugCapsule && SUCCEEDED(Get_TrailMatrix(BladeTop, BladeBot)))
        {
            const _vector vTop = BladeTop.r[3];
            const _vector vBot = BladeBot.r[3];
            const _vector vCenter = (vTop + vBot) * 0.5f;

            _vector vBladeDir = vTop - vBot;
            if (IsEqual(XMVectorGetX(XMVector3LengthSq(vBladeDir)), 0.f))
                vBladeDir = XMVectorSet(0.f, 1.f, 0.f, 0.f);
            vBladeDir = XMVector3Normalize(vBladeDir);

            const _vector vAxisX = XMVectorSet(1.f, 0.f, 0.f, 0.f);
            const _float  fDot = XMVectorGetX(XMVector3Dot(vAxisX, vBladeDir));
            _vector vQuat;
            if (IsEqual(fDot, 1.f))
                vQuat = XMQuaternionIdentity();
            else if (IsEqual(fDot, -1.f))
                vQuat = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), XM_PI);
            else
            {
                const _vector vAxis = XMVector3Normalize(XMVector3Cross(vAxisX, vBladeDir));
                vQuat = XMQuaternionRotationAxis(vAxis, acosf(fDot));
            }

            m_pDebugCapsule->Update_Pose(vCenter, vQuat);
            m_pDebugCapsule->Late_Update(fTimeDelta);
        }
    }
#endif
}

HRESULT CWeapon_Melee::Render()
{
    if (FAILED(__super::Render()))
        return E_FAIL;

    return S_OK;
}

void CWeapon_Melee::Attack_Begin(_float fDamage, HIT_TYPE eType)
{
    m_eHitType = eType;
    m_bCollisionActive = true;
    m_bPrevCenterValid = false;
    m_bHitSuccess = false;
    m_CollisionTargets.clear();
    if (fDamage >= 0.f)
        m_fDamage = fDamage;
    if (m_bTrailUse)
        Start_Trail();
}

void CWeapon_Melee::Attack_End()
{
    m_bCollisionActive = false;
    if (m_bTrailUse)
        Stop_Trail();
}

void CWeapon_Melee::Update_Attack(_float fTimeDelta)
{
    _matrix BladeTop, BladeBot;
    if (FAILED(Get_TrailMatrix(BladeTop, BladeBot)))
    {
        m_bPrevCenterValid = false;
        Stop_GroundSpark();
        return;
    }

    const _vector vTop = BladeTop.r[3];
    const _vector vBot = BladeBot.r[3];
    const _vector vCenter = (vTop + vBot) * 0.5f;

    const _vector vBladeVec = vTop - vBot;
    const _float  fBladeLength = XMVectorGetX(XMVector3Length(vBladeVec));

    _vector vBladeDir = vBladeVec;
    if (IsEqual(XMVectorGetX(XMVector3LengthSq(vBladeDir)), 0.f))
        vBladeDir = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    vBladeDir = XMVector3Normalize(vBladeDir);

    const _vector vAxisX = XMVectorSet(1.f, 0.f, 0.f, 0.f);
    const _float  fDot = XMVectorGetX(XMVector3Dot(vAxisX, vBladeDir));
    _vector vQuat;
    if (IsEqual(fDot, 1.f))
        vQuat = XMQuaternionIdentity();
    else if (IsEqual(fDot, -1.f))
        vQuat = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), XM_PI);
    else
    {
        const _vector vAxis = XMVector3Normalize(XMVector3Cross(vAxisX, vBladeDir));
        vQuat = XMQuaternionRotationAxis(vAxis, acosf(fDot));
    }

    CollisionResult tGroundSweep{};
    const _bool bSweepTouch = m_pGameInstance->Sweep_Capsule(m_fSweepRadius, m_fSweepHalfHeight, vCenter, vQuat,
        XMVectorSet(0.f, -1.f, 0.f, 0.f), 0.01f, FILTER_GROUP::GROUND, tGroundSweep, PxQueryFlag::eSTATIC);

    Update_GroundSpark(fTimeDelta, vBot, vBladeDir, fBladeLength, bSweepTouch);

    if (false == m_bPrevCenterValid)
    {
        XMStoreFloat3(&m_vPrevCenter, vCenter);
        m_bPrevCenterValid = true;
        return;
    }

    const _vector vPrev = XMLoadFloat3(&m_vPrevCenter);
    XMStoreFloat3(&m_vPrevCenter, vCenter);

    if (false == m_bCollisionActive)
        return;

    HitInfo tHit;
    tHit.fDamage = m_fDamage;
    tHit.pOtherActor = m_pOwner;
    tHit.pDamangeCauser = this;
    tHit.eHitType = m_eHitType;

    _bool bDamaged = false;

    const _float fMove = XMVectorGetX(XMVector3Length(vCenter - vPrev));
    if (fMove <= 1e-4f)
    {
        CHitUtil::Overlap_Sphere(m_pGameInstance, m_fSweepRadius + m_fSweepHalfHeight, vCenter,
            tHit, m_CollisionTargets, m_eTargetGroup, &bDamaged);

        if (true == bDamaged)
            m_bHitSuccess = true;

        return;
    }

    CHitUtil::Sweep_Capsule(m_pGameInstance, m_fSweepRadius, m_fSweepHalfHeight,
        vPrev, vCenter, vQuat, tHit, m_CollisionTargets, m_eTargetGroup, &bDamaged);

    if (true == bDamaged)
        m_bHitSuccess = true;
}


HRESULT CWeapon_Melee::Get_TrailMatrix(_matrix& OutTop, _matrix& OutBottom)
{
    if (m_iTrailTopBoneIndex == -1 || m_iTrailBottomBoneIndex == -1)
        return E_FAIL;

    if (FAILED(Get_BoneWorldMatrix(m_iTrailTopBoneIndex, OutTop)))
        return E_FAIL;
    if (FAILED(Get_BoneWorldMatrix(m_iTrailBottomBoneIndex, OutBottom)))
        return E_FAIL;

    return S_OK;
}

void CWeapon_Melee::Start_Trail()
{
    if (nullptr != m_pTrail || nullptr == m_pModelCom || nullptr == m_pTransformCom)
        return;

    if (m_strTrailTop.empty() || m_strTrailBot.empty() || m_strTrailFx.empty())
        return;

    const _float4x4* pTop = m_pModelCom->Get_Bone_CombinedMatPtr(m_strTrailTop.c_str());
    const _float4x4* pBot = m_pModelCom->Get_Bone_CombinedMatPtr(m_strTrailBot.c_str());
    if (nullptr == pTop || nullptr == pBot)
        return;

    m_pTrail = m_pGameInstance->Play_Effect(m_strTrailFx, pTop, pBot, Get_CombinedWorldMatrixPtr());
}

void CWeapon_Melee::Stop_Trail()
{
    if (nullptr != m_pTrail)
    {
        m_pTrail->Stop_Follow();
        m_pTrail->Stop();
        m_pTrail = nullptr;
    }
}

#ifdef _DEBUG
void CWeapon_Melee::Rebuild_DebugCapsule()
{
    Safe_Release(m_pDebugCapsule);

    CDebugCapsule::DEBUGCAPSULE_OBJ_DESC DebugDesc{};
    DebugDesc.fRadius = m_fSweepRadius;
    DebugDesc.fHalfHeight = m_fSweepHalfHeight;
    DebugDesc.iSegment = 10;
    DebugDesc.vColor = _float4{ 1.f, 0.f, 0.f, 1.f };
    m_pDebugCapsule = CDebugCapsule::Create(m_pDevice, m_pContext, DebugDesc);

    m_fLastDbgRadius = m_fSweepRadius;
    m_fLastDbgHalfHeight = m_fSweepHalfHeight;
}
#endif // _DEBUG

CWeapon_Melee* CWeapon_Melee::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CWeapon_Melee* pInstance = new CWeapon_Melee(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CWeapon_Melee");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CWeapon_Melee::Clone(void* pArg)
{
    CWeapon_Melee* pInstance = new CWeapon_Melee(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CWeapon_Melee");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CWeapon_Melee::Free()
{
    Stop_Trail();

    __super::Free();

#ifdef _DEBUG
    Safe_Release(m_pDebugCapsule);
#endif
}
