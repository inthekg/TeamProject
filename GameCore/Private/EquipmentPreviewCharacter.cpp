#include "EquipmentPreviewCharacter.h"
#include "GameInstance.h"
#include "MasterModel.h"
#include "Animator.h"
#include "AnimDriver.h"
#include "BodyPart.h"
#include "Bone.h"
#include "Model.h"
#include "Mesh.h"
#include "Material.h"
#include "RenderStruct.h"
#include "Engine_Utility.h"
#include "PartSet.h"
#include "ItemTable.h"
CEquipmentPreviewCharacter::CEquipmentPreviewCharacter(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject{ pDevice, pContext }
{
}

CEquipmentPreviewCharacter::CEquipmentPreviewCharacter(const CEquipmentPreviewCharacter& Prototype)
    : CGameObject{ Prototype }
{
}

HRESULT CEquipmentPreviewCharacter::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CEquipmentPreviewCharacter::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_MasterModel()))
        return E_FAIL;

    if (FAILED(Ready_Animator()))
        return E_FAIL;

    if (FAILED(Ready_PartObjects()))
        return E_FAIL;

    if (FAILED(Ready_PreviewEquipment()))
        return E_FAIL;

    if (FAILED(Ready_PreviewLight()))
        return E_FAIL;

    // 장비 UI가 열리기 전에는 갱신 및 렌더링하지 않는다.
    m_bPreviewActive = false;

    return S_OK;
}

void CEquipmentPreviewCharacter::Priority_Update(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);

    if (!m_bPreviewActive)
        return;
}

void CEquipmentPreviewCharacter::Update(_float fTimeDelta)
{
    if (!m_bPreviewActive)
        return;

    // 프리뷰가 표시되는 동안에만 애니메이션 시간과 현재 포즈를 갱신한다.
    if (nullptr != m_pAnimator)
        m_pAnimator->Update(fTimeDelta);

}

void CEquipmentPreviewCharacter::Late_Update(_float fTimeDelta)
{
    UNREFERENCED_PARAMETER(fTimeDelta);

    if (!m_bPreviewActive)
        return;

    // Animator가 계산한 현재 포즈로 프리뷰 전용 MasterModel의 최종 본 행렬을 갱신한다.
    if (nullptr != m_pMasterModel && nullptr != m_pAnimator)
        m_pMasterModel->Update_Bones(&m_pAnimator->Get_CurPose());

    Update_PreviewWeaponWorld();

    m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
    m_pGameInstance->Add_RenderGroup(RENDERID::FORWARD, this);
}

HRESULT CEquipmentPreviewCharacter::Render()
{
    if (!m_bPreviewActive)
        return S_OK;

    // 현재 프리뷰에 장착된 몸체·얼굴·머리·가면·액세서리를 렌더링한다.
    if (m_pPreviewPartSet != nullptr && FAILED(m_pPreviewPartSet->Render()))
        return E_FAIL;

    CBodyPart* pPreviewLegionArm = m_ePreviewLegionArmType < LegionArmType::End ? m_pLegionArms[ETOUI(m_ePreviewLegionArmType)] : nullptr;
    if (pPreviewLegionArm != nullptr && FAILED(pPreviewLegionArm->Render()))
        return E_FAIL;    
    
    if (FAILED(Render_PreviewWeapon()))
        return E_FAIL;

    return S_OK;
}

HRESULT CEquipmentPreviewCharacter::Render_Shadow()
{
    if (!m_bPreviewActive)
        return S_OK;

    // 캐릭터 파츠가 생성된 이후 그림자 렌더링을 추가한다.
    return S_OK;
}

void CEquipmentPreviewCharacter::Set_PreviewActive(_bool bActive)
{
    m_bPreviewActive = bActive;

    if (m_iPreviewLightIndex != UINT_MAX)
        m_pGameInstance->Set_LightEnable(LIGHT::POINT, m_iPreviewLightIndex, bActive); 

    if (m_iPreviewFillLightIndex != UINT_MAX)
        m_pGameInstance->Set_LightEnable(LIGHT::POINT, m_iPreviewFillLightIndex, bActive);
}

void CEquipmentPreviewCharacter::Set_PreviewWeaponSlot(_uint iWeaponSlot)
{
    if (iWeaponSlot >= 2)
        return;

    // 아직 모델이 연결되지 않은 슬롯은 선택하지 않는다.
    if (m_pPreviewWeaponModels[iWeaponSlot] == nullptr)
        return;

    m_iPreviewWeaponSlot = iWeaponSlot;
}

