#include <algorithm>
#include "SKEETasks.h"
#include <SKSE/Events.h>
#include <SKSE/API.h>
#include <unordered_set>
#include <iterator>





#include "ActorUpdateManager.h"
#include "ItemDataInterface.h"
#include "TintMaskInterface.h"
#include "ShaderUtilities.h"
#include "OverrideInterface.h"
#include "NifUtils.h"
#include <RE/T/TESObjectARMO.h>
// RE/P/PlayerInfo.h not in CommonLib
#include "Utilities.h"
#include <cstdint>

extern ActorUpdateManager	g_actorUpdateManager;
extern const SKSE::TaskInterface* g_task;
extern TintMaskInterface	g_tintMaskInterface;
extern ItemDataInterface	g_itemDataInterface;
extern StringTable			g_stringTable;

skee_u32 ItemDataInterface::GetVersion()
{
	return kCurrentPluginVersion;
}

RE::BSContainer::ForEachResult ModifiedItemFinder::Visit(RE::InventoryEntryData* pEntryData)
{
	if (!pEntryData)
		return RE::BSContainer::ForEachResult::kContinue;

	RE::BSSimpleList<RE::ExtraDataList*>* pExtendList = pEntryData->extraLists;
	if (!pExtendList)
		return RE::BSContainer::ForEachResult::kContinue;

	for (auto it = pExtendList->begin(); it != pExtendList->end(); ++it)
	{
		RE::ExtraDataList* pExtraDataList = *it;
		if (!pExtraDataList)
			continue;

		bool isMatch = false;
		bool isWorn = false;

		// Check if the item is worn
		bool hasWorn = pExtraDataList->HasType(RE::ExtraDataType::kWorn);
		bool hasWornLeft = pExtraDataList->HasType(RE::ExtraDataType::kWornLeft);
		if (hasWorn || hasWornLeft)
			isWorn = true;

		if ((m_identifier.type & IItemDataInterface::Identifier::kTypeRank) == IItemDataInterface::Identifier::kTypeRank) {
			if (pExtraDataList->HasType(RE::ExtraDataType::kRank)) {
				RE::ExtraRank* extraRank = pExtraDataList->GetByType<RE::ExtraRank>();
				if (extraRank && m_identifier.rankId == extraRank->rank)
					isMatch = true;
			}
		}
		
		if ((m_identifier.type & IItemDataInterface::Identifier::kTypeUID) == IItemDataInterface::Identifier::kTypeUID) {
			if (pExtraDataList->HasType(RE::ExtraDataType::kUniqueID)) {
				RE::ExtraUniqueID* extraUID = pExtraDataList->GetByType<RE::ExtraUniqueID>();
				if (extraUID && m_identifier.uid == extraUID->uniqueID && m_identifier.ownerForm == extraUID->baseID)
					isMatch = true;
			}
		}
		
		if ((m_identifier.type & IItemDataInterface::Identifier::kTypeSlot) == IItemDataInterface::Identifier::kTypeSlot) {
			if (isWorn) {
				RE::TESObjectARMO* armor = pEntryData->object ? pEntryData->object->As<RE::TESObjectARMO>() : nullptr;

				// It's armor and we don't have a slot mask search
				if (armor && m_identifier.slotMask != 0) {
					if ((armor->GetSlotMask().underlying() & m_identifier.slotMask) != 0)
						isMatch = true;
				}
				// Is not an armor, we don't have a slot mask search, and it's equipped
				else if (!armor && m_identifier.slotMask == 0) {
					if ((m_identifier.weaponSlot == IItemDataInterface::Identifier::kHandSlot_Left && hasWornLeft) || (m_identifier.weaponSlot == IItemDataInterface::Identifier::kHandSlot_Right && hasWorn))
						isMatch = true;
				}
			}
		}

		if (isMatch) {
			m_found.pForm = pEntryData->object;
			m_found.pExtraData = pExtraDataList;
			m_found.isWorn = isWorn;
			return RE::BSContainer::ForEachResult::kStop;
		}
	}

	return RE::BSContainer::ForEachResult::kContinue;
}

bool ResolveModifiedIdentifier(RE::TESObjectREFR * reference, IItemDataInterface::Identifier & identifier, ModifiedItem & itemData)
{
	if (!reference)
		return false;

	if (identifier.IsDirect()) {
		itemData.pForm = identifier.form;
		itemData.pExtraData = identifier.extraData;
		if (reference->GetBaseObject() == identifier.form)
			identifier.SetSelf();
		return (itemData.pForm && itemData.pExtraData);
	}

	if (identifier.IsSelf()) {
		itemData.pForm = reference->GetBaseObject();
		itemData.pExtraData = &reference->extraList;
		return (itemData.pForm && itemData.pExtraData);
	}

	RE::ExtraContainerChanges* pContainerChanges = reference->extraList.GetByType<RE::ExtraContainerChanges>();
	if (pContainerChanges && pContainerChanges->changes) {
		ModifiedItemFinder itemFinder(identifier);
		pContainerChanges->changes->VisitInventory(reinterpret_cast<RE::InventoryChanges::IItemChangeVisitor&>(itemFinder));
		itemData = itemFinder.Found();
		return (itemData.pForm && itemData.pExtraData);
	}

	return false;
}

