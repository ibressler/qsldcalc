/*
 * src/mainwindow.h
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H
 
/**
 * \mainpage SLD Calculator Source Code Documentation
 *
 * \section Features
 * - Calculation of Neutron and X-Ray scattering length densities for
 *   compounds
 * - Standalone executable for various platforms (Windows, Linux, Mac)
 * - Internationalization support (switch language at runtime)
 * - free as in freedom (GPL)
 * - Written in C++ and Qt (http://qt.nokia.com/products)
 * .
 * \section Screenshots
 *
 * \image html ex_output.png "Convenient Export of Results"
 * \image latex ex_output.png "Convenient Export of Results"
 * \code
 * molecular volume nm^3;0,280214
 * Cu;0,0959278
 * H;0,00304313
 * O;
 * S;0,0484047
 * neutron scattering;
 * SLD coherent 1/cm^2;1,24917e+10
 * Cu;2,75433e+09
 * H;-2,66868e+09
 * S;1,01601e+09
 * \endcode
 * <hr>
 * \image html ex_completion.png "Chemical Symbol Based Formula Completion"
 * \image latex ex_completion.png "Chemical Symbol Based Formula Completion"
 * <hr>
 * \image html ex_visu.png "Visualization of Element Characteristics"
 * \image latex ex_visu.png "Visualization of Element Characteristics"
 * <hr>
 *
 * \section data_src Data Sources
 *
 * - for a description of the backend data sources, see the edb::Element
 *   documentation.
 *
 * \section license License and Copyright Information
 *
 * <table border="0">
 * <tr><td>
 * - This program is released under the 
 * <a href="http://www.gnu.org/licenses/gpl.html">GNU General Public License</a> (GPL).
 * </td><td>
 * \image html logo_gpl.png
 * \image latex logo_gpl.png width=2cm
 * </td></tr><tr><td>
 * - Initially, it was written by Ingo Bressler (ingob at users.berlios.de) at the 
 * <a href="http://www.chemie.tu-berlin.de/gradzielski/menue/physikalische_chemie_molekulare_materialwissenschaften/">Stranski-Laboratory for Physical and Theoretical Chemistry</a> of the 
 * <a href="http://www.tu-berlin.de">Technische Universit&auml;t Berlin</a>.
 * </td><td>
 * \image html logo_tub.png
 * \image latex logo_tub.png width=2cm
 * </td></tr><tr><td>
 * - It is hosted by <a href="http://developer.berlios.de">BerliOS</a>.
 * </td><td>
 * \htmlonly
 * <a href="http://developer.berlios.de" title="BerliOS Developer"><img src="http://developer.berlios.de/bslogo.php?group_id=11424" width="124px" height="32px" border="0" alt="BerliOS Developer Logo"></a>
 * \endhtmlonly
 * </td></tr>
 * </table>
 *
 * \section rel_prog Related Programs
 *
 * - the web based \e ChemCalc at http://www.chemcalc.org
 * - \e GChemCalc part of the <em>Gnome Chemistry Utils</em> at 
 *   http://gchemutils.nongnu.org/#programs
 * - Small Angle Scattering Analysis Program \e SASfit at
 *   http://kur.web.psi.ch/sans1/SANSSoft/sasfit.html
 */

#include <iostream>
#include <QDir>
#include <QCompleter>
#include <QTranslator>
#include <QStandardItemModel>
#include "ui_mainwindow.h"
#include "inputdata.h"
#include "formulacompleter.h"
#include "datavisualizer.h"
#include "aliasnamedialog.h"

/// List of LangAction Objects. Used to manage actions which trigger a
/// language switch and retranslate at runtime.
typedef QList<QAction *> LangActionList;

/**
 * Main window GUI configuration and management.
 *
 * <b>Handling of GUI input fields</b>: The constructor adds each user input
 * field to the list MainWindow::mFormEntries. When the calculate button is 
 * pressed, all fields are read dynamically (based on the property system 
 * provided by \e QObject) and their data is stored in the InputData object with
 * their object name as key. For example, the value of the input field for 
 * density is stored in InputData with the key \e "ntrDensity", as it's the 
 * object name (and variable name) of the density spinbox.
 * \see saveInitialInput(), saveInput(), resetInput()
 *
 * <b>Localization issues</b>: The results can be exported in ASCII semicolon 
 * separated format. To simplify further work with this data, the user can
 * disable the use of system settings for the decimal operator 
 * (language menu -> checkbox). This makes it possible to have all text 
 * translated but keeping the dot as decimal operator. In consequence, all 
 * floating-point to character string conversion have to be handled by the 
 * respective member functions:
 * \see qstringFromVariant(), qstringFromComplex(), qstringFromDouble()
 * 
 * \todo Eventually, rewrite the GUI with python to avoid the hassle with memory
 * management. Keep the backend in C++ (InputData, edb::ElementDatabase).
 */
