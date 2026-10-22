#include "PipeLine.h"

CPipeLine::CPipeLine()
{
	for (size_t i = 0; i < ETOUI(D3DTS::END); ++i)
	{
		XMStoreFloat4x4(&m_TransformStateMatrices[i], XMMatrixIdentity());
		XMStoreFloat4x4(&m_TransformStateInverseMatrices[i], XMMatrixIdentity());
	}
}

void CPipeLine::Update()
{
	for (size_t i = 0; i < ETOUI(D3DTS::END); ++i)
	{		
		XMStoreFloat4x4(&m_TransformStateInverseMatrices[i], 
			XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_TransformStateMatrices[i])));
	}

	memcpy(&m_vCamPosition, &m_TransformStateInverseMatrices[ETOUI(D3DTS::VIEW)]._41, sizeof m_vCamPosition);

	memcpy(&m_vCamRight, &m_TransformStateInverseMatrices[ETOUI(D3DTS::VIEW)]._11, sizeof m_vCamRight);
	memcpy(&m_vCamLook, &m_TransformStateInverseMatrices[ETOUI(D3DTS::VIEW)]._31, sizeof m_vCamLook);

	XMStoreFloat4(&m_vCamRight, XMVector3Normalize(XMLoadFloat4(&m_vCamRight)));
	XMStoreFloat4(&m_vCamLook, XMVector3Normalize(XMLoadFloat4(&m_vCamLook)));
}

void CPipeLine::Fill_CameraData(PerCameraData& cameraData)
{
	cameraData.ViewMatrix = m_TransformStateMatrices[ETOUI(D3DTS::VIEW)];
	cameraData.ProjMatrix = m_TransformStateMatrices[ETOUI(D3DTS::PROJ)];
	cameraData.ViewInvMatrix = m_TransformStateInverseMatrices[ETOUI(D3DTS::VIEW)];
	cameraData.ProjInvMatrix = m_TransformStateInverseMatrices[ETOUI(D3DTS::PROJ)];
	cameraData.vCamPos = m_vCamPosition;

	// _33 = f / (f - n)  _43 = -nf / (f - n) 		
	const _float4x4 matProj = m_TransformStateMatrices[ETOUI(D3DTS::PROJ)];
	//_float fFar = 1.f - matProj._33 == 0.f ? 500.f :  matProj._43 / (1.f - matProj._33);
	_float fFar = 500.f;

	cameraData.vCamPos.w = fFar;

}

CPipeLine* CPipeLine::Create()
{
	return new CPipeLine();
}

void CPipeLine::Free()
{
	__super::Free();
}
