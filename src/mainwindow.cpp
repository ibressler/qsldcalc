/*
 * src/mainwindow.cpp
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

#include <QMessageBox>
#include <QDate>
#include <QTranslator>
#include <QLocale>
#include <QClipboard>
#include <QStandardItem>
#include <QScrollBar>
#include "mainwindow.h"
#include "aliasnamedialog.h"

MainWindow::MainWindow(QApplication &app, ElementDatabase& db)
	: QMainWindow(),
	  Ui::MainWindow(),
	  mApp(&app), 
	  mCompleter(NULL),
	  mLangPath(":/lang"),
	  mInputData(db),
	  mDB(&db),
	  mDataVisualizer(this, db)
{
	mApp->installTranslator(&mTranslator);
	if ( ! mLangPath.exists() ) 
		std::cerr << "Unable to determine language directory !" << std::endl;

	setupUi(this);

	mFormulaDefaultPalette = ntrFormula->palette();
	tblResult->setModel(&mModel);

	// action setup (menu bar)
	menuFile = new QMenu(menubar);
	menuTools = new QMenu(menubar);
	menuLang = new QMenu(menubar);
	menuHelp = new QMenu(menubar);
	addActions();
	connect(actionQuit, SIGNAL(triggered()), &app, SLOT(quit()));
	connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
	connect(actionAbout_Qt, SIGNAL(triggered()), &app, SLOT(aboutQt()));
	connect(actionAbout, SIGNAL(triggered()), this, SLOT(about()));
	connect(actionVisualizeData, SIGNAL(triggered()), &mDataVisualizer, SLOT(show()));
	connect(actionUseSystemLocale, SIGNAL(triggered()), this, SLOT(useSystemLocaleTriggered()));
	selectDefaultLang();
	actionUseSystemLocale->trigger();

	// action setup result table
	connect(actionExpandAll, SIGNAL(triggered()), this, SLOT(expandAll()));
	connect(actionCollapseAll, SIGNAL(triggered()), this, SLOT(collapseAll()));
	connect(actionSelectAll, SIGNAL(triggered()), tblResult, SLOT(selectAll()));
	connect(actionCopySelection, SIGNAL(triggered()), this, SLOT(copyResultTableSelection()));
	connect(tblResult, SIGNAL(expanded(const QModelIndex&)), this, SLOT(expandItem(const QModelIndex&)));
	connect(tblResult, SIGNAL(collapsed(const QModelIndex&)), this, SLOT(expandItem(const QModelIndex&)));

	// populate list of all known GUI input fields
	mFormEntries.append(FormEntry(ntrFormula,   "currentText", lblFormula));
	mFormEntries.append(FormEntry(ntrDensity,   "value", lblDensity));
	mFormEntries.append(FormEntry(ntrXrayEn,    "value", lblXrayEn));
	mFormEntries.append(FormEntry(ntrNeutronWl, "value", lblNeutronWl));
	saveInitialInput();

	// button connectors
	connect(btnClear, SIGNAL(clicked()), this, SLOT(resetInput()));
	connect(btnAddAlias, SIGNAL(clicked()), this, SLOT(addAlias()));
	connect(btnCalc, SIGNAL(clicked()), this, SLOT(doCalc()));

	if (ntrFormula->lineEdit()) {
		mCompleter = new FormulaCompleter(db, ntrFormula->lineEdit(), btnCalc);
		connect(ntrFormula->lineEdit(), SIGNAL(returnPressed()), btnCalc, SLOT(animateClick()));
	}
	connect(ntrDensity, SIGNAL(editingFinished()), btnCalc, SLOT(animateClick()));
	connect(ntrXrayEn, SIGNAL(editingFinished()), btnCalc, SLOT(animateClick()));
	connect(ntrNeutronWl, SIGNAL(editingFinished()), btnCalc, SLOT(animateClick()));

	connect(lblFormattedFormula, SIGNAL(linkActivated(const QString&)), 
		this, SLOT(showElementData(const QString&)));
	connect(&mDataVisualizer, SIGNAL(elementLinkActivated(const QString&)), 
		this, SLOT(showElementData(const QString&)));
}

// try to determine the desired language file from system locale
// otherwise use the english translation
void MainWindow::selectDefaultLang()
{
	QString locale = QLocale::system().name();
#if DEBUG
	std::cerr << "locale: " << locale.toAscii().data() << std::endl;
#endif
	QStringList filters;
	filters << "language_*"+locale+".qm";
	QStringList langFiles(mLangPath.entryList(filters, QDir::Files));
	if (! langFiles.empty()) 
	{
		loadLanguage(langFiles.front());
	} else {
		loadLanguage("language_en_GB.qm");
	}
}

void MainWindow::loadLanguage(const QString& filename)
{
	QString absFilePath(mLangPath.absoluteFilePath(filename));
	if ( !QFile::exists(absFilePath) ||
	     !mTranslator.load(filename, mLangPath.canonicalPath()) )
	{
		std::cerr << "Could not load translation file '"
			<< filename.toStdString() << "' !";
	}
	mLangFile = filename;
	retranslateUi();
}

void MainWindow::retranslateUi()
{
	Ui::MainWindow::retranslateUi(this);

	setWindowTitle(tr(PROGRAM_NAME));
	menuFile->setTitle(tr("&File"));
	menuTools->setTitle(tr("&Tools"));
	menuLang->setTitle(tr("&Language"));
	menuHelp->setTitle(tr("&Help"));
	actionAbout->setText(tr("about &%1").arg(PROGRAM_NAME));

	// about message box context
	QDate d(QDate::currentDate());
	mAboutTitle = tr("about %1").arg(PROGRAM_NAME);
	mAboutText  = tr("This version of %1 was build on %2-%3-%4.")
	                      .arg(PROGRAM_NAME)
	                      .arg(int(BUILD_YEAR), 4, 10, QLatin1Char('0'))
	                      .arg(int(BUILD_MONTH), 2, 10, QLatin1Char('0'))
	                      .arg(int(BUILD_DAY), 2, 10, QLatin1Char('0'));
	mAboutText.append("<br><br>");
	mAboutText.append(tr("This program is released under the GNU General Public License (GPL). For details, see LICENSE.txt or"));
	mAboutText.append("<br><a href=\"http://www.gnu.org/licenses/gpl.html\">http://www.gnu.org/licenses/gpl.html</a>");
	mAboutText.append("<br><br>");
	mAboutText.append(tr("For documentation and source code see:"));
	mAboutText.append("<br><a href=\"http://developer.berlios.de/projects/qsldcalc/\">https://developer.berlios.de/projects/qsldcalc/</a>");
	mAboutText.append("<br><br>");
	mAboutText.append(tr("The backend data sources are:"));
	mAboutText.append("<br><a href=\"http://www.ncnr.nist.gov/resources/n-lengths/list.html\">http://www.ncnr.nist.gov/resources/n-lengths/list.html</a>");
	mAboutText.append("<br><a href=\"http://skuld.bmsc.washington.edu/scatter/AS_periodic.html\">http://skuld.bmsc.washington.edu/scatter/AS_periodic.html</a>");
	mAboutText.append("<br><br>");
	mAboutText.append(tr("Written by:"));
	mAboutText.append("<br>Ingo Bressler (qsldcalc at ingobressler.net)");
	mAboutText.append("<br>");
	mAboutText.append(tr("[ Comments, suggestions or corrections are welcome! ]"));
	mAboutText.append("<br>");
	updateLangActions(menuLang);

	rebuildResultTable();

	mDataVisualizer.retranslateUi();
}

void MainWindow::useSystemLocaleTriggered()
{
	// reformat spinbox values
	if (actionUseSystemLocale->isChecked()) {
		// use system settings for decimal operator
		mLocale = QLocale::system();
	} else  {
		// force . decimal operator
		mLocale = QLocale::c();
	}
	// disable decimal grouping
	mLocale.setNumberOptions(QLocale::OmitGroupSeparator|QLocale::RejectGroupSeparator);
	// reconfigure input fields
	ntrDensity->setLocale(mLocale);
	ntrXrayEn->setLocale(mLocale);
	ntrNeutronWl->setLocale(mLocale);
	// reload the current language file
	loadLanguage(mLangFile);
}

void MainWindow::about()
{
	QMessageBox::about(this, mAboutTitle, mAboutText);
}

// creates a menu of available languages out of the available translation files
void MainWindow::updateLangActions(QMenu * menuLang)
{
	// get all available language files
	QStringList filters;
	filters << "language_*.qm";
	QStringList langFiles(mLangPath.entryList(filters, QDir::Files));
	menuLang->clear();
	
	// create a menu entry for using the system locale for decimal ops
	menuLang->addAction(actionUseSystemLocale);

	// build the language selection menu
	LangActionList::iterator lit = mLangActionList.begin();
	for(; lit != mLangActionList.end(); lit++) delete *lit;
	mLangActionList.clear();
	QStringList::const_iterator fit = langFiles.begin();
	while(fit != langFiles.end()) {
		// the display name equals the filename directly
		// (but is Qtranslated to a more beautiful name)
		LangAction * act = new LangAction(this, *fit);
		menuLang->addAction(act);
		connect(act, SIGNAL(triggered(const QString&)), this, SLOT(loadLanguage(const QString&)));
		mLangActionList.push_back(act);
		fit++;
	}
	if (langFiles.empty())
	{
		QAction * act = new QAction(this);
		act->setText("No language file found!");
		menuLang->addAction(act);
		connect(act, SIGNAL(triggered()), this, SLOT(selectDefaultLang()));
		mLangActionList.push_back(act);
	}
}

void MainWindow::addActions()
{
	// file menu
	menubar->addAction(menuFile->menuAction());
	menuFile->addAction(actionQuit);
	// tools menu
	menubar->addAction(menuTools->menuAction());
	menuTools->addAction(actionVisualizeData);
	// language menu
	menubar->addAction(menuLang->menuAction());
	// help menu
	menubar->addAction(menuHelp->menuAction());
	menuHelp->addAction(actionAbout_Qt);
	menuHelp->addAction(actionAbout);
	// results table widget
	tblResult->addAction(actionExpandAll);
	tblResult->addAction(actionCollapseAll);
	tblResult->addAction(actionSelectAll);
	tblResult->addAction(actionCopySelection);
	tblResult->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void MainWindow::saveInitialInput()
{
	foreach(FormEntry fe, mFormEntries) {
		QVariant var(fe.widget->property(fe.propertyName.latin1()));
		if (var.isValid()) 
			mInputData.setInit(fe.widget->objectName(), var);
	}
}

void MainWindow::saveInput()
{
	foreach(FormEntry fe, mFormEntries) {
		QVariant var(fe.widget->property(fe.propertyName.latin1()));
		if (var.isValid()) {
			mInputData.set(fe.widget->objectName(), var);
		}
	}
}

void MainWindow::resetInput()
{
	foreach(FormEntry fe, mFormEntries) {
		QVariant var(mInputData.getInit(fe.widget->objectName()));
		if (var.isValid())
			fe.widget->setProperty(fe.propertyName.latin1(), var);
	}
	clearResultTable();
	setFormattedFormula();
}

void MainWindow::doCalc()
{
	saveInput();
	ntrFormula->setPalette(mFormulaDefaultPalette);
//	std::cerr << "InputData: \n" << mInputData << std::endl;
	try {
		mInputData.interpretFormula(ntrFormula->objectName());
	} catch(const cfp::Error& e) {
		size_t start, length;
		const char * eStr = e.what(start, length);
		lblFormattedFormula->setText(eStr);

		QPalette pal(ntrFormula->palette());
		pal.setColor(QPalette::Text, Qt::red);
		ntrFormula->setPalette(pal);
		if (length == 0) start = ntrFormula->currentText().length();
		QLineEdit * lineEdit = ntrFormula->lineEdit();
		if (lineEdit) {
			lineEdit->setCursorPosition(start);
			lineEdit->setSelection(start, length);
		}
		return;
	}
	rebuildResultTable();
}

void MainWindow::addAlias()
{
	if (formulaIsEmpty()) return;
	doCalc();
	AliasNameDialog askForAlias(this);
	int result = askForAlias.exec();
	if (result == QDialog::Accepted) {
		mInputData.addAlias(askForAlias.name());
	}
}

void MainWindow::setFormattedFormula()
{
	if (formulaIsEmpty()) {
		lblFormattedFormula->setText("");
		return;
	}
	QString s;
	foreach(cfp::CompoundElement e, mInputData.empiricalFormula())
	{
		s.append(DataVisualizer::getLinkText(e));
	}
	lblFormattedFormula->setText(s);
}

void MainWindow::fillResultTable() 
{
	setFormattedFormula();

	resultAddData("number of electrons", false);
	resultAddData("molecular mass g/mol", false);
	resultAddData("molecular volume nm^3", false);
	resultAddData("neutron scattering", true);
	resultAddData("xray scattering", true);
	resultAddInput();

	QStringList headerLabels;
	headerLabels << tr("Characteristic") << tr("Value");
	mModel.setHorizontalHeaderLabels(headerLabels);
	tblResult->resizeColumnToContents(0);
}

QString MainWindow::qstringFromDouble(double d) const
{
	return mLocale.toString(d);
}

QString MainWindow::qstringFromComplex(complex c) const
{
	QString str;
	if (c.real() != 0.0) str.append(qstringFromDouble(c.real()));
	if (c.imag() != 0.0) {
		if (c.imag() < 0) str.append("-");
		else              str.append("+");
		str.append("i");
		str.append(qstringFromDouble(fabs(c.imag())));
	}
	if (str.isEmpty()) str.append("0");
	return str;
}

QString MainWindow::qstringFromVariant(const QVariant& var) const
{
	QString result;

	if (var.isValid()) {
		// single double number
		if (var.canConvert(QVariant::Double)) 
		{
			result = qstringFromDouble(var.toDouble());
		// complex number package
		} else if (var.canConvert(QVariant::List)) 
		{
			QVariantList list = var.toList();
			if (list.size() == 2) {
				complex c(list.front().toDouble(),
				          list.back().toDouble());
				result = qstringFromComplex(c);
			}
		} else if (var.canConvert(QVariant::String)) 
		{
			result = var.toString();
		}
	}
	return result;
}

void MainWindow::clearResultTable()
{
	mModel.clear();
}

void MainWindow::rebuildResultTable()
{
	clearResultTable();
	if (!formulaIsEmpty()) fillResultTable();
}

void MainWindow::appendVariantMap(QStandardItem * parent, const QString& name, const QVariantMap& map)
{
	if (!parent) return;

	QList<QStandardItem *> itemList;

	// insert parent item with title and value, eventually
	QStandardItem * parentItem = new QStandardItem(name);
	QString parentValueKey("value");
	itemList.append(parentItem);
	QVariantMap::const_iterator valueIter = map.constFind(parentValueKey);
	if (valueIter != map.constEnd()) {
		itemList.append(
			new QStandardItem(
				qstringFromVariant(valueIter.value()) ));
	} else {
		itemList.append(new QStandardItem());
	}
	parent->appendRow(itemList);

	// add a row for each compound element
	QVariantMap::const_iterator it = map.constBegin();
	while (it != map.constEnd()) 
	{
		if (parentValueKey != it.key()) // bad thing, but yet no alternative
		{
			appendVariant(parentItem, 
				      tr(it.key().toStdString().c_str()), 
				      it.value());
		}
		++it;
	}
}

void MainWindow::appendVariant(QStandardItem * parent, const QString& name, const QVariant& var)
{
	if (!parent) return;

	if (var.isValid() && var.type() == QVariant::Map)
	{
		appendVariantMap(parent, name, var.toMap());
	} else {
		QList<QStandardItem *> itemList;
		itemList.append(new QStandardItem(name));
		itemList.append(new QStandardItem(
			qstringFromVariant(var) ));
		parent->appendRow(itemList);
	} 
}

void MainWindow::resultAddData(const char * key, bool expand)
{
	if (!key) return;

	appendVariant(mModel.invisibleRootItem(), tr(key), mInputData.get(key));
	if (expand) {
		QStandardItem * root = mModel.invisibleRootItem();
		if (root) {
			int idx = root->rowCount()-1;
			if (idx >= 0) {
				QStandardItem * item = mModel.item(idx);
				if (item) {
					tblResult->expand(item->index());
				}
			}
		}
	}
}

void MainWindow::resultAddInput()
{
	QStandardItem * group = new QStandardItem(tr("Input Form Data"));
	mModel.appendRow(group);
	foreach(FormEntry fe, mFormEntries)
	{
		QStandardItem * item = NULL;
		QList<QStandardItem *> itemList;

		// create a cell for the label (incl. unit suffix)
		QString label(fe.label->text());
		QVariant var(fe.widget->property("suffix"));
		if (var.isValid() && var.canConvert(QVariant::String)) {
			label = label+" ("+var.toString().mid(1)+")";
		}
		item = new QStandardItem(label);
		itemList.append(item);

		// create a cell for the value
		var = mInputData.get(fe.widget->objectName());
		if (var.isValid() && var.canConvert(QVariant::String))
		{
			if ( var.type() == QVariant::Double ) 
			{
				item = new QStandardItem(
					qstringFromVariant(var));
			} else {
				item = new QStandardItem(var.toString());
			}
		} else {
			item = new QStandardItem();
		}

		// add the items (a row) to the model
		itemList.append(item);
		group->appendRow(itemList);
	}
}

void appendSubItems(QString& str, QStandardItem * parent, QItemSelectionModel * selection)
{
	if (!parent || !parent->hasChildren()) return;
	for(int row=0; row < parent->rowCount(); row++) 
	{
		if (/*selection->isSelected(parent->index()) ||*/
		    selection->rowIntersectsSelection(row, parent->index()) )
		{
			QStandardItem * child;
			QString rowString;
			bool appendRow = false;
			for(int col=0; col < parent->columnCount(); col++)
			{
				child = parent->child(row, col);
				if (child && 
				    (selection->isSelected(child->index()) /*||
				     selection->isSelected(parent->index())*/ ))
				{
					QVariant var(child->index().data());
					if (var.canConvert(QVariant::String)) {
						rowString.append(var.toString());
						appendRow = true;
					}
				}
				if (col < parent->columnCount()-1) {
					rowString.append(";");
				}
			}
			rowString.append("\n");
			if (appendRow) str.append(rowString);
		}
		appendSubItems(str, parent->child(row, 0), selection);
	}
}

