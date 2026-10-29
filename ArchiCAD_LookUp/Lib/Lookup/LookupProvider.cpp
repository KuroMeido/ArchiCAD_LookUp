#include "APIEnvir.h"
#include "ACAPinc.h"
#include "Lib/Lookup/LookupProvider.hpp"

#include <cstdio>
#include <map>

namespace LookUp {
namespace Lookup {

namespace {

	std::string Utf8 (const GS::UniString& s)
	{
		return std::string (s.ToCStr (0, MaxUSize, CC_UTF8).Get ());
	}

	std::string Num (double v)
	{
		char buf[64];
		snprintf (buf, sizeof (buf), "%.4f", v);
		return buf;
	}

	std::string Int (long long v)
	{
		return std::to_string (v);
	}

	std::string Bool (bool v)
	{
		return v ? "true" : "false";
	}

	std::string Coord (const API_Coord& c)
	{
		return Num (c.x) + ", " + Num (c.y);
	}

	std::string GuidString (const API_Guid& g)
	{
		return Utf8 (APIGuid2GSGuid (g).ToUniString ());
	}

	std::string TypeName (const API_ElemType& type)
	{
		GS::UniString name;
		if (ACAPI_Element_GetElemTypeName (type, name) == NoError)
			return Utf8 (name);
		return "Unknown";
	}

	std::string LayerName (API_AttributeIndex index)
	{
		API_Attribute attr = {};
		GS::UniString name;
		attr.header.typeID = API_LayerID;
		attr.header.index = index;
		attr.header.uniStringNamePtr = &name;
		if (ACAPI_Attribute_Get (&attr) == NoError)
			return Utf8 (name);
		return "?";
	}

	std::string Err (GSErrCode err)
	{
		return "error " + Int (static_cast<long long> (err));
	}

	// ---- element -------------------------------------------------------------------------------------------

	void AddHeader (Sink& out, const API_Elem_Head& head)
	{
		out.Leaf ("Type", TypeName (head.type));
		out.Leaf ("TypeID", Int (head.type.typeID), "API_ElemTypeID");
		out.Leaf ("VariationID", Int (head.type.variationID));
		out.Leaf ("Guid", GuidString (head.guid), "API_Guid");
		out.Leaf ("Group guid", GuidString (head.groupGuid), "API_Guid");
		out.Leaf ("Hotlink guid", GuidString (head.hotlinkGuid), "API_Guid");
		out.Leaf ("Story (floorInd)", Int (head.floorInd), "short");
		out.Leaf ("Layer", LayerName (head.layer));
		out.Leaf ("Modification stamp", Int (static_cast<long long> (head.modiStamp)), "UInt64");
		out.Leaf ("Has memo", Bool (head.hasMemo), "bool");
	}

	// The geometry of the common element types; the others only have the header.
	void AddGeometry (Sink& out, const API_Element& element)
	{
		const API_ElemTypeID id = element.header.type.typeID;
		if (id == API_WallID) {
			out.Leaf ("Start", Coord (element.wall.begC), "API_Coord");
			out.Leaf ("End", Coord (element.wall.endC), "API_Coord");
			out.Leaf ("Thickness", Num (element.wall.thickness), "m");
			out.Leaf ("Thickness (end)", Num (element.wall.thickness1), "m");
			out.Leaf ("Bottom offset", Num (element.wall.bottomOffset), "m");
			out.Leaf ("Height", Num (element.wall.height), "m");
		} else if (id == API_ColumnID) {
			out.Leaf ("Origin", Coord (element.column.origoPos), "API_Coord");
			out.Leaf ("Bottom offset", Num (element.column.bottomOffset), "m");
			out.Leaf ("Height", Num (element.column.height), "m");
			out.Leaf ("Angle", Num (element.column.axisRotationAngle), "rad");
		} else if (id == API_BeamID) {
			out.Leaf ("Start", Coord (element.beam.begC), "API_Coord");
			out.Leaf ("End", Coord (element.beam.endC), "API_Coord");
			out.Leaf ("Level", Num (element.beam.level), "m");
			out.Leaf ("Offset", Num (element.beam.offset), "m");
		} else if (id == API_SlabID) {
			out.Leaf ("Thickness", Num (element.slab.thickness), "m");
			out.Leaf ("Level", Num (element.slab.level), "m");
		} else if (id == API_ZoneID) {
			out.Leaf ("Zone name", Utf8 (GS::UniString (element.zone.roomName)));
			out.Leaf ("Zone number", Utf8 (GS::UniString (element.zone.roomNoStr)));
			out.Leaf ("Base level", Num (element.zone.roomBaseLev), "m");
			out.Leaf ("Height", Num (element.zone.roomHeight), "m");
		} else if (id == API_ObjectID || id == API_LampID) {
			out.Leaf ("Position", Coord (element.object.pos), "API_Coord");
			out.Leaf ("Angle", Num (element.object.angle), "rad");
			out.Leaf ("Library part index", Int (element.object.libInd));
		}
	}

