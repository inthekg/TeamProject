#include "UIImage.h"
#include "GameInstance.h"
#include "Shader.h"

CUIImage::CUIImage(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CUIElement { pDevice, pContext }
{
}

void CUIImage::Set_DrawType(UI_IMAGE_DRAW_TYPE eDrawType)
{
	m_eDrawType = eDrawType;
}

void CUIImage::Set_SliceMargin(const _float4& vSliceMargin)
{
	m_vSliceMargin = vSliceMargin;
}

void CUIImage::Set_TextureSize(const _float2& vTextureSize)
{
	m_vTextureSize = vTextureSize;
}

void CUIImage::Set_SliceBaseSize(const _float2& vSliceBaseSize)
{
	m_vSliceBaseSize = vSliceBaseSize;
}

HRESULT CUIImage::Initialize()
{
	return S_OK;
}

void CUIImage::Priority_Update(_float fTimeDelta)
{
}

void CUIImage::Update(_float fTimeDelta)
{
}

void CUIImage::Late_Update(_float fTimeDelta)
{
}

void CUIImage::Submit_RenderCommand()
{
	Submit_RenderCommand(0);
}

void CUIImage::Submit_RenderCommand(_uint iObjectSortOrder)
{
	if (m_eDrawType == UI_IMAGE_DRAW_TYPE::NORMAL)
	{
		__super::Submit_RenderCommand(iObjectSortOrder);
		return;
	}

	Submit_NineSliceRenderCommand(iObjectSortOrder);
}

void CUIImage::Submit_NineSliceRenderCommand(_uint iObjectSortOrder)
{
	if (Is_HierarchyVisible() == false || m_tRenderData.pShader == nullptr || m_tRenderData.pVIBuffer == nullptr)
		return;

	// fLeft/fTop/fRight/fBottom = 화면에 그릴 고정 모서리 크기
	// u1/u2/v1/v2 = 텍스쳐에서 잘라 쓸 UV 경계
	const _float fWidth = m_tTransform.vSize.x * m_tTransform.vScale.x;
	const _float fHeight = m_tTransform.vSize.y * m_tTransform.vScale.y;

	_float fLeft = min(m_vSliceMargin.x, fWidth * 0.5f);
	_float fRight = min(m_vSliceMargin.z, fWidth * 0.5f);
	_float fTop = min(m_vSliceMargin.y, fHeight * 0.5f);
	_float fBottom = min(m_vSliceMargin.w, fHeight * 0.5f);

	const _float fCenterW = max(fWidth - fLeft - fRight, 0.f);
	const _float fCenterH = max(fHeight - fTop - fBottom, 0.f);

	const _float fBaseW = max(m_vSliceBaseSize.x, 1.f);
	const _float fBaseH = max(m_vSliceBaseSize.y, 1.f);
	 
	if (fLeft + fRight > fBaseW)
	{
		const _float fRatio = fBaseW / (fLeft + fRight);
		fLeft *= fRatio;
		fRight *= fRatio;
	}

	if (fTop + fBottom > fBaseH)
	{
		const _float fRatio = fBaseH / (fTop + fBottom);
		fTop *= fRatio;
		fBottom *= fRatio;
	}

	const _float u0 = m_tRenderData.vUVRect.x;
	const _float v0 = m_tRenderData.vUVRect.y;
	const _float u3 = m_tRenderData.vUVRect.z;
	const _float v3 = m_tRenderData.vUVRect.w;

	const _float uScale = u3 - u0;
	const _float vScale = v3 - v0;

	const _float u1 = u0 + (m_vSliceMargin.x / fBaseW) * uScale;
	const _float u2 = u3 - (m_vSliceMargin.z / fBaseW) * uScale;

	const _float v1 = v0 + (m_vSliceMargin.y / fBaseH) * vScale;
	const _float v2 = v3 - (m_vSliceMargin.w / fBaseH) * vScale;

	const _float fXSize[3] = { fLeft, fCenterW, fRight };
	const _float fYSize[3] = { fTop, fCenterH, fBottom };

	const _float fXEdge[4] =
	{
		-fWidth * m_tTransform.vPivot.x,
		-fWidth * m_tTransform.vPivot.x + fLeft,
		fWidth * (1.f - m_tTransform.vPivot.x) - fRight,
		fWidth * (1.f - m_tTransform.vPivot.x)
	};

	const _float fYEdge[4] =
	{
		fHeight * m_tTransform.vPivot.y,
		fHeight * m_tTransform.vPivot.y - fTop,
		-fHeight * (1.f - m_tTransform.vPivot.y) + fBottom,
		-fHeight * (1.f - m_tTransform.vPivot.y)
	};

	const _float fUEdge[4] = { u0, u1, u2, u3 };
	const _float fVEdge[4] = { v0, v1, v2, v3 };

	const _float fRadian = m_tTransform.fRotation;
	const _float fCos = cosf(fRadian);
	const _float fSin = sinf(fRadian);

	const _uint iFinalSortOrder = iObjectSortOrder * UI_ELEMENT_SORT_RANGE + m_tRenderData.iSortOrder;

	const _float2 vAnchoredPosition = Get_AnchoredPosition();

	for (_uint y = 0; y < 3; ++y)
	{
		for (_uint x = 0; x < 3; ++x)
		{
			if (fXSize[x] <= 0.f || fYSize[y] <= 0.f)
				continue;

			const _float fLocalCenterX = (fXEdge[x] + fXEdge[x + 1]) * 0.5f;
			const _float fLocalCenterY = (fYEdge[y] + fYEdge[y + 1]) * 0.5f;

			const _float fRotatedX = fLocalCenterX * fCos - fLocalCenterY * fSin;
			const _float fRotatedY = fLocalCenterX * fSin + fLocalCenterY * fCos;

			const _float2 vPieceCenter =
			{
				vAnchoredPosition.x + fRotatedX,
				vAnchoredPosition.y + fRotatedY
			};

			const _float2 vPieceSize =
			{
				fXSize[x],
				fYSize[y]
			};

			const  _float4 vPieceUV =
			{
				fUEdge[x],
				fVEdge[y],
				fUEdge[x + 1],
				fVEdge[y + 1],
			};

			Submit_NineSlicePiece(iFinalSortOrder, vPieceCenter, vPieceSize, vPieceUV);
		}
	}
}

void CUIImage::Submit_NineSlicePiece(_uint iFinalSortOrder, const _float2& vCenter, const _float2& vSize, const _float4& vUVRect)
{
	UICommand command = {};

	command.header.etype = RENDER_TYPE::UI;
	command.header.iCommandSize = sizeof(UICommand);
	command.header.iSortKey = iFinalSortOrder;

	XMStoreFloat4x4(
		&command.worldMatrix,
		XMMatrixScaling(vSize.x, vSize.y, 1.f) *
		XMMatrixRotationZ(m_tTransform.fRotation) *
		XMMatrixTranslation(vCenter.x, vCenter.y, 0.f) * 
		Get_ParentWorldLayoutMatrix());

	command.pVIBuffer = m_tRenderData.pVIBuffer;
	command.pVS = m_tRenderData.pShader->Get_VertexShader();
	command.pPS = m_tRenderData.pShader->Get_PixelShader();
	command.pLayout = m_tRenderData.pShader->Get_InputLayout();
	command.pMaterial = m_tRenderData.pMaterial;
	_float4 vFinalColor = m_tRenderData.vColor;

	// Nine Slice의 9개 조각 모두 동일한 계층 Alpha를 사용한다.
	vFinalColor.w = Get_HierarchyAlpha();

	command.vColor = vFinalColor;
	command.vColorLT = vFinalColor;
	command.vColorRT = vFinalColor;
	command.vColorRB = vFinalColor;
	command.vColorLB = vFinalColor;

	command.vUVRect = vUVRect;
	command.fFillAmount = 1.f;
	command.iUseTexture = m_tRenderData.bUseTexture ? 1 : 0;
	command.iBlendMode = 0;

	command.fRevealProgress = m_tRenderData.fRevealProgress;
	command.fRevealSoftness = m_tRenderData.fRevealSoftness;
	command.vRevealCenter = m_tRenderData.vRevealCenter;
	command.iRevealType = static_cast<_int>(m_tRenderData.eRevealType);

	m_pGameInstance->Submit_UI(command);
}

CUIImage* CUIImage::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CUIImage* pInstance = new CUIImage(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CUIImage");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CUIImage::Free()
{
	__super::Free();
}
