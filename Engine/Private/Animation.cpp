#include "Animation.h"
#include "Channel.h"
#include "Engine_Utility.h"
#include "GameInstance.h"

CAnimation::CAnimation()
    :CResource{nullptr, nullptr}
{
}

HRESULT CAnimation::Initialize(AnimationReadData& readData, const vector<CChannel*>& Channels)
{
    m_strAnimationFileName = readData.strAnimationFileName;
    // 추가
    m_iAnimationFileHash = Hash_String(m_strAnimationFileName.c_str());

    if (m_strAnimationName.empty() || m_strAnimationName == L"Empty")
        m_strAnimationName = m_strAnimationFileName;

    m_strAnimationName = readData.strAnimationName;
    m_iAnimNameHash = Hash_String(m_strAnimationName.c_str());

    m_fDuration = readData.fDuration;
    m_fTickPerSecond = readData.fTickPerSecond;
    m_fBlendTime = readData.fBlendTime;
    m_bLoop = readData.bLoop;
    m_bRootMotion = readData.bRootMotion;

    m_iNumChannels = (_uint)Channels.size();
    m_Channels = Channels;

    return S_OK;
}

void CAnimation::Get_Pose(POSE& OutPoseData, _float fTrackPosition, vector<KeyFrameIndices>& vKeyFrameIndices)
{
    for (_uint i = 0; i < (_uint)m_iNumChannels; i++)
    {
        m_Channels[i]->Update_TransformationMatrix(OutPoseData, fTrackPosition, vKeyFrameIndices[i]);
    }
}

void CAnimation::Get_RootTransform(_float fTrackPosition, vector<KeyFrameIndices>& vKeyFrameIndices, _vector& vScale, _vector& vRot, _vector& vTrans)
{
    m_Channels[m_iRootChannelIndex]->Get_TransformationMatrix(fTrackPosition, vKeyFrameIndices[m_iRootChannelIndex], vScale, vRot, vTrans);
}

CAnimation* CAnimation::Create(AnimationReadData& readData, const vector<CChannel*>& Channels)
{
    CAnimation* pInstance = new CAnimation();

    if (FAILED(pInstance->Initialize(readData, Channels)))
    {
        MSG_BOX("Failed to Created : CAnimation");
        Safe_Release(pInstance);
    }

    return pInstance;
}


void CAnimation::Free()
{
    __super::Free();

    for (auto& pChannel : m_Channels)
        Safe_Release(pChannel);

    m_Channels.clear();
}