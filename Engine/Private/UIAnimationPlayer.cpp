#include "UIAnimationPlayer.h"
#include "UIObject.h"
#include "UIElement.h"

const string CUIAnimationPlayer::s_strEmptyClipName = {};

HRESULT CUIAnimationPlayer::Initialize(CUIObject* pOwner, const vector<UI_ANIMATION_CLIP_ASSET_DESC>& Clips, const _float2& vRuntimeScale)
{
	if (pOwner == nullptr)
		return E_FAIL;

	m_pOwner = pOwner;
	m_Clips = Clips;
	m_vRuntimeScale = vRuntimeScale;

	return S_OK;
}

_bool CUIAnimationPlayer::Play(const string& strClipName, _bool bRestart)
{
	const UI_ANIMATION_CLIP_ASSET_DESC* pClip = Find_Clip(strClipName);

	if (pClip == nullptr)
		return false;

	// 같은 Clip이 이미 재생 중이고 Restart 요청이 없으면 그대로 유지한다.
	if (bRestart == false && m_pCurrentClip == pClip && m_bPlaying)
		return true;

	m_pCurrentClip = pClip;
	m_fCurrentTime = 0.f;

	m_bPlaying = true;
	m_bPaused = false;
	m_bFinished = false;

	// 첫 프레임을 기다리지 않고 0초 Keyframe 상태를 즉시 적용한다.
	Evaluate_CurrentClip();

	return true;
}

void CUIAnimationPlayer::Stop()
{
	m_bPlaying = false;
	m_bPaused = false;
}

void CUIAnimationPlayer::Pause()
{
	if (m_bPlaying == false)
		return;

	m_bPaused = true;
}

void CUIAnimationPlayer::Resume()
{
	if (m_bPlaying == false)
		return;

	m_bPaused = false;
}

_bool CUIAnimationPlayer::Set_Time(_float fTime)
{
	if (m_pCurrentClip == nullptr)
		return false;

	m_fCurrentTime = fClamp(fTime, 0.f, m_pCurrentClip->fDuration);
	Evaluate_CurrentClip();

	return true;
}

void CUIAnimationPlayer::Update(_float fTimeDelta)
{
	if (m_bPlaying == false || m_bPaused || m_pCurrentClip == nullptr)
		return;

	const _float fDuration = m_pCurrentClip->fDuration;

	if (fDuration <= FLT_EPSILON)
	{
		Stop();
		m_bFinished = true;
		return;
	}

	m_fCurrentTime += fTimeDelta;

	if (m_pCurrentClip->bLoop)
	{
		// Duration을 초과하면 나머지 시간부터 다시 반복한다.
		m_fCurrentTime = fmodf(m_fCurrentTime, fDuration);
	}
	else if (m_fCurrentTime >= fDuration)
	{
		// 마지막 Keyframe을 정확히 적용한 뒤 재생을 종료한다.
		m_fCurrentTime = fDuration;
		Evaluate_CurrentClip();

		m_bPlaying = false;
		m_bFinished = true;

		return;
	}

	Evaluate_CurrentClip();
}

_bool CUIAnimationPlayer::Has_Clip(const string& strClipName) const
{
	return Find_Clip(strClipName) != nullptr;
}

_bool CUIAnimationPlayer::Is_Playing() const
{
	return m_bPlaying;
}

_bool CUIAnimationPlayer::Is_Paused() const
{
	return m_bPaused;
}

_bool CUIAnimationPlayer::Is_Finished() const
{
	return m_bFinished;
}

_float CUIAnimationPlayer::Get_CurrentTime() const
{
	return m_fCurrentTime;
}

const string& CUIAnimationPlayer::Get_CurrentClipName() const
{
	if (m_pCurrentClip == nullptr)
		return s_strEmptyClipName;

	return m_pCurrentClip->strName;
}

const UI_ANIMATION_CLIP_ASSET_DESC* CUIAnimationPlayer::Find_Clip(const string& strClipName) const
{
	auto iter = find_if(m_Clips.begin(), m_Clips.end(), [&strClipName](const UI_ANIMATION_CLIP_ASSET_DESC& Clip)
		{
			return Clip.strName == strClipName;
		});

	if (iter == m_Clips.end())
		return nullptr;

	return &(*iter);
}

void CUIAnimationPlayer::Evaluate_CurrentClip()
{
	if (m_pOwner == nullptr || m_pCurrentClip == nullptr)
		return;

	for (const UI_ANIMATION_TRACK_ASSET_DESC& Track : m_pCurrentClip->Tracks)
	{
		if (Track.Keyframes.empty())
			continue;

		CUIElement* pTarget = m_pOwner->Find_Element(Track.iTargetElementID);

		if (pTarget == nullptr)
			continue;

		const _float4 vValue = Evaluate_Track(Track, m_fCurrentTime);

		Apply_Value(pTarget, Track.eProperty, vValue);
	}
}

