#ifndef LOOKUPPROVIDER_HPP
#define LOOKUPPROVIDER_HPP

#include "LookUpUi.h"

#include <functional>
#include <string>
#include <vector>

namespace LookUp {
namespace Lookup {

	class Sink;

	// Turns Archicad data (ACAPI) into the lazily expanded tree the Qt window shows. Read only.
	// Node 0 = root (Selection, Project); every other node id indexes `expanders`.
	class Provider {
	public:
		using Expander = std::function<void (Sink&)>;

		Provider ();

		static void	ExpandThunk (void* ctx, int nodeId, LuAddFn add, void* addCtx);
		static int	FindGuidThunk (void* ctx, const char* guid);

		int			Register (Expander expander);	// returns the new node id

	private:
		void		Expand (int nodeId, LuAddFn add, void* addCtx);
		int			FindGuid (const std::string& guid);

		std::vector<Expander>	expanders;
	};

	// Where an expander writes its children.
	class Sink {
	public:
		Sink (Provider& provider, LuAddFn add, void* addCtx) : provider (provider), add (add), addCtx (addCtx) {}

		void Leaf (const std::string& name, const std::string& value, const std::string& type = "");
		void Branch (const std::string& name, const std::string& value, const std::string& type, Provider::Expander expander);

	private:
		Provider&	provider;
		LuAddFn		add;
		void*		addCtx;
	};

} // namespace Lookup
} // namespace LookUp

#endif