std::shared_ptr<ItemAttributeData> ModifiedItem::GetAttributeData(RE::TESObjectREFR * reference, bool makeUnique, bool allowNewEntry, bool isSelf, std::uint32_t * idOut)
{
	RE::ExtraRank* rank = nullptr;

	if (pExtraData->HasType(RE::ExtraDataType::kRank)) {
		rank = pExtraData->GetByType<RE::ExtraRank>();
	} else if (makeUnique) {
		rank = new RE::ExtraRank(0);
		pExtraData->Add(rank);
	}

	RE::ExtraUniqueID* uniqueId = nullptr;
	if (pExtraData->HasType(RE::ExtraDataType::kUniqueID)) {
		uniqueId = pExtraData->GetByType<RE::ExtraUniqueID>();
	} else if (makeUnique && !isSelf) {
		RE::ExtraContainerChanges* pContainerChanges = reference->extraList.GetByType<RE::ExtraContainerChanges>();
		if (pContainerChanges && pContainerChanges->changes) {
			pContainerChanges->changes->SetUniqueID(pExtraData, nullptr, pForm);
			uniqueId = pExtraData->GetByType<RE::ExtraUniqueID>();
		}
	}
	else if (!uniqueId && isSelf) {
		uniqueId = new RE::ExtraUniqueID(reference->formID, 0);
		pExtraData->Add(uniqueId);
	}

	if (rank) {
		auto data = g_itemDataInterface.GetData(rank->rank);
		if (!data && uniqueId && allowNewEntry) {
			rank->rank = g_itemDataInterface.GetNextRankID();
			data = g_itemDataInterface.CreateData(rank->rank, uniqueId ? uniqueId->uniqueID : 0, uniqueId ? uniqueId->baseID : 0, pForm->formID);
			g_itemDataInterface.UseRankID();
		}
		if (idOut) {
			*idOut = rank->rank;
		}
		return data;
	}

	return NULL;
}

std::shared_ptr<ItemAttributeData> ItemDataInterface::GetExistingData(RE::TESObjectREFR * reference, IItemDataInterface::Identifier & identifier)
{
	ModifiedItem foundData;
	if (ResolveModifiedIdentifier(reference, identifier, foundData)) {
		return foundData.GetAttributeData(reference, false, false, identifier.IsSelf());
	}

	return NULL;
}

NIOVTaskUpdateItemDye::NIOVTaskUpdateItemDye(RE::Actor * actor, IItemDataInterface::Identifier & identifier, std::uint32_t flags, bool forced, LayerFunctor layerFunctor)
{
	m_formId = actor->formID;
	m_identifier = identifier;
	m_flags = flags;
	m_forced = forced;
	m_layerFunctor = layerFunctor;
}

void NIOVTaskUpdateItemDye::Run()
{
	RE::TESForm * form = RE::TESForm::LookupByID(m_formId);
	RE::Actor * actor = form ? form->As<RE::Actor>() : nullptr;
	if (actor) {
		ModifiedItem foundData;
		if (ResolveModifiedIdentifier(actor, m_identifier, foundData)) {
			auto data = foundData.GetAttributeData(actor, false, true, m_identifier.IsSelf());
			if (!data && !m_forced) {
				SKSE::log::debug("{} - Failed to acquire item attribute data", __FUNCTION__);
				return;
			}

			// Don't bother with visual update if not wearing it
			if (!foundData.isWorn)
				return;

			RE::TESObjectARMO * armor = foundData.pForm ? foundData.pForm->As<RE::TESObjectARMO>() : nullptr;
			if (armor) {
				for (std::uint32_t i = 0; i < armor->armorAddons.size(); i++)
				{
					RE::TESObjectARMA* arma = armor->armorAddons[i];
					if (arma) {
						VisitArmorAddon(actor, armor, arma, [&](bool isFirstPerson, RE::NiAVObject * rootNode, RE::NiAVObject * parent)
						{
							g_tintMaskInterface.ApplyMasks(actor, isFirstPerson, armor, arma, parent, m_flags, data, m_layerFunctor);
						});
					}
				}
			}
		}
	}
}

skee_u32 ItemDataInterface::GetItemUniqueID(RE::TESObjectREFR * reference, IItemDataInterface::Identifier & identifier, bool makeUnique)
{
	ModifiedItem foundData;
	if (ResolveModifiedIdentifier(reference, identifier, foundData)) {
		std::uint32_t id = 0;
		auto data = foundData.GetAttributeData(reference, makeUnique, true, identifier.IsSelf(), &id);
		if (data) {
			return id;
		}
	}

	return 0;
}

void ItemDataInterface::SetItemTextureLayerColor(skee_u32 uniqueID, skee_i32 textureIndex, skee_i32 layerIndex, skee_u32 color)
{
	auto data = GetData(uniqueID);
	if (data) {
		data->SetLayerColor(textureIndex, layerIndex, color);
	}
}

void ItemDataInterface::Impl_SetItemTextureLayerBlendMode(std::uint32_t uniqueID, std::int32_t textureIndex, std::int32_t layerIndex, SKEEFixedString blendMode)
{
	auto data = GetData(uniqueID);
	if (data) {
		data->SetLayerBlendMode(textureIndex, layerIndex, blendMode);
	}
}

void ItemDataInterface::SetItemTextureLayerType(skee_u32 uniqueID, skee_i32 textureIndex, skee_i32 layerIndex, skee_u32 type)
{
	auto data = GetData(uniqueID);
	if (data) {
		data->SetLayerType(textureIndex, layerIndex, type);
	}
}

void ItemDataInterface::Impl_SetItemTextureLayerTexture(std::uint32_t uniqueID, std::int32_t textureIndex, std::int32_t layerIndex, SKEEFixedString texture)
{
	auto data = GetData(uniqueID);
	if (data) {
		data->SetLayerTexture(textureIndex, layerIndex, texture);
	}
}

skee_u32 ItemDataInterface::GetItemTextureLayerColor(skee_u32 uniqueID, skee_i32 textureIndex, skee_i32 layerIndex)
{
	auto data = GetData(uniqueID);
	if (data)
	{
		return data->GetLayerColor(textureIndex, layerIndex);
	}

	return 0;
}

SKEEFixedString ItemDataInterface::Impl_GetItemTextureLayerBlendMode(std::uint32_t uniqueID, std::int32_t textureIndex, std::int32_t layerIndex)
{
	auto data = GetData(uniqueID);
	if (data)
	{
		return data->GetLayerBlendMode(textureIndex, layerIndex);
	}

	return "";
}

skee_u32 ItemDataInterface::GetItemTextureLayerType(skee_u32 uniqueID, skee_i32 textureIndex, skee_i32 layerIndex)
{
	auto data = GetData(uniqueID);
	if (data)
	{
		return data->GetLayerType(textureIndex, layerIndex);
	}

	return -1;
}

