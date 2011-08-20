/*
 * src/elementdatabase.cpp
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

#include <QTime>
#include <QDir>
#include "elementdatabase.h"
#include "xmlparser.h"

ElementDatabase::~ElementDatabase()
{
	foreach(Element::Ptr ep, mElementHash) {
		delete ep;
	}
}

void 
ElementDatabase::addFromFile(const QString& fn)
{
	XmlParser p;
	foreach(Element::Ptr ep, p.read(fn)) {
		if (ep.isNull() || !ep->isValid()) return;
		mElementHash.insert(makeKey(*ep), ep);
	}
}

void 
ElementDatabase::addFromDirectory(const QString& path)
{
	QTime timer;
	timer.start();
        QDir dir(path);
        QStringList nameFilters;
        nameFilters << "*.xml";
        QStringList dirList = dir.entryList(nameFilters);
        foreach(const QString file, dirList) {
		addFromFile(path + "/" + file);
        }
#if DEBUG
	std::cerr << "element data directory read time: " 
		<< timer.elapsed() << "ms" << std::endl;
#endif
}

const 
ElementDatabase::KeyType ElementDatabase::makeKey(const cfp::ChemicalElementInterface& e)
{
	KeyType key(e.uniqueName().c_str());
	return key;
}

Element::Ptr 
ElementDatabase::getElement(const KeyType& key)
{
	return mElementHash.value(key);
}

Element::Ptr 
ElementDatabase::getElement(const cfp::ChemicalElementInterface& e)
{
	return mElementHash.value(makeKey(e));
}

QStringList 
ElementDatabase::getSymbolList() const
{
	QStringList symList;
	foreach(Element::Ptr ep, mElementHash) {
		symList << ep->uniqueName().c_str();
	}
	symList.sort();
	return symList;
}

std::ostream& 
operator<<(std::ostream& o, const ElementDatabase& db)
{
	o << "elements("<<db.mElementHash.size()<<"): ";
	foreach(QString str, db.getSymbolList()) {
		o << str.toStdString() << " ";
	}
	return o;
}

ElementDatabase::Iterator 
ElementDatabase::begin() const
{
	return mElementHash.constBegin();
}

ElementDatabase::Iterator 
ElementDatabase::end() const
{
	return mElementHash.constEnd();
}

void 
ElementDatabase::addAlias(const KeyType& key, const cfp::Compound& compound)
{
	mAliasHash.insert(key, compound);
}

const cfp::Compound 
ElementDatabase::getAlias(const KeyType& key) const
{
	return mAliasHash.value(key);
}

const cfp::Compound 
ElementDatabase::getAlias(const cfp::ChemicalElementInterface& e) const
{
	return mAliasHash.value(makeKey(e));
}

