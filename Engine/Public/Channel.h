#pragma once
#include "Base.h"
#include "ModelDataStruct.h"

NS_BEGIN(Engine)
class CChannel final : public CBase
{
private:
	CChannel();
	virtual ~CChannel() = default;

public:
	HRESULT									Initialize(const ChannelPassData& channelPassData);

public:
	void									Update_TransformationMatrix(POSE& PoseData, _float fCurrentTrackPosition, KeyFrameIndices& keyFrameIndices);

	void									Get_TransformationMatrix(_float fCurrentTrackPosition, KeyFrameIndices& keyFrameIndices, _vector& vOutScale, _vector& vOutRot, _vector& vOutTrans);
	_uint									Get_BoneIndex() const { return m_iBoneIndex; }

	const vector<KEYFRAME_VEC>&				Get_TranslationKeyFrames() const { return m_KeyFrames_Trans; }
	const vector<KEYFRAME_VEC>&				Get_ScaleKeyFrames() const { return m_KeyFrames_Scale; }
	const vector<KEYFRAME_QUAT>&			Get_RotationKeyFrames() const { return m_KeyFrames_Rot; }

private:
	_uint									m_iBoneIndex = {};

	vector<KEYFRAME_VEC>					m_KeyFrames_Trans;
	vector<KEYFRAME_VEC>					m_KeyFrames_Scale;
	vector<KEYFRAME_QUAT>					m_KeyFrames_Rot;



public:
	static CChannel* Create(const ChannelPassData& channelPassData);
	virtual void Free() override;

};

NS_END