class MainWindow: public QMainWindow, private Ui::MainWindow
{
	Q_OBJECT

	/// Converts a boost variant into a QString. The supplied MainWindow
	/// provides localization-depended double to string conversion (esp.
	/// regarding the decimal operator). 
	/// \see MainWindow::qstringFromDouble(), MainWindow::qstringFromComplex()
	class QStringFromBoostVariant: public boost::static_visitor<QString>
	{
		const MainWindow * mParent;
	public:
		QStringFromBoostVariant(const MainWindow * p): mParent(p) {}
		QString operator()(const int& i) const          { return QString::number(i); }
		QString operator()(const std::string& s) const  { return QString(tr(s.c_str())); }
		QString operator()(const double& d) const       { return mParent->qstringFromDouble(d); }
		QString operator()(const edb::complex& c) const { return mParent->qstringFromComplex(c); }
	};

public:
	/// Creates the main window.
	/// \param[in,out] app QApplication object we are associated with
	/// \param[in,out] db Entirely populated chemical element database.
	MainWindow(QApplication &app, edb::ElementDatabase& db);

	/// Converts any property of a chemical element to a localization
	/// aware character string. \see QStringFromBoostVariant
	/// \param[in] var Property of an database element to convert.
	/// \returns The generated character string.
	QString toString(const edb::Element::PropertyVariant& var) const;

private slots:
	/// Raises a message box with general information about the program.
	void about();

	/// Loads another language file and triggers a retranslate of the GUI.
	/// \param[in] filename Absolute file name to load.
	void loadLanguage(const QString& filename);

	/// Tries to determine the desired language from the system locale
	/// settings. On failure, selects the english translation. Called on
	/// startup.
	void selectDefaultLang();

	/// Resets the GUI input fields to initial values.
	void resetInput();

	/// Calculates compound characteristics from current GUI input data.
	void doCalc();

	/// Adds an alias for the current compound, i.e. formula.
	/// \todo Remember expand/collapse the state of all cells when raising 
	/// the add-alias dialog, i.e. calculating the formula (doCalc()).
	void addAlias();

	/// Copies the current selection of the result to the clipboard.
	/// Exports the selected rows and columns of the tabular widget as
	/// semicolon separated dataset. The content of unselected cells is
	/// not copied but the structure is preserved (empty cells).
	void copyResultTableSelection();

	/// Fills the tabular widget with the results of the last calculation,
	/// i.e. formula parsing.
	void rebuildResultTable();

	/// Displays detailed information of a chemical element in the tabular 
	/// result widget. Is connected to
	/// \e MainWindow::lblFormattedFormula.linkActivated() and
	/// DataVisualizer::elementLinkActivated().
	/// \todo Translate chemical element names when displayed. They appear
	/// as regular data entries in the database and are not processed
	/// further, atm.
	/// \param[in] key Database key for the chemical element to query. See
	///                edb::ElementDatabase::KeyType for details.
	void showElementData(const QString& key);

	/// Expands an item in the tabular result widget.
	/// \param[in] index Index of the item to expand.
	void expandItem(const QModelIndex & index = QModelIndex());

	/// Expands all items in the tabular result widget.
	void expandAll();

	/// Collapses all items in the tabular result widget.
	void collapseAll();
private:
	/// Sets up entries, i.e. available actions, in the menubar and the 
	/// tabular result widget.
	void addActions();

	/// Retranslates the entire application at runtime.
	void retranslateUi();

	/// Updates the menu of available languages. Searches the language
	/// directory for language files and creates a menu entry for each such
	/// file.
	/// \param[in,out] menuLang Language switching menu.
	void updateLangActions(QMenu * menuLang);

	/// Stores user-defined input data as initial settings (for reset later). 
	void saveInitialInput();

	/// Stores user-defined input data for further processing. 
	void saveInput();

	/// Populates the tabular result widget with recent calculation
	/// results from InputData.
	void fillResultTable();

	/// Removes all entries from the tabular result widget.
	void clearResultTable();

	/// Inserts a new item to the tabular result widget.
	/// Helper for fillResultTable()
	/// \param[in] key Key of the dataset within InputData.
	void resultAddData(const char * key);

