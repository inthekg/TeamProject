#include "AnimationPlayer.h"
#include "Engine_Utility.h"

CAnimationPlayer::CAnimationPlayer(CAnimation* pAnimation, _uint iNumBones)
	: m_pAnimation(pAnimation)
	, m_iNumBones(iNumBones)	
{

}

HRESULT CAnimationPlayer::Initailize(POSE* pBasePose)
{
	if (pBasePose == nullptr) return E_FAIL;
	
	m_FinalPose = *pBasePose;
	m_CurKeyFrameIndices.resize(m_iNumBones);

	_vector vScale, vRot, vTrans;	
	m_pAnimation->Get_RootTransform(0, m_CurKeyFrameIndices, vScale, vRot, vTrans);	

	XMStoreFloat4(&m_firstRootTransform.vTranslation, vTrans);
	XMStoreFloat4(&m_firstRootTransform.vScale, vScale);
	XMStoreFloat4(&m_firstRootTransform.vRot, vRot);

	m_prevRootTransform = m_firstRootTransform;
	m_curRootTransform = m_firstRootTransform;

	return S_OK;
}

void CAnimationPlayer::Play(_float fTimeDelta)
{
	if (m_pAnimation == nullptr) return;
	if (m_bEnd) return;

	m_fCurTrackPosition += m_pAnimation->Get_TickPerSecond() * fTimeDelta;

	if(m_pAnimation->Get_Duration() <= m_fCurTrackPosition)
	{
		if (m_pAnimation->IsLoop())
		{
			m_fCurTrackPosition = fmodf(m_fCurTrackPosition, m_pAnimation->Get_Duration());
		}
		else
		{
			m_bEnd = true;
			m_fCurTrackPosition = m_pAnimation->Get_Duration();
		}
	}	
}

void CAnimationPlayer::Play_FromStart(_float fTimeDelta)
{
	m_bEnd = false;
	m_bCanEnd = false;
	m_fCurTrackPosition = 0.f;
	Play(fTimeDelta);
}

void CAnimationPlayer::Get_CurrentPose(POSE& OutPose)
{
	if (m_pAnimation == nullptr) return;
	m_pAnimation->Get_Pose(OutPose, m_fCurTrackPosition, m_CurKeyFrameIndices);
}

void CAnimationPlayer::Get_BlendPose(POSE& BlendPose, _float fBlendFactor, POSE& OutPose)
{
	if (m_pAnimation == nullptr) return;
	fBlendFactor = fClamp(fBlendFactor, 0.f, 1.f);

	m_pAnimation->Get_Pose(m_FinalPose, m_fCurTrackPosition, m_CurKeyFrameIndices);

	for(_uint i = 0; i < m_iNumBones; i++)
	{
		XMStoreFloat4(&OutPose[i].vScale, XMVectorLerp(XMLoadFloat4(&BlendPose[i].vScale), XMLoadFloat4(&m_FinalPose[i].vScale), fBlendFactor));
		XMStoreFloat4(&OutPose[i].vRot, XMQuaternionSlerp(XMLoadFloat4(&BlendPose[i].vRot), XMLoadFloat4(&m_FinalPose[i].vRot), fBlendFactor));
		XMStoreFloat4(&OutPose[i].vTranslation, XMVectorLerp(XMLoadFloat4(&BlendPose[i].vTranslation), XMLoadFloat4(&m_FinalPose[i].vTranslation), fBlendFactor));
	}
}

_float CAnimationPlayer::Get_FrameRatio()
{
	if (!m_pAnimation) return 0.f;
	return m_fCurTrackPosition / m_pAnimation->Get_Duration();
}


void CAnimationPlayer::Set_FramePosition(_float fFramePosition)
{
	if (m_pAnimation == nullptr) return;
	m_bEnd = false;
	if(m_pAnimation->Get_Duration() <= fFramePosition)
	{
		if (m_pAnimation->IsLoop())
		{
			fFramePosition = fmodf(fFramePosition, m_pAnimation->Get_Duration());
		}
		else
		{
			m_bEnd = true;
			fFramePosition = m_pAnimation->Get_Duration();
		}
	}
	m_fCurTrackPosition = fFramePosition;	
	m_fPrevTrackPosition = fFramePosition;	
}

void CAnimationPlayer::Set_FramePosition_Ratio(_float fRatio)
{
	if (m_pAnimation == nullptr) return;
	if (fRatio < 0.f || fRatio > 1.f) return;

	m_fCurTrackPosition = fRatio * m_pAnimation->Get_Duration();

}

void CAnimationPlayer::Compute_RootDeltaTransform(TransformData& OutRootTransform)
{
	_vector curScale , curRot, curTrans;
	m_pAnimation->Get_RootTransform(m_fCurTrackPosition, m_CurKeyFrameIndices, curScale, curRot, curTrans);

	_vector prevTrans = XMLoadFloat4(&m_prevRootTransform.vTranslation);
	_vector prevRot = XMLoadFloat4(&m_prevRootTransform.vRot);
	OutRootTransform.vScale = _float4(1.f, 1.f, 1.f, 1.f);	

	//루프했다면 
	if (m_fCurTrackPosition < m_fPrevTrackPosition)
	{
		_vector startTrans, startRot;
		startTrans = XMLoadFloat4(&m_firstRootTransform.vTranslation);
		startRot = XMLoadFloat4(&m_firstRootTransform.vRot);

		_vector endTrans, endRot, endScale;
		m_pAnimation->Get_RootTransform(m_pAnimation->Get_Duration(), m_CurKeyFrameIndices, endScale, endRot, endTrans);

		_vector finalTrans = (endTrans - prevTrans) + (curTrans - startTrans);
		XMStoreFloat4(&OutRootTransform.vTranslation, finalTrans);	


		_vector deltaRotToEnd = XMQuaternionMultiply(endRot, XMQuaternionInverse(prevRot));
		_vector deltaRotFromStart = XMQuaternionMultiply(curRot, XMQuaternionInverse(startRot));
		_vector finalRot = XMQuaternionMultiply(deltaRotFromStart, deltaRotToEnd);
		XMStoreFloat4(&OutRootTransform.vRot, finalRot);	
	}
	else
	{
		_vector finalTrans = curTrans - prevTrans;
		XMStoreFloat4(&OutRootTransform.vTranslation, finalTrans);

		_vector finalRot = XMQuaternionMultiply(curRot, XMQuaternionInverse(prevRot));
		XMStoreFloat4(&OutRootTransform.vRot, finalRot);
	}


	XMStoreFloat4(&m_prevRootTransform.vRot, curRot);
	XMStoreFloat4(&m_prevRootTransform.vTranslation, curTrans);
	//Scale은 1

	m_fPrevTrackPosition = m_fCurTrackPosition;
}


CAnimationPlayer* CAnimationPlayer::Create(CAnimation* pAnimation, _uint iNumBones, POSE* pBasePose)
{
	CAnimationPlayer* pInstance = new CAnimationPlayer(pAnimation, iNumBones);
	if (FAILED(pInstance->Initailize(pBasePose)))
	{
		MSG_BOX("Animaiton Create Failed");
		Safe_Release(pInstance);
		return nullptr;
	}
	return pInstance;
}

void CAnimationPlayer::Free()
{
	__super::Free();
}
