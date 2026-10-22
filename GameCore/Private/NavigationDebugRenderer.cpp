#include "NavigationDebugRenderer.h"

#ifdef _DEBUG

#include "NavigationDebugLineBuffer.h"
#include "GameInstance.h"
#include "Shader.h"
#include "RenderStruct.h"
#include "ChapterMapManager.h"
#include "AIObject.h"
#include "Navigation.h"
#include "Layer.h"
#include "Transform.h"

NS_BEGIN(GameCore)

CNavigationDebugRenderer::CNavigationDebugRenderer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: Engine::CGameObject{ pDevice, pContext }
	, m_pGameInstance{ Engine::CGameInstance::Get_Instance() }
{
}

HRESULT CNavigationDebugRenderer::Initialize_Prototype()
{
	if (FAILED(__super::Initialize(nullptr)))
		return E_FAIL;

	if (FAILED(Ready_Components()))
		return E_FAIL;

	return S_OK;
}

HRESULT CNavigationDebugRenderer::Ready_Components()
{
	m_pVIBufferCom = CNavigationDebugLineBuffer::Create(m_pDevice, m_pContext, 1 << 20);

	if (nullptr == m_pVIBufferCom)
		return E_FAIL;

	if (FAILED(m_pGameInstance->Get_Shader(L"Shader_LineWorld", &m_pShaderCom)))
		return E_FAIL;

	return S_OK;
}

HRESULT CNavigationDebugRenderer::Set_Lines(const vector<Engine::VTXCOLOR>& Lines)
{
	if (nullptr == m_pVIBufferCom)
		return E_FAIL;

	return m_pVIBufferCom->Set_Lines(Lines);
}

