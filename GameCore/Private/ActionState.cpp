#include "ActionState.h"
#include "Status.h"
#include "StatComponent.h"
#include "Movement.h"
#include "InputComponent.h"
#include "ActionFSM.h"
#include "Player_Data.h"

// -------------------------------------------------
// None State
// -------------------------------------------------
CNoneState::CNoneState()
    :CActionState(PlayerActionID::None)
{
    //전이 조건 생성 . 우선순위, 전이 State ID, 조건 
    AddTransition(25, PlayerActionID::Interact,
        [](StateContext& context)->bool
        {
            _bool bHasInteract = (context.pStatusCom->Get_PendingInteraction() != nullptr);
            return bHasInteract && context.pInputCom->Consume(PlayerActionID::Interact);
        });

    AddTransition(21, PlayerActionID::Dodge,
        [](StateContext& context)->bool
        {
            return context.pInputCom->Consume(PlayerActionID::Dodge) && context.pStatCom->Has_Stamina(PlayerData::g_fDodgeStaminaCost);
        });

    AddTransition(20, PlayerActionID::Roll,
        [](StateContext& context)->bool
        {
            return context.pInputCom->Consume(PlayerActionID::Roll) && context.pStatCom->Has_Stamina(PlayerData::g_fRollStaminaCost);
        });

    AddTransition(12, PlayerActionID::Attack,
        [](StateContext& context)->bool
        {
            //Consume 아님 주의. 여부만 확인하고 AttackState에서 Consume 처리 
            _bool bAttackInput = context.pInputCom->Peek(PlayerActionID::Fable_Arts);
            return bAttackInput && context.pStatCom->Has_Stamina(PlayerData::g_fFableArtsBaseStaminaCost);
        });

    AddTransition(11, PlayerActionID::Attack,
        [](StateContext& context)->bool
        {
            _bool bAttackInput = context.pInputCom->Peek(PlayerActionID::Heavy_Attack);
            return bAttackInput && context.pStatCom->Has_Stamina(PlayerData::g_fHeavyAttackStaminaCost);
        });

    AddTransition(10, PlayerActionID::Attack,
        [](StateContext& context)->bool
        {
            _bool bAttackInput = context.pInputCom->Peek(PlayerActionID::Light_Attack);
            return bAttackInput && context.pStatCom->Has_Stamina(PlayerData::g_fLightAttackStaminaCost);
        });

}

void CNoneState::Begin(StateContext& Context)
{
    //Status 세트로 적용 
    Context.pStatusCom->Apply_Profile(CStatusComponent::g_Profile_Free);
}


// -------------------------------------------------
// Dodge State
// ------------------------------------------------

CDodgeState::CDodgeState()
    :CActionState(PlayerActionID::Dodge)
{
    AddTransition(5, PlayerActionID::None,
        [this](StateContext& Context)->bool
        {
            return m_bFinished;
        });
}

void CDodgeState::Begin(StateContext& Context)
{
    m_bFinished = false;
    Context.pStatCom->Consume_Stamina(PlayerData::g_fDodgeStaminaCost);
    Context.pStatusCom->Apply_Profile(CStatusComponent::g_Profile_Armored);
}

void CDodgeState::Update(StateContext& Context, _float fTimeDelta)
{
    //애니메이션이 끝나면 m_bFinished = false;
}

//-------------------------------------------------
// Interact State
//-------------------------------------------------
CInteractState::CInteractState()
	:CActionState(PlayerActionID::Interact)
{
    AddTransition(5, PlayerActionID::None,
        [this](StateContext& context)->bool
        {
            return m_bFinished;
        });
}

void CInteractState::Begin(StateContext& Context)
{
    m_bFinished = false;

	m_pData = Context.pStatusCom->Get_PendingInteraction();
    if(m_pData == nullptr)
    {
        m_bFinished = true;
        return;
	}

	Context.pStatusCom->Apply_Profile(m_pData->profile);
	Context.pStatusCom->Set_PendingInteraction(nullptr);

}

void CInteractState::Update(StateContext& Context, _float fTimeDelta)
{
    //OnNotify로 Finish = false처리 
}

//-------------------------------------------------
// Attacking State
//-------------------------------------------------

