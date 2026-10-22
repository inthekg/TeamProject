#pragma once
#include "Base.h"
#include "Animation.h"

NS_BEGIN(Engine)

class ENGINE_DLL CAnimationPlayer : public CBase
{

private:
    CAnimationPlayer(CAnimation* pAnimation, _uint iNumBones);
    ~CAnimationPlayer() = default;

public:
    HRESULT Initailize(POSE* pBasePose);

public:
    _bool					CanEnd()                        { return m_bCanEnd; }
	_bool                   IsEnd()                         { return m_bEnd; }

	_float                  Get_BlendTime()                 { return m_pAnimation->Get_BlendTime(); }
    CAnimation*             Get_Animation()     const       { return m_pAnimation; }
    _bool                   Is_RootMotion()                 { return m_pAnimation ? m_pAnimation->Is_RootMotion() : true; }

    _float                  Get_Duration()                  { return m_pAnimation->Get_Duration(); }
    _wstring                Get_Animation_StateName()       { return m_pAnimation->Get_StateName(); }
    _uint64                 Get_Animation_StateHashNum()    { return m_pAnimation->Get_StateHashNum(); }   

public:
    void                    Play(_float fTimeDelta);
    void                    Play_FromStart(_float fTimeDelta);

	void                    Get_CurrentPose(POSE & OutPose);
    void                    Get_BlendPose(POSE& BlendPose, _float fBlendFactor, POSE& OutPose);
    
    _float                  Get_FrameRatio();
    _float                  Get_FramePosition() { return m_fCurTrackPosition; }
    void                    Set_FramePosition(_float fFramePosition);

    //로코모션의 경우 싱크를 맞추기 위해서 프레임을 비율로 설정함  
    void                    Set_FramePosition_Ratio(_float fRatio);

    void                    Compute_RootDeltaTransform(TransformData& OutRootTransform);
	void 				    Reset_RootDeltaTransform() { m_prevRootTransform = m_firstRootTransform; }

private:
    CAnimation*             m_pAnimation = { nullptr };

    _float                  m_fCurTrackPosition = {};

    //Root 계산한 마지막 위치 저장용
    _float                  m_fPrevTrackPosition = {};

    //채널당 현재 재생중인 키프레임 인덱스
    vector<KeyFrameIndices> m_CurKeyFrameIndices;
    POSE                    m_FinalPose;

private:
    _bool                   m_bEnd = {};
    _bool					m_bCanEnd = {};
	_uint                   m_iNumBones = {};    
    
private:
    TransformData           m_firstRootTransform;
    TransformData           m_prevRootTransform;
    TransformData           m_curRootTransform;

public:
    static CAnimationPlayer* Create(CAnimation* pAnimation, _uint iNumBones ,POSE* pBasePose);
    void Free() override;
};

NS_END