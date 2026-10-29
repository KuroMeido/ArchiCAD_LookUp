// Lookup window: navigation list (Selection, Project, GUIDs found) | search box + tree (Name, Value, Type).
// All data comes from the host (LuHost); children are asked for when a node is expanded. No Q_OBJECT (no moc): lambdas only.

#include "LookUpUi.h"
#include "FluentStyle.hpp"

#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <memory>
#include <vector>

#include <windows.h>

namespace {

	constexpr int kIdRole = Qt::UserRole;
	constexpr int kPlaceholderId = -2;
	constexpr int kMaxSearchNodes = 4000;

	struct Row {
		int id;
		QString name, value, type;
		bool hasChildren;
	};

	std::vector<Row> Fetch (const LuHost& host, int nodeId)
	{
		std::vector<Row> rows;
		host.expand (host.ctx, nodeId, [] (void* ctx, int id, const char* name, const char* value, const char* type, int hasChildren) {
			static_cast<std::vector<Row>*> (ctx)->push_back ({ id, QString::fromUtf8 (name), QString::fromUtf8 (value), QString::fromUtf8 (type), hasChildren != 0 });
		}, &rows);
		return rows;
	}

	bool WindowsUsesDarkTheme ()
	{
		QSettings reg (QStringLiteral ("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"), QSettings::NativeFormat);
		return reg.value (QStringLiteral ("AppsUseLightTheme"), 1).toInt () == 0;
	}

	bool IsPlaceholder (QTreeWidgetItem* item)
	{
		return item->data (0, kIdRole).toInt () == kPlaceholderId;
	}

	// Replaces the placeholder of `item` by its real children (once).
	void Load (const LuHost& host, QTreeWidgetItem* item)
	{
		if (item->childCount () != 1 || !IsPlaceholder (item->child (0)))
			return;
		delete item->takeChild (0);
		for (const Row& r : Fetch (host, item->data (0, kIdRole).toInt ())) {
			auto* child = new QTreeWidgetItem (item, QStringList { r.name, r.value, r.type });
			child->setData (0, kIdRole, r.id);
			child->setToolTip (1, r.value);
			if (r.hasChildren)	// placeholder: shows the expand arrow
				(new QTreeWidgetItem (child))->setData (0, kIdRole, kPlaceholderId);
		}
	}

	void FillRoot (const LuHost& host, QTreeWidget* tree, int nodeId)
	{
		tree->clear ();
		for (const Row& r : Fetch (host, nodeId)) {
			auto* item = new QTreeWidgetItem (tree, QStringList { r.name, r.value, r.type });
			item->setData (0, kIdRole, r.id);
			item->setToolTip (1, r.value);
			if (r.hasChildren) {
				auto* ph = new QTreeWidgetItem (item);
				ph->setData (0, kIdRole, kPlaceholderId);
			}
		}
	}

	// true when the item or a loaded descendant contains `needle`; hides the others and opens the path to a match.
	bool Filter (QTreeWidgetItem* item, const QString& needle)
	{
		bool childMatch = false;
		for (int i = 0; i < item->childCount (); ++i)
			if (!IsPlaceholder (item->child (i)) && Filter (item->child (i), needle))
				childMatch = true;
		const bool self = needle.isEmpty () || item->text (0).contains (needle, Qt::CaseInsensitive) || item->text (1).contains (needle, Qt::CaseInsensitive);
		const bool show = self || childMatch;
		item->setHidden (!show);
		if (!needle.isEmpty () && childMatch)
			item->setExpanded (true);
		return show;
	}

	void FilterTree (QTreeWidget* tree, const QString& needle)
	{
		for (int i = 0; i < tree->topLevelItemCount (); ++i)
			Filter (tree->topLevelItem (i), needle);
	}

