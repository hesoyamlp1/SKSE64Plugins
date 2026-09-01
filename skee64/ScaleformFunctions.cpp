#include <vector>
#include "SKEETasks.h"

#include "ItemDataInterface.h"

#include "RE/G/GFxValue.h"
#include "RE/G/GFxMovieView.h"
#include "RE/I/InventoryChanges.h"

#include "TintMaskInterface.h"
#include "ScaleformFunctions.h"
#include "ScaleformUtils.h"
#include <cstdint>
#include <cassert>

extern const SKSE::TaskInterface* g_task;
extern ItemDataInterface	g_itemDataInterface;
extern TintMaskInterface	g_tintMaskInterface;
extern DyeMap			g_dyeMap;

class DyeableItemCollector : public RE::InventoryChanges::IItemChangeVisitor
{
public:
	typedef std::vector<IItemDataInterface::Identifier> FoundItems;

	DyeableItemCollector() {}

	virtual RE::BSContainer::ForEachResult Visit(RE::InventoryEntryData* pEntryData) override
	{
		if (!pEntryData)
			return RE::BSContainer::ForEachResult::kContinue;

		if (pEntryData->countDelta < 1)
			return RE::BSContainer::ForEachResult::kContinue;

		RE::BSSimpleList<RE::ExtraDataList*>* pExtendList = pEntryData->extraLists;
		if (!pExtendList)
			return RE::BSContainer::ForEachResult::kContinue;

		for (RE::ExtraDataList* pExtraDataList : *pExtendList)
		{
			if (!pExtraDataList)
				continue;

			// Only armor right now
			if (RE::TESObjectARMO* armor = pEntryData->object ? pEntryData->object->As<RE::TESObjectARMO>() : nullptr) {
				IItemDataInterface::Identifier itemData;
				if (RE::ExtraRank* extraRank = static_cast<RE::ExtraRank*>(pExtraDataList->GetByType(RE::ExtraDataType::kRank)))
				{
					itemData.type |= IItemDataInterface::Identifier::kTypeRank;
					itemData.rankId = extraRank->rank;
				}
				if (RE::ExtraUniqueID* extraUID = static_cast<RE::ExtraUniqueID*>(pExtraDataList->GetByType(RE::ExtraDataType::kUniqueID)))
				{
					itemData.type |= IItemDataInterface::Identifier::kTypeUID;
					itemData.uid = extraUID->uniqueID;
					itemData.ownerForm = extraUID->baseID;
				}
				if (pExtraDataList->HasType(RE::ExtraDataType::kWorn) || pExtraDataList->HasType(RE::ExtraDataType::kWornLeft))
				{
					itemData.type |= IItemDataInterface::Identifier::kTypeSlot;
					itemData.slotMask = armor->GetSlotMask().underlying();
				}

				if (itemData.type != IItemDataInterface::Identifier::kTypeNone && g_tintMaskInterface.IsDyeable(armor)) {
					itemData.form = pEntryData->object;
					itemData.extraData = pExtraDataList;
					m_found.push_back(itemData);
				}
			}
		}

		return RE::BSContainer::ForEachResult::kContinue;
	}

	FoundItems& Found()
	{
		return m_found;
	}
private:
	FoundItems	m_found;
};

class DyeItemCollector : public RE::InventoryChanges::IItemChangeVisitor
{
public:
	struct FoundData
	{
		RE::TESForm * form;
		std::int32_t	count;
		std::vector<std::uint32_t> colors;
	};
	typedef std::vector<FoundData> FoundItems;

	DyeItemCollector() {}

	virtual RE::BSContainer::ForEachResult Visit(RE::InventoryEntryData* pEntryData) override
	{
		if (!pEntryData)
			return RE::BSContainer::ForEachResult::kContinue;

		if (pEntryData->countDelta < 1)
			return RE::BSContainer::ForEachResult::kContinue;

		if (pEntryData->object->Is(RE::FormType::AlchemyItem)) {
			RE::AlchemyItem * potion = pEntryData->object ? pEntryData->object->As<RE::AlchemyItem>() : nullptr;
			if (potion) {
				FoundData found;
				found.form = nullptr;
				found.count = 0;
				for (RE::Effect* effect : potion->effects) {
					if (effect && g_dyeMap.IsValidDye(effect->baseEffect)) {
						found.form = pEntryData->object;
						found.count = pEntryData->countDelta;
						found.colors.push_back(g_dyeMap.GetDyeColor(effect->baseEffect));
					}
				}

				if (g_dyeMap.IsValidDye(potion)) {
					found.form = potion;
					found.count = pEntryData->countDelta;
					found.colors.clear();
					found.colors.push_back(g_dyeMap.GetDyeColor(potion));
				}

				if (found.form)
					m_found.push_back(found);
			}
		} else if (g_dyeMap.IsValidDye(pEntryData->object)) {
			FoundData found;
			found.form = pEntryData->object;
			found.count = pEntryData->countDelta;
			found.colors.push_back(g_dyeMap.GetDyeColor(found.form));
			m_found.push_back(found);
		}
		return RE::BSContainer::ForEachResult::kContinue;
	}

