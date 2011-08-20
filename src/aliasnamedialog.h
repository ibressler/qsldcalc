/*
 * src/aliasnamedialog.h
 *
 * Copyright (c) 2009 Technische Universität Berlin, 
 * Stranski-Laboratory for Physical und Theoretical Chemistry
 *
 * This file is part of qSLDcalc.
 *
 * qSLDcalc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * qSLDcalc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with qSLDcalc. If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * Author(s) of this file:
 * Ingo Bressler (qsldcalc at ingobressler.net)
 */

#ifndef ALIASNAMEDIALOG_H
#define ALIASNAMEDIALOG_H

#include <QValidator>
#include "ui_aliasnamedialog.h"

/**
 * Validates alias names entered in the AliasNameDialog.
 * Since an alias \e simulates a single chemical element (which is defined by
 * a group of chemical elements), it must follow the naming convention for
 * chemical element symbol names: first letter in upper case and all following
 * letters in lower case which implies that it consists of alphabetical
 * letters only (no numbers, no special characters).
 * Example: \e Aaa, \e Tri, \e Gui, \e Gjkher allowed
 */
class AliasValidator: public QValidator
{
public:
	/// Constructor.
	AliasValidator(QObject * parent);

	/// Tests input for correctness.
	/// Overrides QValidator::validate, see Qt documentation for
	/// reference.
	virtual State
	validate(QString & input, int & pos) const;

	/// Tries to fix an invalid input text.
	/// Overrides QValidator::fixup, see Qt documentation for reference.
	virtual void
	fixup(QString & input) const;
};

/**
 * An interactive dialog which asks the user for a new alias name.
 */
class AliasNameDialog: public QDialog, public Ui::AliasNameDialog
{
	Q_OBJECT
public:
	/// Constructor.
	AliasNameDialog(QWidget * parent);

	/// Cleans up additional internal data structures.
	/// ... especially the previously created AliasValidator.
	~AliasNameDialog();

	/// Returns the validated alias name.
	QString
	name() const;

private:
	/// Verifies the entered alias name to follow the convention for
	/// chemical symbol names.
	/// \see AliasValidator
	AliasValidator * mValidator;
};


#endif
