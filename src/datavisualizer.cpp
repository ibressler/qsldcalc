/*
 * src/datavisualizer.cpp
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

#include <iostream>
#include <QScrollBar>
#include "mainwindow.h"

DataVisualizer::DataVisualizer(MainWindow * parent, ElementDatabase& db)
	: QWidget(parent, Qt::Window),
	  Ui::DataVisualizer(),
	  mDB(&db),
	  mMainWindow(parent)
{
	setupUi(this);
	connect(cbCharacteristic, SIGNAL(activated(int)), this, SLOT(display(int)));
	cbCharacteristic->setCurrentIndex(initialSelection);
}

void
outputRect(const QString& str, const QRectF& r) 
{
	std::cerr << str.toStdString()
		<< " pos: (" << r.x() << "," << r.y() << ") dim: (" 
		<< r.width() << "," << r.height() << ")" << std::endl;
}

/// Prepares PropertyVariant data for sorting before drawing to a
/// graphics scene. Complex values are drawn in a 2-dimensional scene by
/// ignoring their absolute X value. Only the order of the elements is
/// maintained while their Y coordinate keeps its absolute value. Distances of
/// complex-valued elements in X direction are not proporional, i.e. unrealistic.
class PreparePropertyVariantForSorting: public boost::static_visitor<void>
{
public:
	void operator()(int& i) const {}
	void operator()(std::string& s) const {}
	void operator()(double& d) const {}
	/// Special care for std::complex. It is required to be compared by 
	/// X (.real()) values only in drawing related context.
	void operator()(complex& c) const {
		c = complex(c.real(), 0.0);
	}
};

void 
DataVisualizer::display(int index)
{
	if (!mDB || !cbCharacteristic) return;

	int propertyIndex(cbCharacteristic->itemData(index).toInt());
	if (propertyIndex < 0 || propertyIndex > Element::propertyCount())
		return;

	// see if item should be drawn
	Element::Property p(Element::getProperty(propertyIndex));
	Element::PropertyType type(Element::propertyType(p));

	if (type == Element::COMPLEX_TYPE ||
	    type == Element::STRING_TYPE) {
		lblPlotType->setText(tr("[ Horizontal distances are <b>not</b> proportional to the values. ]"));
	} else {
		lblPlotType->setText(tr("[ Horizontal distances are proportional to the values. ]"));
	}

	// reset graphics scene
	SceneType scene(new QGraphicsScene());
	SceneType oldScene(gvDisplay->scene());
	gvDisplay->setScene(scene);
	if (oldScene) delete oldScene;

	ElementDatabase::Iterator begin = mDB->begin();
	ElementDatabase::Iterator end = mDB->end();
	ElementDatabase::Iterator it = begin;
	// copy elements to sorted container first
	typedef QMultiMap<Element::PropertyVariant, Element::Ptr> ElementMap;
	ElementMap elemMap;
	for(; it != end; it++) 
	{
		Element::Ptr elem(it.value());
		if (elem.isNull()) continue;
		Element::PropertyVariant prop = elem->propertyConst(p);
		boost::apply_visitor(PreparePropertyVariantForSorting(), prop);
		elemMap.insert(prop, elem);
	}

	// finally draw the elements
	qreal lastXPos = -1e10; // need an arbitrary small number
	ElementMap::const_iterator mapIter = elemMap.begin();
	for(; mapIter != elemMap.end(); mapIter++) 
	{
		draw(scene, lastXPos, mapIter.value(), propertyIndex);
	}
	// get scene bounding box, add a margin
	QRectF sceneBoundingBox(scene->itemsBoundingRect());
	sceneBoundingBox.setWidth(sceneBoundingBox.width()+2*itemMargin);
	sceneBoundingBox.setHeight(sceneBoundingBox.height()+2*itemMargin);
	sceneBoundingBox.translate(-1.0*itemMargin, -1.0*itemMargin);
	scene->setSceneRect(sceneBoundingBox);
	gvDisplay->show();
	adjustSize();
}

/**
 * Calculates the position of this data element (variant data type) 
 * within the graphics scene.
 */
class DrawingPositionFromPropertyVariant: public boost::static_visitor<QPointF>
{
	static const qreal mScale = 50.0;
public:
	QPointF operator()(const int& i) const {
		return QPointF(qreal(i)*mScale, 0.0);
	}
	QPointF operator()(const std::string& s) const {
		return QPointF(0.0, 0.0);
		// using the operator for (const int&)
//		return operator()(QString::fromStdString(s).toLower().at(0).toAscii()-('a'));
	}
	QPointF operator()(const double& d) const {
		return QPointF(qreal(d)*mScale, 0.0);
	}
	QPointF operator()(const complex& c) const {
		return QPointF(qreal(c.real())*mScale, 
			       qreal(c.imag())*mScale*-1.0);
	}
};

