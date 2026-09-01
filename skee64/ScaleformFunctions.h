#pragma once

#include "RE/G/GFxFunctionHandler.h"

class SKSEScaleform_GetDyeableItems : public RE::GFxFunctionHandler
{
public:
	void Call(RE::GFxFunctionHandler::Params& a_params) override;
};

class SKSEScaleform_GetDyeItems : public RE::GFxFunctionHandler
{
public:
	void Call(RE::GFxFunctionHandler::Params& a_params) override;
};

class SKSEScaleform_SetItemDyeColor : public RE::GFxFunctionHandler
{
public:
	void Call(RE::GFxFunctionHandler::Params& a_params) override;
};

class SKSEScaleform_SetItemDyeColors : public RE::GFxFunctionHandler
{
public:
	void Call(RE::GFxFunctionHandler::Params& a_params) override;
};
