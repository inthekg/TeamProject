#pragma once
#include "Core_Defines.h"

NS_BEGIN(Engine)
class CGameObject;
class CGameInstance;
class CModel;
class CCharacter;
class CDebugCapsule;
NS_END


NS_BEGIN(GameCore)

class CWeapon_Monster;

class CHitBox_Helper final
{
public:
	CHitBox_Helper() = default;
	~CHitBox_Helper() = default;
	CHitBox_Helper(const CHitBox_Helper&) = delete;
	CHitBox_Helper& operator=(const CHitBox_Helper&) = delete;

public:
	void  Set_HitType(HIT_TYPE eType) { m_eHitType = eType; }
	// 등록
	void	Add_BoneCapsule(const _wstring& strId, const string& strTopBone, const string& strBotBone, _float fRadius, _float fHalfHeight, _float fDamage);
	void	Add_BoneSphere(const _wstring& strId, const string& strBone, _float fRadius, _float fDamage);
	void	Add_Weapon(const _wstring& strId, CWeapon_Monster* pWeapon, _float fDamage, const string& strProbeBone = "");

	// 본 인덱스 Resolve + 노티파이 해시맵 + 디버그 캡슐 생성
	HRESULT Build(CModel* pModel, ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

	// 구동
	void	On_StateChanged() { Disarm_All(); }
	_bool	Handle_Notify(_uint64 iHashKey);
	void	Update(CGameInstance* pGI, CCharacter* pOwner, _float fTimeDelta);
	void	Free();

	// Effect_<Id> 노티 이름 해시로 지면 프로브 지점을 찾는다
	_bool	Get_GroundProbe(_uint64 iFxNameHash, CCharacter* pOwner, _float3& OutOrigin, _float& OutRadius);
	_bool	Get_ArmedCenter(const _wstring& strId, CCharacter* pOwner, _float3& OutCenter, _float& OutRadius);


private:
	struct HITBOX
	{
		enum class TYPE { BONE_CAPSULE, BONE_SPHERE, WEAPON };

		_wstring	strId;
		TYPE		eType = { TYPE::BONE_CAPSULE };
		string		strTopBone;
		string		strBotBone;
		_int		iTopBoneIndex = { -1 };
		_int		iBotBoneIndex = { -1 };
		_float		fRadius = { 0.4f };
		_float		fHalfHeight = { 0.5f };
		_float		fDamage = { 20.f };
		CWeapon_Monster* pWeapon = { nullptr };	// WEAPON 전용

		_uint64		iFxNameHash = {};

		_bool		bActive = { false };
		_bool		bPrevValid = { false };
		_float3		vPrevCenter = {};
		unordered_set<CGameObject*>	Targets;
	};
	struct HIT_COMMAND { _bool bOn; _int iIndex; };

	vector<HITBOX>						m_HitBoxes;
	unordered_multimap<_uint64, HIT_COMMAND>	m_HitCommands;
	_uint64								m_iHashHitOffAll = {};
	ID3D11Device* m_pDevice = { nullptr };
	ID3D11DeviceContext* m_pContext = { nullptr };

	HIT_TYPE							m_eHitType = { HIT_TYPE::Normal };

#ifdef _DEBUG
	vector<CDebugCapsule*>				m_DebugCapsules;
#endif

private:
	void	Arm(_int iIndex);
	void	Disarm(_int iIndex);
	void	Disarm_All();
	void	Sweep_One(_int iIndex, CGameInstance* pGI, CCharacter* pOwner, _float fTimeDelta);

	_bool	Get_HitBoxPose(const HITBOX& hb, CCharacter* pOwner, _vector& OutCenter, _vector& OutQuat);

#ifdef _DEBUG
	void	Debug_DrawAll(CGameInstance* pGI, CCharacter* pOwner, _float fTimeDelta);
	void	Rebuild_DebugCapsule(_int iIndex);

public:
	_int	Get_Count() const;
	_float	Get_Radius(_int iIndex) const;
	_float	Get_HalfHeight(_int iIndex) const;
	void	Set_Radius(_int iIndex, _float fRadius);
	void	Set_HalfHeight(_int iIndex, _float fHalfHeight);
#endif // _DEBUG



};

NS_END
