#include "Bone.h"


CBone::CBone()
{
}

HRESULT CBone::Initialize(char* szName, _int iParentIndex, _fmatrix transformMatrix)
{
	strcpy_s(m_szName, szName);

    XMStoreFloat4x4(&m_TransformationMatrix, transformMatrix);
	XMStoreFloat4x4(&m_CombinedTransformationMatrix, XMMatrixIdentity());

	m_iParentIndex = iParentIndex;
	return S_OK;
}

void CBone::Update_CombinedTransformMatrices(const vector<CBone*>& Bones, _fmatrix PreTransformMatrix)
{
	if (m_iParentIndex == -1)
	{
		XMStoreFloat4x4(&m_CombinedTransformationMatrix, XMLoadFloat4x4(&m_TransformationMatrix)* PreTransformMatrix);
	}
	else
	{
		XMStoreFloat4x4(&m_CombinedTransformationMatrix,
			XMLoadFloat4x4(&m_TransformationMatrix) * XMLoadFloat4x4(&Bones[m_iParentIndex]->m_CombinedTransformationMatrix));
	}
}


CBone* CBone::Create(char* szName, _int iParentIndex, _fmatrix transformMatrix)
{
	CBone* pInstance = new CBone();

	if (FAILED(pInstance->Initialize(szName, iParentIndex, transformMatrix)))
	{
		MSG_BOX("Failed to Created : CBone");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CBone* CBone::Clone()
{
	return new CBone(*this);
}

void CBone::Free()
{
	__super::Free();


}
