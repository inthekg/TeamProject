#include "Animator.h"
#include "Model.h"
#include "Animation.h"
#include "GameInstance.h"
#include "Parser.h"
#include "Channel.h"

void CAnimator::Begin_StateBlend(_int iFromClipIndex, _float fFromLocalTime, _float fBlendDuration, _bool bFreezeFromPose)
{
	if (iFromClipIndex < 0 || iFromClipIndex >= m_EditData.ClipSettings.size())
		return;

	if (fBlendDuration <= 0.f)
		return;

	m_bStateBlending = true;
	m_bBlendFromPoseFrozen = bFreezeFromPose;

	m_iBlendFromClipIndex = iFromClipIndex;
	m_fBlendFromLocalTime = fFromLocalTime;
	m_fBlendElapsed = 0.f;
	m_fBlendDuration = fBlendDuration;

	m_BlendFromPose = m_BasePose;
	m_BlendToPose = m_BasePose;

	if (m_bBlendFromPoseFrozen)
	{
		if (!Sample_StateBlendFromPose(m_BlendFromPose))
			Stop_StateBlend();
	}
}

void CAnimator::Update_StateBlend(_float fTimeDelta)
{
	if (!m_bStateBlending)
		return;

	if (m_iBlendFromClipIndex < 0 || m_iBlendFromClipIndex >= m_EditData.ClipSettings.size())
	{
		Stop_StateBlend();
		return;
	}

	const ANIM_CLIP_SETTING& FromClip = m_EditData.ClipSettings[m_iBlendFromClipIndex];
	
	if (!m_bBlendFromPoseFrozen)
	{
		Advance_ClipLocalTime(FromClip, fTimeDelta, FromClip.fPlayRate, FromClip.bLoop, m_fBlendFromLocalTime);
		
		// From pose는 이전 clip을 현재 blend 시간으로 계속 샘플링해서 발 위치 튐을 줄인다
		if (!Sample_StateBlendFromPose(m_BlendFromPose))
		{
			Stop_StateBlend();
			return;
		}
	}

	m_BlendToPose = m_CurPose;

	const ANIM_CLIP_SETTING* pCurClip = Get_CurClip();

	m_fBlendElapsed += fTimeDelta * 1000.f;

	_float fAlpha = 1.f;

	if (m_fBlendDuration > 0.f)
		fAlpha = m_fBlendElapsed / m_fBlendDuration;

	if (fAlpha < 0.f)
		fAlpha = 0.f;
	if (fAlpha > 1.f)
		fAlpha = 1.f;

	_float fFromRootY = 0.f;
	_float fToRootY = 0.f;
	const _bool bCanBlendRootY =
		Try_GetRootModelY(m_BlendFromPose, fFromRootY) &&
		Try_GetRootModelY(m_BlendToPose, fToRootY);


	Blend_Pose(m_BlendFromPose, m_BlendToPose, fAlpha, m_CurPose);

	// 최종 포즈도 X/Z 루트 이동량만 제거한다.
	if (pCurClip != nullptr)
		Strip_RootMotion(m_CurPose, *pCurClip);

	if (bCanBlendRootY)
	{
		// Root Y는 일반 선형 보간보다 부드러운 smoothstep으로 따로 보간한다.
		// Y값 자체는 살리되, From/To 높이 차이 때문에 전환 순간 확 튀는 문제를 줄인다.
		const _float fRootAlpha = fAlpha * fAlpha * (3.f - 2.f * fAlpha);
		const _float fBlendedRootY = fFromRootY + (fToRootY - fFromRootY) * fRootAlpha;

		Set_RootModelY(m_CurPose, fBlendedRootY);
	}

	if (fAlpha >= 1.f)
		Stop_StateBlend();
}

void CAnimator::Stop_StateBlend()
{
	m_bStateBlending = false;
	m_bBlendFromPoseFrozen = false;

	m_iBlendFromClipIndex = -1;
	m_fBlendFromLocalTime = 0.f;
	m_fBlendElapsed = 0.f;
	m_fBlendDuration = 0.f;
}

_bool CAnimator::Sample_StateBlendFromPose(POSE& OutPose)
{
	if (m_iBlendFromClipIndex < 0 || m_iBlendFromClipIndex >= m_EditData.ClipSettings.size())
		return false;

	const ANIM_CLIP_SETTING& Clip = m_EditData.ClipSettings[m_iBlendFromClipIndex];

	return Sample_ClipPose(Clip, m_fBlendFromLocalTime, OutPose, m_BlendFromKeyFrameIndices, true);
}

void CAnimator::Blend_Pose(const POSE& FromPose, const POSE& ToPose, _float fWeight, POSE& OutPose) const
{
	if (FromPose.empty() || ToPose.empty())
		return;

	const _int iPoseCount = min(FromPose.size(), ToPose.size());

	OutPose = FromPose;

	for (_int i = 0; i < iPoseCount; i++)
		Blend_LocalTransform(OutPose[i], ToPose[i], fWeight);
}

void CAnimator::Begin_StateBlendFromPose(_int iFromClipIndex, const POSE& FromPose, _float fBlendDuration)
{
	if (iFromClipIndex < 0 || iFromClipIndex >= static_cast<_int>(m_EditData.ClipSettings.size()))
		return;

	if (fBlendDuration <= 0.f)
		return;

	if (FromPose.empty())
		return;

	m_bStateBlending = true;
	m_bBlendFromPoseFrozen = true;

	m_iBlendFromClipIndex = iFromClipIndex;
	m_fBlendFromLocalTime = 0.f;
	m_fBlendElapsed = 0.f;
	m_fBlendDuration = fBlendDuration;

	m_BlendFromPose = FromPose;

	// 현재 포즈를 그대로 From pose로 얼리는 경로라서,
	// 이전 블렌드나 시퀀스 전환에서 남은 루트 이동량을 여기서 제거해준다.
	Strip_RootMotion(m_BlendFromPose, m_EditData.ClipSettings[iFromClipIndex]);

	m_BlendToPose = m_BasePose;
}
