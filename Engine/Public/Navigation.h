#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CCell;

class ENGINE_DLL CNavigation final : public CBase
{
private:
	typedef struct tagNavigationPortal
	{
		_float3 vLeft = {};
		_float3 vRight = {};
	} NAVIGATION_PORTAL;

public:
	CNavigation();
	virtual ~CNavigation() = default;

public:
	HRESULT Initialize(const _wstring& strNavigationPath, const _wstring& strNeighborPath);

public:
	_bool Set_CurrentCell(const _float3& vPosition, _int* pCurrentCellIndex) const;
	_bool Move_OnNavigation(const _float3& vPosition, _int* pCurrentCellIndex, _float3* pOutPosition) const;
	//_bool Find_Path(const _float3& vStartPosition, const _float3& vGoalPosition, vector<_float3>* pOutPath) const;
	_bool Find_Path(const _float3& vStartPosition, const _float3& vGoalPosition, vector<_float3>* pOutPath, _float fAgentRadius) const;

	// ADD
	_uint Get_CellCount() const;
	_bool Get_CellCenter(_uint iCellIndex, _float3* pOut) const;
	void  Collect_CellsByNeighbor(_int iStartCellIndex, _uint iDepth, vector<_uint>* pOut) const;

#ifdef _DEBUG
	_uint Get_DebugCellCount() const { return ETOUI(m_Cells.size()); }
	_bool Get_DebugCellPoints(_uint iCellIndex, _float3* pOutPoints) const;
#endif

private:
	HRESULT Load_Cells(const _wstring& strNavigationPath, const _wstring& strNeighborPath);
	HRESULT Validate_Neighbors() const;

	_bool Find_Cell(const _float3& vPosition, _int* pOutCellIndex, _float* pOutHeight) const;
	_bool Find_MoveCell(const _float3& vPosition, const _int* pCurrentCellIndex, _int* pOutCellIndex, _float* pOutHeight) const;

	void Clear_Cells();

private:
	_bool Build_CellPath(_uint iStartCellIndex, _uint iGoalCellIndex, vector<_uint>* pOutCellPath) const;
	//_bool Build_Portals(const vector<_uint>& CellPath, const _float3& vStartPosition, const _float3& vGoalPosition, vector<NAVIGATION_PORTAL>* pOutPortals) const;
	_bool Build_Portals(const vector<_uint>& CellPath, const _float3& vStartPosition, const _float3& vGoalPosition, vector<NAVIGATION_PORTAL>* pOutPortals, _float fAgentRadius) const;
	_bool Build_FunnelPath(const vector<NAVIGATION_PORTAL>& Portals, const _float3& vStartPosition, const _float3& vGoalPosition, vector<_float3>* pOutPath) const;

	_float Compute_DistanceXZ(const _float3& vPositionA, const _float3& vPositionB) const;
	_float Compute_TriangleAreaXZ(const _float3& vPositionA, const _float3& vPositionB, const _float3& vPositionC) const;
	_bool Is_SamePointXZ(const _float3& vPositionA, const _float3& vPositionB) const;

private:
	vector<CCell*> m_Cells = {};

public:
	static CNavigation* Create(const _wstring& strNavigationPath, const _wstring& strNeighborPath);
	virtual void Free() override;
};

NS_END