void CEquipmentPreviewCharacter::Rotate_Preview(_float fMouseDeltaX)
{
    if (!m_bPreviewActive || m_pTransformCom == nullptr)
        return;

    // 마우스 1픽셀당 0.25도씩 회전한다.
    constexpr _float fDegreePerPixel = 0.25f;

    m_fPreviewYaw += XMConvertToRadians(fMouseDeltaX * fDegreePerPixel);

    m_fPreviewYaw = fmodf(m_fPreviewYaw, XM_2PI);
    m_pTransformCom->Rotation(0.f, m_fPreviewYaw, 0.f);
}

void CEquipmentPreviewCharacter::Reset_PreviewRotation()
{
    if (m_pTransformCom == nullptr)
        return;

    // Level_GamePlay에서 설정한 초기 180도 방향과 동일하게 맞춘다.
    m_fPreviewYaw = XM_PI;
    m_pTransformCom->Rotation(0.f, m_fPreviewYaw, 0.f);
}

HRESULT CEquipmentPreviewCharacter::Apply_EquipmentPreset()
{
    if (m_pAnimator == nullptr || m_pTransformCom == nullptr)
        return E_FAIL;

    // 리전 암 스탯룸 상태를 먼저 해제하고 무기 스탯룸 상태로 전환한다.
    m_pAnimator->Set_Bool(PARAM_STATROOM_LEGIONARM, false);
    m_pAnimator->Set_Bool(PARAM_STATROOM_WEAPON, true);

    m_bPreviewWeaponVisible = true;

    m_fPreviewYaw = XM_PI;
    m_pTransformCom->Rotation(0.f, m_fPreviewYaw, 0.f);

    return S_OK;
}

HRESULT CEquipmentPreviewCharacter::Apply_LegionArmPreset(LegionArmType eArmType)
{
    if(m_pAnimator == nullptr || eArmType >= LegionArmType::End)
        return E_FAIL;

    m_ePreviewLegionArmType = eArmType;

    m_pAnimator->Set_Bool(PARAM_STATROOM_WEAPON, false);
    m_pAnimator->Set_Bool(PARAM_STATROOM_LEGIONARM, true);
    m_pAnimator->Set_Int(PARAM_LEGIONARM_TYPE, ETOUI(eArmType)); // 진입할 리전 암 상태의 조건을 먼저 맞춘다.

    m_bPreviewWeaponVisible = false;

    m_fPreviewYaw = XM_PI + XMConvertToRadians(-10.f);
    m_pTransformCom->Rotation(0.f, m_fPreviewYaw, 0.f);

    m_pAnimator->Request_Action(L"StatRoom_LegionArm"); // 페이지 진입 시 선택된 리전 암 모션을 처음부터 재생한다.

    return S_OK;
}

void CEquipmentPreviewCharacter::Play_LegionArmStatRoomAction(LegionArmType eArmType)
{
    if (m_pAnimator == nullptr || !m_bPreviewActive || eArmType >= LegionArmType::End)
        return;
    m_ePreviewLegionArmType = eArmType;
    m_pAnimator->Set_Int(PARAM_LEGIONARM_TYPE, ETOUI(eArmType)); // Action 상태 검색 전에 리전 암 타입 조건을 맞춘다.
    m_pAnimator->Request_Action(L"StatRoom_LegionArm"); // 동일 Action에서 타입에 맞는 단발 모션을 요청한다.
}

void CEquipmentPreviewCharacter::Clear_StatRoomPreset()
{
    if (m_pAnimator == nullptr)
        return;

    m_pAnimator->Set_Bool(PARAM_STATROOM_WEAPON, false);
    m_pAnimator->Set_Bool(PARAM_STATROOM_LEGIONARM, false);
    m_pAnimator->Clear_Requests();
}

_bool CEquipmentPreviewCharacter::Equip_PreviewCostume(_uint64 iItemID)
{
    if (m_pPreviewPartSet == nullptr || iItemID == 0)
        return false;

    const ITEM_DATA* pItemData = CItemTable::Find(iItemID);

    // 코스튬 파츠가 아닌 ItemID가 들어오는 것을 방지한다.
    if (pItemData == nullptr || pItemData->eType != ITEM_TYPE::BODY_PART)
        return false;

    return m_pPreviewPartSet->Equip(iItemID);
}

void CEquipmentPreviewCharacter::Unequip_PreviewCostume(PLAYER_PART ePart)
{
    if (m_pPreviewPartSet == nullptr || ePart == PLAYER_PART::END)
        return;

    // 얼굴과 머리카락은 프리뷰의 기본 외형이므로 해제하지 않는다.
    if (ePart == PLAYER_PART::FACE || ePart == PLAYER_PART::HAIR)
        return;

    // 기본 의상도 사라지면 몸체가 비어 보이므로 BODY는 해제하지 않는다.
    if (ePart == PLAYER_PART::BODY)
        return;

    m_pPreviewPartSet->UnEquip(ePart);
}

