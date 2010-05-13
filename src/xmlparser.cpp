/*
 * src/xmlparser.cpp
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

#include <QFile>
#include "xmlparser.h"

#define DBG 0
#define STD(a) (a).toAscii().data()
#define STDR(a) STD((a).toString())

XmlParser::XmlParser()
	: mHandlers(11, &XmlParser::handleNoToken)
{
	mHandlers.at(QXmlStreamReader::Invalid) = &XmlParser::handleInvalid;
	mHandlers.at(QXmlStreamReader::StartDocument) = &XmlParser::handleStartDocument;
	mHandlers.at(QXmlStreamReader::EndDocument) = &XmlParser::handleEndDocument;
	mHandlers.at(QXmlStreamReader::StartElement) = &XmlParser::handleStartElement;
	mHandlers.at(QXmlStreamReader::EndElement) = &XmlParser::handleEndElement;
	mHandlers.at(QXmlStreamReader::Characters) = &XmlParser::handleCharacters;
	mHandlers.at(QXmlStreamReader::DTD) = &XmlParser::handleDTD;
}

XmlParser::~XmlParser()
{
}

void XmlParser::handleNoToken(void)
{
	if (DBG) std::cerr << "NoToken" << std::endl;
}

void XmlParser::handleInvalid(void)
{
	if (DBG) std::cerr << "Invalid" << std::endl;
}

void XmlParser::handleStartDocument(void)
{
	if (DBG) std::cerr << "StartDocument" << std::endl;
	if (mXml.documentEncoding() != "utf-8")
		mXml.raiseError("Document encoding is not UTF-8!");
}

void XmlParser::handleEndDocument(void)
{
	if (DBG) std::cerr << "EndDocument" << std::endl;
}

#define ATTR(xml, name, type) (xml).attributes().value( #name ).toString().to ##type ()

void XmlParser::handleStartElement(void)
{
	if (DBG) std::cerr << "StartElement";
	if (DBG) std::cerr << ", name: '" << STDR(mXml.name()) << "'" << std::endl;
	switch (mCurSection) {
		case IGNORED_SECTION:
			if (mXml.name() == "chemical_element") 
			{
				mCurSection = ELEMENT_CONFIG_SECTION;
				mElement = new Element();
				if (!mElement.isNull())
					mElement->setProperty(
						Element::SYMBOL_PROPERTY,
						ATTR(mXml, symbol, StdString)
					);
			}
			break;
		case ELEMENT_CONFIG_SECTION:
			if (mElement.isNull()) break;
			if (mXml.name() == "name") {
				mElement->setProperty(
					Element::NAME_PROPERTY,
					ATTR(mXml, val, StdString)
				);
			} else if (mXml.name() == "abundance") {
				mElement->setProperty(
					Element::ABUNDANCE_PROPERTY,
					ATTR(mXml, val, Double)
				);
			} else if (mXml.name() == "atomic_weight") {
				mElement->setProperty(
					Element::ATOMIC_MASS_PROPERTY,
					ATTR(mXml, val, Double)
				);
			} else if (mXml.name() == "nucleons") {
				mElement->setProperty(
					Element::NUCLEONS_PROPERTY,
					ATTR(mXml, val, Int)
				);
			} else if (mXml.name() == "electrons") {
				mElement->setProperty(
					Element::ELECTRONS_PROPERTY,
					ATTR(mXml, val, Int)
				);
			} else if (mXml.name() == "neutron_scattering_length") {
				mCurSection = NS_L_SECTION;
			} else if (mXml.name() == "neutron_scattering_cross_section") {
				mCurSection = NS_C_SECTION;
			} else if (mXml.name() == "xray_scattering_anomalous_coefficients") {
				mCurSection = XRAY_COEFFICIENTS_SECTION;
			}
			break;
		case NS_L_SECTION:
			if (mElement.isNull()) break;
			if (mXml.name() == "coherent") 
			{
				mElement->setProperty(
					Element::NS_L_COHERENT_PROPERTY, 
					complex(
						ATTR(mXml, re, Double),
						ATTR(mXml, im, Double)
				));
			} else if (mXml.name() == "incoherent") 
			{
				mElement->setProperty(
					Element::NS_L_INCOHERENT_PROPERTY, 
					complex(
						ATTR(mXml, re, Double), 
						ATTR(mXml, im, Double)
				));
			}
			break;
		case NS_C_SECTION:
			if (mElement.isNull()) break;
			if (mXml.name() == "coherent") {
				mElement->setProperty(
					Element::NS_CS_COHERENT_PROPERTY,
					ATTR(mXml, re, Double)); 
			} else if (mXml.name() == "incoherent") {
				mElement->setProperty(
					Element::NS_CS_INCOHERENT_PROPERTY,
					ATTR(mXml, re, Double));
			} else if (mXml.name() == "total") {
				mElement->setProperty(
					Element::NS_CS_TOTAL_PROPERTY,
					ATTR(mXml, val, Double) );
			} else if (mXml.name() == "absorption") {
				mElement->setProperty(
					Element::NS_CS_ABSORPTION_PROPERTY,
					ATTR(mXml, val, Double) );
			}
			break;
		case XRAY_COEFFICIENTS_SECTION:
			if (mElement.isNull()) break;
			if (mXml.name() == "ev") {
				mElement->addXrayCoefficient(
					ATTR(mXml, val, Double),
					ATTR(mXml, fp, Double),
					ATTR(mXml, fpp, Double) );
			}
			break;
		default:
			break;
	}
}

void XmlParser::handleEndElement(void)
{
	if (DBG) std::cerr << "EndElement";
	if (DBG) std::cerr << ", name: '" << STDR(mXml.name()) << "'" << std::endl;
	if (mXml.name() == "chemical_element") {
		mCurSection = IGNORED_SECTION;
		if (!mElement->isValid()) {
			mXml.raiseError("Element not valid!");
		} else {
			mResultList.push_back(mElement);
			mElement = NULL; // forget about this element now
		}
	} else
	if (mXml.name() == "chemical_element_list" && 
	    mCurSection != IGNORED_SECTION)
	{
		mXml.raiseError("Syntax error, element configuration not finished!");
	} else
	if (mXml.name() == "neutron_scattering_length" ||
	    mXml.name() == "neutron_scattering_cross_section")
	{
		mCurSection = ELEMENT_CONFIG_SECTION;
	}
}

void XmlParser::handleCharacters(void)
{
	if (!mXml.isWhitespace()) {
		mXml.raiseError("Unexpected Characters!");
	}
}

void XmlParser::handleDTD(void)
{
	if (mXml.dtdName() != "chemical_element_list")
	{
		mXml.raiseError(QString("Unexpected DTD name '%1'").arg(
			mXml.dtdName().toString()) + 
			QString(" (expected 'chemical_element_list')!"));
	}
}

const XmlParser::ElementPtrList& XmlParser::read(const QString& filename)
{
	if (!QFile::exists(filename)) 
		std::cerr << "XmlParser::read, Internal Error, "
			<< "Supplied filename does not exist! (" 
			<< STD(filename) <<")"<< std::endl;

	QFile xmlFile(filename);
	if ( !xmlFile.open(QIODevice::ReadOnly) ) {
		mXml.raiseError("Could not open xml file !");
	}
	mXml.setDevice(&xmlFile);

	// parsedata init
	mCurSection = IGNORED_SECTION;
	mResultList.clear();

	while(!mXml.atEnd())
	{
		QXmlStreamReader::TokenType tokenType = mXml.readNext();
		(this->*mHandlers.at(tokenType))();
	}
	if (mCurSection != IGNORED_SECTION) {
		mXml.raiseError("Invalid Element at end of file!");
	}
	if (mXml.error()) {
		std::cerr << "XmlParser::read, Internal Error, ";
		std::cerr << "A XML parse error occured: '";
		std::cerr << STD(mXml.errorString()) << "'" << std::endl;
		std::cerr << "file '"<< STD(xmlFile.fileName());
		std::cerr << ", line: " << mXml.lineNumber()-1;
		std::cerr << ", position: " << mXml.columnNumber();
		std::cerr << std::endl;
		// delete dangling memory
		delete mElement;
		foreach(Element::Ptr ep, mResultList) {
			if (!ep.isNull()) delete ep;
		}
		mResultList.clear();
	}

	mElement = NULL; // forget the last element created (should be NULL already)
	return mResultList;
}


