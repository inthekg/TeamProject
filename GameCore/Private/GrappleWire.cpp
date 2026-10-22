#include "GrappleWire.h"
#include "GameInstance.h"
#include "VIBuffer_Trail.h"
#include "RenderStruct.h"
#include "Engine_Utility.h"
#include "Effect_Emitter.h"

CGrappleWire::CGrappleWire()
	: m_pGameInstance{ CGameInstance::Get_Instance() }
{
}

HRESULT CGrappleWire::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const WIRE_DESC& Desc)
{
	m_Desc = Desc;
	m_Desc.iSegments = max(min(m_Desc.iSegments, CVIBuffer_Trail::MAX_INPUT_POINTS), 2u);
	
	m_CurPos.resize(m_Desc.iSegments);
	m_PrevPos.resize(m_Desc.iSegments);

	m_pBuffer = CVIBuffer_Trail::Create(pDevice, pContext);
	if (m_pBuffer == nullptr)
	{
		MSG_BOX("CGrapple_Wire : Buffer Create Failed");
		return E_FAIL;
	}

	m_pShaderBundle = m_pGameInstance->Get_EffectShaderBundle();
	if (m_pShaderBundle == nullptr || m_pShaderBundle->pTrailVS == nullptr || m_pShaderBundle->pTrailPS == nullptr)
	{
		MSG_BOX("CGrapple_Wire : Effect Shader Bundle Load Failed");
		return E_FAIL;
	}

	return S_OK;
}

void CGrappleWire::Update(_float fTimeDelta)
{
	if (!m_bVisible)
		return;

	m_fSubStepAccum += min(fTimeDelta, MAX_TIMEDELTA);

	//속도는 매번 같은 시간 간격인 베를레의 전제를 맞추기 위해 고정 시간 스텝
	while(m_fSubStepAccum >= m_Desc.fSubStepTime)
	{
		m_fSubStepAccum -= m_Desc.fSubStepTime;
		Update_Points(m_Desc.fSubStepTime);
		Solve_Constraints();
	}

	//리본 갱신
	const _float4x4* pView = m_pGameInstance->Get_Transform(D3DTS::VIEW);
	const _float4x4* pProj = m_pGameInstance->Get_Transform(D3DTS::PROJ);
	const _float4* pCamPos = m_pGameInstance->Get_CamPosition();
	
	if (!pView || !pProj || !pCamPos) return;

	TRAIL_DRAW_DESC Desc{};
	Desc.vColor = m_Desc.vColor;
	Desc.fWidth = m_Desc.fWidth;
	Desc.fTailWidthScale = 1.f;
	Desc.vCamPos = _float3(pCamPos->x, pCamPos->y, pCamPos->z);
	Desc.bFadeOut = false;
	Desc.bSpline = true;		

	const _float2 vVp = m_pGameInstance->Get_ViewportDesc();
	Desc.vScreen = _float2(vVp.x > 1.f ? vVp.x : 1.f, vVp.y > 1.f ? vVp.y : 1.f);
	Desc.pView = pView;
	Desc.pProj = pProj;

	m_pBuffer->Update_Ribbon(m_CurPos, Desc);
}

//베를레 - 속도를 저장하지 않고 (현재 위치− 이전 위치)를 속도로 봄 
void CGrappleWire::Update_Points(_float fTimeDelta)
{
	const _uint iLastPoint = m_Desc.iSegments - 1;
	const _float fGravityDt2 = PhysX::g_fGravity * fTimeDelta * fTimeDelta * m_Profile.fGravityScale;

	//양끝점은 적분 제외
	for (_uint i = 1; i < iLastPoint; i++)
	{
		_vector vCur = XMLoadFloat3(&m_CurPos[i]);
		_vector vVel = (vCur - XMLoadFloat3(&m_PrevPos[i])) * m_Profile.fDamping;

		XMStoreFloat3(&m_CurPos[i], vCur + vVel + XMVectorSet(0.f, fGravityDt2, 0.f, 0.f));
		XMStoreFloat3(&m_PrevPos[i], vCur);
	}

}
 
