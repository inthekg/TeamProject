#include "StanceState.h"
#include "Status.h"
#include "StatComponent.h"
#include "Movement.h"
#include "InputComponent.h"
#include "Player_Data.h"

/* None Stance */
PlayerStanceID CNoneStanceState::Check_NextState(StateContext& Context)
{
	_bool bCanGuard = (!Context.pStatusCom->Is_ActionPlaying() || !Context.pStatusCom->Is_Grounded());

	if (bCanGuard && Context.pInputCom->IsHold(PlayerActionID::Guard)) return PlayerStanceID::Guard;
	return PlayerStanceID::None;
}

/* Guard */
PlayerStanceID CGuardState::Check_NextState(StateContext& Context)
{
	_bool bCancelGuard = (Context.pStatusCom->Is_ActionPlaying() || !Context.pStatusCom->Is_Grounded() || !Context.pInputCom->IsHold(PlayerActionID::Guard));

	if (bCancelGuard) return PlayerStanceID::None;
	return PlayerStanceID::Guard;
}
