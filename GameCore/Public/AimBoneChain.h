#pragma once
#include "Base.h"
#include "Core_Defines.h"

NS_BEGIN(Engine)
class CModel;
class CTransform;
NS_END

NS_BEGIN(GameCore)
//절차적 조준 체인 
//지정된 본들에 회전을 가중치대로 나눠, 애니메이션 포즈에 얹는다
class CAimBoneChain final :public CBase
{
public:
	typedef struct tagBoneData
	{
		const char*			szBoneName		= { nullptr };	//주의 - 문자열 리터럴로만 넘길 것
		_float				fWeight			= { 1.f };		//분배 비율
	} BONE_DATA;

	//조중 설정 데이터
	typedef struct tagAimProfile
	{
		//체인에 연결할 본 리스트
		vector<BONE_DATA>	Bones			= {};
		//각도 제한 
		_float				fMaxYaw			= { XMConvertToRadians(80.f) };	
		_float				fMaxPitchUp		= { XMConvertToRadians(80.f) };	
		_float				fMaxPitchDown	= { XMConvertToRadians(80.f) };	
		//키고 끄는데 걸리는 시간 (0 = 즉시)
		_float				fBlendTime		= { 0.15f };	
		//목표 방향을 따라가는 속도 (0 = 즉시)
		_float				fDirFollowRate	= { 20.f }; 
		//모델 공간 기준 방향 
		_float3				vModelForward	= { 0.f, 0.f, 1.f };
		_float3				vModelUp		= { 0.f, 1.f, 0.f };		
	} AIM_PROFILE;

private:
	//매핑된 본 데이터o
	typedef struct tagChainBone
	{
		_int				iBoneIndex		= { -1 };
		_int				iParentIndex	= { -1 };
		_float				fWeight			= { 0.f };
	} CHAIN_BONE;

private:
	CAimBoneChain() = default;
	virtual ~CAimBoneChain() = default;

public:
	HRESULT					Initialize(CModel* pModel, CTransform* pOwnerTransform, const AIM_PROFILE& Profile);
	void					Update(_float fTimeDelta);
	//포즈에 조준 회전을 얹음
	void					Apply(POSE& Pose) const;
	void					Reset();

public:
	void					Set_Active(_bool bActive) { m_bActive = bActive; }
	void					Set_TargetDirection(_fvector vWorldDir) { XMStoreFloat3(&m_vTargetDirWorld, vWorldDir); }
	_bool					Is_Affecting() const { return  m_fAimWeight > 0.f;}

private:
	HRESULT					Validate_Profile(const AIM_PROFILE& Profile) const;
	//ChainBone 의 이름으로 Model에서의 인덱스 매핑 
	HRESULT					Mapping_Bones(const AIM_PROFILE& Profile);
	//모델 공간 방향을 좌우,상하 제한 이내로 자름
	_vector					Clamp_DirectionModel(_fvector vDirModel) const;
private:
	CModel*					m_pModel			= { nullptr };
	CTransform*				m_pOwnerTransform	= { nullptr };

	vector<CHAIN_BONE>		m_ChainBones;
	_float					m_fMaxYaw			= {};
	_float					m_fMaxPitchUp		= {};
	_float					m_fMaxPitchDown		= {};
	_float					m_fBlendTime		= {};
	_float					m_fDirFollowRate	= {};

	_float3					m_vModelForward		= { 0.f, 0.f, 1.f };
	_float3					m_vModelRight		= { 1.f, 0.f, 0.f };
	_float3					m_vModelUp			= { 0.f, 1.f, 0.f };

	_bool					m_bActive			= { false };
	//Aiming 정도 0~1
	_float					m_fAimWeight		= { 0.f }; 
	_float3					m_vTargetDirWorld	= { 0.f, 0.f, 1.f };
	//보간 중인 실제 조준 방향
	_float3					m_vCurDirWorld		= { 0.f, 0.f, 1.f };		

public:
	static CAimBoneChain*	Create(CModel* pModel, CTransform* pOwnerTransform, const AIM_PROFILE& Profile);
	virtual void			Free() override;
};

NS_END