void MainWindow::copyResultTableSelection()
{
	QString exportData;
	QItemSelectionModel * selection = tblResult->selectionModel();
	QModelIndexList selected(selection->selectedIndexes());
	if (selected.size() == 1) {
		QVariant var(selected.back().data());
		if (var.canConvert(QVariant::String)) {
			exportData.append(var.toString());
		}
	} else {
		appendSubItems(exportData, mModel.invisibleRootItem(), selection);
	}
	mApp->clipboard()->clear();
	mApp->clipboard()->setText(exportData, QClipboard::Clipboard);
	mApp->clipboard()->setText(exportData, QClipboard::Selection);
}

void addModelEntry(QStandardItem * item, 
                   QString label, 
                   QString val1 = QString(), 
                   QString val2 = QString())
{
	if (!item) return;
	QList<QStandardItem *> itemList;
	itemList.append(new QStandardItem(label));
	if (!val1.isEmpty()) itemList.append(new QStandardItem(val1));
	if (!val2.isEmpty()) itemList.append(new QStandardItem(val2));
	item->appendRow(itemList);
}

QString MainWindow::toString(const Element::PropertyVariant& var) const
{
	if (Element::isValidVariant(var)) {
		return QString(boost::apply_visitor(QStringFromBoostVariant(this), var));
	} else {
		return QString(tr("invalid"));
	}
}