bool ItemDataInterface::GetItemTextureLayerBlendMode(skee_u32 uniqueID, skee_i32 textureIndex, skee_i32 layerIndex, IItemDataInterface::StringVisitor& visitor)
{
	auto data = GetData(uniqueID);
	if (data)
	{
		visitor.Visit(data->GetLayerBlendMode(textureIndex, layerIndex).c_str());
		return true;
	}
	return false;
}

bool ItemDataInterface::GetItemTextureLayerTexture(skee_u32 uniqueID, skee_i32 textureIndex, skee_i32 layerIndex, IItemDataInterface::StringVisitor& visitor)
{
	auto data = GetData(uniqueID);
	if (data)
	{
		visitor.Visit(data->GetLayerTexture(textureIndex, layerIndex).c_str());
		return true;
	}
	return false;
}

SKEEFixedString ItemDataInterface::Impl_GetItemTextureLayerTexture(std::uint32_t uniqueID, std::int32_t textureIndex, std::int32_t layerIndex)
{
	auto data = GetData(uniqueID);
	if (data)
	{
		return data->GetLayerTexture(textureIndex, layerIndex);
	}

	return "";
}

void ItemDataInterface::ClearItemTextureLayerColor(skee_u32 uniqueID, skee_i32 textureIndex, skee_i32 layerIndex)
{
	auto data = GetData(uniqueID);
	if (data) {
		data->ClearLayerColor(textureIndex, layerIndex);
	}
}

void ItemDataInterface::ClearItemTextureLayerBlendMode(skee_u32 uniqueID, skee_i32 textureIndex, skee_i32 layerIndex)
{
	auto data = GetData(uniqueID);
	if (data) {
		data->ClearLayerBlendMode(textureIndex, layerIndex);
	}
}

void ItemDataInterface::ClearItemTextureLayerType(skee_u32 uniqueID, skee_i32 textureIndex, skee_i32 layerIndex)
{
	auto data = GetData(uniqueID);
	if (data) {
		data->ClearLayerType(textureIndex, layerIndex);
	}
}

void ItemDataInterface::ClearItemTextureLayerTexture(skee_u32 uniqueID, skee_i32 textureIndex, skee_i32 layerIndex)
{
	auto data = GetData(uniqueID);
	if (data) {
		data->ClearLayerTexture(textureIndex, layerIndex);
	}
}

void ItemDataInterface::ClearItemTextureLayer(skee_u32 uniqueID, skee_i32 textureIndex)
{
	auto data = GetData(uniqueID);
	if (data) {
		data->ClearLayer(textureIndex);
	}
}

std::shared_ptr<ItemAttributeData> ItemDataInterface::GetData(std::uint32_t rankId)
{
	std::lock_guard<std::recursive_mutex> lock(m_lock);
	if (rankId == kInvalidRank)
		return nullptr;

	auto it = std::find_if(m_data.begin(), m_data.end(), [&](ItemAttribute& item)
	{
		if (item.rank == rankId)
			return true;
		return false;
	});

	if (it != m_data.end()) {
		return (*it).data;
	}

	return nullptr;
}

RE::TESForm * ItemDataInterface::GetFormFromUniqueID(skee_u32 uniqueID)
{
	std::lock_guard<std::recursive_mutex> lock(m_lock);
	if (uniqueID == kInvalidRank)
		return nullptr;

	auto it = std::find_if(m_data.begin(), m_data.end(), [&](ItemAttribute& item)
	{
		if (item.rank == uniqueID)
			return true;
		return false;
	});

	if (it != m_data.end()) {
		return RE::TESForm::LookupByID((*it).formId);
	}

	return nullptr;
}

RE::TESForm * ItemDataInterface::GetOwnerOfUniqueID(skee_u32 uniqueID)
{
	std::lock_guard<std::recursive_mutex> lock(m_lock);
	if (uniqueID == kInvalidRank)
		return NULL;

	auto it = std::find_if(m_data.begin(), m_data.end(), [&](ItemAttribute& item)
	{
		if (item.rank == uniqueID)
			return true;
		return false;
	});

	if (it != m_data.end()) {
		return RE::TESForm::LookupByID((*it).ownerForm);
	}

	return nullptr;
}

bool ItemDataInterface::HasItemData(skee_u32 uniqueID, const char* key)
{
	auto data = GetData(uniqueID);
	if (data)
	{
		return data->HasData(key);
	}
	return false;
}

bool ItemDataInterface::GetItemData(skee_u32 uniqueID, const char* key, IItemDataInterface::StringVisitor& visitor)
{
	auto data = GetData(uniqueID);
	if (data)
	{
		visitor.Visit(data->GetData(key).c_str());
		return true;
	}
	return false;
}

SKEEFixedString ItemDataInterface::Impl_GetItemData(std::uint32_t uniqueID, SKEEFixedString key)
{
	auto data = GetData(uniqueID);
	if (data)
	{
		return data->GetData(key);
	}
	return "";
}

void ItemDataInterface::Impl_SetItemData(std::uint32_t uniqueID, SKEEFixedString key, SKEEFixedString value)
{
	auto data = GetData(uniqueID);
	if (data)
	{
		data->SetData(key, value);
	}
}

void ItemDataInterface::Impl_ClearItemData(std::uint32_t uniqueID, SKEEFixedString key)
{
	auto data = GetData(uniqueID);
	if (data)
	{
		data->ClearData(key);
	}
}

std::shared_ptr<ItemAttributeData> ItemDataInterface::CreateData(std::uint32_t rankId, std::uint16_t uid, std::uint32_t ownerId, std::uint32_t formId)
{
	EraseByRank(rankId);
	
	std::shared_ptr<ItemAttributeData> data = std::make_shared<ItemAttributeData>();
	Lock();
	m_data.push_back({ rankId, uid, ownerId, formId, data });
	Release();
	return data;
}

