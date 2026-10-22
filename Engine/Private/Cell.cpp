#include "Cell.h"

CCell::CCell()
{

}

HRESULT CCell::Initialize(const CELL_DESC& Desc)
{
	const _float fAreaEpsilon = 0.0001f;
	const _float fSignedArea = Compute_SignedAreaXZ(Desc.vPoints[0], Desc.vPoints[1], Desc.vPoints[2]);

	if (fabsf(fSignedArea) <= fAreaEpsilon)
	{
		MSG_BOX("Invalid Navigation Cell Area");
		return E_FAIL;
	}

	for (_uint i = 0; i < 3; i++)
	{
		m_vPoints[i] = Desc.vPoints[i];
		m_iNeighborIndices[i] = Desc.iNeighborIndices[i];
	}

	m_bClockwise = fSignedArea < 0.f;

	return S_OK;
}

const _float3& CCell::Get_Point(_uint iPointIndex) const
{
	static const _float3 vInvalidPoint = {};

	if (iPointIndex >= 3)
		return vInvalidPoint;

	return m_vPoints[iPointIndex];
}

_int CCell::Get_NeighborIndex(_uint iEdgeIndex) const
{
	if (iEdgeIndex >= 3)
		return -1;

	return m_iNeighborIndices[iEdgeIndex];
}

_float3 CCell::Get_Center() const
{
	return _float3(
		(m_vPoints[0].x + m_vPoints[1].x + m_vPoints[2].x) / 3.f, 
		(m_vPoints[0].y + m_vPoints[1].y + m_vPoints[2].y) / 3.f,
		(m_vPoints[0].z + m_vPoints[1].z + m_vPoints[2].z) / 3.f);
}

_bool CCell::Is_In(const _float3& vPosition, _uint* pOutExitEdgeIndex) const
{
	const _float fInsideEpsilon = 0.0001f;
	_float fLargestOutsideDistance = 0.f;
	_uint iExitEdgeIndex = UINT_MAX;
	
	for (_uint i = 0; i < 3; ++i)
	{
		const _uint iNextPointIndex = (i + 1) % 3;
		const _float fEdgeSide = Compute_EdgeSideXZ(m_vPoints[i], m_vPoints[iNextPointIndex], vPosition);
		const _float fOutSideDistance = m_bClockwise ? fEdgeSide : -fEdgeSide;

		if (fOutSideDistance <= fInsideEpsilon)
			continue;

		if (UINT_MAX == iExitEdgeIndex || fOutSideDistance > fLargestOutsideDistance)
		{
			fLargestOutsideDistance = fOutSideDistance;
			iExitEdgeIndex = i;
		}
	}

	if (nullptr != pOutExitEdgeIndex)
		*pOutExitEdgeIndex = iExitEdgeIndex;
	
	return UINT_MAX == iExitEdgeIndex;
}

_bool CCell::Compute_Height(const _float3& vPosition, _float* pOutHeight) const
{
	const _float fAreaEpsilon = 0.0001f;

	if (nullptr == pOutHeight)
		return false;

	const _float fDenominator = (m_vPoints[1].z - m_vPoints[2].z) * (m_vPoints[0].x - m_vPoints[2].x) + (m_vPoints[2].x - m_vPoints[1].x) * (m_vPoints[0].z - m_vPoints[2].z);

	if (fabsf(fDenominator) <= fAreaEpsilon)
		return false;

	const _float fWeightA = ((m_vPoints[1].z - m_vPoints[2].z) * (vPosition.x - m_vPoints[2].x) + (m_vPoints[2].x - m_vPoints[1].x) * (vPosition.z - m_vPoints[2].z)) / fDenominator;
	const _float fWeightB = ((m_vPoints[2].z - m_vPoints[0].z) * (vPosition.x - m_vPoints[2].x) + (m_vPoints[0].x - m_vPoints[2].x) * (vPosition.z - m_vPoints[2].z)) / fDenominator;
	const _float fWeightC = 1.f - fWeightA - fWeightB;

	*pOutHeight = fWeightA * m_vPoints[0].y + fWeightB * m_vPoints[1].y + fWeightC * m_vPoints[2].y;

	return true;
}

