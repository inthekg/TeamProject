#pragma once

#include "Resource.h"
#include "ModelDataStruct.h"

NS_BEGIN(Engine)

class CChannel;
class ENGINE_DLL CAnimation final : public CResource
{
private:
	CAnimation();
	virtual ~CAnimation() = default;

public:
	HRESULT					Initialize(AnimationReadData& readData, const vector<CChannel*>& Channels);
	void					Get_Pose(POSE& OutPoseData, _float fTrackPosition, vector<KeyFrameIndices>& vKeyFrameIndices);

public:
	void					Get_RootTransform(_float fTrackPosition, vector<KeyFrameIndices>& vKeyFrameIndices, _vector& vScale, _vector& vRot, _vector& vTrans);
	_uint					Get_NumChannels()		{ return m_iNumChannels; }


	_wstring                Get_StateName()			{ return m_strAnimationName; }
	_uint64                 Get_StateHashNum()		{ return m_iAnimNameHash; }
	_float					Get_TickPerSecond()		{ return m_fTickPerSecond; }
	_float					Get_Duration()			{ return m_fDuration; }
	_float					Get_BlendTime()			{ return m_fBlendTime; }

	// 추가
	_wstring				Get_AnimationFileName() const { return m_strAnimationFileName; }
	_uint64					Get_AnimationFileHash() const { return m_iAnimationFileHash; }

	void					Set_Loop(bool bLoop)	{ m_bLoop = bLoop; }
	_bool					IsLoop()				{ return m_bLoop; }

	_bool					Is_RootMotion()			{ return m_bRootMotion; }

private:
	_wstring				m_strAnimationFileName	= {};

	// 추가
	_uint64					m_iAnimationFileHash	= {};

	_wstring                m_strAnimationName		= {};
	_uint64                 m_iAnimNameHash			= {};
	_float					m_fDuration				= {}; 
	_float					m_fTickPerSecond		= {}; 
	//이 애니메이션으로 전환 할 때 Blend할 시간
	_float					m_fBlendTime			= {};

	// 현재 애니메이션의 재생을 위해 상태를 제어해야하는 뼈의 갯수 
	_uint					m_iNumChannels			= {};

	//TODO : 루트 본의 인덱스 탐색해서 수정하기
	_uint					m_iRootChannelIndex		= { 0 };

	vector<class CChannel*>	m_Channels;

private:
	_bool					m_bLoop					= {};
	_bool					m_bRootMotion			= {};

public:
	static CAnimation*		Create(AnimationReadData& readData, const vector<CChannel*>& Channels);
	virtual void			Free() override;
};

NS_END