bool ItemDataInterface::UpdateUIDByRank(std::uint32_t rankId, std::uint16_t uid, std::uint32_t formId)
{
	std::lock_guard<std::recursive_mutex> lock(m_lock);

	auto it = std::find_if(m_data.begin(), m_data.end(), [&](ItemAttribute& item)
	{
		if (item.rank == rankId) {
			item.uid = uid;
			item.ownerForm = formId;
			return true;
		}

		return false;
	});

	return it != m_data.end();
}

bool ItemDataInterface::UpdateUID(std::uint16_t oldId, std::uint32_t oldFormId, std::uint16_t newId, std::uint32_t newFormId)
{
	std::lock_guard<std::recursive_mutex> lock(m_lock);

	auto it = std::find_if(m_data.begin(), m_data.end(), [&](ItemAttribute& item)
	{
		if (item.uid == oldId && item.ownerForm == oldFormId) {
			item.uid = newId;
			item.ownerForm = newFormId;
			return true;
		}

		return false;
	});

	return it != m_data.end();
}

bool ItemDataInterface::EraseByRank(std::uint32_t rankId)
{
	std::lock_guard<std::recursive_mutex> lock(m_lock);

	auto it = std::find_if(m_data.begin(), m_data.end(), [&](ItemAttribute& item)
	{
		if (item.rank == rankId) {
			return true;
		}

		return false;
	});

	if (it != m_data.end()) {
		m_data.erase(it);
		return true;
	}

	return false;
}

bool ItemDataInterface::EraseByUID(std::uint32_t uid, std::uint32_t formId)
{
	std::lock_guard<std::recursive_mutex> lock(m_lock);

	auto it = std::find_if(m_data.begin(), m_data.end(), [&](ItemAttribute& item)
	{
		if (item.uid == uid && item.ownerForm == formId) {
			return true;
		}

		return false;
	});

	if (it != m_data.end()) {
		if (auto* modEventSource = SKSE::GetModCallbackEventSource()) {
			RE::TESForm* form = RE::TESForm::LookupByID((*it).formId);
			SKSE::ModCallbackEvent evn;
			evn.eventName = "NiOverride_Internal_EraseUID";
			evn.strArg = "";
			evn.numArg = (*it).uid;
			evn.sender = form;
			modEventSource->SendEvent(&evn);
		}

		m_data.erase(it);
		return true;
	}

	return false;
}

void ItemDataInterface::UpdateInventoryItemDye(std::uint32_t rankId, RE::TESObjectARMO * armor, RE::NiAVObject * rootNode)
{
	SKEE_AddTask(g_task, new NIOVTaskDeferredMask(RE::PlayerCharacter::GetSingleton(), false, armor, nullptr, rootNode, rankId ? g_itemDataInterface.GetData(rankId) : nullptr));
}

void ItemDataInterface::ForEachItemAttribute(std::function<void(const ItemAttribute&)> functor)
{
	std::lock_guard<std::recursive_mutex> lock(m_lock);
	for (auto& attribute : m_data)
	{
		functor(attribute);
	}
}

void ItemDataInterface::OnAttach(RE::TESObjectREFR * refr, RE::TESObjectARMO * armor, RE::TESObjectARMA * addon, RE::NiAVObject * object, bool isFirstPerson, RE::NiNode * skeleton, RE::NiNode * root)
{
	std::uint32_t armorMask = armor->GetSlotMask().underlying();
	RE::Actor * actor = refr ? refr->As<RE::Actor>() : nullptr;
	if (actor) {
		IItemDataInterface::Identifier identifier;
		identifier.SetSlotMask(armorMask);
		SKEE_AddTask(g_task, new NIOVTaskDeferredMask(refr, isFirstPerson, armor, addon, object, g_itemDataInterface.GetExistingData(actor, identifier)));
	}
}

void ItemDataInterface::Revert()
{
	std::lock_guard<std::recursive_mutex> lock(m_lock);
	std::vector<ItemAttribute> copy = m_data;
	m_data.clear();

	// We need to trigger a dye update after wiping data to reset
	for (auto & itemAttribute : copy)
	{
		RE::TESForm * ownerForm = RE::TESForm::LookupByID(itemAttribute.ownerForm);
		RE::TESForm * itemForm = RE::TESForm::LookupByID(itemAttribute.formId);

		RE::Actor * actor = ownerForm ? ownerForm->As<RE::Actor>() : nullptr;
		RE::TESObjectARMO * armor = itemForm ? itemForm->As<RE::TESObjectARMO>() : nullptr;
		if (actor && armor) {
			IItemDataInterface::Identifier identifier;
			identifier.SetRankID(itemAttribute.rank);
			identifier.SetUniqueID(itemAttribute.uid, itemAttribute.ownerForm);
			identifier.SetSlotMask(armor->GetSlotMask().underlying());
			m_loadQueue.push_back(new NIOVTaskUpdateItemDye(actor, identifier, TintMaskInterface::kUpdate_All, true));
		}
	}
	m_nextRank = 1;
}

void DyeMap::Revert()
{
	std::lock_guard<std::recursive_mutex> lock(m_lock);
	m_data.clear();
}

bool DyeMap::IsValidDye(RE::TESForm * form)
{
	std::lock_guard<std::recursive_mutex> lock(m_lock);

	auto it = m_data.find(form->formID);
	if (it != m_data.end()) {
		return true;
	}

	return false;
}

std::uint32_t DyeMap::GetDyeColor(RE::TESForm * form)
{
	std::lock_guard<std::recursive_mutex> lock(m_lock);

	auto it = m_data.find(form->formID);
	if (it != m_data.end()) {
		return it->second;
	}

	return 0;
}

void DyeMap::RegisterDyeForm(RE::TESForm * form, std::uint32_t color)
{
	std::lock_guard<std::recursive_mutex> lock(m_lock);
	m_data[form->formID] = color;
}

void DyeMap::UnregisterDyeForm(RE::TESForm * form)
{
	std::lock_guard<std::recursive_mutex> lock(m_lock);
	auto it = m_data.find(form->formID);
	if (it != m_data.end())
		m_data.erase(it);
}

