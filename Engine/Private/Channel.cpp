#include "Channel.h"

CChannel::CChannel()
{
}

HRESULT CChannel::Initialize(const ChannelPassData& channelPassData)
{
	m_iBoneIndex = channelPassData.iBoneIndex;

	m_KeyFrames_Scale = channelPassData.ScaleKeyFrames;
	m_KeyFrames_Trans = channelPassData.TransKeyFrames;
	m_KeyFrames_Rot = channelPassData.RotKeyFrames;

	return S_OK;
}



void CChannel::Update_TransformationMatrix(POSE& PoseData, _float fCurrentTrackPosition
	, KeyFrameIndices& keyFrameIndices)
{
	_vector vScale = XMVectorSet(1.f, 1.f, 1.f, 0.f);
	_vector vRotation = XMQuaternionIdentity();
	_vector vTranslation = XMVectorZero();

	Get_TransformationMatrix(fCurrentTrackPosition, keyFrameIndices, vScale, vRotation, vTranslation);

	XMStoreFloat4(&PoseData[m_iBoneIndex].vScale, vScale);
	XMStoreFloat4(&PoseData[m_iBoneIndex].vRot, vRotation);
	XMStoreFloat4(&PoseData[m_iBoneIndex].vTranslation, vTranslation);

}

void CChannel::Get_TransformationMatrix(_float fCurrentTrackPosition, KeyFrameIndices& keyFrameIndices, _vector& vOutScale, _vector& vOutRot, _vector& vOutTrans)
{
	// Scale ---------------------------------------------------------------------

	if (m_KeyFrames_Scale.empty() == false)
	{
		if (keyFrameIndices.iScaleIndex >= m_KeyFrames_Scale.size() - 1)
			keyFrameIndices.iScaleIndex = 0;

		KEYFRAME_VEC        LastKeyFrameDesc = m_KeyFrames_Scale.back();

		//첫 키프레임 이전(0번에 데이터가 없으면서 단일 키프레임인 경우 대비)
		if (fCurrentTrackPosition <= m_KeyFrames_Scale.front().fTrackPosition)
		{
			keyFrameIndices.iScaleIndex = 0;
			vOutScale = XMLoadFloat3(&m_KeyFrames_Scale.front().vVector3);
		}
		//마지막 키프레임 이후 
		else if (fCurrentTrackPosition >= LastKeyFrameDesc.fTrackPosition)
		{
			vOutScale = XMLoadFloat3(&LastKeyFrameDesc.vVector3);
		}
		else
		{
			if (fCurrentTrackPosition < m_KeyFrames_Scale[keyFrameIndices.iScaleIndex].fTrackPosition)
				keyFrameIndices.iScaleIndex = 0;

			while (fCurrentTrackPosition >= m_KeyFrames_Scale[keyFrameIndices.iScaleIndex + 1].fTrackPosition)
				++(keyFrameIndices.iScaleIndex);

			_vector vSourScale = XMLoadFloat3(&m_KeyFrames_Scale[keyFrameIndices.iScaleIndex].vVector3);
			_vector vDestScale = XMLoadFloat3(&m_KeyFrames_Scale[keyFrameIndices.iScaleIndex + 1].vVector3);

			_float  fRatio = (fCurrentTrackPosition - m_KeyFrames_Scale[keyFrameIndices.iScaleIndex].fTrackPosition) /
				(m_KeyFrames_Scale[keyFrameIndices.iScaleIndex + 1].fTrackPosition - m_KeyFrames_Scale[keyFrameIndices.iScaleIndex].fTrackPosition);

			vOutScale = XMVectorLerp(vSourScale, vDestScale, fRatio);
		}
	}

	// Translation ---------------------------------------------------------------------

	if (m_KeyFrames_Trans.empty() == false)
	{
		if (keyFrameIndices.iTransIndex >= m_KeyFrames_Trans.size() - 1)
			keyFrameIndices.iTransIndex = 0;

		KEYFRAME_VEC        LastKeyFrameDesc = m_KeyFrames_Trans.back();

		if (fCurrentTrackPosition <= m_KeyFrames_Trans.front().fTrackPosition)
		{
			keyFrameIndices.iTransIndex = 0;
			vOutTrans = XMLoadFloat3(&m_KeyFrames_Trans.front().vVector3);
		}
		else if (fCurrentTrackPosition >= LastKeyFrameDesc.fTrackPosition)
		{
			vOutTrans = XMLoadFloat3(&LastKeyFrameDesc.vVector3);
		}
		else
		{
			if (fCurrentTrackPosition < m_KeyFrames_Trans[keyFrameIndices.iTransIndex].fTrackPosition)
				keyFrameIndices.iTransIndex = 0;

			while (fCurrentTrackPosition >= m_KeyFrames_Trans[keyFrameIndices.iTransIndex + 1].fTrackPosition)
				++(keyFrameIndices.iTransIndex);

			_vector vSourTranslation = XMLoadFloat3(&m_KeyFrames_Trans[keyFrameIndices.iTransIndex].vVector3);
			_vector vDestTranslation = XMLoadFloat3(&m_KeyFrames_Trans[keyFrameIndices.iTransIndex + 1].vVector3);

			_float  fRatio = (fCurrentTrackPosition - m_KeyFrames_Trans[keyFrameIndices.iTransIndex].fTrackPosition) /
				(m_KeyFrames_Trans[keyFrameIndices.iTransIndex + 1].fTrackPosition - m_KeyFrames_Trans[keyFrameIndices.iTransIndex].fTrackPosition);

			vOutTrans = XMVectorLerp(vSourTranslation, vDestTranslation, fRatio);
		}
	}


	// Rotation ---------------------------------------------------------------------
	if (m_KeyFrames_Rot.empty() == false)
	{
		if (keyFrameIndices.iRotIndex >= m_KeyFrames_Rot.size() - 1)
			keyFrameIndices.iRotIndex = 0;


		KEYFRAME_QUAT        LastKeyFrameDesc = m_KeyFrames_Rot.back();

		if (fCurrentTrackPosition <= m_KeyFrames_Rot.front().fTrackPosition)
		{
			keyFrameIndices.iRotIndex = 0;
			vOutRot = XMLoadFloat4(&m_KeyFrames_Rot.front().vQuat);
		}
		else if (fCurrentTrackPosition >= LastKeyFrameDesc.fTrackPosition)
		{
			vOutRot = XMLoadFloat4(&LastKeyFrameDesc.vQuat);
		}
		else
		{
			if (fCurrentTrackPosition < m_KeyFrames_Rot[keyFrameIndices.iRotIndex].fTrackPosition)
				keyFrameIndices.iRotIndex = 0;

			while (fCurrentTrackPosition >= m_KeyFrames_Rot[keyFrameIndices.iRotIndex + 1].fTrackPosition)
				++keyFrameIndices.iRotIndex;

			_vector vSourRotation = XMLoadFloat4(&m_KeyFrames_Rot[keyFrameIndices.iRotIndex].vQuat);
			_vector vDestRotation = XMLoadFloat4(&m_KeyFrames_Rot[keyFrameIndices.iRotIndex + 1].vQuat);

			_float  fRatio = (fCurrentTrackPosition - m_KeyFrames_Rot[keyFrameIndices.iRotIndex].fTrackPosition) /
				(m_KeyFrames_Rot[keyFrameIndices.iRotIndex + 1].fTrackPosition - m_KeyFrames_Rot[keyFrameIndices.iRotIndex].fTrackPosition);

			//vOutRot = XMQuaternionNormalize(XMVectorLerp(vSourRotation, vDestRotation, fRatio));
			//vOutRot = XMQuaternionSlerp(vSourRotation, vDestRotation, fRatio);
			if (XMVectorGetX(XMVector4Dot(vSourRotation, vDestRotation)) < 0.f)
				vDestRotation = XMVectorNegate(vDestRotation);
			vOutRot = XMQuaternionNormalize(XMVectorLerp(vSourRotation, vDestRotation, fRatio));
		}
	}
}


CChannel* CChannel::Create(const ChannelPassData& channelPassData)
{
	CChannel* pInstance = new CChannel();

	if (FAILED(pInstance->Initialize(channelPassData)))
	{
		MSG_BOX("Failed to Created : CChannel");
		Safe_Release(pInstance);
	}

	return pInstance;
}


void CChannel::Free()
{
	__super::Free();


}
