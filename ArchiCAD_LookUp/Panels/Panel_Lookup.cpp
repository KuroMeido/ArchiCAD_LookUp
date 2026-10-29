#include "APIEnvir.h"
#include "ACAPinc.h"
#include "Panels/Panel_Lookup.hpp"

#include "Lib/Lookup/LookupProvider.hpp"
#include "Lib/System/ConstantsAndMessages.hpp"
#include "LookUpUi.h"

#include <string>

namespace LookUp {
namespace Panels {

namespace {

	// Folder of this add-on's LookUp_Qt\ subfolder (LookUp_Qt.dll + the Qt runtime).
	std::wstring QtFolder ()
	{
		HMODULE self = nullptr;
		GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
							reinterpret_cast<LPCWSTR> (&QtFolder), &self);
		wchar_t path[MAX_PATH] = {};
		GetModuleFileNameW (self, path, MAX_PATH);
		std::wstring dir (path);
		dir.resize (dir.find_last_of (L'\\'));
		return dir + L"\\LookUp_Qt";
	}

	std::string ToUtf8 (const std::wstring& w)
	{
		const int len = WideCharToMultiByte (CP_UTF8, 0, w.c_str (), static_cast<int> (w.size ()), nullptr, 0, nullptr, nullptr);
		std::string s (static_cast<size_t> (len), '\0');
		WideCharToMultiByte (CP_UTF8, 0, w.c_str (), static_cast<int> (w.size ()), &s[0], len, nullptr, nullptr);
		return s;
	}

	void Open ()
	{
		const std::wstring dir = QtFolder ();
		SetDllDirectoryW (dir.c_str ());	// the Qt DLLs sit next to LookUp_Qt.dll
		HMODULE ui = LoadLibraryExW ((dir + L"\\LookUp_Qt.dll").c_str (), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
		if (ui == nullptr) {
			ACAPI_WriteReport ("Lookup: LookUp_Qt\\LookUp_Qt.dll was not found next to the add-on (build the LookUp_Qt project with Qt 6).", true);
			return;
		}
		LuShowFn show = reinterpret_cast<LuShowFn> (GetProcAddress (ui, "LuShow"));
		if (show == nullptr) {
			ACAPI_WriteReport ("Lookup: LookUp_Qt.dll has no LuShow export.", true);
			return;
		}

		Lookup::Provider provider;
		LuHost host = {};
		host.ctx = &provider;
		host.expand = &Lookup::Provider::ExpandThunk;
		host.findGuid = &Lookup::Provider::FindGuidThunk;

		HWND owner = GetActiveWindow ();
		if (owner == nullptr)
			owner = GetForegroundWindow ();
		show (&host, owner, ToUtf8 (dir).c_str ());
		// LookUp_Qt.dll stays loaded: Qt does not survive being unloaded and reloaded in one process.
		SetDllDirectoryW (nullptr);
	}

} // namespace

	bool Panel_Lookup_HandleMenuItem (short itemIndex)
	{
		if (itemIndex == ConstantsAndMessages::MENU_LOOKUP_OPEN) {
			Open ();
			return true;
		}
		return false;
	}

} // namespace Panels
} // namespace LookUp