	FoundItems& Found()
	{
		return m_found;
	}
private:
	FoundItems	m_found;
};

void SKSEScaleform_GetDyeableItems::Call(RE::GFxFunctionHandler::Params& a_params)
{
	using namespace ScaleformUtils;

	assert(a_params.argCount >= 1);
	assert(a_params.args[0].GetType() == RE::GFxValue::ValueType::kNumber);

	std::uint32_t		formidArg = 0;
	RE::TESForm		* formArg = nullptr;

	if (a_params.argCount >= 1) {
		formidArg = (std::uint32_t)a_params.args[0].GetNumber();
		if (formidArg > 0)
			formArg = RE::TESForm::LookupByID(formidArg);
	}

	RE::Actor * actor = formArg ? formArg->As<RE::Actor>() : nullptr;
	if (!actor) {
		SKSE::log::info("{} - Invalid form type ({:X})", __FUNCTION__, formidArg);
		return;
	}
	
	RE::ExtraContainerChanges * extraContainer = actor->extraList.GetByType<RE::ExtraContainerChanges>();
	if (extraContainer) {
		DyeableItemCollector::FoundItems foundData;
		if (extraContainer->changes) {
			DyeableItemCollector dyeFinder;
			extraContainer->changes->VisitInventory(reinterpret_cast<RE::InventoryChanges::IItemChangeVisitor&>(dyeFinder));
			foundData = dyeFinder.Found();

			if (!foundData.empty()) {
				a_params.movie->CreateArray(a_params.retVal);

				for (auto & item : foundData) {
					RE::GFxValue itm{};
					a_params.movie->CreateObject(&itm);
					RegisterNumber(&itm, "type", item.type);
					RegisterNumber(&itm, "uid", item.uid);
					RegisterNumber(&itm, "owner", item.ownerForm);
					RegisterNumber(&itm, "rankId", item.rankId);
					RegisterNumber(&itm, "slotMask", item.slotMask);
					RegisterNumber(&itm, "weaponSlot", item.weaponSlot);

					const char * itemName = nullptr;
					if (item.form && item.extraData) {
						itemName = item.extraData->GetDisplayName(static_cast<RE::TESBoundObject*>(item.form));
						if (!itemName) {
							RE::TESFullName* pFullName = item.form ? item.form->As<RE::TESFullName>() : nullptr;
							if (pFullName)
								itemName = pFullName->fullName.c_str();
						}

						if (itemName)
							RegisterString(&itm, a_params.movie, "name", itemName);
					}

					std::shared_ptr<ItemAttributeData> itemData;
					if ((item.type & IItemDataInterface::Identifier::kTypeRank) == IItemDataInterface::Identifier::kTypeRank)
						itemData = g_itemDataInterface.GetData(item.rankId);

					// This is an approx color lookup, its possible multiple shapes may have differing color templates but use the same override
					// we can only show one color so last shape wins
					if (item.form && item.form->IsArmor())
					{
						std::map<std::int32_t, std::uint32_t> colorMap;
						g_tintMaskInterface.GetTemplateColorMap(actor, static_cast<RE::TESObjectARMO*>(item.form), colorMap);

						RE::GFxValue baseArray{};
						a_params.movie->CreateArray(&baseArray);
						for (std::uint32_t i = 0; i < 15; i++) {
							RE::GFxValue colorValue;
							colorValue.SetNumber(colorMap[i]);
							baseArray.PushBack(colorValue);
						}

						itm.SetMember("base", baseArray);
					}

					RE::GFxValue colorArray{};
					a_params.movie->CreateArray(&colorArray);
					for (std::uint32_t i = 0; i < 15; i++) {
						std::uint32_t color = 0;
						if (itemData) {
							itemData->GetLayer(0, [&](auto layerData)
							{
								const auto& it = layerData.m_colorMap.find(i);
								if (it != layerData.m_colorMap.end())
									color = it->second;
							});
						}

						RE::GFxValue colorValue{};
						colorValue.SetNumber(color);
						colorArray.PushBack(colorValue);
					}

					itm.SetMember("colors", colorArray);
					a_params.retVal->PushBack(itm);
				}
			}
		}
	}
}

