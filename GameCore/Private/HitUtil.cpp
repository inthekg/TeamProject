#include "HitUtil.h"
#include "GameInstance.h"
#include "GameObject.h"

void CHitUtil::Make_CapsulePose(_fvector vTop, _fvector vBot, _vector& OutCenter, _vector& OutQuat)
{
    OutCenter = (vTop + vBot) * 0.5f;

    _vector vDir = vTop - vBot;
    const _float fLen = XMVectorGetX(XMVector3Length(vDir));
    _vector vBoneDir = (fLen > 1e-4f) ? XMVector3Normalize(vDir) : XMVectorSet(0.f, 1.f, 0.f, 0.f);

    const _vector vAxisX = XMVectorSet(1.f, 0.f, 0.f, 0.f);
    const _float  fDot = XMVectorGetX(XMVector3Dot(vAxisX, vBoneDir));
    if (IsEqual(fDot, 1.f))
        OutQuat = XMQuaternionIdentity();
    else if (IsEqual(fDot, -1.f))
        OutQuat = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), XM_PI);
    else
    {
        const _vector vA = XMVector3Normalize(XMVector3Cross(vAxisX, vBoneDir));
        OutQuat = XMQuaternionRotationAxis(vA, acosf(fDot));
    }
}

_bool CHitUtil::Apply_HitOnce(CGameObject* pTarget, HitInfo tHitInfo, _fvector vHitPos, _fvector vHitNormal, unordered_set<CGameObject*>& Targets, _bool* pOutDamaged)
{
    if (nullptr == pTarget)
        return false;
    if (Targets.find(pTarget) != Targets.end())
        return false;

    Targets.insert(pTarget);

    XMStoreFloat3(&tHitInfo.vHitPosition, vHitPos);
    XMStoreFloat3(&tHitInfo.vHitNormal, vHitNormal);

    const _bool bDamaged = pTarget->Take_Damage(tHitInfo);

    if (nullptr != pOutDamaged && true == bDamaged)
        *pOutDamaged = true;

    return true;
}

_bool CHitUtil::Sweep_Sphere(CGameInstance* pGI, _float fRadius, _fvector vPrev, _fvector vCur, const HitInfo& tHitInfo, unordered_set<CGameObject*>& Targets, FILTER_GROUP eTarget, _bool* pOutDamaged)
{
    if (nullptr == pGI)
    {
        MSG_BOX("Null GameInstance Ptr With Sweep_Sphere : CHitUtil");
        return false;
    }

    _vector vDir = vCur - vPrev;
    const _float fDist = XMVectorGetX(XMVector3Length(vDir));
    if (fDist <= 0.f)
        return false;
    vDir = XMVector3Normalize(vDir);

    CollisionResult tResult{};
    if (false == pGI->Sweep_Sphere(fRadius, vPrev, vDir, fDist, eTarget, tResult))
        return false;

    return Apply_HitOnce(tResult.pOtherActor, tHitInfo,
        XMLoadFloat3(&tResult.tSweepData.vHitPosition),
        XMLoadFloat3(&tResult.tSweepData.vHitNormal), Targets, pOutDamaged);
}

_bool CHitUtil::Sweep_Capsule(CGameInstance* pGI, _float fRadius, _float fHalfHeight, _fvector vPrev, _fvector vCur, _fvector vQuat, const HitInfo& tHitInfo, unordered_set<CGameObject*>& Targets, FILTER_GROUP eTarget, _bool* pOutDamaged)
{
    if (nullptr == pGI)
    {
        MSG_BOX("Null GameInstance Ptr With Sweep_Capsule : CHitUtil");
        return false;
    }

    _vector vDir = vCur - vPrev;
    const _float fDist = XMVectorGetX(XMVector3Length(vDir));
    if (fDist <= 0.f)
        return false;
    vDir = XMVector3Normalize(vDir);

    CollisionResult tResult{};
    if (false == pGI->Sweep_Capsule(fRadius, fHalfHeight, vPrev, vQuat, vDir, fDist, eTarget, tResult))
        return false;

    return Apply_HitOnce(tResult.pOtherActor, tHitInfo,
        XMLoadFloat3(&tResult.tSweepData.vHitPosition),
        XMLoadFloat3(&tResult.tSweepData.vHitNormal), Targets, pOutDamaged);
}

void CHitUtil::Overlap_Sphere(CGameInstance* pGI, _float fRadius, _fvector vCenter, const HitInfo& tHitInfo, unordered_set<CGameObject*>& Targets, FILTER_GROUP eTarget, _bool* pOutDamaged)
{
    if (nullptr == pGI)
    {
        MSG_BOX("Null GameInstance Ptr With Sweep_Capsule : CHitUtil");
        return;
    }

    vector<CGameObject*> Hits;
    if (false == pGI->Overlap_Sphere(fRadius, vCenter, eTarget, Hits))
        return;

    const _vector vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    for (CGameObject* pTarget : Hits)
        Apply_HitOnce(pTarget, tHitInfo, vCenter, vUp, Targets, pOutDamaged);
}
