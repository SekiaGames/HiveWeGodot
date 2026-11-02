#pragma once

#include "ui_minimap.h"

#include "texture.hpp"

class Minimap : public QWidget {
	Q_OBJECT

public:
	Minimap(QWidget* parent = nullptr);

public:
	void set_minimap(Texture texture);

signals:
	/// point contains the location clicked on the minimap in the range [0..1]
	void clicked(QPointF point);
private:
	Ui::Minimap ui;

	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
};
