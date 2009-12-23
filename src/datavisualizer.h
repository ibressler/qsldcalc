/*
 * src/datavisualizer.h
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

#ifndef DATA_VISUALIZER_H
#define DATA_VISUALIZER_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItem>
#include "ui_datavisualizer.h"
#include "elementdatabase.h"

class MainWindow;

/**
 * Displays all known chemical elements in a two-dimensional viewport
 * organized by one of their property, selected by the user.
 */
class DataVisualizer: public QWidget, private Ui::DataVisualizer
{
	Q_OBJECT

	typedef QPointer<QGraphicsScene> SceneType; //!< Shortcut.
	typedef QPointer<QGraphicsView> ViewType;   //!< Shortcut.

public:
	/// Constructor.
	/// \param[in,out] parent Parent window (main window).
	/// \param[in,out] db Database of all known chemical elements. Uses
	///                read access only. Because of QPointer it's
	///                read/write formally.
	DataVisualizer(MainWindow * parent, edb::ElementDatabase& db);

	/// Generate a link text for the specified element to display detailed
	/// element data in the mainwindow.
	///
	/// Example for H4:
	/// \code
	/// <style>a{text-decoration:none;}</style><a href="H">H<sub>4</sub></a>
	/// \endcode
	///
	/// Example for O:
	/// \code
	/// <style>a{text-decoration:none;}</style><a href="O">O</a>
	/// \endcode
	///
	/// \param[in] e The chemical element to generate a link for.
	/// \returns The link text.
	/// \see MainWindow::showElementData(const QString& key)
	static QString getLinkText(const cfp::ChemicalElementInterface& e);

	/// Retranslates this subwindow at runtime.
	void retranslateUi();
public slots:
	/// Shows this data visualization window to the user and selects the
	/// first property to display initially.
	void show();
signals:
	/// Emits a link text of the element whose detailed information should
	/// be shown in the main window and which was previously clicked at by the
	/// user.
	void elementLinkActivated(const QString& link);
private slots:
	/// Displays and draws all chemical elements sorted by their property
	/// specified by \e index.
	/// \param[in] index Property to sort all chemical elements by.
	/// \see edb::Element::getProperty(int index)
	void display(int index);

	/// Signal forwarder to display element details in the mainwindow. Is
	/// called by every single item drawn in the graphics scene.
	/// \param[in] link The link text.
	/// \see getLinkText()
	void emitElementLinkActivated(const QString& link);
private:
	/// Draws the selected property of an element to the scene in a way
	/// that it doesn't overlap with other items.
	/// \param[in,out] scene The QGraphicsScene to draw to.
	/// \param[in,out] lastXPos Position in x direction of the last
	///                element drawed. Assumes, the items are sorted by
	///                the selected property in ascending order.
	/// \param[in] e The Element whose property should be drawn.
	/// \param[in] propertyIndex The index of the element property to
	///            draw. \see edb::Element::getProperty(int index)
	void draw(SceneType scene, qreal& lastXPos, edb::Element::Ptr e, int propertyIndex);

	/// Sets the position of an item in a graphics scene in a way it
	/// doesn't collide or overlap with other items.
	/// \param[in,out] item The item to position in the scene.
	/// \param[in] pos The desired position of the item. To avoid
	///            overlapping, it is changed in vertical direction (y) only. 
	void setItemPos(QGraphicsItem * item, QPointF pos);

	/// Selects another Element characteristic and rebuilds the graphics scene.
	/// \param[in] index The index of the element property to draw.
	///            \see edb::Element::getProperty(int index)
	void selectIndex(int index);

	/// Adjusts the height of the data visualization window to show
	/// as much data elements as possible.
	void adjustSize(void);
private:
	/// A link to the data base with all known chemical elements
	edb::ElementDatabase::Ptr mDB;
	/// Margin around items to not contact each other or the graphics view
	/// boundary.
	static const qreal itemMargin = 5.0;
	/// Initially selected element characteristic to show.
	static const int initialSelection = 0;
	/// The parent window.
	MainWindow * mMainWindow;
};

#endif
