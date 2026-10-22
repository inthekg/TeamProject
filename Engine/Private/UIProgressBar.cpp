#include "UIProgressBar.h"
#include "GameInstance.h"
#include "Shader.h"
#include "Material.h"

CUIProgressBar::CUIProgressBar(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CUIElement{ pDevice, pContext }
{
}

void CUIProgressBar::Set_FillAmount(_float fFillAmount)
{
	UI_PROGRESS_SLOT* pFill = Find_Slot(UI_PROGRESS_SLOT_TYPE::FILL);

	if (pFill == nullptr)
		return;

	pFill->fAmount = Clamp01(fFillAmount);
	pFill->fStartAmount = 0.f;
	pFill->fEndAmount = pFill->fAmount;
}

void CUIProgressBar::Set_LengthScale(_float fLengthScale)
{
	m_fLengthScale = max(0.f, fLengthScale);
}

void CUIProgressBar::Set_FillColor(const _float4& vFillColor)
{
	UI_PROGRESS_SLOT* pFill = Find_Slot(UI_PROGRESS_SLOT_TYPE::FILL);

	if (pFill == nullptr)
		return;

	pFill->vColor = vFillColor;
	pFill->vColorLT = vFillColor;
	pFill->vColorRT = vFillColor;
	pFill->vColorRB = vFillColor;
	pFill->vColorLB = vFillColor;
}

void CUIProgressBar::Set_BackgroundMaterial(CMaterial* pMaterial)
{
	UI_PROGRESS_SLOT* pBackground = Find_Slot(UI_PROGRESS_SLOT_TYPE::BACKGROUND);

	if (pBackground == nullptr)
		return;

	Safe_AddRef(pMaterial);
	Safe_Release(pBackground->pMaterial);
	pBackground->pMaterial = pMaterial;
}

void CUIProgressBar::Set_FillMaterial(CMaterial* pMaterial)
{
	UI_PROGRESS_SLOT* pFill = Find_Slot(UI_PROGRESS_SLOT_TYPE::FILL);

	if (pFill == nullptr)
		return;

	Safe_AddRef(pMaterial);
	Safe_Release(pFill->pMaterial);
	pFill->pMaterial = pMaterial;
}

void CUIProgressBar::Set_FillType(UI_PROGRESS_FILL_TYPE eType)
{
	m_eFillType = eType;
}

void CUIProgressBar::Set_FillDirection(UI_PROGRESS_FILL_DIRECTION eDirection)
{
	m_eFillDirection = eDirection;
}

void CUIProgressBar::Set_ResizeDirection(UI_PROGRESS_RESIZE_DIRECTION eDirection)
{
	m_eResizeDirection = eDirection;
}

void CUIProgressBar::Set_SlotRange(_int iSlotID, _float fStart, _float fEnd)
{
	UI_PROGRESS_SLOT* pSlot = Find_Slot(iSlotID);

	if (pSlot == nullptr)
		return;

	pSlot->fStartAmount = Clamp01(fStart);
	pSlot->fEndAmount = Clamp01(fEnd);

	if (pSlot->fEndAmount < pSlot->fStartAmount)
		pSlot->fEndAmount = pSlot->fStartAmount;

	pSlot->fAmount = pSlot->fEndAmount - pSlot->fStartAmount;
}

void CUIProgressBar::Set_SlotAmount(_int iSlotID, _float fAmount)
{
	UI_PROGRESS_SLOT* pSlot = Find_Slot(iSlotID);

	if (pSlot == nullptr)
		return;

	pSlot->fAmount = Clamp01(fAmount);
	pSlot->fStartAmount = 0.f;
	pSlot->fEndAmount = pSlot->fAmount;
}

void CUIProgressBar::Set_SlotVisible(_int iSlotID, _bool bVisible)
{
	UI_PROGRESS_SLOT* pSlot = Find_Slot(iSlotID);

	if (pSlot == nullptr)
		return;

	pSlot->bVisible = bVisible;
}

HRESULT CUIProgressBar::Add_Slot(const UI_PROGRESS_SLOT& Slot)
{
	if (Slot.iSlotID < 0)
		return E_FAIL;

	if (Find_Slot(Slot.iSlotID) != nullptr)
		return E_FAIL;

	UI_PROGRESS_SLOT NewSlot = Slot;

	Safe_AddRef(NewSlot.pMaterial);

	m_Slots.push_back(NewSlot);

	return S_OK;
}

_int CUIProgressBar::Find_SlotID(const _wstring& strSlotName) const
{
	for (const auto& Slot : m_Slots)
	{
		if (Slot.strName == strSlotName)
			return Slot.iSlotID;
	}

	return -1;
}

CUIProgressBar::UI_PROGRESS_SLOT* CUIProgressBar::Find_Slot(_int iSlotID)
{
	for (auto& Slot : m_Slots)
	{
		if (Slot.iSlotID == iSlotID)
			return &Slot;
	}

	return nullptr;
}

CUIProgressBar::UI_PROGRESS_SLOT* CUIProgressBar::Find_Slot(UI_PROGRESS_SLOT_TYPE eType)
{
	for (auto& Slot : m_Slots)
	{
		if (Slot.eType == eType)
			return &Slot;
	}

	return nullptr;
}

CUIProgressBar::UI_PROGRESS_SLOT* CUIProgressBar::Find_Slot(const _wstring& strSlotName)
{
	for (auto& Slot : m_Slots)
	{
		if (Slot.strName == strSlotName)
			return &Slot;
	}

	return nullptr;
}

HRESULT CUIProgressBar::Initialize()
{
	return S_OK;
}

void CUIProgressBar::Priority_Update(_float fTimeDelta)
{
}

void CUIProgressBar::Update(_float fTimeDelta)
{
}

void CUIProgressBar::Late_Update(_float fTimeDelta)
{
}

void CUIProgressBar::Submit_RenderCommand()
{
	Submit_RenderCommand(0);
}

void CUIProgressBar::Submit_RenderCommand(_uint iObjectSortOrder)
{
	if (!Is_HierarchyVisible() || m_tRenderData.pShader == nullptr || m_tRenderData.pVIBuffer == nullptr)
		return;

	const _uint iBaseSortOrder = iObjectSortOrder * UI_ELEMENT_SORT_RANGE + m_tRenderData.iSortOrder;

	vector<UI_PROGRESS_SLOT> RenderSlots = m_Slots;

	sort(RenderSlots.begin(), RenderSlots.end(), [](const UI_PROGRESS_SLOT& Left, const UI_PROGRESS_SLOT& Right)
		{
			if (Left.iSortOffset == Right.iSortOffset)
				return Left.iSlotID < Right.iSlotID;

			return Left.iSortOffset < Right.iSortOffset;
		});

	for (const auto& Slot : RenderSlots)
	{
		if (!Slot.bVisible)
			continue;

		_float fStart = 0.f;
		_float fEnd = 1.f;

		if (Slot.eType == UI_PROGRESS_SLOT_TYPE::BACKGROUND)
		{
			fStart = 0.f;
			fEnd = 1.f;
		}
		else if (Slot.eType == UI_PROGRESS_SLOT_TYPE::FILL)
		{
			fStart = Clamp01(Slot.fStartAmount);
			fEnd = Clamp01(Slot.fEndAmount);
		}
		else if (Slot.eType == UI_PROGRESS_SLOT_TYPE::FX)
		{
			_float fTargetEndAmount = 1.f;

			UI_PROGRESS_SLOT* pTargetFill = Find_Slot(Slot.iTargetSlotID);

			if (pTargetFill != nullptr && pTargetFill->eType == UI_PROGRESS_SLOT_TYPE::FILL)
			{
				fTargetEndAmount = Clamp01(pTargetFill->fEndAmount);
			}
			else
			{
				UI_PROGRESS_SLOT* pDefaultFill = Find_Slot(UI_PROGRESS_SLOT_TYPE::FILL);
				
				if (pDefaultFill != nullptr)
					fTargetEndAmount = Clamp01(pDefaultFill->fEndAmount);
			}

			const _float fFXWidth = Clamp01(Slot.fEndAmount - Slot.fStartAmount);

			fEnd = fTargetEndAmount;
			fStart = fEnd - fFXWidth;

			if (fStart < 0.f)
				fStart = 0.f;
		}

		if (fEnd <= fStart)
			continue;

		Submit_ProgressSlotCommand(iBaseSortOrder + Slot.iSortOffset, Slot, fStart, fEnd);
	}
}

void CUIProgressBar::Make_ProgressWorldMatrix(_float4x4* pOutWorldMatrix) const
{
	const _float fWidth = m_tTransform.vSize.x * m_tTransform.vScale.x * m_fLengthScale;
	const _float fHeight = m_tTransform.vSize.y * m_tTransform.vScale.y;

	const _float fPivotX = (m_tTransform.vPivot.x - 0.5f) * fWidth;
	const _float fPivotY = (0.5f - m_tTransform.vPivot.y) * fHeight;

	const _float2 vAnchoredPosition = Get_AnchoredPosition();

	XMStoreFloat4x4(
		pOutWorldMatrix,
		XMMatrixScaling(fWidth, fHeight, 1.f) *
		XMMatrixTranslation(-fPivotX, -fPivotY, 0.f) *
		XMMatrixRotationZ(m_tTransform.fRotation) *
		XMMatrixTranslation(vAnchoredPosition.x, vAnchoredPosition.y, 0.f) *
		Get_ParentWorldLayoutMatrix());
}

void CUIProgressBar::Make_ProgressWorldMatrix(_float4x4* pOutWorldMatrix, const UI_PROGRESS_SLOT& Slot, _float fStart, _float fEnd) const
{
	const _float fFullWidth = m_tTransform.vSize.x * m_tTransform.vScale.x * m_fLengthScale;
	const _float fFullHeight = m_tTransform.vSize.y * m_tTransform.vScale.y;

	const _float2 vAnchoredPosition = Get_AnchoredPosition();

	_float fSlotWidth = fFullWidth;
	_float fSlotHeight = fFullHeight;

	_float fOffsetX = 0.f;
	_float fOffsetY = 0.f;

	const _float fRange = fEnd - fStart;
	const _float fCenterRatio = (fStart + fEnd) * 0.5f;

	if (Slot.eType != UI_PROGRESS_SLOT_TYPE::BACKGROUND)
	{
		if (m_eFillDirection == UI_PROGRESS_FILL_DIRECTION::RIGHT_TO_LEFT)
		{
			fSlotWidth = fFullWidth * fRange;
			fOffsetX = fFullWidth * 0.5f - fFullWidth * fCenterRatio;
		}
		else if (m_eFillDirection == UI_PROGRESS_FILL_DIRECTION::TOP_TO_BOTTOM)
		{
			fSlotHeight = fFullHeight * fRange;
			fOffsetY = fFullHeight * 0.5f - fFullHeight * fCenterRatio;
		}
		else if (m_eFillDirection == UI_PROGRESS_FILL_DIRECTION::BOTTOM_TO_TOP)
		{
			fSlotHeight = fFullHeight * fRange;
			fOffsetY = -fFullHeight * 0.5f + fFullHeight * fCenterRatio;
		}
		else
		{
			fSlotWidth = fFullWidth * fRange;
			fOffsetX = -fFullWidth * 0.5f + fFullWidth * fCenterRatio;
		}
	}

	const _float fPivotX = (m_tTransform.vPivot.x - 0.5f) * fFullWidth;
	const _float fPivotY = (0.5f - m_tTransform.vPivot.y) * fFullHeight;

	XMStoreFloat4x4(
		pOutWorldMatrix,
		XMMatrixScaling(fSlotWidth, fSlotHeight, 1.f) *
		XMMatrixTranslation(fOffsetX - fPivotX, fOffsetY - fPivotY, 0.f) *
		XMMatrixRotationZ(m_tTransform.fRotation) *
		XMMatrixTranslation(vAnchoredPosition.x, vAnchoredPosition.y, 0.f) *
		Get_ParentWorldLayoutMatrix());
}

_float4 CUIProgressBar::Make_SlotUVRect(const UI_PROGRESS_SLOT& Slot, _float fStart, _float fEnd) const
{
	if (Slot.bStretchUVToRange || Slot.eType == UI_PROGRESS_SLOT_TYPE::BACKGROUND)
		return Slot.vUVRect;

	_float4 vUV = Slot.vUVRect;

	if (m_eFillDirection == UI_PROGRESS_FILL_DIRECTION::RIGHT_TO_LEFT)
	{
		const _float fU0 = Slot.vUVRect.z + (Slot.vUVRect.x - Slot.vUVRect.z) * fEnd;
		const _float fU1 = Slot.vUVRect.z + (Slot.vUVRect.x - Slot.vUVRect.z) * fStart;

		vUV.x = fU0;
		vUV.z = fU1;
	}
	else if (m_eFillDirection == UI_PROGRESS_FILL_DIRECTION::TOP_TO_BOTTOM)
	{
		const _float fV0 = Slot.vUVRect.y + (Slot.vUVRect.w - Slot.vUVRect.y) * fStart;
		const _float fV1 = Slot.vUVRect.y + (Slot.vUVRect.w - Slot.vUVRect.y) * fEnd;

		vUV.y = fV0;
		vUV.w = fV1;
	}
	else if (m_eFillDirection == UI_PROGRESS_FILL_DIRECTION::BOTTOM_TO_TOP)
	{
		const _float fV0 = Slot.vUVRect.w + (Slot.vUVRect.y - Slot.vUVRect.w) * fEnd;
		const _float fV1 = Slot.vUVRect.w + (Slot.vUVRect.y - Slot.vUVRect.w) * fStart;

		vUV.y = fV0;
		vUV.w = fV1;
	}
	else
	{
		const _float fU0 = Slot.vUVRect.x + (Slot.vUVRect.z - Slot.vUVRect.x) * fStart;
		const _float fU1 = Slot.vUVRect.x + (Slot.vUVRect.z - Slot.vUVRect.x) * fEnd;

		vUV.x = fU0;
		vUV.z = fU1;
	}

	return vUV;
}

void CUIProgressBar::Submit_ProgressCommand(_uint iSortOrder, CMaterial* pMaterial, const _float4& vColor, _float fFillAmount)
{
	UICommand command = {};

	command.header.etype = RENDER_TYPE::UI;
	command.header.iCommandSize = sizeof(UICommand);
	command.header.iSortKey = iSortOrder;

	Make_ProgressWorldMatrix(&command.worldMatrix);

	command.pVIBuffer = m_tRenderData.pVIBuffer;
	command.pVS = m_tRenderData.pShader->Get_VertexShader();
	command.pPS = m_tRenderData.pShader->Get_PixelShader();
	command.pLayout = m_tRenderData.pShader->Get_InputLayout();
	command.pMaterial = pMaterial;

	command.vColor = vColor;
	command.vUVRect = m_tRenderData.vUVRect;
	command.fFillAmount = fFillAmount;
	command.iUseTexture = m_tRenderData.bUseTexture ? 1 : 0;

	command.fRevealProgress = m_tRenderData.fRevealProgress;
	command.fRevealSoftness = m_tRenderData.fRevealSoftness;
	command.vRevealCenter = m_tRenderData.vRevealCenter;
	command.iRevealType = static_cast<_int>(m_tRenderData.eRevealType);

	m_pGameInstance->Submit_UI(command);
}

void CUIProgressBar::Submit_ProgressSlotCommand(_uint iSortOrder, const UI_PROGRESS_SLOT& Slot, _float fStart, _float fEnd)
{
	UICommand command = {};

	command.header.etype = RENDER_TYPE::UI;
	command.header.iCommandSize = sizeof(UICommand);
	command.header.iSortKey = iSortOrder;

	Make_ProgressWorldMatrix(&command.worldMatrix, Slot, fStart, fEnd);

	command.pVIBuffer = m_tRenderData.pVIBuffer;
	command.pVS = m_tRenderData.pShader->Get_VertexShader();
	command.pPS = m_tRenderData.pShader->Get_PixelShader();
	command.pLayout = m_tRenderData.pShader->Get_InputLayout();
	command.pMaterial = Slot.pMaterial;

	const _float fHierarchyAlpha = Get_HierarchyAlpha();

	command.vColor = Slot.vColor;
	command.vColorLT = Slot.vColorLT;
	command.vColorRT = Slot.vColorRT;
	command.vColorRB = Slot.vColorRB;
	command.vColorLB = Slot.vColorLB;

	// 슬롯 자체 Alpha와 ProgressBar 계층 Alpha를 함께 적용한다.
	command.vColor.w *= fHierarchyAlpha;
	command.vColorLT.w *= fHierarchyAlpha;
	command.vColorRT.w *= fHierarchyAlpha;
	command.vColorRB.w *= fHierarchyAlpha;
	command.vColorLB.w *= fHierarchyAlpha;

	command.vUVRect = Make_SlotUVRect(Slot, fStart, fEnd);
	
	command.fFillAmount = 1.f;
	command.iUseTexture = m_tRenderData.bUseTexture ? 1 : 0;
	command.iBlendMode =
		Slot.eBlendMode == UI_PROGRESS_BLEND_MODE::ADDITIVE ? 1 : 0;

	command.fRevealProgress = m_tRenderData.fRevealProgress;
	command.fRevealSoftness = m_tRenderData.fRevealSoftness;
	command.vRevealCenter = m_tRenderData.vRevealCenter;
	command.iRevealType = static_cast<_int>(m_tRenderData.eRevealType);

	m_pGameInstance->Submit_UI(command);
}

_float CUIProgressBar::Clamp01(_float fValue) const
{
	return min(1.f, max(0.f, fValue));
}

void CUIProgressBar::Clear_Slots()
{
	for (auto& Slot : m_Slots)
		Safe_Release(Slot.pMaterial);

	m_Slots.clear();
}

CUIProgressBar* CUIProgressBar::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CUIProgressBar* pInstance = new CUIProgressBar(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CUIProgressBar");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CUIProgressBar::Free()
{
	Clear_Slots();

	__super::Free();
}