	// Search on Enter looks inside collapsed nodes too: loads them (bounded) before filtering.
	void LoadAll (const LuHost& host, QTreeWidget* tree)
	{
		std::vector<QTreeWidgetItem*> stack;
		for (int i = 0; i < tree->topLevelItemCount (); ++i)
			stack.push_back (tree->topLevelItem (i));
		int budget = kMaxSearchNodes;
		while (!stack.empty () && budget > 0) {
			QTreeWidgetItem* item = stack.back ();
			stack.pop_back ();
			Load (host, item);
			for (int i = 0; i < item->childCount (); ++i, --budget)
				stack.push_back (item->child (i));
		}
	}

	bool LooksLikeGuid (const QString& text)
	{
		const QString t = text.trimmed ();
		return t.size () >= 32 && t.size () <= 38 && t.count ('-') >= 4;
	}

	int g_argc = 1;
	char g_arg0[] = "ArchiCAD_LookUp";
	char* g_argv[] = { g_arg0, nullptr };

} // namespace


extern "C" void LuShow (const LuHost* hostPtr, void* ownerHwnd, const char* moduleDir)
{
	const LuHost host = *hostPtr;

	if (QApplication::instance () == nullptr) {
		QCoreApplication::addLibraryPath (QString::fromUtf8 (moduleDir));	// platforms\qwindows.dll next to this DLL
		static QApplication app (g_argc, g_argv);
	}
	QApplication::setStyle (QStringLiteral ("Fusion"));

	QDialog dialog;
	dialog.setWindowTitle (QStringLiteral ("Archicad Lookup"));
	dialog.resize (1180, 720);
	dialog.setObjectName (QStringLiteral ("root"));
	bool dark = WindowsUsesDarkTheme ();
	dialog.setStyleSheet (FluentStyle::Sheet (dark));

	// ---- left: navigation ----
	auto* title = new QLabel (QStringLiteral ("Lookup"));
	title->setObjectName (QStringLiteral ("title"));
	auto* subtitle = new QLabel (QStringLiteral ("Archicad data, read only"));
	subtitle->setObjectName (QStringLiteral ("sub"));
	auto* nav = new QListWidget;
	nav->setObjectName (QStringLiteral ("nav"));
	nav->setFixedWidth (230);
	auto* themeButton = new QPushButton (dark ? QStringLiteral ("Light theme") : QStringLiteral ("Dark theme"));

	auto* left = new QVBoxLayout;
	left->addWidget (title);
	left->addWidget (subtitle);
	left->addSpacing (12);
	left->addWidget (nav, 1);
	left->addWidget (themeButton);

	// ---- right: search + tree ----
	auto* search = new QLineEdit;
	search->setPlaceholderText (QStringLiteral ("Filter by name or value. Enter = search inside collapsed nodes, or paste an element GUID"));
	search->setClearButtonEnabled (true);
	auto* refresh = new QPushButton (QStringLiteral ("Refresh"));
	refresh->setObjectName (QStringLiteral ("accent"));

	auto* tree = new QTreeWidget;
	tree->setColumnCount (3);
	tree->setHeaderLabels ({ QStringLiteral ("Name"), QStringLiteral ("Value"), QStringLiteral ("Type") });
	tree->header ()->setSectionResizeMode (QHeaderView::Interactive);
	tree->setColumnWidth (0, 320);
	tree->setColumnWidth (1, 380);
	tree->setUniformRowHeights (true);
	tree->setAnimated (false);
	tree->setSelectionBehavior (QAbstractItemView::SelectRows);

	auto* status = new QLabel;
	status->setObjectName (QStringLiteral ("status"));
	auto* copyValue = new QPushButton (QStringLiteral ("Copy value"));
	auto* copyRow = new QPushButton (QStringLiteral ("Copy row"));
	auto* close = new QPushButton (QStringLiteral ("Close"));

	auto* searchRow = new QHBoxLayout;
	searchRow->addWidget (search, 1);
	searchRow->addWidget (refresh);
	auto* bottom = new QHBoxLayout;
	bottom->addWidget (status, 1);
	bottom->addWidget (copyValue);
	bottom->addWidget (copyRow);
	bottom->addWidget (close);
	auto* right = new QVBoxLayout;
	right->addLayout (searchRow);
	right->addWidget (tree, 1);
	right->addLayout (bottom);

	auto* main = new QHBoxLayout (&dialog);
	main->setContentsMargins (16, 16, 16, 16);
	main->setSpacing (16);
	main->addLayout (left);
	main->addLayout (right, 1);

	// ---- behaviour ----
	auto loadNav = [&host, nav] (int keepRow) {
		nav->clear ();
		for (const Row& r : Fetch (host, 0)) {
			auto* item = new QListWidgetItem (r.value.isEmpty () ? r.name : r.name + QStringLiteral ("   ") + r.value);
			item->setData (kIdRole, r.id);
			nav->addItem (item);
		}
		nav->setCurrentRow (nav->count () > 0 ? qBound (0, keepRow, nav->count () - 1) : -1);
	};
	auto showNode = [&host, tree, search, status] (int nodeId) {
		FillRoot (host, tree, nodeId);
		search->clear ();
		status->setText (QStringLiteral ("%1 item(s)").arg (tree->topLevelItemCount ()));
		if (tree->topLevelItemCount () == 1)
			tree->topLevelItem (0)->setExpanded (true);
	};

	QObject::connect (nav, &QListWidget::currentItemChanged, [showNode] (QListWidgetItem* current, QListWidgetItem*) {
		if (current != nullptr)
			showNode (current->data (kIdRole).toInt ());
	});
	QObject::connect (tree, &QTreeWidget::itemExpanded, [&host] (QTreeWidgetItem* item) { Load (host, item); });
	QObject::connect (search, &QLineEdit::textChanged, [tree] (const QString& text) { FilterTree (tree, text.trimmed ()); });
	QObject::connect (search, &QLineEdit::returnPressed, [&host, tree, search, nav, status] () {
		const QString text = search->text ().trimmed ();
		if (LooksLikeGuid (text)) {
			const int id = host.findGuid (host.ctx, text.toUtf8 ().constData ());
			if (id < 0) {
				status->setText (QStringLiteral ("No element with this GUID."));
				return;
			}
			auto* item = new QListWidgetItem (QStringLiteral ("GUID  ") + text.left (8) + QStringLiteral ("..."));
			item->setData (kIdRole, id);
			nav->addItem (item);
			nav->setCurrentItem (item);
			return;
		}
		LoadAll (host, tree);
		FilterTree (tree, text);
	});
	QObject::connect (refresh, &QPushButton::clicked, [loadNav, nav] () { loadNav (nav->currentRow ()); });
	QObject::connect (themeButton, &QPushButton::clicked, [&dialog, &dark, themeButton] () {
		dark = !dark;
		dialog.setStyleSheet (FluentStyle::Sheet (dark));
		themeButton->setText (dark ? QStringLiteral ("Light theme") : QStringLiteral ("Dark theme"));
	});
	QObject::connect (copyValue, &QPushButton::clicked, [tree] () {
		if (QTreeWidgetItem* item = tree->currentItem ())
			QApplication::clipboard ()->setText (item->text (1));
	});
	QObject::connect (copyRow, &QPushButton::clicked, [tree] () {
		if (QTreeWidgetItem* item = tree->currentItem ())
			QApplication::clipboard ()->setText (item->text (0) + QLatin1Char ('\t') + item->text (1) + QLatin1Char ('\t') + item->text (2));
	});
	QObject::connect (close, &QPushButton::clicked, &dialog, &QDialog::accept);

	loadNav (0);

	// modal over Archicad: owner disabled while the window is open (the nested Qt loop runs on Archicad's main thread)
	HWND owner = static_cast<HWND> (ownerHwnd);
	dialog.winId ();
	if (owner != nullptr) {
		SetWindowLongPtrW (reinterpret_cast<HWND> (dialog.winId ()), GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR> (owner));
		EnableWindow (owner, FALSE);
	}
	dialog.exec ();
	if (owner != nullptr) {
		EnableWindow (owner, TRUE);
		SetForegroundWindow (owner);
	}
}