void SKSEScaleform_GetDyeItems::Call(RE::GFxFunctionHandler::Params& a_params)
{
	using namespace ScaleformUtils;

	assert(a_params.argCount >= 1);
	assert(a_params.args[0].GetType() == RE::GFxValue::ValueType::kNumber);

	std::uint32_t		formidArg = 0;
	RE::TESForm		* formArg = nullptr;

	if (a_params.argCount >= 1) {
		formidArg = (std::uint32_t)a_params.args[0].GetNumber();
		if (formidArg > 0)
			formArg = RE::TESForm::LookupByID(formidArg);
	}

	RE::Actor * reference = formArg ? formArg->As<RE::Actor>() : nullptr;
	if (!reference) {
		SKSE::log::info("{} - Invalid form type ({:X})", __FUNCTION__, formidArg);
		return;
	}

	RE::ExtraContainerChanges * extraContainer = reference->extraList.GetByType<RE::ExtraContainerChanges>();
	if (extraContainer) {
		DyeItemCollector::FoundItems foundData;
		if (extraContainer->changes) {
			DyeItemCollector dyeFinder;
			extraContainer->changes->VisitInventory(reinterpret_cast<RE::InventoryChanges::IItemChangeVisitor&>(dyeFinder));
			foundData = dyeFinder.Found();

			if (!foundData.empty()) {
				a_params.movie->CreateArray(a_params.retVal);

				for (auto & item : foundData) {
					RE::GFxValue itm{};
					a_params.movie->CreateObject(&itm);
					RegisterNumber(&itm, "formId", item.form ? item.form->formID : 0);
					RegisterNumber(&itm, "count", item.count);

					RE::GFxValue colorArray{};
					a_params.movie->CreateArray(&colorArray);
					for (auto color : item.colors) {
						RE::GFxValue itemColor{};
						itemColor.SetNumber(color);
						colorArray.PushBack(itemColor);
					}
					itm.SetMember("colors", colorArray);

					const char * itemName = nullptr;
					if (item.form) {
						RE::TESFullName* pFullName = item.form ? item.form->As<RE::TESFullName>() : nullptr;
						if (pFullName)
							RegisterString(&itm, a_params.movie, "name", pFullName->fullName.c_str());
					}

					a_params.retVal->PushBack(itm);
				}
			}
		}
	}
}

void SKSEScaleform_SetItemDyeColor::Call(RE::GFxFunctionHandler::Params& a_params)
{
	assert(a_params.argCount >= 2);
	assert(a_params.args[0].GetType() == RE::GFxValue::ValueType::kNumber);
	assert(a_params.args[1].GetType() == RE::GFxValue::ValueType::kObject);
	assert(a_params.args[2].GetType() == RE::GFxValue::ValueType::kNumber);

	std::uint32_t		formidArg = 0;
	RE::TESForm		* formArg = nullptr;
	std::uint32_t		maskIndex = a_params.args[2].GetNumber();
	std::uint32_t		color = 0;
	bool		clear = false;

	if (a_params.argCount >= 3) {
		if (a_params.args[3].GetType() == RE::GFxValue::ValueType::kUndefined || a_params.args[3].GetType() == RE::GFxValue::ValueType::kNull)
			clear = true;
		else
			color = a_params.args[3].GetNumber();
	} else {
		clear = true;
	}

	if (a_params.argCount >= 1) {
		formidArg = (std::uint32_t)a_params.args[0].GetNumber();
		if (formidArg > 0)
			formArg = RE::TESForm::LookupByID(formidArg);
	}

	RE::Actor * actor = formArg ? formArg->As<RE::Actor>() : nullptr;
	if (!actor) {
		SKSE::log::info("{} - Invalid form type ({:X})", __FUNCTION__, formidArg);
		return;
	}

	IItemDataInterface::Identifier identifier;
	RE::GFxValue param[6] = {};

	if (a_params.args[1].HasMember("type")) {
		a_params.args[1].GetMember("type", &param[0]);
		identifier.type = param[0].GetNumber();
	}
	if (a_params.args[1].HasMember("uid")) {
		a_params.args[1].GetMember("uid", &param[1]);
		identifier.uid = param[1].GetNumber();
	}
	if (a_params.args[1].HasMember("owner")) {
		a_params.args[1].GetMember("owner", &param[2]);
		identifier.ownerForm = param[2].GetNumber();
	}
	if (a_params.args[1].HasMember("rankId")) {
		a_params.args[1].GetMember("rankId", &param[3]);
		identifier.rankId = param[3].GetNumber();
	}
	if (a_params.args[1].HasMember("slotMask")) {
		a_params.args[1].GetMember("slotMask", &param[4]);
		identifier.slotMask = param[4].GetNumber();
	}
	if (a_params.args[1].HasMember("weaponSlot")) {
		a_params.args[1].GetMember("weaponSlot", &param[5]);
		identifier.weaponSlot = param[5].GetNumber();
	}
	std::map<std::int32_t, std::uint32_t> slotTextureIndexMap;
	std::uint32_t uniqueId = g_itemDataInterface.GetItemUniqueID(actor, identifier, true);
	RE::TESForm* dyedItem = g_itemDataInterface.GetFormFromUniqueID(uniqueId);
	if (dyedItem && dyedItem->IsArmor())
	{
		g_tintMaskInterface.GetSlotTextureIndexMap(actor, static_cast<RE::TESObjectARMO*>(dyedItem), slotTextureIndexMap);
	}

	if (clear)
		g_itemDataInterface.ClearItemTextureLayerColor(uniqueId, slotTextureIndexMap[maskIndex], maskIndex);
	else
		g_itemDataInterface.SetItemTextureLayerColor(uniqueId, slotTextureIndexMap[maskIndex], maskIndex, color);

	SKEE_AddTask(g_task, new NIOVTaskUpdateItemDye(actor, identifier, TintMaskInterface::kUpdate_All, false));
}

