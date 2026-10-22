#pragma once
#include "Base.h"
#include "UI_AssetData.h"

NS_BEGIN(Engine)

class CUIObject;
class CUIElement;

class ENGINE_DLL  CUIAnimationPlayer final : public CBase
{
private:
	CUIAnimationPlayer() = default;
	virtual ~CUIAnimationPlayer() = default;

public:
	HRESULT									Initialize(CUIObject* pOwner, const vector<UI_ANIMATION_CLIP_ASSET_DESC>& Clips, const _float2& vRuntimeScale);

public:
	// 지정한 이름의 애니메이션을 처음부터 재생한다.
	_bool									Play(const string& strClipName, _bool bRestart = true);
	// 현재 애니메이션 재생을 중지한다. 현재 화면 상태는 유지한다.
	void									Stop();
	// 현재 재생 시간을 유지한 채 일시 정지한다.
	void									Pause();
	// 일시 정지된 애니메이션을 이어서 재생한다.
	void									Resume();

	// 현재 애니메이션의 특정 시간을 즉시 적용한다.
	// 에디터 타임라인의 Scrub 기능에서도 사용할 수 있다.
	_bool									Set_Time(_float fTime);

	// 현재 UI 애니메이션 시간을 진행하고 Track 값을 Element에 적용한다.
	void									Update(_float fTimeDelta);

public:
	_bool									Has_Clip(const string& strClipName) const;
	_bool									Is_Playing() const;
	_bool									Is_Paused() const;
	_bool									Is_Finished() const;
	_float									Get_CurrentTime() const;
	const string&							Get_CurrentClipName() const;

private:
	const UI_ANIMATION_CLIP_ASSET_DESC*		Find_Clip(const string& strClipName) const;

	// 현재 시간의 모든 Track 값을 계산하여 실제 UI Element에 적용한다.
	void									Evaluate_CurrentClip();
	// Keyframe 사이의 현재 값을 계산한다.
	_float4									Evaluate_Track(const UI_ANIMATION_TRACK_ASSET_DESC& Track, _float fTime) const;

	// 보간 종류에 맞게 0~1 비율을 변환한다.
	_float									Apply_Interpolation(_float fRatio, UI_ANIMATION_INTERPOLATION eInterpolation) const;

	// Property 종류에 따라 Element의 Position, Color 등의 값을 변경한다.
	void									Apply_Value(CUIElement* pTarget, UI_ANIMATION_PROPERTY eProperty, const _float4& vValue);

private:
	CUIObject*								m_pOwner = { nullptr };

	vector<UI_ANIMATION_CLIP_ASSET_DESC>	m_Clips;
	const UI_ANIMATION_CLIP_ASSET_DESC*		m_pCurrentClip = { nullptr };

	_float2									m_vRuntimeScale = { 1.f, 1.f };
	_float									m_fCurrentTime = {};

	_bool									m_bPlaying = { false };
	_bool									m_bPaused = { false };
	_bool									m_bFinished = { false };

private:
	static const string s_strEmptyClipName;

public:
	static CUIAnimationPlayer* Create(CUIObject* pOwner, const vector<UI_ANIMATION_CLIP_ASSET_DESC>& Clips, const _float2& vRuntimeScale);
	virtual void Free() override;
};

NS_END