HRESULT CNavigationDebugRenderer::Update_DebugLines(CChapterMapManager* pChapterMapManager)
{
	if (false == m_bEnable || nullptr == pChapterMapManager)
		return Set_Lines(vector<Engine::VTXCOLOR>{});

	vector<Engine::VTXCOLOR> Lines;
	Lines.reserve(8192);

	const _float4 vNavigationColor = _float4(1.f, 0.45f, 0.f, 1.f);
	const _float4 vMonsterCellColor = _float4(1.f, 0.f, 0.f, 1.f);
	const _float4 vPathColor = _float4(1.f, 0.75f, 0.f, 1.f);
	const _float4 vTargetColor = _float4(0.f, 1.f, 1.f, 1.f);
	const _float4 vReachColor = _float4(0.2f, 1.f, 0.4f, 1.f);

	auto PushLine = [&Lines](const _float3& vStart, const _float3& vEnd, const _float4& vColor)
		{
			Engine::VTXCOLOR vLineStart = {};
			Engine::VTXCOLOR vLineEnd = {};

			vLineStart.vPosition = vStart;
			vLineStart.vColor = vColor;
			vLineEnd.vPosition = vEnd;
			vLineEnd.vColor = vColor;

			Lines.push_back(vLineStart);
			Lines.push_back(vLineEnd);
		};

	auto PushCell = [&PushLine](const _float3* pPoints, const _float4& vColor, _float fYOffset)
		{
			_float3 vPoints[3] = { pPoints[0], pPoints[1], pPoints[2] };

			vPoints[0].y += fYOffset;
			vPoints[1].y += fYOffset;
			vPoints[2].y += fYOffset;

			PushLine(vPoints[0], vPoints[1], vColor);
			PushLine(vPoints[1], vPoints[2], vColor);
			PushLine(vPoints[2], vPoints[0], vColor);
		};

	auto PushCross = [&PushLine](const _float3& vPos, _float fSize, const _float4& vColor)
		{
			PushLine(_float3(vPos.x - fSize, vPos.y, vPos.z), _float3(vPos.x + fSize, vPos.y, vPos.z), vColor);
			PushLine(_float3(vPos.x, vPos.y - fSize, vPos.z), _float3(vPos.x, vPos.y + fSize, vPos.z), vColor);
			PushLine(_float3(vPos.x, vPos.y, vPos.z - fSize), _float3(vPos.x, vPos.y, vPos.z + fSize), vColor);
		};

	auto PushRing = [&PushLine](const _float3& vCenter, _float fRadius, const _float4& vColor)
		{
			const _uint iSegment = 24;
			_float3 vPrev = _float3(vCenter.x + fRadius, vCenter.y, vCenter.z);

			for (_uint i = 1; i <= iSegment; ++i)
			{
				_float fAngle = (static_cast<_float>(i) / static_cast<_float>(iSegment)) * XM_2PI;
				_float3 vCur = _float3(vCenter.x + fRadius * cosf(fAngle), vCenter.y, vCenter.z + fRadius * sinf(fAngle));
				PushLine(vPrev, vCur, vColor);
				vPrev = vCur;
			}
		};

	_uint iSectorID = pChapterMapManager->Get_CurrentSectorID();
	Engine::CLayer* pPlayerLayer = m_pGameInstance->Get_Layer(ETOUI(LEVEL::GAMEPLAY), TEXT("Layer_Character"));

	if (nullptr != pPlayerLayer && false == pPlayerLayer->Get_Objects().empty())
	{
		Engine::CGameObject* pPlayer = pPlayerLayer->Get_Objects().front();

		if (nullptr != pPlayer && nullptr != pPlayer->Get_Transform())
		{
			_vector vPlayerPosition = pPlayer->Get_Transform()->Get_State(STATE::POSITION);
			const _uint iPlayerSectorID = pChapterMapManager->Find_PrimarySectorByPosition(vPlayerPosition);

			if (UINT_MAX != iPlayerSectorID)
				iSectorID = iPlayerSectorID;
		}
	}

	Engine::CNavigation* pSectorNavigation = pChapterMapManager->Find_Navigation(iSectorID);

	if (nullptr != pSectorNavigation)
	{
		for (_uint iCellIndex = 0; iCellIndex < pSectorNavigation->Get_DebugCellCount(); ++iCellIndex)
		{
			_float3 vCellPoints[3] = {};

			if (true == pSectorNavigation->Get_DebugCellPoints(iCellIndex, vCellPoints))
				PushCell(vCellPoints, vNavigationColor, 0.05f);
		}
	}

	Engine::CLayer* pMonsterLayer = m_pGameInstance->Get_Layer(ETOUI(LEVEL::GAMEPLAY), TEXT("Layer_Monster"));

	if (nullptr != pMonsterLayer)
	{
		for (Engine::CGameObject* pObject : pMonsterLayer->Get_Objects())
		{
			CAIObject* pAIObject = dynamic_cast<CAIObject*>(pObject);

			if (nullptr == pAIObject || nullptr == pAIObject->Get_Transform())
				continue;

			Engine::CNavigation* pNavigation = pAIObject->Get_DebugNavigation();

			if (nullptr == pNavigation)
				continue;

			_float3 vMonsterPosition = {};
			XMStoreFloat3(&vMonsterPosition, pAIObject->Get_Transform()->Get_State(STATE::POSITION));

			_int iMonsterCellIndex = -1;

			if (true == pNavigation->Set_CurrentCell(vMonsterPosition, &iMonsterCellIndex))
			{
				_float3 vCellPoints[3] = {};

				if (true == pNavigation->Get_DebugCellPoints(ETOUI(iMonsterCellIndex), vCellPoints))
					PushCell(vCellPoints, vMonsterCellColor, 0.12f);
			}

			const vector<_float3>& NaviPath = pAIObject->Get_DebugNaviPath();

			for (_uint iPathIndex = 0; iPathIndex + 1 < ETOUI(NaviPath.size()); ++iPathIndex)
			{
				_float3 vStart = NaviPath[iPathIndex];
				_float3 vEnd = NaviPath[iPathIndex + 1];

				vStart.y += 0.18f;
				vEnd.y += 0.18f;

				PushLine(vStart, vEnd, vPathColor);
			}

			_uint iTargetIndex = pAIObject->Get_DebugNaviPathIndex();

			if (iTargetIndex < ETOUI(NaviPath.size()))
			{
				_float3 vTarget = NaviPath[iTargetIndex];
				vTarget.y += 0.18f;

				PushCross(vTarget, 0.35f, vTargetColor);
				PushRing(vTarget, pAIObject->Get_DebugReachRadius(), vReachColor);
			}
		}
	}

	return Set_Lines(Lines);
}


void CNavigationDebugRenderer::Late_Update(_float fTimeDelta)
{
	if (false == m_bEnable || nullptr == m_pVIBufferCom)
		return;

	if (0 == m_pVIBufferCom->Get_LineVertexCount())
		return;

	m_pGameInstance->Add_RenderGroup(RENDERID::DEBUG, this);
}

HRESULT CNavigationDebugRenderer::Render()
{
	if (nullptr == m_pVIBufferCom || nullptr == m_pShaderCom)
		return E_FAIL;

	StaticCommand Command = {};
	Command.header.etype = RENDER_TYPE::DEBUG;
	Command.header.iCommandSize = sizeof(StaticCommand);
	Command.header.iSortKey = Engine::RenderHeader::MakeSortKey(m_pShaderCom->Get_ID(), 0);
	Command.pVIBuffer = m_pVIBufferCom;
	Command.pVS = m_pShaderCom->Get_VertexShader();
	Command.pPS = m_pShaderCom->Get_PixelShader();
	Command.pLayout = m_pShaderCom->Get_InputLayout();
	Command.pMaterial = nullptr;

	XMStoreFloat4x4(&Command.worldMatrix, XMMatrixIdentity());

	m_pGameInstance->Submit_Static(Command);

	return S_OK;
}

CNavigationDebugRenderer* CNavigationDebugRenderer::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CNavigationDebugRenderer* pInstance = new CNavigationDebugRenderer(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CNavigationDebugRenderer");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CNavigationDebugRenderer::Free()
{
	Safe_Release(m_pShaderCom);
	Safe_Release(m_pVIBufferCom);

	__super::Free();
}

NS_END

#endif