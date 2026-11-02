#pragma once

#include <QDialog>

#include "ui_pathing_palette.h"

#include "palette.h"
#include "pathing_brush.h"

#include "pathing_map.hpp"
#include "world_undo_manager.hpp"

class PathingPalette : public Palette {
	Q_OBJECT

public:
	PathingPalette(QWidget* parent, Brush*& active_brush, PathingMap& pathing_map, WorldUndoManager& world_undo);
	~PathingPalette();

private:
	bool event(QEvent *e) override;

	PathingMap& pathing_map;

	Ui::PathingPalette ui;
	PathingBrush brush;

	QRibbonTab* ribbon_tab = new QRibbonTab;
	QRibbonButton* selection_mode = new QRibbonButton;
	QRibbonButton* import_pathing = new QRibbonButton;
	QRibbonButton* export_pathing = new QRibbonButton;

public:
	void deactivate(QRibbonTab* tab) override;
};