_bool CCell::Is_SameEdge(_uint iEdgeIndex, const CCell* pOtherCell, _uint* pOutOtherEdgeIndex) const
{
	if (iEdgeIndex >= 3 || nullptr == pOtherCell)
		return false;

	const _uint iNextPointIndex = (iEdgeIndex + 1) % 3;
	const _float3& vEdgeStart = m_vPoints[iEdgeIndex];
	const _float3& vEdgeEnd = m_vPoints[iNextPointIndex];

	for (_uint i = 0; i < 3; ++i)
	{
		const _uint iOtherNextPointIndex = (i + 1) % 3;
		const _float3& vOtherEdgeStart = pOtherCell->Get_Point(i);
		const _float3& vOtherEdgeEnd = pOtherCell->Get_Point(iOtherNextPointIndex);
		const _bool bSameDirection = Is_SamePoint(vEdgeStart, vOtherEdgeStart) && Is_SamePoint(vEdgeEnd, vOtherEdgeEnd);
		const _bool bReverseDirection = Is_SamePoint(vEdgeStart, vOtherEdgeEnd) && Is_SamePoint(vEdgeEnd, vOtherEdgeStart);

		if (false == bSameDirection && false == bReverseDirection)
			continue;

		if (nullptr != pOutOtherEdgeIndex)
			*pOutOtherEdgeIndex = i;

		return true;
	}

	return false;

}

_bool CCell::Get_SharedEdge(const CCell* pOtherCell, _float3* pOutPointA, _float3* pOutPointB) const
{
	if (nullptr == pOtherCell || nullptr == pOutPointA || nullptr == pOutPointB)
		return false;

	for (_uint i = 0; i < 3; ++i)
	{
		const _uint iNextPointIndex = (i + 1) % 3;
		const _float3& vEdgeStart = m_vPoints[i];
		const _float3& vEdgeEnd = m_vPoints[iNextPointIndex];

		for (_uint j = 0; j < 3; ++j)
		{
			const _uint iOtherNextPointIndex = (j + 1) % 3;
			const _float3& vOtherEdgeStart = pOtherCell->Get_Point(j);
			const _float3& vOtherEdgeEnd = pOtherCell->Get_Point(iOtherNextPointIndex);

			const _bool bSameDirection = Is_SamePoint(vEdgeStart, vOtherEdgeStart) && Is_SamePoint(vEdgeEnd, vOtherEdgeEnd);
			const _bool bReverseDirection = Is_SamePoint(vEdgeStart, vOtherEdgeEnd) && Is_SamePoint(vEdgeEnd, vOtherEdgeStart);

			if (false == bSameDirection && false == bReverseDirection)
				continue;

			*pOutPointA = vEdgeStart;
			*pOutPointB = vEdgeEnd;

			return true;
		}
	}

	return false;
}


_float CCell::Compute_SignedAreaXZ(const _float3& vPointA, const _float3& vPointB, const _float3& vPointC) const
{
	return (vPointB.x - vPointA.x) * (vPointC.z - vPointA.z) - (vPointB.z - vPointA.z) * (vPointC.x - vPointA.x);
}


_float CCell::Compute_EdgeSideXZ(const _float3& vEdgeStart, const _float3& vEdgeEnd, const _float3& vPosition) const
{
	return (vEdgeEnd.x - vEdgeStart.x) * (vPosition.z - vEdgeStart.z) - (vEdgeEnd.z - vEdgeStart.z) * (vPosition.x - vEdgeStart.x);
}

_bool CCell::Is_SamePoint(const _float3& vPointA, const _float3& vPointB) const
{
	const _float fPointEpsilon = 0.0001f;

	return fabsf(vPointA.x - vPointB.x) <= fPointEpsilon && fabsf(vPointA.y - vPointB.y) <= fPointEpsilon && fabsf(vPointA.z - vPointB.z) <= fPointEpsilon;
}

CCell* CCell::Create(const CELL_DESC& Desc)
{
	CCell* pInstance = new CCell();

	if (FAILED(pInstance->Initialize(Desc)))
	{
		MSG_BOX("Failed to Created : CCell");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CCell::Free()
{
	__super::Free();
}