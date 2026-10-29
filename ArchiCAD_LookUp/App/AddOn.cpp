#include "APIEnvir.h"
#include "ACAPinc.h"
#include "Lib/System/ConstantsAndMessages.hpp"
#include "Panels/Panel_Lookup.hpp"

using namespace LookUp;

namespace {

	GSErrCode __ACENV_CALL MenuCommandHandler (const API_MenuParams* menuParams)
	{
		if (menuParams->menuItemRef.menuResID == ConstantsAndMessages::MENU_STRINGS_LOOKUP)
			Panels::Panel_Lookup_HandleMenuItem (static_cast<short> (menuParams->menuItemRef.itemIndex));
		return NoError;
	}

} // namespace

API_AddonType __ACENV_CALL CheckEnvironment (API_EnvirParams* envir)
{
	RSGetIndString (&envir->addOnInfo.name, ConstantsAndMessages::ADDON_NAME_STRINGS, 1, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, ConstantsAndMessages::ADDON_NAME_STRINGS, 2, ACAPI_GetOwnResModule ());
	return APIAddon_Preload;	// Normal add-ons are only fully loaded once one of their commands is placed in the Work Environment
}

GSErrCode __ACENV_CALL RegisterInterface (void)
{
	return ACAPI_MenuItem_RegisterMenu (ConstantsAndMessages::MENU_STRINGS_LOOKUP, 0, MenuCode_UserDef, MenuFlag_Default);
}

GSErrCode __ACENV_CALL Initialize (void)
{
	// Qt (QApplication, plugins) must not be unloaded between two openings of the window
	ACAPI_KeepInMemory (true);
	return ACAPI_MenuItem_InstallMenuHandler (ConstantsAndMessages::MENU_STRINGS_LOOKUP, MenuCommandHandler);
}

GSErrCode __ACENV_CALL FreeData (void)
{
	return NoError;
}