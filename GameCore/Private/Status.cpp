#include "Status.h"
#include "GameObject.h"
#include "AIObject.h"

// InputLock, MovementLock, RotationLock, JumpLock, CameraLock, SuperArmor, Invincible
StatusProfile CStatusComponent::g_Profile_Free = { false, false,false,false,false,false,false };
StatusProfile CStatusComponent::g_Profile_Attack = { false, true,true,true,false,false,false };
StatusProfile CStatusComponent::g_Profile_Armored = { false, false,false,false,false,true,false };
StatusProfile CStatusComponent::g_Profile_Invincible = { false, false,false,false,false,false,true };
StatusProfile CStatusComponent::g_Profile_Interacting = { true, true,true,true,true,false,true };
StatusProfile CStatusComponent::g_Profile_Stunned = { true, true,true,true,true,false,true };

CStatusComponent::CStatusComponent(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CComponent{ pDevice, pContext }
{
}

CStatusComponent::CStatusComponent(const CStatusComponent& Prototype)
	: CComponent{ Prototype }
{
}

void CStatusComponent::Apply_Profile(const StatusProfile& profile)
{
	Set_InputLock(profile.bInputLocked);
	Set_JumpLock(profile.bJumpLocked);	
	Set_MovementLock(profile.bMovementLocked);
	Set_RotationLock(profile.bRotationLocked);
	Set_CamMoveLock(profile.bCameraLocked);
	Set_SuperArmor(profile.bSuperArmor);
	Set_Invincible(profile.bInvincible);
}

void CStatusComponent::Reset_FrameStatus()
{
	m_State.m_bJustLanded = false;
	m_State.m_bJustJumped = false;
	m_State.m_bTakeDamage = false;
	m_State.m_bTakeGuardHit = false;
}

_bool CStatusComponent::Is_LockedOn() const
{
	return Get_LockedTarget() != nullptr;
}

CAIObject* CStatusComponent::Get_LockedTarget() const
{
	CAIObject* pTarget = m_State.m_pLockedTarget;

	if (pTarget == nullptr || pTarget->Is_InActive() || pTarget->Is_Dead())
		return nullptr;
	
	return pTarget; 
}


void CStatusComponent::Clear()
{
	m_State = STATUS_STATE{};
}

CStatusComponent* CStatusComponent::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CStatusComponent* pInstance = new CStatusComponent(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CStatusComponent");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CComponent* CStatusComponent::Clone(void* pArg)
{
	CStatusComponent* pInstance = new CStatusComponent(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone : CStatusComponent");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CStatusComponent::Free()
{
	__super::Free();
}
