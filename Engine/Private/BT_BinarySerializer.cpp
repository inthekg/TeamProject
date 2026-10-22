#include "BT_BinarySerializer.h"
#include "BT_JsonSerializer.h"  
#include "BT_NodeModel.h"

NS_BEGIN(Engine)

constexpr char     BTB_MAGIC[4] = { 'B', 'T', 'B', '1' };
constexpr uint32_t BTB_VERSION = 1u;

constexpr uint32_t BTB_MAX_STR = 1u << 24;
constexpr uint32_t BTB_MAX_COUNT = 1u << 20;

template <typename T>
void WritePOD(std::ofstream& os, const T& v)
{
	os.write(reinterpret_cast<const char*>(&v), sizeof(T));
}

void WriteString(std::ofstream& os, const string& s)
{
	const uint32_t iLen = static_cast<uint32_t>(s.size());
	WritePOD(os, iLen);
	if (iLen > 0)
		os.write(s.data(), static_cast<std::streamsize>(iLen));
}

template <typename T>
_bool ReadPOD(std::ifstream& is, T& vOut)
{
	is.read(reinterpret_cast<char*>(&vOut), sizeof(T));
	return static_cast<_bool>(is.good());
}

_bool ReadString(std::ifstream& is, string& sOut)
{
	uint32_t iLen = 0;
	if (false == ReadPOD(is, iLen))
		return false;

	if (iLen > BTB_MAX_STR)
		return false;

	sOut.clear();
	if (iLen > 0)
	{
		sOut.resize(iLen);
		is.read(&sOut[0], static_cast<std::streamsize>(iLen));
		if (false == is.good())
			return false;
	}
	return true;
}

void CBT_BinarySerializer::Write(const BT_NODE_MODEL* pNode, std::ofstream& os)
{
	if (nullptr == pNode)
		return;

	WritePOD(os, static_cast<uint32_t>(pNode->eType));
	WriteString(os, pNode->strName);
	WriteString(os, pNode->strFuncKey);

	WritePOD(os, static_cast<int32_t>(pNode->iRepeatCount));
	WritePOD(os, static_cast<float>(pNode->fSeconds));
	WritePOD(os, static_cast<float>(pNode->fInterval));
	WritePOD(os, static_cast<int32_t>(pNode->iSuccessPolicy));
	WritePOD(os, static_cast<int32_t>(pNode->iFailurePolicy));


	const uint32_t iWeightCount = static_cast<uint32_t>(pNode->Weights.size());
	WritePOD(os, iWeightCount);
	for (uint32_t i = 0; i < iWeightCount; ++i)
		WritePOD(os, static_cast<float>(pNode->Weights[i]));

	const uint32_t iParamCount = static_cast<uint32_t>(pNode->Params.size());
	WritePOD(os, iParamCount);
	for (const auto& kv : pNode->Params)
	{
		WriteString(os, kv.first);
		WriteString(os, kv.second);
	}


	const uint32_t iChildCount = static_cast<uint32_t>(pNode->Children.size());
	WritePOD(os, iChildCount);
	for (uint32_t i = 0; i < iChildCount; ++i)
		Write(pNode->Children[i], os);
}

