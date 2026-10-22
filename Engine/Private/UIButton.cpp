#include "UIButton.h"
#include "GameInstance.h"

CUIButton::CUIButton(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CUIElement{ pDevice, pContext }
{
}

void CUIButton::Set_CommandKey(const string& strCommandKey)
{
	m_strCommandKey = strCommandKey;
}

void CUIButton::Set_FocusOnHover(_bool bFocusOnHover)
{
	m_bFocusOnHover = bFocusOnHover;
}

void CUIButton::Set_SelectOnFocus(_bool bSelectOnFocus)
{
	m_bSelectOnFocus = bSelectOnFocus;
}

void CUIButton::Set_CommandOnPress(_bool bCommandOnPress)
{
	m_bCommandOnPress = bCommandOnPress;
}

void CUIButton::Set_FocusGroup(const string& strFocusGroup)
{
	m_strFocusGroup = strFocusGroup;
}

void CUIButton::Set_FocusOrder(_int iFocusOrder)
{
	m_iFocusOrder = iFocusOrder;
}

void CUIButton::Set_Interactable(_bool bInteractable)
{
	if (m_bInteractable == bInteractable)
		return;

	m_bInteractable = bInteractable;

	Refresh_VisualState();
}

void CUIButton::Set_Selected(_bool bSelected)
{
	if (m_bSelected == bSelected)
		return;

	m_bSelected = bSelected;

	Refresh_VisualState();
}

void CUIButton::Set_Focused(_bool bFocused)
{
	if (m_bFocused == bFocused)
		return;

	m_bFocused = bFocused;

	Refresh_VisualState();
}

void CUIButton::Set_PointerState(UI_BUTTON_POINTER_STATE ePointerState)
{
	if (m_ePointerState == ePointerState)
		return;

	m_ePointerState = ePointerState;

	Refresh_VisualState();
}

const string& CUIButton::Get_CommandKey() const
{
	return m_strCommandKey;
}

const string& CUIButton::Get_FocusGroup() const
{
	return m_strFocusGroup;
}

_int CUIButton::Get_FocusOrder() const
{
	return m_iFocusOrder;
}

_bool CUIButton::Is_Interactable() const
{
	return m_bInteractable;
}

_bool CUIButton::Is_Selected() const
{
	return m_bSelected;
}

_bool CUIButton::Is_Focused() const
{
	return m_bFocused;
}

// UI 중앙 좌표가 Button의 실제 변환된 영역 안인지 검사
_bool CUIButton::Is_PointInside(const _float2& vUIPoint) const
{
	// 비활성화되었거나 부모 중 하나가 숨겨졌다면 입력 대상이 아님
	if (!Is_InputAvailable())
		return false;

	return __super::Is_PointInside(vUIPoint);
}

// 현재 Button이 Pointer/Focus 입력을 받을 수 있는 상태인지 확인
_bool CUIButton::Is_InputAvailable() const
{
	return m_bInteractable && Is_HierarchyVisible();
}

_bool CUIButton::Should_FocusOnHover() const
{
	return m_bFocusOnHover;
}

_bool CUIButton::Should_SelectOnFocus() const
{
	return m_bSelectOnFocus;
}

_bool CUIButton::Should_CommandOnPress() const
{
	return m_bCommandOnPress;
}

UI_BUTTON_POINTER_STATE CUIButton::Get_PointerState() const
{
	return m_ePointerState;
}

// Builder가 상태별 Target을 모두 생성한 후 등록
HRESULT CUIButton::Add_VisualOverride(UI_BUTTON_VISUAL_STATE eState, const UI_BUTTON_VISUAL_OVERRIDE& Override)
{
	const _uint iStateIndex = static_cast<_uint>(eState);

	if (iStateIndex >= static_cast<_uint>(UI_BUTTON_VISUAL_STATE::END))
		return E_FAIL;

	if (Override.pTarget == nullptr)
		return E_FAIL;

	const _uint iVisualIndex = Find_OrAddBaseVisual(Override.pTarget);

	if (iVisualIndex == UINT_MAX)
		return E_FAIL;

	UI_BUTTON_RESOLVED_OVERRIDE Resolved = {};

	Resolved.iVisualIndex = iVisualIndex;
	Resolved.bOverrideVisible = Override.bOverrideVisible;
	Resolved.bVisible = Override.bVisible;
	Resolved.bOverrideColor = Override.bOverrideColor;
	Resolved.vColor = Override.vColor;
	Resolved.bOverrideScale = Override.bOverrideScale;
	Resolved.vScale = Override.vScale;

	m_StateOverrides[iStateIndex].push_back(Resolved);

	return S_OK;
}

// 현재 상태의 목표값을 다시 계산하고 즉시 적용
void CUIButton::Refresh_VisualState()
{
	Resolve_TargetVisuals();
	Apply_TargetVisualsImmediately();
}

HRESULT CUIButton::Initialize()
{
	if (FAILED(__super::Initialize()))
		return E_FAIL;

	return S_OK;
}

void CUIButton::Priority_Update(_float fTimeDelta)
{
}

void CUIButton::Update(_float fTimeDelta)
{
}

void CUIButton::Late_Update(_float fTimeDelta)
{
}

void CUIButton::Submit_RenderCommand()
{
	// Button 자신은 렌더링하지 않는다
}

void CUIButton::Submit_RenderCommand(_uint iObjectSortOrder)
{
	// Button 자신은 렌더링하지 않는다
}

_uint CUIButton::Find_OrAddBaseVisual(CUIElement* pTarget)
{
	if (pTarget == nullptr)
		return UINT_MAX;

	for (_uint i = 0; i < static_cast<_uint>(m_BaseVisuals.size()); ++i)
	{
		if (m_BaseVisuals[i].pTarget == pTarget)
			return i;
	}

	UI_BUTTON_VISUAL_VALUE BaseValue = {};

	BaseValue.pTarget = pTarget;
	BaseValue.bVisible = pTarget->Get_RenderData().bVisible;
	BaseValue.vColor = pTarget->Get_RenderData().vColor;
	BaseValue.vScale = pTarget->Get_UITransform().vScale;

	m_BaseVisuals.push_back(BaseValue);

	return static_cast<_uint>(m_BaseVisuals.size() - 1);
}

void CUIButton::Resolve_TargetVisuals()
{
	// 매번 원래 값부터 시작하므로 이전 상태의 흔적이 남지 않는다.
	m_TargetVisuals = m_BaseVisuals;

	if (!m_bInteractable)
	{
		Apply_ResolvedState(UI_BUTTON_VISUAL_STATE::DISABLED);

		return;
	}

	if (m_bSelected)
	{
		Apply_ResolvedState(UI_BUTTON_VISUAL_STATE::SELECTED);
	}

	switch (m_ePointerState)
	{
	case UI_BUTTON_POINTER_STATE::HOVERED:
		Apply_ResolvedState(UI_BUTTON_VISUAL_STATE::HOVERED);
		break;

	case UI_BUTTON_POINTER_STATE::PRESSED:
		Apply_ResolvedState(UI_BUTTON_VISUAL_STATE::PRESSED);
		break;

	default:
		break;
	}

	if (m_bFocused)
	{
		Apply_ResolvedState(UI_BUTTON_VISUAL_STATE::FOCUSED);
	}
}

void CUIButton::Apply_ResolvedState(UI_BUTTON_VISUAL_STATE eState)
{
	const _uint iStateIndex = static_cast<_uint>(eState);

	if (iStateIndex >= static_cast<_uint>(UI_BUTTON_VISUAL_STATE::END))
		return;

	for (const UI_BUTTON_RESOLVED_OVERRIDE& Override : m_StateOverrides[iStateIndex])
	{
		if (Override.iVisualIndex >= m_TargetVisuals.size())
			continue;

		UI_BUTTON_VISUAL_VALUE& Target = m_TargetVisuals[Override.iVisualIndex];

		if (Override.bOverrideVisible)
			Target.bVisible = Override.bVisible;

		if (Override.bOverrideColor)
			Target.vColor = Override.vColor;

		if (Override.bOverrideScale)
			Target.vScale = Override.vScale;
	}
}

void CUIButton::Apply_TargetVisualsImmediately()
{
	for (const UI_BUTTON_VISUAL_VALUE& Target
		: m_TargetVisuals)
	{
		if (Target.pTarget == nullptr)
			continue;

		Target.pTarget->Set_Visible(Target.bVisible);
		Target.pTarget->Set_Color(Target.vColor);
		Target.pTarget->Set_Scale(Target.vScale);
	}
}

CUIButton* CUIButton::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CUIButton* pInstance = new CUIButton(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CUIButton");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CUIButton::Free()
{
	for (_uint i = 0; i < static_cast<_uint>(UI_BUTTON_VISUAL_STATE::END); ++i)
	{
		m_StateOverrides[i].clear();
	}

	m_TargetVisuals.clear();
	m_BaseVisuals.clear();

	__super::Free();
}
