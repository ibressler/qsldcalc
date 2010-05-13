/*
 * src/formulacompleter.cpp
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

#include <QCompleter>
#include <QLineEdit>
#include <QPushButton>
#include "formulacompleter.h"

FormulaCompleter::FormulaCompleter(const ElementDatabase & db, 
                                   QLineEdit                  * ntr,
                                   QPushButton                * btn)
	: mCompleter(NULL),
	  mLineEdit(ntr),
	  mBtn(btn),
	  mTokenStart(0), mTokenEnd(0)
{
	mCompleter = new QCompleter(db.getSymbolList(), ntr);
	mCompleter->setCaseSensitivity(Qt::CaseSensitive);
	mCompleter->setModelSorting(QCompleter::CaseSensitivelySortedModel);
	mCompleter->setWidget(ntr);
	connect(ntr, SIGNAL(textEdited(const QString&)), 
	        this, SLOT(complete(const QString&)));
	connect(mCompleter, SIGNAL(highlighted(const QString&)), 
	        this, SLOT(highlighted(const QString&)));
	connect(mCompleter, SIGNAL(activated(const QString&)), 
	        this, SLOT(activated(const QString&)));
}

void FormulaCompleter::activated(const QString&)
{
	if (mBtn) {
		connect(mLineEdit, SIGNAL(returnPressed()), 
		        mBtn, SLOT(animateClick()));
	}
}

void FormulaCompleter::highlighted(const QString& text)
{
	if (mBtn) { // don't interfer with main calc button
		disconnect(mLineEdit, SIGNAL(returnPressed()), 
		           mBtn, SLOT(animateClick()));
	}
	mLineEdit->setText(
		mCurText.mid(0, mTokenStart)+
		text+
		mCurText.mid(mTokenEnd+1) );
	mLineEdit->setCursorPosition(mTokenStart+text.length());
}

void FormulaCompleter::complete(const QString& text)
{
	int cursorPos = mLineEdit->cursorPosition();
	mCurText = mLineEdit->text();
	mToken = text;
	QRegExp regexp("[^a-zA-Z0-9\\.\\,]");
	mTokenStart = mToken.lastIndexOf(regexp, (cursorPos > 0) ? cursorPos-1 : 0)+1;
	mTokenEnd  = mToken.indexOf(regexp, cursorPos)-1;
	if (mTokenStart < 0) mTokenStart = 0;
	if (mTokenEnd < 0) mTokenEnd = mToken.length()-1;
	mToken.remove(0, mTokenStart);
	int tmpPrefixEnd = mTokenEnd+1-mTokenStart; // after remove above
	mToken.remove(tmpPrefixEnd, mToken.length()-tmpPrefixEnd);
	mCompleter->setCompletionPrefix(mToken);
	mCompleter->complete();
}

