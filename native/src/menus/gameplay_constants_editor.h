#pragma once

#include <QDialog>
#include <QLineEdit>

#include "gameplay_constants.hpp"
#include "trigger_strings.hpp"

class GameplayConstantsEditor : public QDialog {
	Q_OBJECT
public:
	GameplayConstantsEditor(QWidget* parent, GameplayConstants& constants, TriggerStrings& trigger_strings);

private:
	QLineEdit* search;
};