//늘어진 마디의 간격을 당겨 보정함
void CGrappleWire::Solve_Constraints()
{
	const _uint iLastPoint = m_Desc.iSegments - 1;
	
	_vector vStart = XMLoadFloat3(&m_vStart);
	_vector vEnd = XMLoadFloat3(&m_vEnd);
	_float fDist = XMVectorGetX(XMVector3Length(vEnd - vStart));
	//제약 길이
	_float fRestLen = (fDist * m_Profile.fSlack) / static_cast<_float>(iLastPoint);

	for (_uint iCount = 0; iCount < m_Desc.iSolverCount; iCount++)
	{
		for (_uint i = 0; i < iLastPoint; i++)
		{
			_vector vAPoint = XMLoadFloat3(&m_CurPos[i]);
			_vector vBPoint = XMLoadFloat3(&m_CurPos[i + 1]);

			_vector vDelta = vBPoint - vAPoint;
			_float fLen = XMVectorGetX(XMVector3Length(vDelta));
			
			if (IsEqual(fLen, 0.f)) continue;
			
			_float fOverRatio = (fLen - fRestLen) / fLen;
			if (fOverRatio <= 0.f) continue;

			//끝점은 고정(0) 나머지는 1. 넘친 길이를 비율대로 분담함 
			_float fValueA = (i == 0) ? 0.f : 1.f;
			_float fValueB = (i + 1 == iLastPoint) ? 0.f : 1.f;
			_float fValueSum = fValueA + fValueB;
			if (fValueSum <= 0.f) continue;

			const _float fWeightA = fValueA / fValueSum;
			const _float fWeightB = fValueB / fValueSum;

			XMStoreFloat3(&m_CurPos[i], vAPoint + vDelta * fOverRatio * fWeightA);
			XMStoreFloat3(&m_CurPos[i + 1], vBPoint - vDelta * fOverRatio * fWeightB);
		}
	}

}


HRESULT CGrappleWire::Render()
{
	if (!m_bVisible)
		return S_OK;

	if (m_pGameInstance->Get_CurRenderPass() != RENDERID::FORWARD)
		return S_OK;

	EffectCommand cmd{};
	cmd.header.etype = RENDER_TYPE::EFFECT;
	cmd.header.iCommandSize = sizeof(EffectCommand);
	cmd.pVIBuffer = m_pBuffer;
	cmd.pVS = m_pShaderBundle->pTrailVS;
	cmd.pPS = m_pShaderBundle->pTrailPS;
	cmd.pLayout = m_pShaderBundle->pTrailLayout;
	cmd.pMaterial = nullptr;
	cmd.iBlendMode = 0; //알파블랜드로 
	cmd.bZWrite = 0;
	cmd.bZTest = 1;
	cmd.iAlignment = 2;
	XMStoreFloat4x4(&cmd.worldMatrix, XMMatrixIdentity());

	m_pGameInstance->Submit_Effect(cmd);

	return S_OK;
}

void CGrappleWire::Activate(_fvector vStart, _fvector vEnd)
{
	XMStoreFloat3(&m_vStart, vStart);
	XMStoreFloat3(&m_vEnd, vEnd);

	const _uint iLast = m_Desc.iSegments - 1;
	for (_uint i = 0; i <= iLast; i++)
	{
		_float fRatio = i / static_cast<_float>(iLast);
		XMStoreFloat3(&m_CurPos[i], XMVectorLerp(vStart, vEnd, fRatio));
		m_PrevPos[i] = m_CurPos[i];
	}

	m_fSubStepAccum = 0.f;
	m_bVisible = true;
}

void CGrappleWire::Set_Endpoints(_fvector vStart, _fvector vEnd)
{
	XMStoreFloat3(&m_vStart, vStart);
	XMStoreFloat3(&m_vEnd, vEnd);

	m_CurPos.front() = m_vStart;
	m_CurPos.back() = m_vEnd;
}


CGrappleWire* CGrappleWire::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const WIRE_DESC& Desc)
{
	CGrappleWire* pInstance = new CGrappleWire();
	if (FAILED(pInstance->Initialize(pDevice, pContext, Desc)))
	{
		MSG_BOX("Failed to Created : CGrappleWire");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CGrappleWire::Free()
{
	Safe_Release(m_pBuffer);
	__super::Free();
}