HRESULT CEquipmentPreviewCharacter::Ready_MasterModel()
{
    // 실제 플레이어와 동일한 모델 축 및 단위 보정 행렬을 사용한다.
    _matrix PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixRotationY(XMConvertToRadians(-90.f));

    // 원본 리소스는 공유하지만 현재 본 행렬은 프리뷰 객체가 독립적으로 소유한다.
    m_pMasterModel = CMasterModel::Create(m_pDevice, m_pContext, L"Pino", L"Pino_AllParts", PreTransformMatrix);
    if (nullptr == m_pMasterModel)
        return E_FAIL;

    return S_OK;
}

HRESULT CEquipmentPreviewCharacter::Ready_Animator()
{
    if (nullptr == m_pMasterModel)
        return E_FAIL;

    CAnimator::ANIMATOR_DESC Desc{};

    // 애니메이션이 없거나 초기화될 때 사용할 기준 포즈를 복사한다.
    Desc.BasePose = *m_pMasterModel->Get_BindPosePtr();

    // 실제 플레이어와 동일한 Pino 원본 애니메이션 묶음을 사용한다.
    Desc.RawAnimationLoadDesc.strFolderName = L"Pino";
    Desc.RawAnimationLoadDesc.strAnimationBaseName = L"Pino_AllParts";
    Desc.RawAnimationLoadDesc.eTimeUnit = ANIMATION_TIME_UNIT::TICKS;
    Desc.RawAnimationLoadDesc.bStaticAnimation = true;

    const vector<CBone*>& Bones = m_pMasterModel->Get_Bones();

    Desc.Bones.clear();
    Desc.Bones.reserve(Bones.size());

    // MasterModel의 본 이름과 부모 관계를 Animator용 설명 데이터로 복사한다.
    for (_uint i = 0; i < Bones.size(); ++i)
    {
        CAnimator::ANIMATOR_BONE_DESC BoneDesc{};

        if (nullptr != Bones[i])
        {
            BoneDesc.strName = Bones[i]->Get_Name();

            _wstring strBoneName;
            strBoneName.assign(BoneDesc.strName.begin(), BoneDesc.strName.end());

            BoneDesc.iNameHash = Hash_String(strBoneName.c_str());
            BoneDesc.iParentIndex = Bones[i]->Get_ParentIndex();
        }

        Desc.Bones.push_back(BoneDesc);
    }

    // 모델에 적용한 축 및 크기 보정 행렬을 Animator에도 동일하게 전달한다.
    Desc.PreTransformMatrix = m_pMasterModel->Get_PreTransformMatrix();

    m_pAnimator = CAnimator::Create(Desc);
    if (nullptr == m_pAnimator)
        return E_FAIL;

    // 루트 본을 지정한다. 현재는 루트 모션을 월드 Transform에 적용하지 않는다.
    const _int iPelvisBoneIndex = m_pMasterModel->Get_BoneIndex("Bip001-Pelvis");
    if (0 > iPelvisBoneIndex)
        return E_FAIL;

    m_pAnimator->Set_RootBoneIndex(iPelvisBoneIndex);

    // 우선 프로젝트에서 실제 플레이어가 사용 중인 확실한 원본 애니메이션으로 연결 상태를 검사한다.
    if (FAILED(m_pAnimator->Play_Clip(Hash_String(L"AS_Pino_O_Idle"))))
        return E_FAIL;

    return S_OK;
}

HRESULT CEquipmentPreviewCharacter::Ready_PartObjects()
{
    if (m_pMasterModel == nullptr || m_pTransformCom == nullptr)
        return E_FAIL;

    CPartSet::PARTSET_DESC PartSetDesc{};
    PartSetDesc.pMasterModel = m_pMasterModel;
    PartSetDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();

    // 실제 플레이어와 동일한 파츠 관리 구조를 프리뷰 캐릭터에도 생성한다.
    m_pPreviewPartSet = CPartSet::Create(m_pDevice, m_pContext, PartSetDesc);
    if (m_pPreviewPartSet == nullptr)
        return E_FAIL;

    // 프리뷰 캐릭터의 기본 외형을 구성한다.
    if (!m_pPreviewPartSet->Equip(ItemID::BODY_BASE))
        return E_FAIL;

    if (!m_pPreviewPartSet->Equip(ItemID::FACE_BASE))
        return E_FAIL;

    if (!m_pPreviewPartSet->Equip(ItemID::HAIR_BASE))
        return E_FAIL;

    return S_OK;
}

