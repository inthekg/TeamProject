#include "Boss_Raxasia.h"
#include "GameInstance.h"

CBoss_Raxasia::CBoss_Raxasia(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CAIObject(pDevice, pContext)
{
	m_eActorType = ACTOR_TYPE::BOSS;
}

CBoss_Raxasia::CBoss_Raxasia(const CBoss_Raxasia& Prototype)
	: CAIObject(Prototype)
{
	m_strBTPath = Prototype.m_strBTPath;
}

HRESULT CBoss_Raxasia::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CBoss_Raxasia::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	CAIObject::CHARACTER_SETUP_DESC SetupDesc{};
	SetupDesc.strModelPrototypeTag = TEXT("Prototype_Component_Model_Raxasia");
	SetupDesc.strAnimFolderTag = TEXT("Raxasia_P1");
	SetupDesc.strAnimTag = TEXT("Raxasia_P1");
	SetupDesc.pRootBoneName = "Base";
	SetupDesc.bUseCCT = true;
	SetupDesc.fCCTRadius = 1.2f;
	SetupDesc.fCCTHeight = 3.0f;

	if (nullptr != pArg)
		SetupDesc.iModelLevel = static_cast<AIOBJECT_DESC*>(pArg)->iProtoLevel;
	else
		SetupDesc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);

	if (FAILED(Ready_Character(SetupDesc)))
		return E_FAIL;

	return S_OK;
}

void CBoss_Raxasia::Priority_Update(_float fTimeDelta)
{
	__super::Priority_Update(fTimeDelta);
}

void CBoss_Raxasia::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);
}

void CBoss_Raxasia::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);
}

void CBoss_Raxasia::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);
	m_pGameInstance->Add_RenderGroup(RENDERID::FORWARD, this);
}

HRESULT CBoss_Raxasia::Render()
{
	return __super::Render();
}

CBoss_Raxasia* CBoss_Raxasia::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CBoss_Raxasia* pInstance = new CBoss_Raxasia(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CBoss_Raxasia");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CBoss_Raxasia::Clone(void* pArg)
{
	CBoss_Raxasia* pInstance = new CBoss_Raxasia(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBoss_Raxasia");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CBoss_Raxasia::Free()
{
	__super::Free();
}
