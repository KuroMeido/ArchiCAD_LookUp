#ifndef FLUENTSTYLE_HPP
#define FLUENTSTYLE_HPP

#include <QString>

// Fluent-like look for plain Qt Widgets: Segoe UI, 6 px corners, accent colour, light and dark palettes.
// Tokens (@name) are replaced from the palette, so the sheet below is written once.
namespace FluentStyle {

	inline QString Sheet (bool dark)
	{
		struct Palette { const char* name; const char* value; };
		const Palette light[] = {
			{ "@window", "#F3F3F3" }, { "@card", "#FFFFFF" }, { "@border", "#E0E0E0" }, { "@text", "#1A1A1A" },
			{ "@sub", "#5C5C5C" }, { "@accentText", "#FFFFFF" }, { "@accent", "#0F6CBD" }, { "@accentHover", "#115EA3" },
			{ "@hover", "#F0F0F0" }, { "@select", "#E3EFFA" }, { "@control", "#FBFBFB" }, { "@scroll", "#B8B8B8" },
		};
		const Palette darkP[] = {
			{ "@window", "#202020" }, { "@card", "#2B2B2B" }, { "@border", "#3D3D3D" }, { "@text", "#FFFFFF" },
			{ "@sub", "#B5B5B5" }, { "@accentText", "#0B0B0B" }, { "@accent", "#479EF5" }, { "@accentHover", "#62ABF5" },
			{ "@hover", "#353535" }, { "@select", "#1F3A52" }, { "@control", "#333333" }, { "@scroll", "#6B6B6B" },
		};
		QString s = QStringLiteral(R"(
* { font-family: "Segoe UI Variable Text", "Segoe UI"; font-size: 10pt; color: @text; }
QDialog, QWidget#root { background: @window; }
QLabel#title { font-size: 15pt; font-weight: 600; }
QLabel#status, QLabel#sub { color: @sub; }

QListWidget#nav { background: transparent; border: none; outline: 0; }
QListWidget#nav::item { padding: 9px 12px; margin: 2px 0; border-radius: 6px; }
QListWidget#nav::item:hover { background: @hover; }
QListWidget#nav::item:selected { background: @select; color: @text; border-left: 3px solid @accent; }

QLineEdit { background: @control; border: 1px solid @border; border-bottom: 2px solid @sub; border-radius: 6px;
            padding: 6px 10px; selection-background-color: @accent; selection-color: @accentText; }
QLineEdit:focus { border-bottom: 2px solid @accent; }

QTreeWidget { background: @card; border: 1px solid @border; border-radius: 8px; outline: 0; padding: 4px;
              alternate-background-color: @card; }
QTreeWidget::item { padding: 4px 2px; border-radius: 4px; }
QTreeWidget::item:hover { background: @hover; }
QTreeWidget::item:selected { background: @select; color: @text; }
QHeaderView::section { background: @card; color: @sub; border: none; border-bottom: 1px solid @border; padding: 6px 8px; }

QPushButton { background: @control; border: 1px solid @border; border-radius: 6px; padding: 6px 16px; }
QPushButton:hover { background: @hover; }
QPushButton:pressed { background: @select; }
QPushButton#accent { background: @accent; color: @accentText; border: 1px solid @accent; }
QPushButton#accent:hover { background: @accentHover; }

QScrollBar:vertical { background: transparent; width: 12px; margin: 2px; }
QScrollBar::handle:vertical { background: @scroll; border-radius: 4px; min-height: 28px; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
QScrollBar:horizontal { background: transparent; height: 12px; margin: 2px; }
QScrollBar::handle:horizontal { background: @scroll; border-radius: 4px; min-width: 28px; }
QToolTip { background: @card; color: @text; border: 1px solid @border; padding: 4px; }
)");
		for (const Palette& p : dark ? darkP : light)
			s.replace(QString::fromLatin1(p.name), QString::fromLatin1(p.value));
		return s;
	}

} // namespace FluentStyle

#endif
