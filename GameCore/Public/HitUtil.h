#pragma once
#include "Core_Defines.h"

NS_BEGIN(Engine)
class CGameInstance;
class CGameObject;
NS_END

NS_BEGIN(GameCore)

/* 사용 설명서 */
// Apply_HitOnce : Once-Per-Target 체크 -> TakeDamage
// 공격자 (pOtherActor) / p(DamageCuaser)  호출자가 지정
// 몬스터 바디 타격  = this , this 
// 무기 트레일 ( owner,  weapon)


class CHitUtil final
{
public :
	static void		Make_CapsulePose(_fvector vTop, _fvector vBot, _vector& OutCenter, _vector& OutQuat);

	/* tHit  값으로 받아 복사본에 스탬프 = 호출자 템플릿*/
	static _bool	Apply_HitOnce(CGameObject* pTarget, HitInfo tHitInfo, _fvector vHitPos,
		_fvector vHitNormal, unordered_set<CGameObject*>& Targets,
		_bool* pOutDamaged = nullptr);

	/* prev->cur 구 스윕 맞으면 Apply_HitOnce 새로 맞췄으면 true*/
	static _bool	Sweep_Sphere(CGameInstance* pGI, _float fRadius,
		_fvector vPrev, _fvector vCur, const HitInfo& tHitInfo,
		unordered_set<CGameObject*>& Targets, FILTER_GROUP eTarget = FILTER_GROUP::PLAYER,
		_bool* pOutDamaged = nullptr);

	/* prev->cur 캡슐 스윕 */
	static _bool	Sweep_Capsule(CGameInstance* pGI, _float fRadius, _float fHalfHeight,
		_fvector vPrev, _fvector vCur, _fvector vQuat, const HitInfo& tHitInfo,
		unordered_set<CGameObject*>& Targets, FILTER_GROUP eTarget = FILTER_GROUP::PLAYER,
		_bool* pOutDamaged = nullptr);

	/* 정지 구 오버랩 범위 내 전부 Apply_HitOnce */
	static void	Overlap_Sphere(CGameInstance* pGI, _float fRadius, _fvector vCenter, const HitInfo& tHitInfo,
		unordered_set<CGameObject*>& Targets, FILTER_GROUP eTarget = FILTER_GROUP::PLAYER,
		_bool* pOutDamaged = nullptr);
								 
};

NS_END