HRESULT CEquipmentPreviewCharacter::Ready_PreviewEquipment()
{
    if (m_pMasterModel == nullptr || m_pTransformCom == nullptr)
        return E_FAIL;

    const _wstring LegionArmModelTags[ETOUI(LegionArmType::End)] =
    {
        L"Prototype_Component_Model_Player_ProtoArm",
        L"Prototype_Component_Model_Player_GrappleGun",
        L"Prototype_Component_Model_Player_ChargeGun"
    };

    for (_uint iArmIndex = 0; iArmIndex < ETOUI(LegionArmType::End); ++iArmIndex)
    {
        CBodyPart::BODYPART_DESC LegionArmDesc{};
        LegionArmDesc.pMasterModel = m_pMasterModel;
        LegionArmDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
        LegionArmDesc.iLevelIndex = ETOUI(LEVEL::GAMEPLAY);
        LegionArmDesc.strModelTag = LegionArmModelTags[iArmIndex];

        m_pLegionArms[iArmIndex] = m_pGameInstance->Clone_Prototype<CBodyPart>(ETOUI(LEVEL::GAMEPLAY), L"Prototype_GameObject_BodyPart", &LegionArmDesc);

        if (m_pLegionArms[iArmIndex] == nullptr)
            return E_FAIL;
    }

    m_ePreviewLegionArmType = LegionArmType::ProtoArm;


    // 현재 플레이어의 0번 무기와 동일한 모델을 프리뷰 슬롯 1에 생성한다.
    if (FAILED(Add_Component(ETOUI(LEVEL::GAMEPLAY), L"Prototype_Component_Model_GreatSword", L"Com_PreviewWeaponPrimary", reinterpret_cast<CComponent**>(&m_pPreviewWeaponModels[0]))))
        return E_FAIL;
    // 애니메이션이 없는 무기 모델이므로 초기 본 행렬을 한 번 계산
    m_pPreviewWeaponModels[0]->Update_Bones();

    if (FAILED(Add_Component(ETOUI(LEVEL::GAMEPLAY), L"Prototype_Component_Model_Scissor_Merge", L"Com_PreviewWeaponSecondary", reinterpret_cast<CComponent**>(&m_pPreviewWeaponModels[1]))))
        return E_FAIL;
    // 정적 상태에서 렌더링할 수 있도록 초기 본 행렬을 계산한다.
    m_pPreviewWeaponModels[1]->Update_Bones();

    m_pRightHandSocketMatrix = m_pMasterModel->Get_Bone_CombinedMatPtr("BN_Weapon_R");
    if (m_pRightHandSocketMatrix == nullptr)
        return E_FAIL;

    m_iPreviewWeaponSlot = 0;

    return S_OK;
}

void CEquipmentPreviewCharacter::Update_PreviewWeaponWorld()
{
    if (m_pRightHandSocketMatrix == nullptr || m_pTransformCom == nullptr)
        return;

    _matrix SocketMatrix = XMLoadFloat4x4(m_pRightHandSocketMatrix);

    _vector vScale{};
    _vector vRotation{};
    _vector vTranslation{};

    // 캐릭터 본에 포함된 스케일은 제거하고 회전과 위치만 무기에 적용한다.
    if (XMMatrixDecompose(&vScale, &vRotation, &vTranslation, SocketMatrix))
    {
        SocketMatrix = XMMatrixAffineTransformation(XMVectorSet(1.f, 1.f, 1.f, 0.f), XMVectorZero(), vRotation, vTranslation);
    }

    const _matrix PreviewWorldMatrix = SocketMatrix * XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr());
    XMStoreFloat4x4(&m_PreviewWeaponWorldMatrix, PreviewWorldMatrix);
}

