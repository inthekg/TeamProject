#include "SplitWeapon.h"
#include "Shader.h"	
#include "Model.h"
#include "Transform.h"
#include "GameInstance.h"
#include "Status.h"
#include "Player.h"
#include "DebugCapsule.h"
#include "AIObject.h"
#include "Effect.h"

CSplitWeapon::CSplitWeapon(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CSkeletalWeapon{ pDevice, pContext }
{
}

CSplitWeapon::CSplitWeapon(const CSplitWeapon& Prototype)
	:CSkeletalWeapon{ Prototype }
{

}

HRESULT CSplitWeapon::Initialize(void* pArg)
{
	if (pArg == nullptr) return E_FAIL;
	if (FAILED(CPartObject::Initialize(pArg))) return E_FAIL;

	m_fCurDurability = MAX_DURABILITY;
	WEAPON_DESC* desc = static_cast<WEAPON_DESC*>(pArg);
	m_pOwner = desc->pOwner;
	m_pSocketBoneMatrix = desc->pSocketBoneMatrix;
	m_iLevelIndex = desc->iLevelIndex;


	if (FAILED(Ready_Components()))
		return E_FAIL;

	if (FAILED(Ready_WeaponData()))
		return E_FAIL;


	return S_OK;
}

void CSplitWeapon::Late_Update(_float fTimeDelta)
{
	_matrix PlayerWorld = XMLoadFloat4x4(m_pOwner->Get_WorldTransform());

	for (_int i = 0; i < PIECE_END; ++i)
	{
		WEAPON_PIECE& Piece = m_Pieces[i];
		if (!Piece.bActive || !Piece.pSocketBoneMatrix) continue;

		_matrix SocketMatrix = XMLoadFloat4x4(m_Pieces[i].pSocketBoneMatrix);
		_vector vScale, vRotation, vTranslation;

		if (XMMatrixDecompose(&vScale, &vRotation, &vTranslation, SocketMatrix))
		{
			SocketMatrix = XMMatrixAffineTransformation(
				XMVectorSet(1.f, 1.f, 1.f, 0.f), XMVectorZero(), vRotation, vTranslation);
		}

		XMStoreFloat4x4(&m_Pieces[i].CombinedWorld, (XMLoadFloat4x4(&m_Pieces[i].LocalOffset) * SocketMatrix) * PlayerWorld);

		if (Piece.Sweep.bCollisionActive)
		{
			_matrix BladeTop, BladeBot;
			if (FAILED(Get_PieceBoneWorldMatrix(Piece, Piece.iTrailTopIndex, BladeTop))) continue;
			if (FAILED(Get_PieceBoneWorldMatrix(Piece, Piece.iTrailBottomIndex, BladeBot))) continue;

			Sweeping(Piece.Sweep, BladeTop, BladeBot);
		}

#ifdef _DEBUG
		if (Piece.Sweep.pDebugCapsule && Piece.Sweep.bCollisionActive)
		{
			_bool bRender = m_pGameInstance->Get_DebugFlag();
			if (bRender) Piece.Sweep.pDebugCapsule->Late_Update(fTimeDelta);
		}
#endif

	}
	//대표 월드 
	m_CombinedWorldMatrix = m_Pieces[m_bSplit ? PIECE_RIGHT : PIECE_MERGE].CombinedWorld;


	if (m_bVisibility)
	{
		m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
		m_pGameInstance->Add_RenderGroup(RENDERID::SHADOW, this);
	}




}

HRESULT CSplitWeapon::Render()
{
	if (m_bSwapping) return S_OK;

	for (int i = 0; i < PIECE_END; ++i)
	{
		WEAPON_PIECE& Piece = m_Pieces[i];
		if (!Piece.bActive) continue;
		if (!Piece.pModel)
		{
			MSG_BOX("CSplitWeapon : Render Failed");
			continue;
		}

		const auto& units = Piece.pModel->Get_RenderUnits();
		_uint iCount = static_cast<_uint>(units.size());
		const RENDERID eCurPass = m_pGameInstance->Get_CurRenderPass();

		for (_uint i = 0; i < iCount; i++)
		{
			CMesh* pMesh = units[i].pMesh;
			CMaterial* pMaterialCom = units[i].pMaterial;

			if (pMesh == nullptr || pMaterialCom == nullptr)
			{
				MSG_BOX("CBodyPart : Render_Anim Failed");
				return E_FAIL;
			}

			if (pMaterialCom->Get_RenderGroup() != eCurPass)
				continue;

			SkinnedCommand command{};
			command.header.etype = RENDER_TYPE::SKINNED;
			command.iBoneCount = pMesh->Get_BoneCount();
			//command.header.iCommandSize = sizeof(SkinnedCommand) + sizeof(XMFLOAT4X4) * command.iBoneCount;
			command.header.iCommandSize = sizeof(SkinnedCommand);
			command.pVIBuffer = pMesh;
			command.pMaterial = pMaterialCom;
			command.worldMatrix = Piece.CombinedWorld;
			//command.pBones = Piece.pModel->Get_TransposedBoneMatrices(i);

			if (FAILED(Piece.pModel->Fill_SkinnedCommandBoneInfo(command, i)))
				continue;

			command.fDissolveAmount = m_fDissolveAmount;
			command.vDissolveColor = m_vDissolveColor;

			m_pGameInstance->Submit_Skinned_Resolved(command);
		}
	}
	return S_OK;
}

HRESULT CSplitWeapon::Render_Shadow()
{
	if (m_bSwapping) return S_OK;

	for (int i = 0; i < PIECE_END; ++i)
	{
		WEAPON_PIECE& Piece = m_Pieces[i];
		if (!Piece.bActive) continue;
		if (!Piece.pModel)
		{
			MSG_BOX("CSplitWeapon : Render Shadow Failed");
			continue;
		}

		const auto& units = Piece.pModel->Get_RenderUnits();
		_uint iCount = static_cast<_uint>(units.size());

		for (_uint i = 0; i < iCount; i++)
		{
			CMesh* pMesh = units[i].pMesh;
			CMaterial* pMat = units[i].pMaterial;
			if (pMesh == nullptr || pMat == nullptr)
				continue;

			SkinnedCommand command{};
			command.header.etype = RENDER_TYPE::SKINNED;
			command.iBoneCount = pMesh->Get_BoneCount();
			//command.header.iCommandSize = sizeof(SkinnedCommand) + sizeof(XMFLOAT4X4) * command.iBoneCount;
			command.header.iCommandSize = sizeof(SkinnedCommand);
			command.pVIBuffer = pMesh;
			command.pMaterial = pMat;
			command.worldMatrix = Piece.CombinedWorld;
			//command.pBones = Piece.pModel->Get_TransposedBoneMatrices(i);

			if (FAILED(Piece.pModel->Fill_SkinnedCommandBoneInfo(command, i)))
				continue;

			command.fDissolveAmount = m_fDissolveAmount;
			command.vDissolveColor = m_vDissolveColor;

			m_pGameInstance->Submit_Skinned_Resolved(command);
		}
	}

	return S_OK;
}

void CSplitWeapon::On_Unequipped()
{
	for (_int i = 0; i < PIECE_END; ++i)
	{
		m_Pieces[i].bHitWindow = false;
		Off_Collision(static_cast<PIECE_TYPE>(i));
		Set_PieceTrail(static_cast<PIECE_TYPE>(i), false);
	}
	Set_Split(false);

	__super::On_Unequipped();
}

void CSplitWeapon::On_Collision(AttackType eType, _uint iStep, PIECE_TYPE ePiece)
{
	if (ePiece >= PIECE_TYPE::PIECE_END || !m_Pieces[ePiece].bActive) return;
	Begin_Sweep(m_Pieces[ePiece].Sweep, eType, iStep);
}

void CSplitWeapon::Off_Collision(PIECE_TYPE ePiece)
{
	if (ePiece >= PIECE_TYPE::PIECE_END) return;
	End_Sweep(m_Pieces[ePiece].Sweep);
}

void CSplitWeapon::Set_Split(_bool bSplit)
{
	if (m_bSplit == bSplit) return;

	m_bSplit = bSplit;

	m_Pieces[PIECE_MERGE].bActive = !bSplit;
	m_Pieces[PIECE_LEFT].bActive = bSplit;
	m_Pieces[PIECE_RIGHT].bActive = bSplit;

	for (_int i = 0; i < PIECE_END; ++i)
	{
		if (!m_Pieces[i].bActive) 
			Deactivate_Piece(static_cast<PIECE_TYPE>(i));
	}

	
	m_pGameInstance->Play_Effect("Effect_Player_Scissor_Spark", m_pOwner->Get_Model()->Get_Bone_CombinedMatPtr("BN_Weapon_R"), m_pOwner->Get_Transform()->Get_WorldMatrixPtr());
}

HRESULT CSplitWeapon::Setup_Piece(PIECE_TYPE ePiece, CModel* pModel, const _float4x4* pSocket, const string& strTop, const string& strBottom, const string& strEffect)
{
	WEAPON_PIECE& Piece = m_Pieces[ePiece];

	Piece.pModel = pModel;
	if(ePiece == PIECE_MERGE)
	{
		m_pModelCom = pModel;
		Safe_AddRef(m_pModelCom); // Weapon에서 해제함 
	}

	Piece.pSocketBoneMatrix = pSocket;
	XMStoreFloat4x4(&Piece.LocalOffset, XMMatrixIdentity());
	Piece.bActive = (ePiece == PIECE_MERGE);

	Piece.iTrailTopIndex = pModel->Get_BoneIndex(strTop.c_str());
	Piece.iTrailBottomIndex = pModel->Get_BoneIndex(strBottom.c_str());
	Piece.iEffectIndex = pModel->Get_BoneIndex(strEffect.c_str());

	if (Piece.iTrailTopIndex == -1 || Piece.iTrailBottomIndex == -1)
	{
		MSG_BOX("CSplitWeapon : Find TrailBone Index Failed ");
		return E_FAIL;
	}

	if (Piece.iEffectIndex == -1)
	{
		MSG_BOX("CSplitWeapon : Find EffectBone Index Failed ");
		return E_FAIL;
	}

	return S_OK;
}

void CSplitWeapon::Drive_PieceHit(PIECE_TYPE ePiece, _bool bInHitWindow)
{
	WEAPON_PIECE& Piece = m_Pieces[ePiece];
	if (bInHitWindow == Piece.bHitWindow) return;
	Piece.bHitWindow = bInHitWindow;

	if (bInHitWindow)
	{
		On_Collision(m_eCurAttackType, m_iCurComboStep, ePiece);
		Set_PieceTrail(ePiece, true);
	}
	else
	{
		Off_Collision(ePiece);
		Set_PieceTrail(ePiece, false);
	}
}

void CSplitWeapon::Set_PieceTrail(PIECE_TYPE ePiece, _bool bActive)
{
	if (ePiece >= PIECE_TYPE::PIECE_END) return;

	WEAPON_PIECE& Piece = m_Pieces[ePiece];

	if(bActive && Piece.pTrailEffect == nullptr)
	{
		if (m_eCurAttackType == AttackType::End) return;
		if (m_iCurComboStep < 0 || m_iCurComboStep >= static_cast<_int>(MAX_COMBO)) return;

		const string& strFX = m_FXNames[ETOUI(m_eCurAttackType)][m_iCurComboStep];
		if (strFX.empty() || Piece.iTrailTopIndex < 0 || Piece.iTrailBottomIndex < 0) return;

		const _float4x4* pTopBoneMat = Piece.pModel->Get_Bone_CombinedMatPtr(Piece.iTrailTopIndex);
		const _float4x4* pBottomBoneMat = Piece.pModel->Get_Bone_CombinedMatPtr(Piece.iTrailBottomIndex);
		if (pTopBoneMat == nullptr || pBottomBoneMat == nullptr) return;
		
		Piece.pTrailEffect = m_pGameInstance->Play_Effect(strFX, pTopBoneMat, pBottomBoneMat, &Piece.CombinedWorld);
	}
	else if(!bActive && Piece.pTrailEffect != nullptr)
	{
		Piece.pTrailEffect->Stop();
		Piece.pTrailEffect = nullptr;
	}

}

void CSplitWeapon::Deactivate_Piece(PIECE_TYPE ePiece)
{
	if (ePiece >= PIECE_TYPE::PIECE_END) return;

	WEAPON_PIECE& Piece = m_Pieces[ePiece];
	End_Sweep(Piece.Sweep);
	Piece.bHitWindow = false;

	Set_PieceTrail(ePiece, false);
}

HRESULT CSplitWeapon::Get_PieceBoneWorldMatrix(const WEAPON_PIECE& Piece, _int iBoneIndex, _matrix& OutMatrix)
{
	if (iBoneIndex <= -1 || Piece.pModel == nullptr) return E_FAIL;
	
	const _float4x4* pBoneMat = Piece.pModel->Get_Bone_CombinedMatPtr(iBoneIndex);
	if (pBoneMat == nullptr) return E_FAIL;

	OutMatrix = XMLoadFloat4x4(pBoneMat) * XMLoadFloat4x4(&Piece.CombinedWorld);
	return S_OK;
}


void CSplitWeapon::Free()
{
	__super::Free();	

	for(_int i = 0; i < PIECE_END; ++i)
	{
		Safe_Release(m_Pieces[i].pModel);
		Safe_Release(m_Pieces[i].Sweep.pDebugCapsule);
	}
}

