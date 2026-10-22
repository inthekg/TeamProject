#pragma once

#include "MapGameplayObject.h"
#include "CutScene_Data.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CCutScene_Actor final : public CMapGameplayObject
{
public:
	struct CUTSCENE_ACTOR_DESC : public CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC
	{
		CUTSCENE_TRANSFORM_VALUE InitialTransform = {};
	};

private:
	CCutScene_Actor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CCutScene_Actor(const CCutScene_Actor& Prototype);
	virtual ~CCutScene_Actor() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;
	virtual void Update(_float fTimeDelta) override;
	virtual void Late_Update(_float fTimeDelta) override;

public:
	void Queue_Evaluation(const CUTSCENE_TRANSFORM_VALUE& Transform, const vector<CUTSCENE_ANIMATION_SECTION>& Sections, _float fCutSceneTime, _bool bSeek);
	void Set_CutSceneVisible(_bool bVisible) { m_bCutSceneVisible = bVisible; }
	void Set_CutSceneDissolve(_float fAmount, const _float4& vColor) { m_fDissolveAmount = max(0.f, min(fAmount, 1.f)); m_vDissolveColor = vColor; }
	void Reset_CutScene();
	void Set_CutSceneAttachment(CCutScene_Actor* pParentActor, const string& strParentBoneName, const string& strChildBoneName, _float fAttachmentBlendWeight, _bool bInheritParentRotation, _float fParentRotationBlendWeight);
	void Set_CutSceneAttachmentActive(_bool bActive);
	void Clear_CutSceneAttachment();
	_bool Get_CutSceneBoneWorld(const string& strBoneName, _float4x4* pOutWorldMatrix) const;
	const vector<const ANIM_NOTIFY*>& Get_CutSceneNotifies() const { return m_TriggeredNotifies; }

private:
	void Apply_QueuedEvaluation();
	_int Find_LatestStartedSectionIndex(const vector<CUTSCENE_ANIMATION_SECTION>& Sections, _float fCutSceneTime) const;
	_int Find_ActiveSectionIndex(const vector<CUTSCENE_ANIMATION_SECTION>& Sections) const;
	void Begin_AnimationSection(const CUTSCENE_ANIMATION_SECTION& Section);
	void Rebuild_AnimationTimeline(const vector<CUTSCENE_ANIMATION_SECTION>& Sections, _float fCutSceneTime);
	void Advance_AnimationSection(const CUTSCENE_ANIMATION_SECTION& Section, _float fPreviousCutSceneTime, _float fCurrentCutSceneTime, _bool bCollectNotifies);
	void Advance_Animator(_float fTimeDelta, _bool bCollectNotifies);
	void Dispatch_EffectNotifies();
	void Accumulate_RootMotion(const ANIM_ROOT_MOTION_DELTA& RootMotion);
	void Apply_FinalTransform();
	void Apply_AttachmentTransform();
	_bool Get_CutSceneAttachmentLocal(const string& strChildBoneName, _float4x4* pOutLocalMatrix) const;
	void Reset_AnimationState();
	virtual void Configure_RenderCommand(Engine::RenderCommand* pCommand) const override;

private:
	CUTSCENE_TRANSFORM_VALUE m_QueuedTransform = {};
	const vector<CUTSCENE_ANIMATION_SECTION>* m_pQueuedSections = { nullptr };
	_bool m_bEvaluationQueued = { false };
	_bool m_bSeekQueued = { false };
	_float m_fQueuedCutSceneTime = {};

	CUTSCENE_ANIMATION_SECTION m_ActiveSection = {};
	_bool m_bHasActiveSection = { false };
	_uint m_iActiveSectionID = { UINT_MAX };
	_float m_fLastEvaluatedCutSceneTime = {};
	_float4x4 m_RootMotionMatrix = {};
	_float4x4 m_LocalWorldMatrix = {};
	vector<const ANIM_NOTIFY*> m_TriggeredNotifies = {};
	_bool m_bCutSceneVisible = { true };
	_float m_fDissolveAmount = {};
	_float4 m_vDissolveColor = { 0.05f, 0.45f, 1.f, 6.f };
	CCutScene_Actor* m_pParentActor = { nullptr };
	string m_strAttachBoneName = {};
	string m_strChildAttachBoneName = {};
	_bool m_bCutSceneAttachmentActive = { false };
	_float4x4 m_AttachmentBlendStartWorld = {};
	_float m_fAttachmentBlendWeight = { 1.f };
	_bool m_bAttachmentBlendActive = { false };
	_bool m_bInheritParentRotation = { true };
	_float m_fParentRotationBlendWeight = { 1.f };
	_float4 m_vLockedAttachmentRotation = { 0.f, 0.f, 0.f, 1.f };
	// 부착을 시작한 순간의 자식 Bone 행렬을 고정해 이후 자식 Root Animation이 역행렬로 상쇄되지 않게 한다.
	_float4x4 m_ChildAttachmentReferenceLocal = {};
	_bool m_bChildAttachmentReferenceValid = { false };

public:
	static CCutScene_Actor* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END
