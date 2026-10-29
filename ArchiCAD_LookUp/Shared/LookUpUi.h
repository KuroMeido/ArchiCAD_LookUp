#ifndef LOOKUPUI_H
#define LOOKUPUI_H

// C interface between the native add-on (ArchiCAD_LookUp.apx, ACAPI) and the Qt window (LookUp_Qt.dll).
// No Archicad or Qt types cross it, all text is UTF-8. Both sides are built from this file.
// The add-on owns the data: the window asks for the children of a node when the user expands it.
//
// Node ids are owned by the host. Id 0 is the root: its children are the entries of the navigation list
// (Selection, Project...). The ids stay valid until LuShow returns.

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*LuAddFn) (void* addCtx, int id, const char* name, const char* value, const char* type, int hasChildren);

struct LuHost {
	void*	ctx;

	// Calls `add` once per child of `nodeId`.
	void	(*expand) (void* ctx, int nodeId, LuAddFn add, void* addCtx);

	// Element GUID typed in the search box: returns the id of a node for it, or -1 when no such element.
	int		(*findGuid) (void* ctx, const char* guid);
};

typedef void (*LuShowFn) (const LuHost* host, void* ownerHwnd, const char* moduleDir);

// Modal: returns when the window is closed. `moduleDir` = folder of LookUp_Qt.dll (Qt plugins are looked up there).
#ifdef LOOKUPUI_EXPORTS
__declspec (dllexport)
#endif
void LuShow (const LuHost* host, void* ownerHwnd, const char* moduleDir);

#ifdef __cplusplus
}
#endif

#endif