void ItemAttributeData::TintData::Save(SKSE::SerializationInterface * intfc, std::uint32_t kVersion)
{
	struct DataMap
	{
		std::uint32_t* color = nullptr;
		StringTableItem* texture = nullptr;
		StringTableItem* blendMode = nullptr;
		std::uint8_t* type = nullptr;
	};

	std::unordered_map<std::int32_t, DataMap> layerMap;
	for (auto & element : m_colorMap)
	{
		layerMap[element.first].color = &element.second;
	}
	for (auto & element : m_textureMap)
	{
		layerMap[element.first].texture = &element.second;
	}
	for (auto & element : m_blendMap)
	{
		layerMap[element.first].blendMode = &element.second;
	}
	for (auto & element : m_typeMap)
	{
		layerMap[element.first].type = &element.second;
	}

	std::uint32_t numTints = layerMap.size();
	intfc->WriteRecordData(&numTints, sizeof(numTints));

	for (auto tint : layerMap)
	{
		std::int32_t maskIndex = tint.first;
		auto & layerData = tint.second;

		intfc->WriteRecordData(&maskIndex, sizeof(maskIndex));
		std::uint8_t overrideFlags = 0;
		if (layerData.color) overrideFlags |= OverrideFlags::kColor;
		if (layerData.texture) overrideFlags |= OverrideFlags::kTextureMap;
		if (layerData.blendMode) overrideFlags |= OverrideFlags::kBlendMap;
		if (layerData.type) overrideFlags |= OverrideFlags::kTypeMap;

		intfc->WriteRecordData(&overrideFlags, sizeof(overrideFlags));

		if (layerData.color) {
			intfc->WriteRecordData(layerData.color, sizeof(*layerData.color));
		}
		if (layerData.texture) {
			g_stringTable.WriteString(intfc, *layerData.texture);
		}
		if (layerData.blendMode) {
			intfc->WriteRecordData(layerData.blendMode, sizeof(*layerData.blendMode));
		}
		if (layerData.type) {
			intfc->WriteRecordData(layerData.type, sizeof(*layerData.type));
		}
	}
}

bool ItemAttributeData::TintData::Load(SKSE::SerializationInterface * intfc, std::uint32_t kVersion, const StringIdMap & stringTable)
{
	bool error = false;

	std::uint32_t tintCount;
	if (!intfc->ReadRecordData(&tintCount, sizeof(tintCount)))
	{
		SKSE::log::error("{} - Error loading tint count", __FUNCTION__);
		error = true;
		return error;
	}

	for (std::uint32_t i = 0; i < tintCount; i++)
	{
		std::int32_t maskIndex = 0;
		if (!intfc->ReadRecordData(&maskIndex, sizeof(maskIndex)))
		{
			SKSE::log::error("{} - Error loading mask index", __FUNCTION__);
			error = true;
			return error;
		}

		std::uint8_t overrideFlags = 0;
		if (kVersion >= ItemDataInterface::kSerializationVersion2)
		{
			if (!intfc->ReadRecordData(&overrideFlags, sizeof(overrideFlags)))
			{
				SKSE::log::error("{} - Error loading override flags", __FUNCTION__);
				error = true;
				return error;
			}
		}

		if (kVersion == ItemDataInterface::kSerializationVersion1)
		{
			std::uint32_t maskColor = 0;
			if (!intfc->ReadRecordData(&maskColor, sizeof(maskColor)))
			{
				SKSE::log::error("{} - Error loading mask color", __FUNCTION__);
				error = true;
				return error;
			}
		}

		if (kVersion >= ItemDataInterface::kSerializationVersion2)
		{
			if (overrideFlags & OverrideFlags::kColor)
			{
				std::uint32_t maskColor = 0;
				if (!intfc->ReadRecordData(&maskColor, sizeof(maskColor)))
				{
					SKSE::log::error("{} - Error loading mask color", __FUNCTION__);
					error = true;
					return error;
				}

				m_colorMap[maskIndex] = maskColor;
			}
			if (overrideFlags & OverrideFlags::kTextureMap)
			{
				m_textureMap[maskIndex] = StringTable::ReadString(intfc, stringTable);
			}
			if (overrideFlags & OverrideFlags::kBlendMap)
			{
				m_blendMap[maskIndex] = StringTable::ReadString(intfc, stringTable);
			}
			if (overrideFlags & OverrideFlags::kTypeMap)
			{
				std::uint8_t textureType = 0;
				if (!intfc->ReadRecordData(&textureType, sizeof(textureType)))
				{
					SKSE::log::error("{} - Error loading blend mode", __FUNCTION__);
					error = true;
					return error;
				}
				m_typeMap[maskIndex] = textureType;
			}
		}
	}

	return error;
}

void ItemAttributeData::Save(SKSE::SerializationInterface * intfc, std::uint32_t kVersion)
{
	utils::scoped_lock<> locker(m_lock);
	std::uint32_t numSubrecords = m_tintData.size() + m_data.size();

	intfc->WriteRecordData(&numSubrecords, sizeof(numSubrecords));

	for (auto& layer : m_tintData)
	{
		if (!intfc->OpenRecord('TINT', kVersion)) {
			SKSE::log::error("{} - Failed to open record", __FUNCTION__);
		}

		std::int32_t layerIndex = layer.first;
		intfc->WriteRecordData(&layerIndex, sizeof(layerIndex));

		layer.second.Save(intfc, kVersion);
	}
	for (auto& kvp : m_data)
	{
		if (!intfc->OpenRecord('IKVP', kVersion)) {
			SKSE::log::error("{} - Failed to open record", __FUNCTION__);
		}
		g_stringTable.WriteString(intfc, kvp.first);
		g_stringTable.WriteString(intfc, kvp.second);
	}
}

