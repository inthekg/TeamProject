#include "AimBoneChain.h"
#include "Model.h"
#include "Bone.h"
#include "Transform.h"


HRESULT CAimBoneChain::Initialize(CModel* pModel, CTransform* pOwnerTransform, const AIM_PROFILE& Profile)
{
    if (pModel == nullptr || pOwnerTransform == nullptr)
    {
        MSG_BOX("CAimBoneChain : Model or OwnerTransform is nullptr");
        return E_FAIL;
    }

    m_pModel = pModel;
    m_pOwnerTransform = pOwnerTransform;

    if(FAILED(Validate_Profile(Profile)))
        return E_FAIL;

    if (FAILED(Mapping_Bones(Profile)))
        return E_FAIL;

    m_fMaxYaw = Profile.fMaxYaw;
    m_fMaxPitchUp = Profile.fMaxPitchUp;
    m_fMaxPitchDown = Profile.fMaxPitchDown;
    m_fBlendTime = Profile.fBlendTime;
    m_fDirFollowRate = Profile.fDirFollowRate;

    _vector vForward = XMVector3Normalize(XMLoadFloat3(&Profile.vModelForward));
    _vector vRight = XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&Profile.vModelUp), vForward));
    _vector vUp = XMVector3Cross(vForward, vRight);

    XMStoreFloat3(&m_vModelForward, vForward);
    XMStoreFloat3(&m_vModelRight, vRight);
    XMStoreFloat3(&m_vModelUp, vUp);

    return S_OK;
}


HRESULT CAimBoneChain::Validate_Profile(const AIM_PROFILE& Profile) const
{
    if (Profile.Bones.empty())
    {
        MSG_BOX("CAimBoneChain : Bone list is empty");
        return E_FAIL;
    }

    _float fWeightSum = 0.f;
    for (const auto& AimBone : Profile.Bones)
    {
        if (AimBone.szBoneName == nullptr || AimBone.fWeight < 0.f)
        {
            MSG_BOX("CAimBoneChain : Invalid BoneName or Weight");
            return E_FAIL;
        }
        fWeightSum += AimBone.fWeight;
    }

    if (fWeightSum <= 0.f)
    {
        MSG_BOX("CAimBoneChain : Sum of weights must be greater than 0");
        return E_FAIL;
    }

    if (Profile.fMaxYaw < 0.f || Profile.fMaxYaw >= XM_PI
        || Profile.fMaxPitchUp < 0.f || Profile.fMaxPitchUp >= XM_PIDIV2
        || Profile.fMaxPitchDown < 0.f || Profile.fMaxPitchDown >= XM_PIDIV2)
    {
        MSG_BOX("CAimBoneChain : Yaw must be in [0, 180), Pitch must be in [0, 90)");
        return E_FAIL;
    }

    if (Profile.fBlendTime < 0.f || Profile.fDirFollowRate < 0.f)
    {
        MSG_BOX("CAimBoneChain : BlendTime and DirFollowRate must not be negative");
        return E_FAIL;
    }
    
    //축이 0이거나 서로 평행하면 회전 기준축을 만들 수 없음 
    _vector vCross = XMVector3Cross(XMLoadFloat3(&Profile.vModelUp), XMLoadFloat3(&Profile.vModelForward));
    if (IsEqual(XMVectorGetX(XMVector3LengthSq(vCross)), 0.f))
    {
        MSG_BOX("CAimBoneChain : Invalid ModelForward and ModelUp");
        return E_FAIL;
    }

    return S_OK;
}

HRESULT CAimBoneChain::Mapping_Bones(const AIM_PROFILE& Profile)
{
    const vector<CBone*>& ModelBones = m_pModel->Get_Bones();
    const _int iNumBones = static_cast<_int>(ModelBones.size());

    _float fWeightSum = 0.f;
    for (const auto& AimBone : Profile.Bones)
        fWeightSum += AimBone.fWeight;

    m_ChainBones.clear();
    m_ChainBones.reserve(Profile.Bones.size());

    for (const auto& AimBone : Profile.Bones)
    {
        CHAIN_BONE chainBone = {};
        chainBone.iBoneIndex = m_pModel->Get_BoneIndex(AimBone.szBoneName);
        if (chainBone.iBoneIndex < 0 || chainBone.iBoneIndex >= iNumBones)
        {
            MSG_BOX("CAimBoneChain : Failed to Find Bone Name");
            return E_FAIL;
        }

        chainBone.iParentIndex = ModelBones[chainBone.iBoneIndex]->Get_ParentIndex();
        //부모 본을 기준으로 회전하므로 부모 없는 루트본은 체인에 넣을 수 없음 
        if (chainBone.iParentIndex < 0)
        {
            MSG_BOX("CAimBoneChain : Root Bone cannot be in Chain");
            return E_FAIL;
        }

        //같은 본이 중복으로 들어왔으면 FAIL 반환 
        _bool bDuplicate = any_of(m_ChainBones.begin(), m_ChainBones.end(), [&](const CHAIN_BONE& prevBone)
            {
                return prevBone.iBoneIndex == chainBone.iBoneIndex;
            });

        if (bDuplicate)
        {
            MSG_BOX("CAimBoneChain : Duplicate Bone in Chain");
            return E_FAIL;
        }

        //가중치 합이 1이 되도록 보정함
        chainBone.fWeight = AimBone.fWeight / fWeightSum;
        m_ChainBones.emplace_back(chainBone);
    }
    return S_OK;
}

