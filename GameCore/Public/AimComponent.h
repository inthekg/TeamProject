#pragma once
#include "Component.h"
#include "Core_Defines.h"
#include "AimBoneChain.h"

NS_BEGIN(Engine)
class CModel;
class CTransform;
class CSpringArm;
NS_END

NS_BEGIN(GameCore)
/* 절차적 조준 Procedural Aim */
class CAimComponent final : public CComponent
{
public:
	typedef struct tagAimComponentDesc
	{
		CModel*				pModel			= { nullptr };
		CTransform*			pOwnerTransform = { nullptr };
		CSpringArm*			pSpringArm		= { nullptr };
	} AIM_COMPONENT_DESC;

	static constexpr _int	INVALID_PROFILE = { -1 };

private:
	CAimComponent();
	virtual ~CAimComponent() = default;

public:
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Clear() override;

public:
	//조준 자세 프로필. 등록 성공하면 handle(인덱스), 실패하면 INVALID_PROFILE 반환 
	_int					Register_AimProfile(const CAimBoneChain::AIM_PROFILE& Profile);

	//조준 자세 요청
	//매 프레임 불러야 유지되고, 끊기면 풀림
	void					Request_AimPoseToCamera(_int iProfile);
	void					Request_AimPoseToPoint(_int iProfile, _fvector vWorldPos);

	_bool					Update_AimPose(_float fTimeDelta);
	void					Apply_AimPose(POSE& Pose);

private:
	void					Compute_CameraRay(_vector& vStart, _vector& vDir) const;
	_bool					Is_ValidProfile(_int iProfile) const 
	{ 
		return iProfile >= 0 && iProfile < static_cast<_int>(m_AimChains.size()); 
	}

private:
	CModel*					m_pModel			= { nullptr };
	CTransform*				m_pOwnerTransform	= { nullptr };
	CSpringArm*				m_pSpringArm		= { nullptr };

	//프로필별 조준 체인 (프로필 번호 = 인덱스)
	vector<CAimBoneChain*>	m_AimChains;

	//이번 프레임 조준 자세 요청 플래그. 해당 프레임에서만 유효 
	_int					m_iRequestedProfile = { INVALID_PROFILE };
	_float3					m_vRequestedDir		= { 0.f, 0.f, 1.f };

public:
	static CAimComponent*	Create(void* pArg);
	virtual void			Free() override;
};

NS_END