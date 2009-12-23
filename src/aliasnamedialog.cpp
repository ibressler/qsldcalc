/*
 * src/aliasnamedialog.cpp
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
 * Ingo Bressler (ingo at cs.tu-berlin.de)
 */

#include "aliasnamedialog.h"

// AliasValidator //

AliasValidator::AliasValidator(QObject * parent)
	:QValidator(parent)
{}

QValidator::State
AliasValidator::validate(QString & input, int & pos) const
{
	bool valid = false;
	const QChar * c = input.constData();
	if (input.length() > 0) 
		valid = (c[0].isLetter() && c[0].isUpper());
	else
		return QValidator::Intermediate;
	for(int i=1; valid && i < input.length(); i++)
	{
		valid &= (c[i].isLetter() && c[i].isLower());
	}
	if (valid) return QValidator::Acceptable;
	else       return QValidator::Invalid;
}

void 
AliasValidator::fixup(QString & input) const
{
	input.toLower();
	if (input.length() > 0) {
		input.constData()->toUpper();
	}
}

// AliasNameDialog //

AliasNameDialog::AliasNameDialog(QWidget * parent)
	: QDialog(parent),
	  Ui::AliasNameDialog(),
	  mValidator(new AliasValidator(this))
{
	setupUi(this);
	ntrAliasName->setValidator(mValidator);
}

AliasNameDialog::~AliasNameDialog()
{
	delete mValidator;
}

QString
AliasNameDialog::name() const
{
	if (!ntrAliasName) return QString();
	else               return ntrAliasName->text();
}