bool ItemAttributeData::Load(SKSE::SerializationInterface * intfc, std::uint32_t kVersion, const StringIdMap & stringTable)
{
	std::uint32_t type, length, version;
	bool error = false;

	// Handle count
	std::uint32_t numSubrecords = 0;
	if (!intfc->ReadRecordData(&numSubrecords, sizeof(numSubrecords)))
	{
		SKSE::log::error("{} - Error loading number of attribute sub records", __FUNCTION__);
		error = true;
		return error;
	}

	for (std::uint32_t i = 0; i < numSubrecords; i++)
	{
		if (intfc->GetNextRecordInfo(type, version, length))
		{
			std::uint32_t ownerForm = 0, itemForm = 0;
				std::uint64_t ownerHandle = 0, itemHandle = 0, newOwnerHandle = 0, newItemHandle = 0;
				switch (type)
			{
				case 'TINT':
					{
						std::int32_t layerIndex = 0;
						if (version >= ItemDataInterface::kSerializationVersion2)
						{
							if (!intfc->ReadRecordData(&layerIndex, sizeof(layerIndex)))
							{
								SKSE::log::error("{} - Error loading layer count", __FUNCTION__);
								continue;
							}
						}

						utils::scoped_lock<> locker(m_lock);
						if (m_tintData[layerIndex].Load(intfc, version, stringTable))
						{
							error = true;
							return error;
						}
					}
					break;
				case 'IKVP':
				{
					auto key = StringTable::ReadString(intfc, stringTable);
					auto value = StringTable::ReadString(intfc, stringTable);
					if (key && value)
					{
						utils::scoped_lock<> locker(m_lock);
						m_data.emplace(key, value);
					}
					break;
				}
				default:
					SKSE::log::error("{} - Error loading unexpected chunk type {:08X} ({:.4})", __FUNCTION__, type, std::string(reinterpret_cast<char*>(&type), 4));
					error = true;
					break;
			}
		}
	}

	return error;
}

RE::BSEventNotifyControl ItemDataInterface::ProcessEvent(const RE::TESUniqueIDChangeEvent* evn, RE::BSTEventSource<RE::TESUniqueIDChangeEvent>* dispatcher)
{
	if (evn->oldBaseID != 0) {
		g_itemDataInterface.UpdateUID(evn->oldUniqueID, evn->oldBaseID, evn->newUniqueID, evn->newBaseID);
	}
	if (evn->newBaseID == 0) {
		g_itemDataInterface.EraseByUID(evn->oldUniqueID, evn->oldBaseID);
	}
	return RE::BSEventNotifyControl::kContinue;
}

void ItemDataInterface::Save(SKSE::SerializationInterface * intfc, std::uint32_t kVersion)
{
	if (!intfc->OpenRecord('ITEE', kVersion)) {
		SKSE::log::error("{} - Failed to open record", __FUNCTION__);
	}

	std::uint32_t nextRank = m_nextRank;
	intfc->WriteRecordData(&nextRank, sizeof(nextRank));

	std::uint32_t numItems = m_data.size();
	intfc->WriteRecordData(&numItems, sizeof(numItems));

	for (auto & attribute : m_data) {
		if (!intfc->OpenRecord('IDAT', kVersion)) {
			SKSE::log::error("{} - Failed to open record", __FUNCTION__);
		}

		std::uint32_t rankId = attribute.rank;
		std::uint16_t uid = attribute.uid;

		std::uint32_t ownerFormId = attribute.ownerForm;
		std::uint32_t itemFormId = attribute.formId;
		
		intfc->WriteRecordData(&rankId, sizeof(rankId));
		intfc->WriteRecordData(&uid, sizeof(uid));
		intfc->WriteRecordData(&ownerFormId, sizeof(ownerFormId));
		intfc->WriteRecordData(&itemFormId, sizeof(itemFormId));

		const std::shared_ptr<ItemAttributeData> & data = attribute.data;
		if (data) {
			data->Save(intfc, kVersion);
		}
	}
}