	/// Appends a \e QVariant to the tabular result widget.
	/// This is the most generic case. Helper for fillResultTable()
	/// \param[in,out] parent Item to add this \e QVariant to. Root item of
	///                the tabular widget for top-level position.
	/// \param[in] name Display name of the new item.
	/// \param[in] var \e QVariant to add.
	void appendVariant(QStandardItem * parent, const QString& name, const QVariant& var);

	/// Appends a \e QVariantMap to the tabular result widget.
	/// Specialized for a map of variants. Helper for fillResultTable()
	/// \param[in,out] parent Item to add this \e QVariantMap to. Root item of
	///                the tabular widget for top-level position.
	/// \param[in] name Display name of the new item.
	/// \param[in] map \e QVariantMap to add.
	void appendVariantMap(QStandardItem * parent, const QString& name, const QVariantMap& map);


	/// Adds the user input to the result output. Enables the user to
	/// export this data together with the results for improved consistency.
	/// Helper for fillResultTable()
	void resultAddInput();

	/// Converts a \e QVariant to a character string. Special handling of 
	/// floating point numbers. \sa qstringFromDouble(), qstringFromComplex()
	QString qstringFromVariant(const QVariant& var) const;

	/// Converts a double to a character string. Accounts for the locale
	/// settings of the MainWindow.
	QString qstringFromDouble(double d) const;

	/// Converts a complex number to a character string. Accounts for the 
	/// locale settings of the MainWindow.
	QString qstringFromComplex(edb::complex c) const;

	/// Displays the HTML formatted formula with links for detailed
	/// information of every chemical element.
	void setFormattedFormula();

	/// Tests if there is a formula in the input field currently.
	bool formulaIsEmpty() const;
private:
	QMenu            * menuFile;  //!< Menubar entry \e file.
	QMenu            * menuTools; //!< Menubar entry \e tools.
	QMenu            * menuLang;  //!< Menubar entry \e language.
	QMenu            * menuHelp;  //!< Menubar entry \e help.
	QApplication     * mApp;      //!< Reference to the parent application.
	FormulaCompleter * mCompleter; //!< Formula piecewise completion feature.
	QTranslator        mTranslator; //!< Translates all GUI texts.
	
	/// To restore the original color of the input formula text.
	QPalette           mFormulaDefaultPalette;

	/// Actions for switching to a certain available language.
	LangActionList     mLangActionList;

	/// Directory path to search for translations
	QDir               mLangPath; 

	QString	                  mAboutTitle;
	QString                   mAboutText;

	/// Stores input data and calculates compound characteristics.
	InputData                 mInputData;

	/// Database of all known chemical elements.
	edb::ElementDatabase::Ptr mDB;

	/// Visualizes selected characteristics of all known chemical elements.
	DataVisualizer            mDataVisualizer;

	/// Backend model for the tabular result widget.
	/// \todo Check if InputData could be used as \e QAbstractItemModel.
	QStandardItemModel        mModel;

	/// Required information of an user input field for dynamical
	/// processing at runtime. \see MainWindow::mFormEntries
	struct FormEntry {
		QWidget       * widget;      //!< The widget to manage.
		QLatin1String   propertyName;//!< Object name.
		QLabel        * label;       //!< Displayed description.
		FormEntry(QWidget * w, const char * p, QLabel * l)
			: widget(w), propertyName(p), label(l)
		{}
	};
	QList<FormEntry> mFormEntries; //!< List of all user input fields to process.
};

/**
 * QAction for language switching purposes.
 * Represents a menu entry for a certain language and knows about the
 * translation filename for this language. If a language is loaded, the
 * filename is translated into the name of the language. Otherwise, the plain
 * filename is displayed in a menu, for example.
 */
class LangAction: public QAction
{
	Q_OBJECT
public:
	/**
	 * Constructor.
	 * \param parent Parent QObject (usually Qt widget).
	 * \param fn Filename of the language translation file.
	 */
	LangAction(QObject * parent, const QString& fn):
		QAction(parent), langFilename(fn)
	{
		setText(tr(fn.toAscii().data()));
		connect(this, SIGNAL(triggered()), this, SLOT(triggeredSlot()));
	}
public slots:
	/// Emits a signal with the language filename as parameter.
	/// \sa triggered(const QString&)
	void triggeredSlot() { emit triggered(langFilename); }
signals:
	/// Same signal as QAction::triggered() but with the language
	/// filename as parameter.
	void triggered(const QString&);
private:
	const QString	langFilename; //!< Language translation filename.
};

#endif
