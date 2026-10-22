#include "LocoState.h"
#include "Status.h"
#include "StatComponent.h"
#include "Movement.h"
#include "InputComponent.h"

#include "Player_Data.h"

//----------------------------------
// Grounded (블랜드 스페이스)
//----------------------------------
PlayerLocoID GroundedState::Check_NextState(StateContext& Context)
{
	return Context.pStatusCom->Is_Grounded() ? PlayerLocoID::Grounded : PlayerLocoID::Airborne;
}

//----------------------------------
// Airborne
//----------------------------------
PlayerLocoID AirborneState::Check_NextState(StateContext& Context)
{
	return Context.pStatusCom->Is_Grounded() ? PlayerLocoID::Grounded : PlayerLocoID::Airborne;
}