void SKSEScaleform_SetItemDyeColors::Call(RE::GFxFunctionHandler::Params& a_params)
{
	assert(a_params.argCount >= 2);
	assert(a_params.args[0].GetType() == RE::GFxValue::ValueType::kNumber);
	assert(a_params.args[1].GetType() == RE::GFxValue::ValueType::kObject);
	assert(a_params.args[2].GetType() == RE::GFxValue::ValueType::kArray);

	std::uint32_t		formidArg = 0;
	RE::TESForm		* formArg = nullptr;

	if (a_params.argCount >= 1) {
		formidArg = (std::uint32_t)a_params.args[0].GetNumber();
		if (formidArg > 0)
			formArg = RE::TESForm::LookupByID(formidArg);
	}

	RE::Actor * actor = formArg ? formArg->As<RE::Actor>() : nullptr;
	if (!actor) {
		SKSE::log::info("{} - Invalid form type ({:X})", __FUNCTION__, formidArg);
		return;
	}

	IItemDataInterface::Identifier identifier;
	RE::GFxValue param[6] = {};

	if (a_params.args[1].HasMember("type")) {
		a_params.args[1].GetMember("type", &param[0]);
		identifier.type = param[0].GetNumber();
	}
	if (a_params.args[1].HasMember("uid")) {
		a_params.args[1].GetMember("uid", &param[1]);
		identifier.uid = param[1].GetNumber();
	}
	if (a_params.args[1].HasMember("owner")) {
		a_params.args[1].GetMember("owner", &param[2]);
		identifier.ownerForm = param[2].GetNumber();
	}
	if (a_params.args[1].HasMember("rankId")) {
		a_params.args[1].GetMember("rankId", &param[3]);
		identifier.rankId = param[3].GetNumber();
	}
	if (a_params.args[1].HasMember("slotMask")) {
		a_params.args[1].GetMember("slotMask", &param[4]);
		identifier.slotMask = param[4].GetNumber();
	}
	if (a_params.args[1].HasMember("weaponSlot")) {
		a_params.args[1].GetMember("weaponSlot", &param[5]);
		identifier.weaponSlot = param[5].GetNumber();
	}

	std::map<std::int32_t, std::uint32_t> slotTextureIndexMap;
	
	std::uint32_t uniqueId = g_itemDataInterface.GetItemUniqueID(actor, identifier, true);
	RE::TESForm* dyedItem = g_itemDataInterface.GetFormFromUniqueID(uniqueId);
	if (dyedItem && dyedItem->IsArmor())
	{
		g_tintMaskInterface.GetSlotTextureIndexMap(actor, static_cast<RE::TESObjectARMO*>(dyedItem), slotTextureIndexMap);
	}

	std::uint32_t size = a_params.args[2].GetArraySize();
	for (std::uint32_t i = 0; i < size; ++i)
	{
		RE::GFxValue element{};
		a_params.args[2].GetElement(i, &element);

		if (element.GetType() == RE::GFxValue::ValueType::kUndefined || element.GetType() == RE::GFxValue::ValueType::kNull) {
			g_itemDataInterface.ClearItemTextureLayerColor(uniqueId, slotTextureIndexMap[i], i);
		}
		else {
			std::uint32_t color = element.GetNumber();
			g_itemDataInterface.SetItemTextureLayerColor(uniqueId, slotTextureIndexMap[i], i, color);
		}
	}

	SKEE_AddTask(g_task, new NIOVTaskUpdateItemDye(actor, identifier, TintMaskInterface::kUpdate_All, false));
}