_vector CAimBoneChain::Clamp_DirectionModel(_fvector vDirModel) const
{
    _vector vForward = XMLoadFloat3(&m_vModelForward);
    _vector vRight = XMLoadFloat3(&m_vModelRight);
    _vector vUp = XMLoadFloat3(&m_vModelUp);

    //투영하여 각 축 방향으로의 길이 연산 
    _float fX = XMVectorGetX(XMVector3Dot(vDirModel, vRight));
    _float fY = XMVectorGetX(XMVector3Dot(vDirModel, vUp));
    _float fZ = XMVectorGetX(XMVector3Dot(vDirModel, vForward));
    
    _float fYaw = atan2f(fX, fZ);
    _float fPitch = atan2f(fY, sqrtf(fX*fX + fZ*fZ)); 

    fYaw = fClamp(fYaw, -m_fMaxYaw, m_fMaxYaw);
    fPitch = fClamp(fPitch, -m_fMaxPitchDown, m_fMaxPitchUp);

    //기준 축 기준 벡터 재구성 
    _float fNewY = sinf(fPitch);
    _float fNewXZ = cosf(fPitch);
    _float fNewZ = fNewXZ * cosf(fYaw);
    _float fNewX = fNewXZ * sinf(fYaw);

    //Clamp된 새 벡터 반환 
    return vRight * fNewX + vUp * fNewY + vForward * fNewZ;
}

void CAimBoneChain::Update(_float fTimeDelta)
{
    _vector vTarget = XMLoadFloat3(&m_vTargetDirWorld);
    if (m_bActive)
    {
        _vector vTargetDir = XMVector3Normalize(vTarget);
        //보완전히 꺼진 상태거나 보간 속도가 0이면 바로 목표 방향으로 설정 
        if (m_fAimWeight <= 0.f || m_fDirFollowRate <= 0.f)
        {
            XMStoreFloat3(&m_vCurDirWorld, vTargetDir);
        }
        else
        {
            _float fRatio = 1.f - expf(-m_fDirFollowRate * fTimeDelta);
            _vector vNext = XMVector3Normalize(XMVectorLerp(XMLoadFloat3(&m_vCurDirWorld), vTargetDir, fRatio));
            XMStoreFloat3(&m_vCurDirWorld, vNext);
        }
    }

    //AimWeight(Aim 진행도) 블랜드
    _float fTargetWeight = m_bActive ? 1.f : 0.f;
    _float fWeightStep = (m_fBlendTime > 0.f) ? fTimeDelta / m_fBlendTime : 1.f;

    m_fAimWeight = MoveTowards(m_fAimWeight, fTargetWeight, fWeightStep);
}

void CAimBoneChain::Reset()
{
    m_bActive = false;
    m_fAimWeight = 0.f;
}

void CAimBoneChain::Apply(POSE& Pose) const
{
    //포즈와 모델 본 수가 다르면 인덱스 대응이 깨지므로 변형하지 않음
    if (Pose.size() != m_pModel->Get_Bones().size())
        return;

    //현재 보간중인 목표 방향(m_vCurDirWorld)을 월드 -> 모델 공간으로 변환 
    _vector vDirWorld = XMLoadFloat3(&m_vCurDirWorld);
    if (IsEqual(XMVectorGetX(XMVector3LengthSq(vDirWorld)), 0.f))
        return;

    _vector vOwnerQuat = m_pOwnerTransform->Get_Quat();
	_vector vModelDir = XMVector3Rotate(vDirWorld, XMQuaternionInverse(vOwnerQuat));
    
    //모델공간에서 방향 클램핑
    _vector vClampedDir = Clamp_DirectionModel(vModelDir);

	_vector vForward = XMVector3Normalize(XMLoadFloat3(&m_vModelForward));
	_float fDot = max(-1.f, min(1.f, XMVectorGetX(XMVector3Dot(vForward, vClampedDir))));
	_float fAngle = XMScalarACos(fDot);

    if (IsEqual(fAngle, 0.f)) return;

    //모델 공간 기준 공통 회전축. 각도 클랭핑때문에 정반대의 경우는 제외됨 
	_vector vAxisModel = XMVector3Cross(vForward, vClampedDir);
    vAxisModel = XMVector3Normalize(vAxisModel);

	fAngle *= SmoothStep(m_fAimWeight);

    for (const auto& chainBone : m_ChainBones)
    {
        //Model에 있는 부모Combined 행렬은 이전 데이터이므로 해당 부모 본만 이번 프레임 포즈로 계산 
        _matrix PrtCombined = m_pModel->Compute_CombinedFromPose(Pose, chainBone.iParentIndex);
        //공통 회전축을 부모 본 기준으로 변환한뒤 기준으로 잡고 회전함 
		_vector vPrtBoneAxis = XMVector3TransformNormal(vAxisModel, XMMatrixInverse(nullptr, PrtCombined));
		_vector vPrtQuat = XMQuaternionRotationAxis(vPrtBoneAxis, fAngle * chainBone.fWeight);

        _vector vLocalQuat = XMLoadFloat4(&Pose[chainBone.iBoneIndex].vRot);
        XMStoreFloat4(&Pose[chainBone.iBoneIndex].vRot, XMQuaternionNormalize(XMQuaternionMultiply(vLocalQuat, vPrtQuat)));
    }
}

CAimBoneChain* CAimBoneChain::Create(CModel* pModel, CTransform* pOwnerTransform, const AIM_PROFILE& Profile)
{
	CAimBoneChain* pInstance = new CAimBoneChain();
    if(FAILED(pInstance->Initialize(pModel, pOwnerTransform, Profile)))
    {
        MSG_BOX("Failed to Created : CAimBoneChain");
        Safe_Release(pInstance);
	}
    return pInstance;
}

void CAimBoneChain::Free()
{
    __super::Free();
}