	void AddProperties (Sink& out, const API_Guid& guid)
	{
		GS::Array<API_PropertyDefinition> definitions;
		GSErrCode err = ACAPI_Element_GetPropertyDefinitions (guid, API_PropertyDefinitionFilter_All, definitions);
		if (err != NoError) {
			out.Leaf ("Definitions", Err (err));
			return;
		}
		GS::Array<API_Property> properties;
		err = ACAPI_Element_GetPropertyValues (guid, definitions, properties);
		if (err != NoError) {
			out.Leaf ("Values", Err (err));
			return;
		}

		// grouped by property group, in the order Archicad returns them
		std::vector<std::string> groupOrder;
		std::map<std::string, std::vector<std::pair<std::string, std::string>>> groups;
		for (UIndex i = 0; i < properties.GetSize () && i < definitions.GetSize (); ++i) {
			API_PropertyGroup group;
			group.guid = definitions[i].groupGuid;
			std::string groupName = "?";
			if (ACAPI_Property_GetPropertyGroup (group) == NoError)
				groupName = Utf8 (group.name);

			GS::UniString text;
			if (properties[i].status != API_Property_HasValue)
				text = properties[i].status == API_Property_NotAvailable ? "(not available)" : "(not evaluated)";
			else if (ACAPI_Property_GetPropertyValueString (properties[i], &text) != NoError)
				text = "(no string form)";

			if (groups.find (groupName) == groups.end ())
				groupOrder.push_back (groupName);
			groups[groupName].emplace_back (Utf8 (definitions[i].name), Utf8 (text));
		}
		for (const std::string& name : groupOrder) {
			const auto items = groups[name];
			out.Branch (name, Int (static_cast<long long> (items.size ())) + " properties", "Group", [items] (Sink& s) {
				for (const auto& item : items)
					s.Leaf (item.first, item.second, "API_Property");
			});
		}
	}

	void AddClassifications (Sink& out, const API_Guid& guid)
	{
		GS::Array<GS::Pair<API_Guid, API_Guid>> pairs;
		const GSErrCode err = ACAPI_Element_GetClassificationItems (guid, pairs);
		if (err != NoError) {
			out.Leaf ("Classification", Err (err));
			return;
		}
		for (const auto& pair : pairs) {
			API_ClassificationSystem system = {};
			system.guid = pair.first;
			const bool hasSystem = ACAPI_Classification_GetClassificationSystem (system) == NoError;
			API_ClassificationItem item = {};
			item.guid = pair.second;
			const bool hasItem = ACAPI_Classification_GetClassificationItem (item) == NoError;
			out.Leaf (hasSystem ? Utf8 (system.name) : "System " + GuidString (pair.first),
					  hasItem ? Utf8 (item.id) + "  " + Utf8 (item.name) : GuidString (pair.second), "API_ClassificationItem");
		}
		if (pairs.IsEmpty ())
			out.Leaf ("(none)", "");
	}

	void ExpandElement (Sink& out, API_Guid guid)
	{
		API_Element element = {};
		element.header.guid = guid;
		const GSErrCode err = ACAPI_Element_Get (&element);
		if (err != NoError) {
			out.Leaf ("ACAPI_Element_Get", Err (err));
			return;
		}
		const API_Elem_Head head = element.header;
		out.Branch ("Header", TypeName (head.type), "API_Elem_Head", [head] (Sink& s) { AddHeader (s, head); });
		out.Branch ("Geometry", "", "", [element] (Sink& s) { AddGeometry (s, element); });
		out.Branch ("Properties", "", "", [guid] (Sink& s) { AddProperties (s, guid); });
		out.Branch ("Classification", "", "", [guid] (Sink& s) { AddClassifications (s, guid); });
	}

	void AddElementNode (Sink& out, const API_Guid& guid)
	{
		API_Elem_Head head = {};
		head.guid = guid;
		const bool ok = ACAPI_Element_GetHeader (&head) == NoError;
		const std::string guidText = GuidString (guid);
		out.Branch (ok ? TypeName (head.type) : "Element", guidText, "Element", [guid] (Sink& s) { ExpandElement (s, guid); });
	}

	// ---- project -------------------------------------------------------------------------------------------

	void AddAttributes (Sink& out, API_AttrTypeID typeID)
	{
		UInt32 count = 0;
		if (ACAPI_Attribute_GetNum (typeID, count) != NoError)
			return;
		for (UInt32 i = 1; i <= count; ++i) {
			API_Attribute attr = {};
			GS::UniString name;
			attr.header.typeID = typeID;
			attr.header.index = ACAPI_CreateAttributeIndex (static_cast<Int32> (i));
			attr.header.uniStringNamePtr = &name;
			if (ACAPI_Attribute_Get (&attr) == NoError)
				out.Leaf (Utf8 (name), "index " + Int (i), "API_Attribute");
		}
	}

