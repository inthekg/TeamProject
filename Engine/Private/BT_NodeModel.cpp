#include "BT_NodeModel.h"

NS_BEGIN(Engine)

BT_NODE_MODEL::~BT_NODE_MODEL()
{
    for (auto* pChild : Children)
        Safe_Delete(pChild);

    Children.clear();
}

BT_NODE_MODEL* BT_NODE_MODEL::Add_Child(BT_NodeType eType, const string& strName)
{
    if (false == Can_AddChild())
        return nullptr;

    BT_NODE_MODEL* pChild = new BT_NODE_MODEL();
    pChild->eType = eType;
    pChild->strName = strName;

    Children.push_back(pChild);
    return pChild;
}

_bool BT_NODE_MODEL::Add_Child(BT_NODE_MODEL* pChild)
{
    if (nullptr == pChild || false == Can_AddChild())
        return false;

    Children.push_back(pChild);
    return true;
}

void BT_NODE_MODEL::Remove_Child(BT_NODE_MODEL* pChild)
{
    if (nullptr == pChild)
        return;

    for (auto iter = Children.begin(); iter != Children.end(); ++iter)
    {
        if (*iter == pChild)
        {
            Safe_Delete(*iter);
            Children.erase(iter);
            return;
        }
    }
}

_bool BT_NODE_MODEL::Can_AddChild() const
{
    return ETOI(Children.size()) < BTNode_MaxChildren(eType);
}

std::string BT_NODE_MODEL::Get_Param(const std::string& key, const std::string& def) const
{
    auto it = Params.find(key);
    if (it == Params.end())
        return def;
    return it->second;
}

NS_END

CBT_TreeModel::CBT_TreeModel()
{
}

void CBT_TreeModel::Set_Root(BT_NODE_MODEL* pRoot)
{
    if (m_pRoot == pRoot)
        return;

    Safe_Delete(m_pRoot);
    m_pRoot = pRoot;
}

BT_NODE_MODEL* CBT_TreeModel::New_Root(BT_NodeType eType, const string& strName)
{
    Safe_Delete(m_pRoot);

    m_pRoot = new BT_NODE_MODEL();
    m_pRoot->eType = eType;
    m_pRoot->strName = strName;

    return m_pRoot;
}

void CBT_TreeModel::Clear()
{
    Safe_Delete(m_pRoot);
}

CBT_TreeModel* CBT_TreeModel::Create()
{
    CBT_TreeModel* pInstance = new CBT_TreeModel();

    if (nullptr == pInstance)
    {
        MSG_BOX("Failed to Created : CBT_TreeModel");
        return nullptr;
    }

    return pInstance;
}

void CBT_TreeModel::Free()
{
    Clear();

    __super::Free();
}