_float4 CUIAnimationPlayer::Evaluate_Track(const UI_ANIMATION_TRACK_ASSET_DESC& Track, _float fTime) const
{
	if (Track.Keyframes.empty())
		return {};

	if (Track.Keyframes.size() == 1 || fTime <= Track.Keyframes.front().fTime)
		return Track.Keyframes.front().vValue;

	if (fTime >= Track.Keyframes.back().fTime)
		return Track.Keyframes.back().vValue;

	// 현재 시간보다 뒤에 있는 첫 번째 Keyframe을 찾는다.
	auto RightIter = upper_bound(Track.Keyframes.begin(), Track.Keyframes.end(), fTime, [](_float fValue, const UI_ANIMATION_KEYFRAME_ASSET_DESC& Keyframe)
		{
			return fValue < Keyframe.fTime;
		});

	if (RightIter == Track.Keyframes.begin())
		return RightIter->vValue;

	if (RightIter == Track.Keyframes.end())
		return Track.Keyframes.back().vValue;

	const UI_ANIMATION_KEYFRAME_ASSET_DESC& RightKeyframe = *RightIter;
	const UI_ANIMATION_KEYFRAME_ASSET_DESC& LeftKeyframe = *(RightIter - 1);

	const _float fKeyDuration = RightKeyframe.fTime - LeftKeyframe.fTime;

	if (fKeyDuration <= FLT_EPSILON)
		return RightKeyframe.vValue;

	_float fRatio = (fTime - LeftKeyframe.fTime) / fKeyDuration;
	fRatio = fClamp(fRatio, 0.f, 1.f);
	fRatio = Apply_Interpolation(fRatio, LeftKeyframe.eInterpolation);

	_float4 vResult = {};

	XMStoreFloat4(&vResult, XMVectorLerp(XMLoadFloat4(&LeftKeyframe.vValue), XMLoadFloat4(&RightKeyframe.vValue), fRatio));

	return vResult;
}

_float CUIAnimationPlayer::Apply_Interpolation(_float fRatio, UI_ANIMATION_INTERPOLATION eInterpolation) const
{
	switch (eInterpolation)
	{
	case UI_ANIMATION_INTERPOLATION::STEP:
		return 0.f;

	case UI_ANIMATION_INTERPOLATION::LINEAR:
		return fRatio;

	case UI_ANIMATION_INTERPOLATION::EASE_IN:
		return fRatio * fRatio;

	case UI_ANIMATION_INTERPOLATION::EASE_OUT:
		return 1.f - (1.f - fRatio) * (1.f - fRatio);

	case UI_ANIMATION_INTERPOLATION::EASE_IN_OUT:
		return fRatio * fRatio * (3.f - 2.f * fRatio);
	}

	return fRatio;
}

void CUIAnimationPlayer::Apply_Value(CUIElement* pTarget, UI_ANIMATION_PROPERTY eProperty, const _float4& vValue)
{
	if (pTarget == nullptr)
		return;

	switch (eProperty)
	{
	case UI_ANIMATION_PROPERTY::POSITION:
		// JSON은 기준 해상도 좌표이므로 실제 런타임 해상도 배율을 적용한다.
		pTarget->Set_Position(_float2(vValue.x * m_vRuntimeScale.x, vValue.y * m_vRuntimeScale.y));
		break;

	case UI_ANIMATION_PROPERTY::SIZE:
		pTarget->Set_Size(_float2(vValue.x * m_vRuntimeScale.x, vValue.y * m_vRuntimeScale.y));
		break;

	case UI_ANIMATION_PROPERTY::SCALE:
		pTarget->Set_Scale(_float2(vValue.x, vValue.y));
		break;

	case UI_ANIMATION_PROPERTY::ROTATION:
		// 현재 UIElement의 Rotation은 Radian 단위다.
		pTarget->Set_Rotation(vValue.x);
		break;

	case UI_ANIMATION_PROPERTY::COLOR:
		pTarget->Set_Color(vValue);
		break;

	case UI_ANIMATION_PROPERTY::ALPHA:
	{
		// 기존 RGB는 유지하고 Alpha만 변경한다.
		_float4 vColor = pTarget->Get_RenderData().vColor;
		vColor.w = fClamp(vValue.x, 0.f, 1.f);

		pTarget->Set_Color(vColor);
		break;
	}

	case UI_ANIMATION_PROPERTY::UV_RECT:
		pTarget->Set_UVRect(vValue);
		break;

	case UI_ANIMATION_PROPERTY::REVEAL_PROGRESS:
		// Keyframe의 x값을 Reveal 진행률로 사용한다.
		pTarget->Set_RevealProgress(vValue.x);
		break;		
	}
}

CUIAnimationPlayer* CUIAnimationPlayer::Create(CUIObject* pOwner, const vector<UI_ANIMATION_CLIP_ASSET_DESC>& Clips, const _float2& vRuntimeScale)
{
	CUIAnimationPlayer* pInstance = new CUIAnimationPlayer();

	if (FAILED(pInstance->Initialize(pOwner, Clips, vRuntimeScale)))
	{
		MSG_BOX("Failed to Created : CUIAnimationPlayer");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CUIAnimationPlayer::Free()
{
	__super::Free();

	m_pCurrentClip = nullptr;
	m_pOwner = nullptr;

	m_Clips.clear();
}