bool ItemDataInterface::Load(SKSE::SerializationInterface * intfc, std::uint32_t kVersion, const StringIdMap & stringTable)
{
	std::uint32_t type, length, version;
	bool error = false;

	// Resolve formIds from any prior queues (if there are entries here its because Revert queued them)
	for (auto & task : m_loadQueue)
	{
		// If the task was queued from a previous save we need to resolve the actor's formId first
		std::uint32_t newItemForm = 0;
		if (!ResolveAnyForm(intfc, task->m_formId, &newItemForm)) {
			SKSE::log::warn("{} - actor {:08X} could not be found, skipping dye update", __FUNCTION__, task->m_formId);
			continue;
		}
		task->m_formId = newItemForm;
	}

	std::uint32_t nextRank;
	if (!intfc->ReadRecordData(&nextRank, sizeof(nextRank)))
	{
		SKSE::log::error("{} - Error loading next rank ID", __FUNCTION__);
		error = true;
		return error;
	}

	m_nextRank = nextRank;

	std::uint32_t numItems;
	if (!intfc->ReadRecordData(&numItems, sizeof(numItems)))
	{
		SKSE::log::error("{} - Error loading number of items", __FUNCTION__);
		error = true;
		return error;
	}

	for (std::uint32_t i = 0; i < numItems; i++)
	{
		if (intfc->GetNextRecordInfo(type, version, length))
		{
			std::uint32_t ownerForm = 0, itemForm = 0;
				std::uint64_t ownerHandle = 0, itemHandle = 0, newOwnerHandle = 0, newItemHandle = 0;
				switch (type)
			{
				case 'IDAT':
				{
					std::uint32_t rankId;
					if (!intfc->ReadRecordData(&rankId, sizeof(rankId)))
					{
						SKSE::log::error("{} - Error loading Rank ID", __FUNCTION__);
						error = true;
						return error;
					}

					std::uint16_t uid;
					if (!intfc->ReadRecordData(&uid, sizeof(uid)))
					{
						SKSE::log::error("{} - Error loading Unique ID", __FUNCTION__);
						error = true;
						return error;
					}
					
					
					if (version >= kSerializationVersion2)
					{
						if (!intfc->ReadRecordData(&ownerForm, sizeof(ownerForm)))
						{
							SKSE::log::error("{} - Error loading owner form", __FUNCTION__);
							error = true;
							return error;
						}
						if (!intfc->ReadRecordData(&itemForm, sizeof(itemForm)))
						{
							SKSE::log::error("{} - Error loading item form", __FUNCTION__);
							error = true;
							return error;
						}

						std::uint32_t newOwnerForm = 0;
						if (!ResolveAnyForm(intfc, ownerForm, &newOwnerForm)) {
							SKSE::log::warn("{} - owner {:08X} could not be found, skipping", __FUNCTION__, ownerForm);
							continue;
						}
						newOwnerHandle = newOwnerForm;

						std::uint32_t newItemForm = 0;
						if (!ResolveAnyForm(intfc, itemForm, &newItemForm)) {
							SKSE::log::warn("{} - item {:08X} could not be found, skipping", __FUNCTION__, itemForm);
							continue;
						}
						newItemHandle = newItemForm;
					}
					else if (version >= kSerializationVersion1)
					{
						if (!intfc->ReadRecordData(&ownerHandle, sizeof(ownerHandle)))
						{
							SKSE::log::error("{} - Error loading owner handle", __FUNCTION__);
							error = true;
							return error;
						}
						if (!intfc->ReadRecordData(&itemHandle, sizeof(itemHandle)))
						{
							SKSE::log::error("{} - Error loading item handle", __FUNCTION__);
							error = true;
							return error;
						}
						if (!ResolveAnyHandle(intfc, ownerHandle, &newOwnerHandle)) {
							SKSE::log::warn("{} - owner handle %016llX could not be found, skipping", __FUNCTION__, ownerHandle);
							continue;
						}
						RE::VMHandle _resolved;
						if (!intfc->ResolveHandle(static_cast<RE::VMHandle>(itemHandle), _resolved)) {
							SKSE::log::warn("{} - item handle %016llX could not be found, skipping", __FUNCTION__, itemHandle);
							continue;
						}
						newItemHandle = static_cast<std::uint64_t>(_resolved);
					

					std::shared_ptr<ItemAttributeData> data = std::make_shared<ItemAttributeData>();
					if (data->Load(intfc, version, stringTable)) {
						SKSE::log::error("{} - Failed to load item data for owner %016llX item %016llX", __FUNCTION__, ownerHandle, itemHandle);
						error = true;
						return error;
					}

					if (rankId != kInvalidRank) {
						std::uint32_t ownerFormId = newOwnerHandle & 0xFFFFFFFF;
						std::uint32_t itemFormId = newItemHandle & 0xFFFFFFFF;

						Lock();
						m_data.push_back({rankId, uid, ownerFormId, itemFormId, data});
						Release();

						RE::TESForm* ownerForm = RE::TESForm::LookupByID(ownerFormId);
						RE::Actor* actor = ownerForm ? ownerForm->As<RE::Actor>() : nullptr;
						RE::TESForm * itemForm = RE::TESForm::LookupByID(itemFormId);
						if (actor)
						{
							IItemDataInterface::Identifier identifier;
							identifier.SetRankID(rankId);
							identifier.SetUniqueID(uid, ownerFormId);

							RE::TESObjectARMO* armor = itemForm ? itemForm->As<RE::TESObjectARMO>() : nullptr;
							if (armor) {
								identifier.SetSlotMask(armor->GetSlotMask().underlying());
							}

							m_loadQueue.push_back(new NIOVTaskUpdateItemDye(actor, identifier, TintMaskInterface::kUpdate_All, false));
						}
					}
				}
				break;
				default:
					SKSE::log::error("Error loading unexpected chunk type {:08X} ({:.4})", type, std::string(reinterpret_cast<char*>(&type), 4));
					error = true;
					break;
			}
		}
	}
	}

	// Sort task list
	std::sort(m_loadQueue.begin(), m_loadQueue.end(), [](NIOVTaskUpdateItemDye * a, NIOVTaskUpdateItemDye * b)
	{
		if (a->GetActor() == b->GetActor())
		{
			return a->GetRankID() < b->GetRankID();
		}
		return a->GetActor() < b->GetActor();
	});

	std::unordered_set<NIOVTaskUpdateItemDye*> uniqueTasks;
	std::unique_copy(m_loadQueue.begin(), m_loadQueue.end(), std::inserter(uniqueTasks, uniqueTasks.end()), [](NIOVTaskUpdateItemDye * a, NIOVTaskUpdateItemDye * b)
	{
		return a->GetActor() == b->GetActor() && a->GetRankID() == b->GetRankID();
	});

	// Push the loads onto the task queue
	for (auto & task : m_loadQueue)
	{
		auto foundTask = uniqueTasks.find(task);
		if (foundTask != uniqueTasks.end())
		{
			g_actorUpdateManager.AddDyeUpdate_Internal(task);
		}
		else
		{
			task->Dispose();
		}
	}

	m_loadQueue.clear();
	return error;
}

void ItemAttributeData::SetLayerColor(std::int32_t textureIndex, std::int32_t layerIndex, std::uint32_t color)
{
	utils::scoped_lock<> locker(m_lock);
	m_tintData[textureIndex].m_colorMap[layerIndex] = color;
}

void ItemAttributeData::SetLayerType(std::int32_t textureIndex, std::int32_t layerIndex, std::uint32_t type)
{
	utils::scoped_lock<> locker(m_lock);
	m_tintData[textureIndex].m_typeMap[layerIndex] = static_cast<std::uint8_t>(type);
}

void ItemAttributeData::SetLayerBlendMode(std::int32_t textureIndex, std::int32_t layerIndex, SKEEFixedString blendMode)
{
	utils::scoped_lock<> locker(m_lock);
	m_tintData[textureIndex].m_blendMap[layerIndex] = g_stringTable.GetString(blendMode);
}

void ItemAttributeData::SetLayerTexture(std::int32_t textureIndex, std::int32_t layerIndex, SKEEFixedString texture)
{
	utils::scoped_lock<> locker(m_lock);
	m_tintData[textureIndex].m_textureMap[layerIndex] = g_stringTable.GetString(texture);
}

std::uint32_t ItemAttributeData::GetLayerColor(std::int32_t textureIndex, std::int32_t layerIndex)
{
	utils::scoped_lock<> locker(m_lock);
	auto layerData = m_tintData.find(textureIndex);
	if (layerData != m_tintData.end())
	{
		auto it = layerData->second.m_colorMap.find(layerIndex);
		if (it != layerData->second.m_colorMap.end()) {
			return it->second;
		}
	}
	return 0;
}

