#include "VIBuffer_Trail.h"

namespace
{
	struct TrailCB
	{
		_float4x4	matVP;
		_float4		vFlags;
		_float4		vTEAP;
		_float4		vBaseTS;
		_float4		vPowDens;
		_float4		vNoiseTS;
		_float4		vMisc;
		_float4		vMisc2;
	};
}

static const _uint TRAIL_CAM_SLOT = 4;

CVIBuffer_Trail::CVIBuffer_Trail(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CVIBuffer { pDevice, pContext }
{

}

CVIBuffer_Trail::CVIBuffer_Trail(const CVIBuffer_Trail& Prototype)
	: CVIBuffer { Prototype }
{

}

HRESULT CVIBuffer_Trail::Initialize_Prototype()
{
	Set_Name(TEXT("VIBUFFER_TRAIL"));

	m_iNumVertexBuffers = 1;
	m_iNumVertices = MAX_OUTPUT_POINTS * 2;
	m_iVertexStride = sizeof(VTXCOLORTEX);
	m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

	D3D11_BUFFER_DESC vbDesc{};
	vbDesc.ByteWidth = m_iVertexStride * m_iNumVertices;
	vbDesc.Usage = D3D11_USAGE_DYNAMIC;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	VTXCOLORTEX* pVertices = new VTXCOLORTEX[m_iNumVertices];
	ZeroMemory(pVertices, sizeof(VTXCOLORTEX) * m_iNumVertices);

	D3D11_SUBRESOURCE_DATA vbInit{};
	vbInit.pSysMem = pVertices;

	HRESULT hr = m_pDevice->CreateBuffer(&vbDesc, &vbInit, &m_pVB);
	Safe_Delete_Array(pVertices);
	if (FAILED(hr))
		return E_FAIL;

	D3D11_BUFFER_DESC cbDesc{};
	cbDesc.ByteWidth = sizeof(TrailCB);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	if (FAILED(m_pDevice->CreateBuffer(&cbDesc, nullptr, &m_pCameraCB)))
		return E_FAIL;

	return S_OK;
}

HRESULT CVIBuffer_Trail::Initialize(void* pArg)
{
	return S_OK;
}

void CVIBuffer_Trail::Build_SplinePoints(const vector<_float3>& In, vector<_float3>& Out, _bool bSpline)
{
	Out.clear();

	const size_t n = In.size();
	if (!bSpline || n < 3)
	{
		const size_t iCopy = (n > MAX_OUTPUT_POINTS) ? MAX_OUTPUT_POINTS : n;
		for (size_t i = 0; i < iCopy; ++i)
			Out.push_back(In[i]);
		return;
	}

	const size_t segs = n - 1;

	_uint subdiv = MAX_SUBDIV;
	while (subdiv > 1 && (segs * subdiv + 1) > MAX_OUTPUT_POINTS)
		--subdiv;

	auto NextKnot = [](_float t, _fvector a, _fvector b) -> _float
		{
			const _float d = XMVectorGetX(XMVector3Length(XMVectorSubtract(b, a)));
			return t + sqrtf(d) + 1e-5f;
		};

	for (size_t i = 0; i < segs; ++i)
	{
		_vector p0 = XMLoadFloat3(&In[(i == 0) ? 0 : i - 1]);
		_vector p1 = XMLoadFloat3(&In[i]);
		_vector p2 = XMLoadFloat3(&In[i + 1]);
		_vector p3 = XMLoadFloat3(&In[(i + 2 < n) ? i + 2 : n - 1]);

		const _float t0 = 0.f;
		const _float t1 = NextKnot(t0, p0, p1);
		const _float t2 = NextKnot(t1, p1, p2);
		const _float t3 = NextKnot(t2, p2, p3);

		for (_uint s = 0; s < subdiv; ++s)
		{
			const _float u = s / static_cast<_float>(subdiv);
			const _float t = t1 + u * (t2 - t1);

			_vector A1 = XMVectorLerp(p0, p1, (t - t0) / (t1 - t0));
			_vector A2 = XMVectorLerp(p1, p2, (t - t1) / (t2 - t1));
			_vector A3 = XMVectorLerp(p2, p3, (t - t2) / (t3 - t2));
			_vector B1 = XMVectorLerp(A1, A2, (t - t0) / (t2 - t0));
			_vector B2 = XMVectorLerp(A2, A3, (t - t1) / (t3 - t1));
			_vector C = XMVectorLerp(B1, B2, (t - t1) / (t2 - t1));

			_float3 pt;
			XMStoreFloat3(&pt, C);
			Out.push_back(pt);
			if (Out.size() >= MAX_OUTPUT_POINTS)
				return;
		}
	}

	Out.push_back(In[n - 1]);
	if (Out.size() > MAX_OUTPUT_POINTS)
		Out.resize(MAX_OUTPUT_POINTS);
}

void CVIBuffer_Trail::Update_Ribbon(const vector<_float3>& Points, const TRAIL_DRAW_DESC& Desc)
{
	if (Points.size() < 2 || Desc.pView == nullptr || Desc.pProj == nullptr)
	{
		m_iDrawVertexCount = 0;
		return;
	}

	vector<_float3> Spline;
	Build_SplinePoints(Points, Spline, Desc.bSpline);

	const _uint iPoints = static_cast<_uint>(Spline.size());
	if (iPoints < 2)
	{
		m_iDrawVertexCount = 0;
		return;
	}

	vector<_float3> SplineHalf;
	_bool bDual = false;
	if (Desc.bDualSocket && Desc.pHalf != nullptr && Desc.pHalf->size() == Points.size())
	{
		Build_SplinePoints(*Desc.pHalf, SplineHalf, Desc.bSpline);
		bDual = (static_cast<_uint>(SplineHalf.size() == iPoints));
	}

	TrailCB cb{};
	XMStoreFloat4x4(&cb.matVP, XMMatrixTranspose(XMLoadFloat4x4(Desc.pView) * XMLoadFloat4x4(Desc.pProj)));
	cb.vFlags = _float4(
		(m_pDiffuseSRV != nullptr) ? 1.f : 0.f,
		(m_pNoiseSRV != nullptr) ? 1.f : 0.f,
		Desc.bUseAlphaY ? 1.f : 0.f,
		static_cast<_float>(Desc.iAlphaYMode));
	cb.vTEAP = _float4(Desc.fTime, Desc.fEmiIntensity, Desc.fAlphaYPower,
		(Desc.bUseDistortion && m_pSceneSRV != nullptr) ? 1.f : 0.f);
	cb.vBaseTS = _float4(Desc.vBaseTile.x, Desc.vBaseTile.y, Desc.vBaseSpeed.x, Desc.vBaseSpeed.y);
	cb.vPowDens = _float4(Desc.fBasePower, Desc.fBaseDensity, Desc.fNoisePower, Desc.fNoiseDensity);
	cb.vNoiseTS = _float4(Desc.vNoiseTile.x, Desc.vNoiseTile.y, Desc.vNoiseSpeed.x, Desc.vNoiseSpeed.y);
	cb.vMisc = _float4(Desc.fAlphaYPower2, Desc.fDistortion, Desc.vScreen.x, Desc.vScreen.y);
	cb.vMisc2 = _float4(Desc.fAlphaYPivot, 0.f, 0.f, 0.f);
	m_pContext->UpdateSubresource(m_pCameraCB, 0, nullptr, &cb, 0, 0);

	vector<_float> cumLen(iPoints, 0.f);
	for (_uint i = 1; i < iPoints; ++i)
	{
		const _float3& fA = Spline[i - 1];
		const _float3& fB = Spline[i];
		const _float fDx = fB.x - fA.x, fDy = fB.y - fA.y, fDz = fB.z - fA.z;
		cumLen[i] = cumLen[i - 1] + sqrt(fDx * fDx + fDy * fDy + fDz * fDz);
	}

	_vector vCamPos = XMLoadFloat3(&Desc.vCamPos);
	const _float fHalf = Desc.fWidth * 0.5f;
	const _float fTileLen = (Desc.fWidth > 1e-4f) ? Desc.fWidth : 1e-4f;
	const _int iLUT = static_cast<_int>(Desc.iColorLUTSize);

	D3D11_MAPPED_SUBRESOURCE mapped{};

	if (FAILED(m_pContext->Map(m_pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
	{
		m_iDrawVertexCount = 0;
		return;
	}

	VTXCOLORTEX* pV = reinterpret_cast<VTXCOLORTEX*>(mapped.pData);

	_vector vPrevSide = XMVectorZero();

	for (_uint i = 0; i < iPoints; ++i)
	{
		_vector vPos = XMLoadFloat3(&Spline[i]);
		_vector vHalf;

		const _float fT = (iPoints > 1) ? (i / static_cast<_float>(iPoints - 1)) : 1.f;

		if (bDual)
		{
			vHalf = XMLoadFloat3(&SplineHalf[i]);
		}
		else
		{
			_vector vTangent;

			if (i == 0)
				vTangent = XMVectorSubtract(XMLoadFloat3(&Spline[1]), XMLoadFloat3(&Spline[0]));
			else if (i == iPoints - 1)
				vTangent = XMVectorSubtract(XMLoadFloat3(&Spline[iPoints - 1]), XMLoadFloat3(&Spline[iPoints - 2]));
			else
				vTangent = XMVectorSubtract(XMLoadFloat3(&Spline[i + 1]), XMLoadFloat3(&Spline[i - 1]));

			_vector vView = XMVectorSubtract(vCamPos, vPos);
			_vector vSide = XMVector3Cross(vTangent, vView);

			if (XMVectorGetX(XMVector3LengthSq(vSide)) < 1e-8f)
			{
				vSide = (i > 0) ? vPrevSide : XMVectorSet(1.f, 0.f, 0.f, 0.f);
			}
			else
			{
				vSide = XMVector3Normalize(vSide);
				if (i > 0 && XMVectorGetX(XMVector3Dot(vSide, vPrevSide)) < 0.f)
					vSide = XMVectorNegate(vSide);
			}
			vPrevSide = vSide;

			if (Desc.fWaveAmplitude != 0.f)
			{
				const _float fPhase = cumLen[i] * XM_2PI + Desc.fTime * Desc.fSinSpeed;
				vPos = XMVectorAdd(vPos, XMVectorScale(vSide, sinf(fPhase) * Desc.fWaveAmplitude));
			}

			vHalf = XMVectorScale(vSide, fHalf);
		}

		const _float fWidthScale = Desc.fTailWidthScale + (1.f - Desc.fTailWidthScale) * fT;
		vHalf = XMVectorScale(vHalf, fWidthScale);

		_vector vTop = XMVectorAdd(vPos, vHalf);
		_vector vBot = XMVectorSubtract(vPos, vHalf);

		_float4 vBase = Desc.vColor;
		if (Desc.bColorCurve && Desc.pColorLUT != nullptr && iLUT > 0)
		{
			const _float fCurveT = 1.f - fT;
			_int idx = static_cast<_int>(fCurveT * (iLUT - 1) + 0.5f);
			if (idx < 0) idx = 0;
			if (idx > iLUT - 1) idx = iLUT - 1;
			vBase = Desc.pColorLUT[idx];
		}
		else if (m_pDiffuseSRV != nullptr)
		{
			vBase = _float4(1.f, 1.f, 1.f, 1.f);
		}

		const _float fAlpha = vBase.w * (Desc.bFadeOut ? fT : 1.f);
		const _float4 vCol = _float4(vBase.x, vBase.y, vBase.z, fAlpha);
		
		const _float fU = (Desc.iTexMode == 1) ? (cumLen[i] / fTileLen) : fT;

		VTXCOLORTEX& top = pV[i * 2];
		VTXCOLORTEX& bot = pV[i * 2 + 1];
		XMStoreFloat3(&top.vPosition, vTop);
		XMStoreFloat3(&bot.vPosition, vBot);
		top.vColor = vCol; bot.vColor = vCol;
		top.vTexcoord = _float2(fU, 0.f);
		bot.vTexcoord = _float2(fU, 1.f);

	}

	m_pContext->Unmap(m_pVB, 0);
	m_iDrawVertexCount = iPoints * 2;
}

HRESULT CVIBuffer_Trail::Bind_Resources()
{
	ID3D11Buffer* pBuffers[] = { m_pVB };
	_uint iStrides[] = { m_iVertexStride };
	_uint iOffsets[] = { 0 };

	m_pContext->IASetVertexBuffers(0, m_iNumVertexBuffers, pBuffers, iStrides, iOffsets);
	m_pContext->IASetPrimitiveTopology(m_ePrimitiveType);
	m_pContext->VSSetConstantBuffers(TRAIL_CAM_SLOT, 1, &m_pCameraCB);
	m_pContext->PSSetConstantBuffers(TRAIL_CAM_SLOT, 1, &m_pCameraCB);
	m_pContext->PSSetShaderResources(0, 1, &m_pDiffuseSRV);
	m_pContext->PSSetShaderResources(1, 1, &m_pNoiseSRV);
	m_pContext->PSSetShaderResources(3, 1, &m_pSceneSRV);

	return S_OK;
}

HRESULT CVIBuffer_Trail::Render() 
{
	if (m_iDrawVertexCount >= 4)
		m_pContext->Draw(m_iDrawVertexCount, 0);

	return S_OK;
}

CVIBuffer_Trail* CVIBuffer_Trail::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CVIBuffer_Trail* pInstance = new CVIBuffer_Trail(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CVIBuffer_Trail");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CComponent* CVIBuffer_Trail::Clone(void* pArg)
{
	CVIBuffer_Trail* pInstance = new CVIBuffer_Trail(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CVIBuffer_Trail");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CVIBuffer_Trail::Free()
{
	Safe_Release(m_pCameraCB);
	__super::Free();
}