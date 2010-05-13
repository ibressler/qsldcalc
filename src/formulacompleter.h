/*
 * src/formulacompleter.h
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

#ifndef FORMULACOMPLETER_H
#define FORMULACOMPLETER_H
 
#include "elementdatabase.h"

class QCompleter;
class QLineEdit;
class QPushButton;

/// Chemical element completion during formula input.
/// \anchor objDescr
/// It does some calculations
/// and string processing to enable completion for single tokens enclosed by 
/// whitespace. The default is to just complete the while line of a QTextEdit.
/// But we want to complete single Tokens within that line. \see complete()
class FormulaCompleter: public QObject
{
	Q_OBJECT
public:
	/// Constructs this Completer.
	/// \param[in] db An ElementDatabase which provides a list of
	///            possible tokens to complete (in this case, chemical 
	///            element names). Calls ElementDatabase::getSymbolList().
	/// \param[in,out] ntr A QLineEdit which gets this completion.
	/// \param[in,out] btn A QPushButton which is to the QLineEdit::returnPressed() 
	///                signal connected, if any.
	FormulaCompleter(const ElementDatabase & db, 
	                 QLineEdit                  * ntr, 
                         QPushButton                * btn);
private slots:
	/// Calls the QCompleter::complete(const QRect &) slot. Calculations
	/// for piecewise completion of the supplied string occurs here. See
	/// \ref objDescr "above".
	/// \param[in] text The text of the QLineEdit.
	void complete(const QString& text);

	/// Connects to the QCompleter::activated(const QString &) signal.
	/// It connects the previously disconnected QLineEdit::returnPressed()
	/// signal to its original receiver (the supplied QPushButton).
	/// \param[in] text The finally selected item from the popup dropdown
	///            list.
	void activated(const QString& text);

	/// Connects to the QCompleter::highlighted(const QString &) signal.
	/// It replaces the current token with the selected one within the
	/// QLineEdit text. 
	/// If the QPushButton is set, it temporarily disconnects the 
	/// QLineEdit::returnPressed() signal from it when an item is selected. 
	/// This avoids interference with the original receiver of this signal.
	/// \param[in] text Selected item in the popup dropdown list.
	void highlighted(const QString& text);
private:
	QCompleter * mCompleter; //!< Does the completion job.
	QLineEdit * mLineEdit;   //!< Widget to enable completion for.
	QPushButton * mBtn;      //!< Button with interfering connection.
	QString mToken;          //!< Current token for completion.
	QString mCurText;        //!< Current text in mLineEdit.
	int mTokenStart;         //!< Start of current token in QLineEdit text.
	int mTokenEnd;           //!< End of current token in QLineEdit text.
};

#endif