	void ExpandProject (Sink& out)
	{
		API_ProjectInfo info = {};
		if (ACAPI_ProjectOperation_Project (&info) == NoError) {
			out.Leaf ("Untitled", Bool (info.untitled), "bool");
			if (info.projectName != nullptr)
				out.Leaf ("Project name", Utf8 (*info.projectName));
			if (info.projectPath != nullptr)
				out.Leaf ("Project path", Utf8 (*info.projectPath));
			out.Leaf ("Teamwork", Bool (info.teamwork), "bool");
		}

		struct Kind { const char* label; API_AttrTypeID id; };
		const Kind kinds[] = {
			{ "Layers", API_LayerID },
			{ "Building materials", API_BuildingMaterialID },
			{ "Composites", API_CompWallID },
			{ "Surfaces", API_MaterialID },
			{ "Fill types", API_FilltypeID },
			{ "Line types", API_LinetypeID },
			{ "Zone categories", API_ZoneCatID },
		};
		for (const Kind& kind : kinds) {
			UInt32 count = 0;
			ACAPI_Attribute_GetNum (kind.id, count);
			const API_AttrTypeID id = kind.id;
			out.Branch (kind.label, Int (count), "Attributes", [id] (Sink& s) { AddAttributes (s, id); });
		}
	}

	void ExpandSelection (Sink& out)
	{
		API_SelectionInfo selInfo = {};
		GS::Array<API_Neig> selNeigs;
		const GSErrCode err = ACAPI_Selection_Get (&selInfo, &selNeigs, false);
		BMKillHandle (reinterpret_cast<GSHandle*> (&selInfo.marquee.coords));
		if (err != NoError) {
			out.Leaf ("ACAPI_Selection_Get", Err (err));
			return;
		}
		for (const API_Neig& neig : selNeigs)
			AddElementNode (out, neig.guid);
		if (selNeigs.IsEmpty ())
			out.Leaf ("Nothing is selected", "select elements in Archicad, then press Refresh");
	}

	size_t SelectionCount ()
	{
		API_SelectionInfo selInfo = {};
		GS::Array<API_Neig> selNeigs;
		ACAPI_Selection_Get (&selInfo, &selNeigs, false);
		BMKillHandle (reinterpret_cast<GSHandle*> (&selInfo.marquee.coords));
		return selNeigs.GetSize ();
	}

} // namespace


void Sink::Leaf (const std::string& name, const std::string& value, const std::string& type)
{
	add (addCtx, -1, name.c_str (), value.c_str (), type.c_str (), 0);
}

void Sink::Branch (const std::string& name, const std::string& value, const std::string& type, Provider::Expander expander)
{
	add (addCtx, provider.Register (std::move (expander)), name.c_str (), value.c_str (), type.c_str (), 1);
}


Provider::Provider ()
{
	expanders.emplace_back ([] (Sink& out) {	// node 0
		out.Branch ("Selection", Int (static_cast<long long> (SelectionCount ())) + " element(s)", "", [] (Sink& s) { ExpandSelection (s); });
		out.Branch ("Project", "", "", [] (Sink& s) { ExpandProject (s); });
	});
}

int Provider::Register (Expander expander)
{
	expanders.push_back (std::move (expander));
	return static_cast<int> (expanders.size ()) - 1;
}

void Provider::Expand (int nodeId, LuAddFn add, void* addCtx)
{
	if (nodeId < 0 || static_cast<size_t> (nodeId) >= expanders.size ())
		return;
	Sink sink (*this, add, addCtx);
	const Expander expander = expanders[static_cast<size_t> (nodeId)];	// copy: Register may reallocate the vector
	expander (sink);
}

int Provider::FindGuid (const std::string& text)
{
	GS::Guid gsGuid;
	if (gsGuid.ConvertFromString (text.c_str ()) != NoError)
		return -1;
	const API_Guid guid = GSGuid2APIGuid (gsGuid);
	API_Elem_Head head = {};
	head.guid = guid;
	if (ACAPI_Element_GetHeader (&head) != NoError)
		return -1;
	return Register ([guid] (Sink& out) { ExpandElement (out, guid); });
}

void Provider::ExpandThunk (void* ctx, int nodeId, LuAddFn add, void* addCtx)
{
	static_cast<Provider*> (ctx)->Expand (nodeId, add, addCtx);
}

int Provider::FindGuidThunk (void* ctx, const char* guid)
{
	return static_cast<Provider*> (ctx)->FindGuid (guid);
}

} // namespace Lookup
} // namespace LookUp
