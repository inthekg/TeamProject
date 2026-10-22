#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL CCell final: public  CBase
{
public:
	struct CELL_DESC
	{
		_float3 vPoints[3] = {};
		_int iNeighborIndices[3] = { -1,-1, -1 };
	};

private:
	CCell();
	virtual ~CCell() = default;

public:
	HRESULT Initialize(const CELL_DESC& Desc);

public:
	const _float3& Get_Point(_uint iPointIndex) const;
	_int Get_NeighborIndex(_uint iEdgeIndex) const;
	_float3 Get_Center() const;
	const _float3* Get_Points() const { return m_vPoints; }
	const _int* Get_NeighborIndices() const { return m_iNeighborIndices; }

	_bool Is_In(const _float3& vPosition, _uint* pOutExitEdgeIndex) const;
	_bool Compute_Height(const _float3& vPosition, _float* pOutHeight)const;
	_bool Is_SameEdge(_uint iEdgeIndex, const CCell* pOtherCell, _uint* pOutOtherEdgeIndex) const;

	_bool Get_SharedEdge(const CCell* pOtherCell, _float3* pOutPointA, _float3* pOutPointB) const;

private:
	_float Compute_SignedAreaXZ(const _float3& vPointA, const _float3& vPointB, const _float3& vPointC) const;
	_float Compute_EdgeSideXZ(const _float3& vEdgeStart, const _float3& vEdgeEnd, const _float3& vPosition) const;
	_bool Is_SamePoint(const _float3& vPointA, const _float3& vPointB) const;

private:
	_float3 m_vPoints[3] = {};
	_int m_iNeighborIndices[3] = { -1, -1, -1 };
	_bool m_bClockwise = false;

public:
	static CCell* Create(const CELL_DESC& Desc);
	virtual void Free() override;


};

NS_END