HRESULT CEquipmentPreviewCharacter::Render_PreviewWeapon()
{
    if (!m_bPreviewWeaponVisible)
        return S_OK;

    if (m_iPreviewWeaponSlot >= 2)
        return S_OK;

    CModel* pWeaponModel = m_pPreviewWeaponModels[m_iPreviewWeaponSlot];

    if (pWeaponModel == nullptr)
        return S_OK;

    const auto& RenderUnits = pWeaponModel->Get_RenderUnits();
    const RENDERID eCurrentPass = m_pGameInstance->Get_CurRenderPass();

    for (_uint iMeshIndex = 0; iMeshIndex < RenderUnits.size(); ++iMeshIndex)
    {
        CMesh* pMesh = RenderUnits[iMeshIndex].pMesh;
        CMaterial* pMaterial = RenderUnits[iMeshIndex].pMaterial;

        if (pMesh == nullptr || pMaterial == nullptr)
            return E_FAIL;

        if (pMaterial->Get_RenderGroup() != eCurrentPass)
            continue;

        SkinnedCommand Command{};
        Command.header.etype = RENDER_TYPE::SKINNED;
        Command.header.iCommandSize = sizeof(SkinnedCommand);
        Command.iBoneCount = pMesh->Get_BoneCount();
        Command.pVIBuffer = pMesh;
        Command.pMaterial = pMaterial;
        Command.worldMatrix = m_PreviewWeaponWorldMatrix;

        if (FAILED(pWeaponModel->Fill_SkinnedCommandBoneInfo(Command, iMeshIndex)))
            continue;

        m_pGameInstance->Submit_Skinned_Resolved(Command);
    }

    return S_OK;
}

HRESULT CEquipmentPreviewCharacter::Ready_PreviewLight()
{
    LIGHT_DESC LightDesc{};

    LightDesc.eType = LIGHT::POINT;

    // 캐릭터 앞쪽에서 비추는 따뜻한 전면광
    LightDesc.vDiffuse = _float4(2.1f, 2.15f, 2.3f, 1.f);
    LightDesc.vAmbient = _float4(0.f, 0.f, 0.f, 1.f);
    LightDesc.vSpecular = _float4(0.f, 0.f, 0.f, 1.f);

    LightDesc.vPosition = _float4(0.f, -998.7f, -8.f, 1.f);
    LightDesc.fRange = 28.f;

    // UI 프리뷰에서 별도의 Point Shadow는 불필요하다.
    LightDesc.bShadow = false;

    m_iPreviewLightIndex = m_pGameInstance->Get_LightCount(LIGHT::POINT);

    if (FAILED(m_pGameInstance->Add_Light(LIGHT::POINT, LightDesc)))
    {
        m_iPreviewLightIndex = UINT_MAX;
        return E_FAIL;
    }

    LIGHT_DESC FillLightDesc{};

    FillLightDesc.eType = LIGHT::POINT;

    // 반대편의 어두운 면을 들어 올리는 차가운 보조광
    FillLightDesc.vDiffuse = _float4(0.55f, 0.65f, 0.9f, 1.f);
    FillLightDesc.vPosition = _float4(2.5f, -998.8f, -7.f, 1.f);
    FillLightDesc.fRange = 25.f;
    FillLightDesc.bShadow = false;
    
    m_iPreviewFillLightIndex = m_pGameInstance->Get_LightCount(LIGHT::POINT);

    if (FAILED(m_pGameInstance->Add_Light(LIGHT::POINT, FillLightDesc)))
    {
        m_iPreviewFillLightIndex = UINT_MAX;
        return E_FAIL;
    }

    // 장비창이 열리기 전까지 조명을 끈다.
    if (FAILED(m_pGameInstance->Set_LightEnable(LIGHT::POINT, m_iPreviewLightIndex, false)))
        return E_FAIL;
    if (FAILED(m_pGameInstance->Set_LightEnable(LIGHT::POINT, m_iPreviewFillLightIndex, false)))
        return E_FAIL;

    return S_OK;
}

CEquipmentPreviewCharacter* CEquipmentPreviewCharacter::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CEquipmentPreviewCharacter* pInstance = new CEquipmentPreviewCharacter(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CEquipmentPreviewCharacter");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CEquipmentPreviewCharacter::Clone(void* pArg)
{
    CEquipmentPreviewCharacter* pInstance = new CEquipmentPreviewCharacter(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CEquipmentPreviewCharacter");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CEquipmentPreviewCharacter::Free()
{
    if (m_iPreviewFillLightIndex != UINT_MAX)
        m_pGameInstance->Remove_Light(LIGHT::POINT, m_iPreviewFillLightIndex);

    if (m_iPreviewLightIndex != UINT_MAX)
        m_pGameInstance->Remove_Light(LIGHT::POINT, m_iPreviewLightIndex);

    m_iPreviewFillLightIndex = UINT_MAX;
    m_iPreviewLightIndex = UINT_MAX;

    for (_uint iArmIndex = 0; iArmIndex < ETOUI(LegionArmType::End); ++iArmIndex)
        Safe_Release(m_pLegionArms[iArmIndex]);

    for (_uint iIndex = 0; iIndex < 2; ++iIndex)
        Safe_Release(m_pPreviewWeaponModels[iIndex]);

    Safe_Release(m_pPreviewPartSet);

    Safe_Release(m_pAnimator);
    Safe_Release(m_pMasterModel);

    __super::Free();
}