void 
DataVisualizer::draw(SceneType scene, qreal& lastXPos, Element::Ptr e, int propertyIndex)
{
	if (e.isNull()) return;

	// see if item should be drawn
	Element::Property p(Element::getProperty(propertyIndex));
	Element::PropertyType type(Element::propertyType(p));
	Element::PropertyVariant var(e->propertyConst(p));

	if (p == Element::NUCLEONS_PROPERTY && !e->isIsotope()) return;
	if (!Element::isValidVariant(var)) return;

	// build the text item
	QGraphicsItem * item(0);
	QGraphicsTextItem * text = new QGraphicsTextItem();
	text->setHtml( getLinkText(*e) + "<br>" +
		mMainWindow->toString(var));
	text->setTextInteractionFlags(Qt::TextBrowserInteraction);
	text->setToolTip(tr("formattedFormulaToolTip"));
	connect(text, SIGNAL(linkActivated(const QString&)), 
		this, SLOT(emitElementLinkActivated(const QString&)));
	item = text;

	// build the encapsulating box item
	QGraphicsRectItem * rect = new QGraphicsRectItem(
		item->boundingRect() );
	rect->setBrush(QBrush(QColor(196, 255, 196)));

	// combine both and add them to the scene
	item->setParentItem(rect);
	scene->addItem(rect);

	QPointF pos(boost::apply_visitor(DrawingPositionFromPropertyVariant(), var));
	if (type == Element::COMPLEX_TYPE ||
	    type == Element::STRING_TYPE) {
		// no stacking for 2D drawing
		// item positions are stretched in X direction
		if (lastXPos > pos.x()) {
		       	pos.setX(lastXPos);
		}
		rect->setPos(pos);
		// update drawing position for the next item
		lastXPos = pos.x() + rect->boundingRect().width() + itemMargin;
	} else {
		setItemPos(rect, pos);
	}
}

void 
DataVisualizer::adjustSize(void)
{
	// determine the current difference between window size and viewport size
	int margin = size().height() - gvDisplay->viewport()->size().height();
	int scrollBarWidth = 0;
	// get scrollbar height offset
	QScrollBar * vscroll = gvDisplay->horizontalScrollBar();
	if (vscroll) scrollBarWidth = vscroll->sizeHint().height();
	margin += scrollBarWidth;

	QSize newSize(size());
	// update height only
	newSize.setHeight(int(gvDisplay->sceneRect().height()) + margin);
	// resize nescessary ?
	if (size().height() != newSize.height()) {
		resize(newSize);
	}
}

void 
DataVisualizer::setItemPos(QGraphicsItem * item, QPointF pos)
{
	typedef QList<QGraphicsItem *> QGraphicsItemList;
	SceneType scene(gvDisplay->scene());
	if (!item || !scene) return;
	// set the desired position
	item->setPos(pos);
	// check for collisions with other items
	QList<QGraphicsItem *> collides = scene->collidingItems(item);
	if (collides.size() == 0) return;
	QGraphicsItem * cItem = 0;
	QGraphicsItemList::const_iterator it = collides.constBegin();
	while(it != collides.constEnd()) {
		cItem = *it;
		if (cItem && !cItem->parentItem()) break;
		it++;
	}
	if (!cItem || cItem->parentItem()) return;
	// item has no parent => top-level, bounding rect
	qreal newYPos = cItem->y() + 
			0.5*cItem->boundingRect().height() + 
			0.5*item->boundingRect().height() +
			itemMargin;
	QPointF newPos(pos);
	newPos.setY(newYPos);
	setItemPos(item, newPos);
}

void 
DataVisualizer::show()
{
	QWidget::show();
	selectIndex(initialSelection);
}

void 
DataVisualizer::selectIndex(int index)
{
	cbCharacteristic->setCurrentIndex(index);
	display(index);
}

void 
DataVisualizer::emitElementLinkActivated(const QString& link)
{
	emit elementLinkActivated(link);
}

QString 
DataVisualizer::getLinkText(const cfp::ChemicalElementInterface& e)
{
	QString s;
	s.append("<style>a{text-decoration:none;}</style><a href=\"");
	s.append(ElementDatabase::makeKey(e));
	s.append("\">");
	s.append(e.toMarkup().c_str());
	s.append("</a>");
	return s;
}

void 
DataVisualizer::retranslateUi()
{
	Ui::DataVisualizer::retranslateUi(this);

	int selectedIndex = cbCharacteristic->currentIndex();
	// remove all characteristics items first, add the retranslated again
	cbCharacteristic->clear();
	int count = Element::propertyCount()-1;
	for(int i=0; i < count; i++)
	{
		cbCharacteristic->insertItem(
			cbCharacteristic->count(), 
			tr(Element::propertyName(Element::getProperty(i))), 
			QVariant(i));
	}

	// redisplay the data
	selectIndex(selectedIndex);
}