std::uint32_t ItemAttributeData::GetLayerType(std::int32_t textureIndex, std::int32_t layerIndex)
{
	utils::scoped_lock<> locker(m_lock);
	auto layerData = m_tintData.find(textureIndex);
	if (layerData != m_tintData.end())
	{
		auto it = layerData->second.m_typeMap.find(layerIndex);
		if (it != layerData->second.m_typeMap.end()) {
			return it->second;
		}
	}
	return -1;
}

SKEEFixedString ItemAttributeData::GetLayerBlendMode(std::int32_t textureIndex, std::int32_t layerIndex)
{
	utils::scoped_lock<> locker(m_lock);
	auto layerData = m_tintData.find(textureIndex);
	if (layerData != m_tintData.end())
	{
		auto it = layerData->second.m_blendMap.find(layerIndex);
		if (it != layerData->second.m_blendMap.end()) {
			return it->second ? *it->second : SKEEFixedString("");
		}
	}
	return SKEEFixedString("");
}

SKEEFixedString ItemAttributeData::GetLayerTexture(std::int32_t textureIndex, std::int32_t layerIndex)
{
	utils::scoped_lock<> locker(m_lock);
	auto layerData = m_tintData.find(textureIndex);
	if (layerData != m_tintData.end())
	{
		auto it = layerData->second.m_textureMap.find(layerIndex);
		if (it != layerData->second.m_textureMap.end()) {
			return it->second ? *it->second : SKEEFixedString("");
		}
	}
	return SKEEFixedString("");
}

void ItemAttributeData::ClearLayerColor(std::int32_t textureIndex, std::int32_t layerIndex)
{
	utils::scoped_lock<> locker(m_lock);
	auto layerData = m_tintData.find(textureIndex);
	if (layerData != m_tintData.end())
	{
		auto it = layerData->second.m_colorMap.find(layerIndex);
		if (it != layerData->second.m_colorMap.end()) {
			layerData->second.m_colorMap.erase(it);
		}

		// The whole layer is now empty as a result, erase the parent
		if (layerData->second.empty()) {
			m_tintData.erase(layerData);
		}
	}
}

void ItemAttributeData::ClearLayerType(std::int32_t textureIndex, std::int32_t layerIndex)
{
	utils::scoped_lock<> locker(m_lock);
	auto layerData = m_tintData.find(textureIndex);
	if (layerData != m_tintData.end())
	{
		auto it = layerData->second.m_typeMap.find(layerIndex);
		if (it != layerData->second.m_typeMap.end()) {
			layerData->second.m_typeMap.erase(it);
		}

		// The whole layer is now empty as a result, erase the parent
		if (layerData->second.empty()) {
			m_tintData.erase(layerData);
		}
	}
}

void ItemAttributeData::ClearLayerBlendMode(std::int32_t textureIndex, std::int32_t layerIndex)
{
	utils::scoped_lock<> locker(m_lock);
	auto layerData = m_tintData.find(textureIndex);
	if (layerData != m_tintData.end())
	{
		auto it = layerData->second.m_blendMap.find(layerIndex);
		if (it != layerData->second.m_blendMap.end()) {
			layerData->second.m_blendMap.erase(it);
		}

		// The whole layer is now empty as a result, erase the parent
		if (layerData->second.empty()) {
			m_tintData.erase(layerData);
		}
	}
}

void ItemAttributeData::ClearLayerTexture(std::int32_t textureIndex, std::int32_t layerIndex)
{
	utils::scoped_lock<> locker(m_lock);
	auto layerData = m_tintData.find(textureIndex);
	if (layerData != m_tintData.end())
	{
		auto it = layerData->second.m_textureMap.find(layerIndex);
		if (it != layerData->second.m_textureMap.end()) {
			layerData->second.m_textureMap.erase(it);
		}

		// The whole layer is now empty as a result, erase the parent
		if (layerData->second.empty()) {
			m_tintData.erase(layerData);
		}
	}
}

void ItemAttributeData::ClearLayer(std::int32_t textureIndex)
{
	utils::scoped_lock<> locker(m_lock);
	auto layerData = m_tintData.find(textureIndex);
	if (layerData != m_tintData.end()) {
		m_tintData.erase(layerData);
	}
}

void ItemAttributeData::SetData(SKEEFixedString key, SKEEFixedString value)
{
	utils::scoped_lock<> locker(m_lock);
	m_data[g_stringTable.GetString(key)] = g_stringTable.GetString(value);
}

SKEEFixedString ItemAttributeData::GetData(SKEEFixedString key)
{
	utils::scoped_lock<> locker(m_lock);
	auto it = m_data.find(g_stringTable.GetString(key));
	if (it != m_data.end())
	{
		return *it->second;
	}
	return "";
}

bool ItemAttributeData::HasData(SKEEFixedString key)
{
	utils::scoped_lock<> locker(m_lock);
	auto it = m_data.find(g_stringTable.GetString(key));
	if (it != m_data.end())
	{
		return true;
	}
	return false;
}

void ItemAttributeData::ClearData(SKEEFixedString key)
{
	utils::scoped_lock<> locker(m_lock);
	auto it = m_data.find(g_stringTable.GetString(key));
	if (it != m_data.end())
	{
		m_data.erase(it);
	}
}

void ItemAttributeData::ForEachLayer(std::function<bool(std::int32_t, TintData&)> functor)
{
	utils::scoped_lock<> locker(m_lock);
	for (auto it : m_tintData)
	{
		if (functor(it.first, it.second))
		{
			break;
		}
	}
}

bool ItemAttributeData::GetLayer(std::int32_t layerIndex, std::function<void(TintData&)> functor)
{
	utils::scoped_lock<> locker(m_lock);
	auto it = m_tintData.find(layerIndex);
	if (it != m_tintData.end())
	{
		functor(it->second);
		return true;
	}
	return false;
}