CAttackingState::CAttackingState()
	:CActionState(PlayerActionID::Attack)
{
    /* Dodge 캔슬 */
    AddTransition(20, PlayerActionID::Dodge,
        [this](StateContext& Context)->bool
        {
            return Context.pStatusCom->CanCancel()
                && Context.pInputCom->Consume(PlayerActionID::Dodge)
                && Context.pStatCom->Has_Stamina(PlayerData::g_fDodgeStaminaCost);
        });

    /* Roll 캔슬 */
    AddTransition(19, PlayerActionID::Roll,
        [this](StateContext& Context)->bool
        {
            return Context.pStatusCom->CanCancel()
                && Context.pInputCom->Consume(PlayerActionID::Roll)
                && Context.pStatCom->Has_Stamina(PlayerData::g_fRollStaminaCost);
        });

    /* Guard 캔슬 */
    AddTransition(15, PlayerActionID::None,
        [this](StateContext& Context)->bool
        {
            return Context.pStatusCom->CanCancel()
                && Context.pInputCom->IsHold(PlayerActionID::Guard);
        });

    /* CanCancel + 이동 캔슬 */
    AddTransition(10, PlayerActionID::None,
        [this](StateContext& Context)->bool
        {
            return Context.pStatusCom->CanCancel()
                && Context.pInputCom->Has_MoveInput();
        });

    /* 종료 */
    AddTransition(5, PlayerActionID::None,
        [this](StateContext& Context)->bool
        {
            return m_bFinished;
        });
}

void CAttackingState::Begin(StateContext& Context)
{
	m_bFinished = false;
	Context.pStatusCom->Apply_Profile(CStatusComponent::g_Profile_Attack);  
}

void CAttackingState::Update(StateContext& Context, _float fTimeDelta)
{
    if (Context.pStatusCom->CanCancel())
    {
        if (Context.pInputCom->Consume(PlayerActionID::Light_Attack))
        {
            //애니메이션 호출
        }
        else if (Context.pInputCom->Consume(PlayerActionID::Heavy_Attack))
        {
            //애니메이션 호출
        }
        else if (Context.pInputCom->Consume(PlayerActionID::Fable_Arts))
        {
            //애니메이션 호출
        }
    }
}

void CAttackingState::End(StateContext& Context)
{
    AddTransition(5, PlayerActionID::None,
        [this](StateContext& Context)->bool
        {
            return m_bFinished;
        });
}

//void CAttackingState::OnNotify(StateContext& Context, NotifyEvent eEventType, void* pArg)
//{
//    //콤보 시작 
//    if (eEventType == NotifyEvent::ComboStart)
//    {
//        if((pArg == nullptr))
//        {
//            m_bComboEnded = true;
// 		    m_bFinished = true;
//            return;
//		}
//
//        const CActionFSM::NotifyDesc* pComboData = static_cast<const CActionFSM::NotifyDesc*>(pArg);
//        m_iComboStep = pComboData->iComboStep;
//		ComboData* pNextCombo = Context.pEquipmentCom->Get_Weapon()->Get_ComboData(m_iComboStep);
//        if (pNextCombo == nullptr)
//        {
//            m_bComboEnded = true;
//            return;
//        }
//
//        m_pCurComboData = pNextCombo;
//		Context.pStatCom->Consume_Stamina(m_pCurComboData->fStaminaCost);
//		Context.pStatusCom->Apply_Profile(m_pCurComboData->profile);
//
//    }
//    //공격 캔슬 
//    else if (eEventType == NotifyEvent::CanCancle)
//    {
//        Context.pStatusCom->CanCancel();
//    }
//    else if (eEventType == NotifyEvent::ComboEnd)
//    {
//        m_pCurComboData = nullptr;
//        m_iComboStep = 0;
//        m_bComboEnded = true;
//    }
//}

CHitState::CHitState()
	:CActionState(PlayerActionID::Hit)
{
}

void CHitState::Begin(StateContext& Context)
{
    m_bFinished = false;
	_bool bGuardHit = Context.pStatusCom->Is_TakeGuardHit();

    if(bGuardHit) Context.pStatusCom->Apply_Profile(CStatusComponent::g_Profile_Free);
    else Context.pStatusCom->Apply_Profile(CStatusComponent::g_Profile_Invincible);
}

void CHitState::Update(StateContext& Context, _float fTimeDelta)
{
}
