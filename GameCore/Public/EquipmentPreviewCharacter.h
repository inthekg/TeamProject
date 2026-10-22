#pragma once
#include "Core_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)

class CMasterModel;
class CAnimator;
class CModel;
class CBodyPart;

NS_END

NS_BEGIN(GameCore)

class CBodyPart;
class CPartSet;

class GAMECORE_DLL CEquipmentPreviewCharacter final : public CGameObject
{
private:
    CEquipmentPreviewCharacter(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CEquipmentPreviewCharacter(const CEquipmentPreviewCharacter& Prototype);
    virtual ~CEquipmentPreviewCharacter() = default;

public:
    virtual HRESULT     Initialize_Prototype() override;
    virtual HRESULT     Initialize(void* pArg) override;

    virtual void        Priority_Update(_float fTimeDelta) override;
    virtual void        Update(_float fTimeDelta) override;
    virtual void        Late_Update(_float fTimeDelta) override;

    virtual HRESULT     Render() override;
    virtual HRESULT     Render_Shadow() override;

public:
    // 프리뷰 캐릭터의 갱신 및 렌더링 활성 상태를 설정
    void                Set_PreviewActive(_bool bActive);

    // 프리뷰 캐릭터가 현재 활성 상태인지 반환
    _bool               Is_PreviewActive() const { return m_bPreviewActive; }

    void                Set_PreviewWeaponSlot(_uint iWeaponSlot); // 클릭한 무기 슬롯의 프리뷰 모델을 선택한다.
    _uint               Get_PreviewWeaponSlot() const { return m_iPreviewWeaponSlot; }

public:
    void                Rotate_Preview(_float fMouseDeltaX);
    void                Reset_PreviewRotation();

public:
    HRESULT             Apply_EquipmentPreset();
    HRESULT             Apply_LegionArmPreset(LegionArmType eArmType); // 리전 암 페이지 진입 시 프리셋과 진입 모션을 적용한다.
    void                Play_LegionArmStatRoomAction(LegionArmType eArmType); // 리전 암을 바꿔 낄 때 해당 모션을 처음부터 요청한다.
    void                Clear_StatRoomPreset(); // 장비 UI를 벗어날 때 스탯룸 파라미터를 해제한다.

public:
    _bool               Equip_PreviewCostume(_uint64 iItemID);       // ItemID에 해당하는 코스튬을 프리뷰 캐릭터에 장착한다.
    void                Unequip_PreviewCostume(PLAYER_PART ePart);   // 지정 코스튬 파츠를 프리뷰 캐릭터에서 해제한다.

private:
    HRESULT             Ready_MasterModel();
    HRESULT             Ready_Animator();
    HRESULT             Ready_PartObjects();

    HRESULT             Ready_PreviewEquipment(); // 기본 리전 암과 슬롯 1 무기 모델을 생성한다.
    void                Update_PreviewWeaponWorld(); // 오른손 소켓을 기준으로 무기 월드 행렬을 갱신한다.
    HRESULT             Render_PreviewWeapon(); // 현재 선택된 무기 모델만 렌더 명령으로 제출한다.
    
    HRESULT             Ready_PreviewLight();

private:
    CMasterModel*       m_pMasterModel = { nullptr };
    CAnimator*          m_pAnimator = { nullptr };
    CPartSet*           m_pPreviewPartSet = { nullptr }; // 프리뷰 캐릭터가 보유하고 장착할 전체 코스튬 파츠

    _bool               m_bPreviewActive = { false };

    _float              m_fPreviewYaw = { XM_PI };

private:
    CBodyPart*          m_pLegionArms[ETOUI(LegionArmType::End)] = {};    // 프리뷰용 리전 암 시각 파츠
    LegionArmType       m_ePreviewLegionArmType = { LegionArmType::ProtoArm };

    CModel*             m_pPreviewWeaponModels[2] = {}; // 0번은 현재 기본 무기, 1번은 추후 획득 무기
    const _float4x4*    m_pRightHandSocketMatrix = { nullptr }; // BN_Weapon_R의 현재 본 행렬
    _float4x4           m_PreviewWeaponWorldMatrix = {}; // 오른손 소켓과 캐릭터 월드를 결합한 행렬

    _uint               m_iPreviewWeaponSlot = { 0 }; // 현재 프리뷰에 표시할 무기 슬롯
    _uint               m_iPreviewLightIndex = { UINT_MAX };
    _uint               m_iPreviewFillLightIndex = { UINT_MAX };

    _bool               m_bPreviewWeaponVisible = { true };

public:
    static CEquipmentPreviewCharacter* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void Free() override;
};

NS_END