BT_NODE_MODEL* CBT_BinarySerializer::Read(std::ifstream& is)
{
	uint32_t iType = 0;
	if (false == ReadPOD(is, iType))
		return nullptr;

	if (iType == 0
		|| iType >= ETOUI(BT_NodeType::End)
		|| nullptr == BTNode_TypeName(static_cast<BT_NodeType>(iType))
		|| '\0' == BTNode_TypeName(static_cast<BT_NodeType>(iType))[0])
		return nullptr;

	BT_NODE_MODEL* pNode = new BT_NODE_MODEL();
	pNode->eType = static_cast<BT_NodeType>(iType);

	if (false == ReadString(is, pNode->strName)) { delete pNode; return nullptr; }
	if (false == ReadString(is, pNode->strFuncKey)) { delete pNode; return nullptr; }

	int32_t iRepeat = -1;
	float   fSeconds = 0.f;
	float   fInterval = 0.f;
	int32_t iSuccess = 0;
	int32_t iFailure = 0;

	if (false == ReadPOD(is, iRepeat)) { delete pNode; return nullptr; }
	if (false == ReadPOD(is, fSeconds)) { delete pNode; return nullptr; }
	if (false == ReadPOD(is, fInterval)) { delete pNode; return nullptr; }
	if (false == ReadPOD(is, iSuccess)) { delete pNode; return nullptr; }
	if (false == ReadPOD(is, iFailure)) { delete pNode; return nullptr; }

	pNode->iRepeatCount = static_cast<_int>(iRepeat);
	pNode->fSeconds = static_cast<_float>(fSeconds);
	pNode->fInterval = static_cast<_float>(fInterval);

	pNode->iSuccessPolicy = (iSuccess == 1) ? 1 : 0;
	pNode->iFailurePolicy = (iFailure == 1) ? 1 : 0;


	uint32_t iWeightCount = 0;
	if (false == ReadPOD(is, iWeightCount) || iWeightCount > BTB_MAX_COUNT)
	{
		delete pNode;
		return nullptr;
	}
	pNode->Weights.reserve(iWeightCount);
	for (uint32_t i = 0; i < iWeightCount; ++i)
	{
		float fWeight = 0.f;
		if (false == ReadPOD(is, fWeight)) { delete pNode; return nullptr; }
		pNode->Weights.push_back(static_cast<_float>(fWeight));
	}

	uint32_t iParamCount = 0;
	if (false == ReadPOD(is, iParamCount) || iParamCount > BTB_MAX_COUNT)
	{
		delete pNode;
		return nullptr;
	}
	for (uint32_t i = 0; i < iParamCount; ++i)
	{
		string strKey;
		string strValue;
		if (false == ReadString(is, strKey)) { delete pNode; return nullptr; }
		if (false == ReadString(is, strValue)) { delete pNode; return nullptr; }
		pNode->Params[strKey] = strValue;
	}


	uint32_t iChildCount = 0;
	if (false == ReadPOD(is, iChildCount) || iChildCount > BTB_MAX_COUNT)
	{
		delete pNode;
		return nullptr;
	}
	pNode->Children.reserve(iChildCount);
	for (uint32_t i = 0; i < iChildCount; ++i)
	{
		BT_NODE_MODEL* pChild = Read(is);
		if (nullptr == pChild)
		{
			
			delete pNode;
			return nullptr;
		}
		pNode->Children.push_back(pChild);
	}

	return pNode;
}

_bool CBT_BinarySerializer::Save(const BT_NODE_MODEL* pRoot, const string& strPath)
{
	if (nullptr == pRoot)
	{
		MSG_BOX("CBT_BinarySerializer::Save - Null Root");
		return false;
	}

	std::ofstream os(strPath, std::ios::binary | std::ios::trunc);
	if (false == os.is_open())
	{
		MSG_BOX("CBT_BinarySerializer::Save - Open Failed");
		return false;
	}
	
	os.write(BTB_MAGIC, sizeof(BTB_MAGIC));
	WritePOD(os, BTB_VERSION);

	
	Write(pRoot, os);

	os.flush();
	const _bool bOk = static_cast<_bool>(os.good());
	os.close();

	if (false == bOk)
	{
		MSG_BOX("CBT_BinarySerializer::Save - Write Failed");
		return false;
	}
	return true;
}

_bool CBT_BinarySerializer::Load(const string& strPath, BT_NODE_MODEL** ppOutRoot)
{
	if (nullptr == ppOutRoot)
		return false;

	*ppOutRoot = nullptr;

	std::ifstream is(strPath, std::ios::binary);
	if (false == is.is_open())
	{
		MSG_BOX("CBT_BinarySerializer::Load - Open Failed");
		return false;
	}

	// ��� ����: ����.
	char szMagic[4] = { 0, };
	is.read(szMagic, sizeof(szMagic));
	if (false == is.good()
		|| szMagic[0] != BTB_MAGIC[0] || szMagic[1] != BTB_MAGIC[1]
		|| szMagic[2] != BTB_MAGIC[2] || szMagic[3] != BTB_MAGIC[3])
	{
		MSG_BOX("CBT_BinarySerializer::Load - Bad Magic");
		return false;
	}

	// ����.
	uint32_t iVersion = 0;
	if (false == ReadPOD(is, iVersion) || iVersion != BTB_VERSION)
	{
		MSG_BOX("CBT_BinarySerializer::Load - Bad Version");
		return false;
	}

	// ��Ʈ ��� �б�.
	BT_NODE_MODEL* pRoot = Read(is);
	if (nullptr == pRoot)
	{
		MSG_BOX("CBT_BinarySerializer::Load - Read Failed");
		return false;
	}

	*ppOutRoot = pRoot;
	return true;
}

_bool CBT_BinarySerializer::ConvertJsonToBinary(const string& strJsonPath, const string& strBinPath)
{
	BT_NODE_MODEL* pRoot = nullptr;
	if (false == CBT_JsonSerializer::Load(strJsonPath, &pRoot) || nullptr == pRoot)
	{
		MSG_BOX("CBT_BinarySerializer::ConvertJsonToBinary - Json Load Failed");
		return false;
	}

	const _bool bOk = Save(pRoot, strBinPath);

	delete pRoot;
	pRoot = nullptr;

	return bOk;
}

NS_END