void MainWindow::showElementData(const QString& key)
{
//	std::cerr << "key: " << key.toStdString() << std::endl;
	clearResultTable();
	Element::Ptr ep = mDB->getElement(key);
	if (ep.isNull()) {
		cfp::Compound comp = mDB->getAlias(key);
		if (comp.size() > 0) {
			ntrFormula->setEditText(QString::fromStdString(cfp::toString(comp)));
			doCalc();
		} else {
			std::cerr << "MainWindow::showElementData: '"
				  << key.toStdString()
				  <<"' not found in Database !" << std::endl;
		}
		return;
	}
	QStandardItem * root = mModel.invisibleRootItem();
	QStandardItem * item = root;
	QStringFromBoostVariant qstringFromBoostVariant(this);
	Element::PropertyValueIteratorConst it = ep->beginConst();
	int propCount = Element::propertyCount() - 1;
	for(int i=0; i < propCount; i++)
	{
		Element::Property prop = Element::getProperty(i);
		QString valueStr = toString(ep->propertyConst(prop));
		if (prop == Element::NUCLEONS_PROPERTY)
		{
			if (!ep->isIsotope()) {
				valueStr = tr("natural mixture");
			}
		}
		if (prop == Element::NS_L_COHERENT_PROPERTY) // first one
		{
			item = new QStandardItem(tr("neutron scattering"));
			root->appendRow(item);
		}
		addModelEntry(item,
		              tr(Element::propertyName(prop)), valueStr);
	}

	if (ep->isIsotope()) { 
		// has no xray-scattering data, 
		// use that of the natural element (without nucleon number)
		ep = mDB->getElement(ElementDatabase::KeyType(ep->symbol().c_str()));
		if (ep.isNull()) 
		{
			std::cerr << "MainWindow::showElementData: '"
				<< ep->symbol().c_str()
				<<"' not found in Database !" << std::endl;
			return;
		}
	}
	QStringList headerLabels;
	headerLabels << tr("Characteristic") << tr("Value");
	if (!ep->xrayCoefficients().empty())
	{
		item = new QStandardItem(tr("xray scattering"));
		root->appendRow(item);
		addModelEntry(item, tr("anomalous scattering coefficients"));
		addModelEntry(item, tr("energy"), tr("fp"), tr("fpp"));
		MapTriple::const_iterator it = ep->xrayCoefficients().begin();
		MapTriple::const_iterator end = ep->xrayCoefficients().end();
		while(it != end) {
			addModelEntry(item, 
				qstringFromDouble(it->first), 
				qstringFromDouble(it->second.first), 
				qstringFromDouble(it->second.second));
			it++;
		}
		headerLabels << tr("Value");
	}
	mModel.setHorizontalHeaderLabels(headerLabels);
	tblResult->resizeColumnToContents(0);
	tblResult->resizeColumnToContents(1);
	tblResult->resizeColumnToContents(2);
}

void MainWindow::expandAll()
{
	tblResult->expandAll();
	expandItem();
}

void MainWindow::collapseAll()
{
	tblResult->collapseAll();
	expandItem();
}

void MainWindow::expandItem(const QModelIndex& index)
{
	int colWidthSum = 0;
	for(int i=0; i < 3; i++) {
		tblResult->resizeColumnToContents(i);
		colWidthSum += tblResult->columnWidth(i);
	}
	int visibleWidth = tblResult->viewport()->width();

	int scrollBarWidth = 0;
	QScrollBar * vscroll = tblResult->verticalScrollBar();
	if (vscroll) scrollBarWidth = vscroll->sizeHint().width();
	if (!vscroll->isVisible()) visibleWidth -= scrollBarWidth;

	int margin = width() - visibleWidth;
	QSize mainSize(size());

	mainSize.setWidth( colWidthSum + margin );
	colWidthSum -= scrollBarWidth;
	if (colWidthSum != visibleWidth) resize(mainSize);
}

bool MainWindow::formulaIsEmpty() const
{
	return ntrFormula->currentText().isEmpty();
}

