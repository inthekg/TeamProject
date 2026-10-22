#include "Navigation.h"
#include "Cell.h"

CNavigation::CNavigation()
{
}

HRESULT CNavigation::Initialize(const _wstring& strNavigationPath, const _wstring& strNeighborPath)
{
	if (strNavigationPath.empty() || strNeighborPath.empty())
		return E_FAIL;

	if (FAILED(Load_Cells(strNavigationPath, strNeighborPath)))
	{
		MSG_BOX("Failed to Load Navigation Files");
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CNavigation::Load_Cells(const _wstring& strNavigationPath, const _wstring& strNeighborPath)
{
	ifstream NavigationFile(strNavigationPath, ios::binary | ios::ate);
	ifstream NeighborFile(strNeighborPath, ios::binary | ios::ate);

	if (false == NavigationFile.is_open() || false == NeighborFile.is_open())
		return E_FAIL;

	const streamoff iNavigationFileSize = NavigationFile.tellg();
	const streamoff iNeighborFileSize = NeighborFile.tellg();
	const streamoff iNavigationCellSize = static_cast<streamoff>(sizeof(_float3) * 3);
	const streamoff iNeighborCellSize = static_cast<streamoff>(sizeof(_int) * 3);

	if (iNavigationFileSize <= 0 || iNeighborFileSize <= 0)
		return E_FAIL;

	if (0 != iNavigationFileSize % iNavigationCellSize || 0 != iNeighborFileSize % iNeighborCellSize)
		return E_FAIL;

	const size_t iNavigationCellCount = static_cast<size_t>(iNavigationFileSize / iNavigationCellSize);
	const size_t iNeighborCellCount = static_cast<size_t>(iNeighborFileSize / iNeighborCellSize);

	if (iNavigationCellCount != iNeighborCellCount)
		return E_FAIL;

	NavigationFile.seekg(0, ios::beg);
	NeighborFile.seekg(0, ios::beg);

	vector<CCell*> Cells;
	Cells.reserve(iNavigationCellCount);

	for (size_t i = 0; i < iNavigationCellCount; ++i)
	{
		CCell::CELL_DESC Desc = {};

		if (false == static_cast<_bool>(NavigationFile.read(reinterpret_cast<char*>(Desc.vPoints), sizeof(_float3) * 3)) || false == static_cast<_bool>(NeighborFile.read(reinterpret_cast<char*>(Desc.iNeighborIndices), sizeof(_int) * 3)))
		{
			for (CCell* pCell : Cells)
				Safe_Release(pCell);

			return E_FAIL;
		}

		CCell* pCell = CCell::Create(Desc);

		if (nullptr == pCell)
		{
			for (CCell* pCreatedCell : Cells)
				Safe_Release(pCreatedCell);

			return E_FAIL;
		}

		Cells.push_back(pCell);
	}

	Clear_Cells();
	m_Cells = move(Cells);

	if (FAILED(Validate_Neighbors()))
	{
		Clear_Cells();
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CNavigation::Validate_Neighbors() const
{
	for (size_t i = 0; i < m_Cells.size(); ++i)
	{
		const CCell* pCell = m_Cells[i];

		if (nullptr == pCell)
			return E_FAIL;

		for (_uint iEdgeIndex = 0; iEdgeIndex < 3; ++iEdgeIndex)
		{
			const _int iNeighborIndex = pCell->Get_NeighborIndex(iEdgeIndex);

			if (-1 == iNeighborIndex)
				continue;

			if (iNeighborIndex < 0 || static_cast<size_t>(iNeighborIndex) >= m_Cells.size())
				return E_FAIL;

			if (static_cast<size_t>(iNeighborIndex) == i)
				return E_FAIL;

			const CCell* pNeighborCell = m_Cells[static_cast<size_t>(iNeighborIndex)];
			_uint iNeighborEdgeIndex = UINT_MAX;

			if (nullptr == pNeighborCell || false == pCell->Is_SameEdge(iEdgeIndex, pNeighborCell, &iNeighborEdgeIndex))
				return E_FAIL;

			if (pNeighborCell->Get_NeighborIndex(iNeighborEdgeIndex) != static_cast<_int>(i))
				return E_FAIL;
		}
	}

	return S_OK;
}

_bool CNavigation::Find_Cell(const _float3& vPosition, _int* pOutCellIndex, _float* pOutHeight) const
{
	if (nullptr == pOutCellIndex || nullptr == pOutHeight)
		return false;

	_bool bFound = false;
	_float fNearestHeightDifference = FLT_MAX;

	for (size_t i = 0; i < m_Cells.size(); ++i)
	{
		const CCell* pCell = m_Cells[i];

		if (nullptr == pCell || false == pCell->Is_In(vPosition, nullptr))
			continue;

		_float fHeight = 0.f;

		if (false == pCell->Compute_Height(vPosition, &fHeight))
			continue;

		const _float fHeightDifference = fabsf(vPosition.y - fHeight);

		if (bFound && fHeightDifference >= fNearestHeightDifference)
			continue;

		bFound = true;
		fNearestHeightDifference = fHeightDifference;
		*pOutCellIndex = static_cast<_int>(i);
		*pOutHeight = fHeight;
	}

	return bFound;
}

_bool CNavigation::Set_CurrentCell(const _float3& vPosition, _int* pCurrentCellIndex) const
{
	if (nullptr == pCurrentCellIndex)
		return false;

	_int iCellIndex = -1;
	_float fHeight = 0.f;

	if (false == Find_Cell(vPosition, &iCellIndex, &fHeight))
		return false;

	*pCurrentCellIndex = iCellIndex;

	return true;
}

_bool CNavigation::Find_MoveCell(const _float3& vPosition, const _int* pCurrentCellIndex, _int* pOutCellIndex, _float* pOutHeight) const
{
	if (nullptr == pCurrentCellIndex || nullptr == pOutCellIndex || nullptr == pOutHeight)
		return false;

	if (*pCurrentCellIndex < 0 || static_cast<size_t>(*pCurrentCellIndex) >= m_Cells.size())
		return false;

	_int iCellIndex = *pCurrentCellIndex;

	for (size_t i = 0; i < m_Cells.size(); ++i)
	{
		if (iCellIndex < 0 || static_cast<size_t>(iCellIndex) >= m_Cells.size())
			return false;

		const CCell* pCell = m_Cells[static_cast<size_t>(iCellIndex)];

		if (nullptr == pCell)
			return false;

		_uint iExitEdgeIndex = UINT_MAX;

		if (pCell->Is_In(vPosition, &iExitEdgeIndex))
		{
			if (false == pCell->Compute_Height(vPosition, pOutHeight))
				return false;

			*pOutCellIndex = iCellIndex;
			return true;
		}

		if (iExitEdgeIndex >= 3)
			return false;

		iCellIndex = pCell->Get_NeighborIndex(iExitEdgeIndex);
	}

	return false;
}

_bool CNavigation::Move_OnNavigation(const _float3& vPosition, _int* pCurrentCellIndex, _float3* pOutPosition) const
{
	if (nullptr == pCurrentCellIndex || nullptr == pOutPosition)
		return false;

	_int iCellIndex = -1;
	_float fHeight = 0.f;

	if (false == Find_MoveCell(vPosition, pCurrentCellIndex, &iCellIndex, &fHeight))
		return false;

	*pOutPosition = vPosition;
	pOutPosition->y = fHeight;
	*pCurrentCellIndex = iCellIndex;

	return true;
}

void CNavigation::Clear_Cells()
{
	for (CCell* pCell : m_Cells)
		Safe_Release(pCell);

	m_Cells.clear();
}

_bool CNavigation::Build_CellPath(_uint iStartCellIndex, _uint iGoalCellIndex, vector<_uint>* pOutCellPath) const
{
	if (nullptr == pOutCellPath)
		return false;

	pOutCellPath->clear();

	const _uint iCellCount = ETOUI(m_Cells.size());

	if (iStartCellIndex >= iCellCount || iGoalCellIndex >= iCellCount)
		return false;

	vector<_float> GCosts(iCellCount, FLT_MAX);
	vector<_float> HCosts(iCellCount, FLT_MAX);
	vector<_int> ParentIndices(iCellCount, -1);
	vector<_bool> OpenList(iCellCount, false);
	vector<_bool> ClosedList(iCellCount, false);

	GCosts[iStartCellIndex] = 0.f;
	HCosts[iStartCellIndex] = Compute_DistanceXZ(m_Cells[iStartCellIndex]->Get_Center(), m_Cells[iGoalCellIndex]->Get_Center());
	OpenList[iStartCellIndex] = true;

	while (true)
	{
		_int iCurrentCellIndex = -1;
		_float fBestCost = FLT_MAX;

		for (_uint i = 0; i < iCellCount; ++i)
		{
			if (false == OpenList[i] || ClosedList[i])
				continue;

			const _float fCost = GCosts[i] + HCosts[i];

			if (fCost >= fBestCost)
				continue;

			fBestCost = fCost;
			iCurrentCellIndex = static_cast<_int>(i);
		}

		if (0 > iCurrentCellIndex)
			return false;

		if (ETOUI(iCurrentCellIndex) == iGoalCellIndex)
			break;

		OpenList[iCurrentCellIndex] = false;
		ClosedList[iCurrentCellIndex] = true;

		const CCell* pCurrentCell = m_Cells[iCurrentCellIndex];

		if (nullptr == pCurrentCell)
			return false;

		for (_uint i = 0; i < 3; ++i)
		{
			const _int iNeighborCellIndex = pCurrentCell->Get_NeighborIndex(i);

			if (0 > iNeighborCellIndex)
				continue;

			if (ETOUI(iNeighborCellIndex) >= iCellCount)
				continue;

			if (ClosedList[iNeighborCellIndex])
				continue;

			const _float fMoveCost = Compute_DistanceXZ(m_Cells[iCurrentCellIndex]->Get_Center(), m_Cells[iNeighborCellIndex]->Get_Center());
			const _float fNewGCost = GCosts[iCurrentCellIndex] + fMoveCost;

			if (OpenList[iNeighborCellIndex] && fNewGCost >= GCosts[iNeighborCellIndex])
				continue;

			ParentIndices[iNeighborCellIndex] = iCurrentCellIndex;
			GCosts[iNeighborCellIndex] = fNewGCost;
			HCosts[iNeighborCellIndex] = Compute_DistanceXZ(m_Cells[iNeighborCellIndex]->Get_Center(), m_Cells[iGoalCellIndex]->Get_Center());
			OpenList[iNeighborCellIndex] = true;
		}
	}

	vector<_uint> ReverseCellPath = {};
	_int iCurrentCellIndex = static_cast<_int>(iGoalCellIndex);

	while (0 <= iCurrentCellIndex)
	{
		ReverseCellPath.push_back(ETOUI(iCurrentCellIndex));

		if (ETOUI(iCurrentCellIndex) == iStartCellIndex)
			break;

		iCurrentCellIndex = ParentIndices[iCurrentCellIndex];
	}

	if (ReverseCellPath.empty() || ReverseCellPath.back() != iStartCellIndex)
		return false;

	pOutCellPath->assign(ReverseCellPath.rbegin(), ReverseCellPath.rend());

	return true;
}

#pragma region [ORIGIN]
/*
* _bool CNavigation::Build_Portals(const vector<_uint>& CellPath, const _float3& vStartPosition, const _float3& vGoalPosition, vector<NAVIGATION_PORTAL>* pOutPortals) const
{
	if (nullptr == pOutPortals)
		return false;

	pOutPortals->clear();

	if (CellPath.size() < 2)
		return false;

	NAVIGATION_PORTAL StartPortal = {};
	StartPortal.vLeft = vStartPosition;
	StartPortal.vRight = vStartPosition;
	pOutPortals->push_back(StartPortal);

	for (_uint i = 0; i + 1 < ETOUI(CellPath.size()); ++i)
	{
		const CCell* pCurrentCell = m_Cells[CellPath[i]];
		const CCell* pNextCell = m_Cells[CellPath[i + 1]];

		if (nullptr == pCurrentCell || nullptr == pNextCell)
			return false;

		_float3 vPointA = {};
		_float3 vPointB = {};

		if (false == pCurrentCell->Get_SharedEdge(pNextCell, &vPointA, &vPointB))
			return false;

		const _float3 vCurrentCenter = pCurrentCell->Get_Center();
		const _float3 vNextCenter = pNextCell->Get_Center();
		const _float fPointASide = Compute_TriangleAreaXZ(vCurrentCenter, vNextCenter, vPointA);

		NAVIGATION_PORTAL Portal = {};

		if (fPointASide >= 0.f)
		{
			Portal.vLeft = vPointA;
			Portal.vRight = vPointB;
		}
		else
		{
			Portal.vLeft = vPointB;
			Portal.vRight = vPointA;
		}

		pOutPortals->push_back(Portal);
	}

	NAVIGATION_PORTAL GoalPortal = {};
	GoalPortal.vLeft = vGoalPosition;
	GoalPortal.vRight = vGoalPosition;
	pOutPortals->push_back(GoalPortal);

	return true;
}
*/
#pragma endregion

#pragma region [TEST]
_bool CNavigation::Build_Portals(const vector<_uint>& CellPath, const _float3& vStartPosition, const _float3& vGoalPosition, vector<NAVIGATION_PORTAL>* pOutPortals, _float fAgentRadius) const
{
	if (nullptr == pOutPortals)
		return false;

	pOutPortals->clear();

	if (CellPath.size() < 2)
		return false;

	NAVIGATION_PORTAL StartPortal = {};
	StartPortal.vLeft = vStartPosition;
	StartPortal.vRight = vStartPosition;
	pOutPortals->push_back(StartPortal);

	for (_uint i = 0; i + 1 < ETOUI(CellPath.size()); ++i)
	{
		const CCell* pCurrentCell = m_Cells[CellPath[i]];
		const CCell* pNextCell = m_Cells[CellPath[i + 1]];

		if (nullptr == pCurrentCell || nullptr == pNextCell)
			return false;

		_float3 vPointA = {};
		_float3 vPointB = {};

		if (false == pCurrentCell->Get_SharedEdge(pNextCell, &vPointA, &vPointB))
			return false;

		const _float3 vCurrentCenter = pCurrentCell->Get_Center();
		const _float3 vNextCenter = pNextCell->Get_Center();
		const _float fPointASide = Compute_TriangleAreaXZ(vCurrentCenter, vNextCenter, vPointA);

		NAVIGATION_PORTAL Portal = {};

		if (fPointASide >= 0.f)
		{
			Portal.vLeft = vPointA;
			Portal.vRight = vPointB;
		}
		else
		{
			Portal.vLeft = vPointB;
			Portal.vRight = vPointA;
		}

		if (fAgentRadius > 0.f)
		{
			const _float fDX = Portal.vRight.x - Portal.vLeft.x;
			const _float fDZ = Portal.vRight.z - Portal.vLeft.z;
			const _float fLen = sqrtf(fDX * fDX + fDZ * fDZ);

			if (fLen > 1e-4f)
			{
				_float fInset = fAgentRadius;
				const _float fMaxInset = fLen * 0.5f - 1e-3f;

				if (fInset > fMaxInset)
					fInset = fMaxInset;

				if (fInset > 0.f)
				{
					const _float fNX = fDX / fLen;
					const _float fNZ = fDZ / fLen;

					Portal.vLeft.x += fNX * fInset;
					Portal.vLeft.z += fNZ * fInset;
					Portal.vRight.x -= fNX * fInset;
					Portal.vRight.z -= fNZ * fInset;
				}
			}
		}

		pOutPortals->push_back(Portal);
	}

	NAVIGATION_PORTAL GoalPortal = {};
	GoalPortal.vLeft = vGoalPosition;
	GoalPortal.vRight = vGoalPosition;
	pOutPortals->push_back(GoalPortal);

	return true;
}
#pragma endregion


#pragma region [ORIGIN]
/*
* _bool CNavigation::Build_FunnelPath(const vector<NAVIGATION_PORTAL>& Portals, const _float3& vStartPosition, const _float3& vGoalPosition, vector<_float3>* pOutPath) const
{
	if (nullptr == pOutPath)
		return false;

	pOutPath->clear();

	if (Portals.size() < 2)
		return false;

	_float3 vApex = vStartPosition;
	_float3 vLeft = Portals[0].vLeft;
	_float3 vRight = Portals[0].vRight;

	_uint iApexIndex = 0;
	_uint iLeftIndex = 0;
	_uint iRightIndex = 0;

	pOutPath->push_back(vStartPosition);

	for (_uint i = 1; i < ETOUI(Portals.size()); ++i)
	{
		const _float3 vNewLeft = Portals[i].vLeft;
		const _float3 vNewRight = Portals[i].vRight;

		if (Compute_TriangleAreaXZ(vApex, vRight, vNewRight) <= 0.f)
		{
			if (Is_SamePointXZ(vApex, vRight) || Compute_TriangleAreaXZ(vApex, vLeft, vNewRight) > 0.f)
			{
				vRight = vNewRight;
				iRightIndex = i;
			}
			else
			{
				pOutPath->push_back(vLeft);

				vApex = vLeft;
				iApexIndex = iLeftIndex;

				vLeft = vApex;
				vRight = vApex;

				iLeftIndex = iApexIndex;
				iRightIndex = iApexIndex;

				i = iApexIndex;
				continue;
			}
		}

		if (Compute_TriangleAreaXZ(vApex, vLeft, vNewLeft) >= 0.f)
		{
			if (Is_SamePointXZ(vApex, vLeft) || Compute_TriangleAreaXZ(vApex, vRight, vNewLeft) < 0.f)
			{
				vLeft = vNewLeft;
				iLeftIndex = i;
			}
			else
			{
				pOutPath->push_back(vRight);

				vApex = vRight;
				iApexIndex = iRightIndex;

				vLeft = vApex;
				vRight = vApex;

				iLeftIndex = iApexIndex;
				iRightIndex = iApexIndex;

				i = iApexIndex;
				continue;
			}
		}
	}

	if (pOutPath->empty() || false == Is_SamePointXZ(pOutPath->back(), vGoalPosition))
		pOutPath->push_back(vGoalPosition);

	return true;
}
*/
#pragma endregion

#pragma region [TEST]
_bool CNavigation::Build_FunnelPath(const vector<NAVIGATION_PORTAL>& Portals, const _float3& vStartPosition, const _float3& vGoalPosition, vector<_float3>* pOutPath) const
{
	if (nullptr == pOutPath)
		return false;

	pOutPath->clear();

	if (Portals.size() < 2)
		return false;

	_float3 vApex = vStartPosition;
	_float3 vLeft = Portals[0].vLeft;
	_float3 vRight = Portals[0].vRight;

	_uint iApexIndex = 0;
	_uint iLeftIndex = 0;
	_uint iRightIndex = 0;

	pOutPath->push_back(vStartPosition);

	for (_uint i = 1; i < ETOUI(Portals.size()); ++i)
	{
		const _float3 vNewLeft = Portals[i].vLeft;
		const _float3 vNewRight = Portals[i].vRight;

		if (Compute_TriangleAreaXZ(vApex, vRight, vNewRight) >= 0.f)
		{
			if (Is_SamePointXZ(vApex, vRight) || Compute_TriangleAreaXZ(vApex, vLeft, vNewRight) < 0.f)
			{
				vRight = vNewRight;
				iRightIndex = i;
			}
			else
			{
				pOutPath->push_back(vLeft);

				vApex = vLeft;
				iApexIndex = iLeftIndex;

				vLeft = vApex;
				vRight = vApex;

				iLeftIndex = iApexIndex;
				iRightIndex = iApexIndex;

				i = iApexIndex;
				continue;
			}
		}

		if (Compute_TriangleAreaXZ(vApex, vLeft, vNewLeft) <= 0.f)
		{
			if (Is_SamePointXZ(vApex, vLeft) || Compute_TriangleAreaXZ(vApex, vRight, vNewLeft) > 0.f)
			{
				vLeft = vNewLeft;
				iLeftIndex = i;
			}
			else
			{
				pOutPath->push_back(vRight);

				vApex = vRight;
				iApexIndex = iRightIndex;

				vLeft = vApex;
				vRight = vApex;

				iLeftIndex = iApexIndex;
				iRightIndex = iApexIndex;

				i = iApexIndex;
				continue;
			}
		}
	}

	if (pOutPath->empty() || false == Is_SamePointXZ(pOutPath->back(), vGoalPosition))
		pOutPath->push_back(vGoalPosition);

	return true;
}

#pragma endregion


#pragma region [ORIGIN]
/*
* _bool CNavigation::Find_Path(const _float3& vStartPosition, const _float3& vGoalPosition, vector<_float3>* pOutPath) const
{
	if (nullptr == pOutPath)
		return false;

	pOutPath->clear();

	if (m_Cells.empty())
		return false;

	_int iStartCellIndex = -1;
	_int iGoalCellIndex = -1;
	_float fStartHeight = 0.f;
	_float fGoalHeight = 0.f;

	if (false == Find_Cell(vStartPosition, &iStartCellIndex, &fStartHeight))
		return false;

	if (false == Find_Cell(vGoalPosition, &iGoalCellIndex, &fGoalHeight))
		return false;

	_float3 vStart = vStartPosition;
	_float3 vGoal = vGoalPosition;

	vStart.y = fStartHeight;
	vGoal.y = fGoalHeight;

	if (iStartCellIndex == iGoalCellIndex)
	{
		pOutPath->push_back(vStart);
		pOutPath->push_back(vGoal);
		return true;
	}

	vector<_uint> CellPath = {};

	if (false == Build_CellPath(ETOUI(iStartCellIndex), ETOUI(iGoalCellIndex), &CellPath))
		return false;

	vector<NAVIGATION_PORTAL> Portals = {};

	if (false == Build_Portals(CellPath, vStart, vGoal, &Portals))
		return false;

	if (false == Build_FunnelPath(Portals, vStart, vGoal, pOutPath))
		return false;

	return false == pOutPath->empty();
}
*/
#pragma endregion

#pragma region [ TEST ]
_bool CNavigation::Find_Path(const _float3& vStartPosition, const _float3& vGoalPosition, vector<_float3>* pOutPath, _float fAgentRadius) const
{
	if (nullptr == pOutPath)
		return false;

	pOutPath->clear();

	if (m_Cells.empty())
		return false;

	_int iStartCellIndex = -1;
	_int iGoalCellIndex = -1;
	_float fStartHeight = 0.f;
	_float fGoalHeight = 0.f;

	if (false == Find_Cell(vStartPosition, &iStartCellIndex, &fStartHeight))
		return false;

	if (false == Find_Cell(vGoalPosition, &iGoalCellIndex, &fGoalHeight))
		return false;

	_float3 vStart = vStartPosition;
	_float3 vGoal = vGoalPosition;

	vStart.y = fStartHeight;
	vGoal.y = fGoalHeight;

	if (iStartCellIndex == iGoalCellIndex)
	{
		pOutPath->push_back(vStart);
		pOutPath->push_back(vGoal);
		return true;
	}

	vector<_uint> CellPath = {};

	if (false == Build_CellPath(ETOUI(iStartCellIndex), ETOUI(iGoalCellIndex), &CellPath))
		return false;

	vector<NAVIGATION_PORTAL> Portals = {};

	if (false == Build_Portals(CellPath, vStart, vGoal, &Portals, fAgentRadius))
		return false;

	if (false == Build_FunnelPath(Portals, vStart, vGoal, pOutPath))
		return false;

	return false == pOutPath->empty();
}
_uint CNavigation::Get_CellCount() const
{
	return ETOUI(m_Cells.size());
}
_bool CNavigation::Get_CellCenter(_uint iCellIndex, _float3* pOut) const
{
	if (nullptr == pOut || iCellIndex >= m_Cells.size())
		return false;

	*pOut = m_Cells[iCellIndex]->Get_Center();
	return true;
}

// BFS 탐색 ( SPAWN 때 한번만 모든 애들은 아니고 나비 용 왕복 패턴 시 사용할 Cell 모음) 
void CNavigation::Collect_CellsByNeighbor(_int iStartCellIndex, _uint iDepth, vector<_uint>* pOut) const
{
	if (nullptr == pOut)
		return;

	pOut->clear();

	if (iStartCellIndex < 0 || iStartCellIndex >= static_cast<_int>(m_Cells.size()))
		return;

	unordered_set<_int>	Visited;
	vector<_int>		Frontier;
	Visited.insert(iStartCellIndex);
	Frontier.push_back(iStartCellIndex);

	for (_uint iStep = 0; iStep <= iDepth && false == Frontier.empty(); ++iStep)
	{
		vector<_int> Next;

		for (_int iCell : Frontier)
		{
			pOut->push_back(static_cast<_uint>(iCell));

			const _int* pNeighbors = m_Cells[iCell]->Get_NeighborIndices();
			for (_uint iEdge = 0; iEdge < 3; ++iEdge)
			{
				const _int iNext = pNeighbors[iEdge];
				if (iNext >= 0 && Visited.find(iNext) == Visited.end())
				{
					Visited.insert(iNext);
					Next.push_back(iNext);
				}
			}
		}

		Frontier = Next;
	}
}
#pragma endregion


#ifdef _DEBUG
_bool CNavigation::Get_DebugCellPoints(_uint iCellIndex, _float3* pOutPoints) const
{
	// 디버그 렌더용으로 셀 삼각형 점 3개를 복사한다.
	if (nullptr == pOutPoints || iCellIndex >= ETOUI(m_Cells.size()))
		return false;

	const CCell* pCell = m_Cells[iCellIndex];

	if (nullptr == pCell)
		return false;

	pOutPoints[0] = pCell->Get_Point(0);
	pOutPoints[1] = pCell->Get_Point(1);
	pOutPoints[2] = pCell->Get_Point(2);

	return true;
}
#endif

_float CNavigation::Compute_DistanceXZ(const _float3& vPositionA, const _float3& vPositionB) const
{
	const _float fDeltaX = vPositionA.x - vPositionB.x;
	const _float fDeltaZ = vPositionA.z - vPositionB.z;

	return sqrtf(fDeltaX * fDeltaX + fDeltaZ * fDeltaZ);
}

_float CNavigation::Compute_TriangleAreaXZ(const _float3& vPositionA, const _float3& vPositionB, const _float3& vPositionC) const
{
	return (vPositionB.x - vPositionA.x) * (vPositionC.z - vPositionA.z) - (vPositionB.z - vPositionA.z) * (vPositionC.x - vPositionA.x);
}

_bool CNavigation::Is_SamePointXZ(const _float3& vPositionA, const _float3& vPositionB) const
{
	const _float fPointEpsilon = 0.0001f;

	return fabsf(vPositionA.x - vPositionB.x) <= fPointEpsilon && fabsf(vPositionA.z - vPositionB.z) <= fPointEpsilon;
}

CNavigation* CNavigation::Create(const _wstring& strNavigationPath, const _wstring& strNeighborPath)
{
	CNavigation* pInstance = new CNavigation();

	if (FAILED(pInstance->Initialize(strNavigationPath, strNeighborPath)))
	{
		MSG_BOX("Failed to Created : CNavigation");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CNavigation::Free()
{
	Clear_Cells();
	__super::Free();
}