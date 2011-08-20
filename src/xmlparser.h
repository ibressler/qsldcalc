/*
 * src/xmlparser.h
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

#ifndef XML_PARSER
#define XML_PARSER

#include <QXmlStreamReader>
#include "element.h"

/**
 * Parses real-world element description files.
 *
 * \anchor dtd
 * The Document Type Definition for those element data files follows:
 * \include chemical_elements.dtd
 */
class XmlParser
{
private:
	/// Prototype of a function to handle all the XML tokens a
	/// QXmlStreamReader may encounter. Those are defined in 
	/// <em>enum QXmlStreamReader::TokenType</em>. 
	typedef void (XmlParser::*XmlTokenHandler) (void);
public:
	/// List of guarded pointers to database elements.
	typedef std::list<Element::Ptr> ElementPtrList;
public:
	XmlParser();  //!< Default constructor.
	~XmlParser(); //!< Destructor.

	/// Reads decriptions of elements from file.
	/// \param[in] filename Filename of a XML file containing one or more
	///            descriptions of real-world elements. This file has to 
	///            be valid according to the \refDTD above.
	/// \returns A list of all new elements extracted from the specified
	///          file.
	const ElementPtrList& read(const QString& filename);
private:
	/// Handles \e QXmlStreamReader::NoToken, \e QXmlStreamReader::Comment,
	/// \e QXmlStreamReader::EntityReference and \e QXmlStreamReader::ProcessingInstruction.
	/// Does nothing.
	void handleNoToken(void);

	/// Handles \e QXmlStreamReader::Invalid. Does nothing.
	void handleInvalid(void);

	/// Handles \e QXmlStreamReader::StartDocument. 
	/// Tests for correct document encoding (UTF-8).
	void handleStartDocument(void);

	/// Handles \e QXmlStreamReader::EndDocument. Does nothing.
	void handleEndDocument(void);

	/// Handles \e QXmlStreamReader::StartElement. Creates and configures
	/// an chemical element eventually. This depends on the name and
	/// attributes of the xml element as well as the current section for
	/// this configuration. \sa SectionType
	void handleStartElement(void);

	/// Handles \e QXmlStreamReader::EndDocument. Updates the current
	/// configuration section and tests the recently configured chemical
	/// element for validity. \sa SectionType
	void handleEndElement(void);

	/// Handles \e QXmlStreamReader::Characters. Raises an error if there
	/// are characters except whitspace. The current \refDTD does not 
	/// allow this.
	void handleCharacters(void);

	/// Handles \e QXmlStreamReader::DTD. Tests the current DTD name and
	/// raises an error if it is unknown.
	void handleDTD(void);
private:
	/// Descibes the current element configuration state.
	typedef enum {
		/// Outside the element configuration.
		IGNORED_SECTION,
		/// Inside the element configuration.
		ELEMENT_CONFIG_SECTION,
		/// Neutron scattering length parameter within 
		/// the element configuration.
		NS_L_SECTION, 
		/// Neutron scattering cross section parameter within 
		/// the element configuration.
		NS_C_SECTION,
		/// Xray scattering factors within 
		/// the element configuration.
		XRAY_COEFFICIENTS_SECTION
	} SectionType;

	/// The actual Parser.
	QXmlStreamReader             mXml;
	/// Contains all functions to handle all possible \e QXmlStreamReader::TokenType.
	std::vector<XmlTokenHandler> mHandlers;
	/// The last section processed.
	SectionType                  mCurSection;
	/// The current Element being configured.
	Element::Ptr            mElement;
	/// All Element Objects created by the current read() call.
	ElementPtrList               mResultList;
};

#endif

