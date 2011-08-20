/*
 * src/elementdatabase.h
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

#ifndef EDB_ELEMENTDATABASE_H
#define EDB_ELEMENTDATABASE_H

#include <QHash>
#include <iostream>
#include "element.h"

class ElementDatabase;

/// Outputs the string representation of an element database to an
/// output stream.
/// \param[out] o Stream to write to.
/// \param[in] db Element database to stringify.
std::ostream& operator<<(std::ostream& o, const ElementDatabase& db);

/**
 * Contains all chemical elements currently known by this program. It
 * stores the alias names for element compounds separately because an
 * ordinary database element can not store the associated
 * cfp::Compound (for aliases). 
 *
 * The element datasets are not sorted in any way but can be retrieved
 * in constant time complexity.
 *
 * \todo Combine single chemical elements and aliases (cfp::Compound)
 * into a single data type. Pro: aliases could be easily visualized in
 * the same way as regular elements. Con: Additional (unused) data
 * fields for ordinary chemical elements (\b minor) and all the
 * properties of a single element have to be calculated for compounds
 * too: At alias creation time or at access time (\b major, regarding
 * visualization).
 *
 * \todo Add a preconfigured list of aliases, similar to
 * http://www.chemcalc.org .
 *
 * \todo Save user-defined aliases to a file in the users
 * home-directory to restore them after restart of the application.
 */
class ElementDatabase: public QObject
{
public:
	/// Guarded pointer to this database.
	typedef QPointer<ElementDatabase> Ptr;
	/// Key data type by which all elements are identified within 
	/// the database. The chemical element symbol name. Isotopes 
	/// have the nucleon number prepended.
	typedef QString KeyType;
private:
	/// The hash table type used for chemical elements.
	typedef QHash<KeyType, Element::Ptr> ElementHash;
	/// The hash table type used for aliases of compunds.
	typedef QHash<KeyType, cfp::Compound> AliasHash;
public:
	/// An iterator over the whole element database.
	typedef ElementHash::const_iterator Iterator;
public:
	/// Cleanup, frees internal data.
	~ElementDatabase();

	/// Adds all elements within a single file to the database.
	/// This file must follow the syntax defined and used by XmlParser.
	/// \param[in] fn Filename of the XML file containing chemical
	///            element definitions.
	void addFromFile(const QString& fn);

	/// Adds all elements from all XML files within the specified
	/// directory to the database.
	/// \param[in] path Full path to a directory which contains
	///            XML files with chemical element definitions.
	void addFromDirectory(const QString& path);

	/// Retrieves an element dataset with the specified key from
	/// the database.
	Element::Ptr getElement(const KeyType& key);

	/// Retrieves an element dataset for the specified chemical
	/// element signature from the database.
	Element::Ptr getElement(const cfp::ChemicalElementInterface& e);

	/// Generates a symbol list of all available elements in the
	/// database.
	/// \returns A list of all available symbols according to KeyType.
	QStringList getSymbolList() const; // for autocompletion

	/// Returns an iterator to begin with when iterating over all elements.
	Iterator begin() const;

	/// Returns an iterator to end with when interating over all elements.
	Iterator end() const;

	/// Generates a key based on the given chemical element
	/// signature for retrieval of database elements.
	/// \param[in] e Chemical element signature to generate a key for.
	static const KeyType makeKey(const cfp::ChemicalElementInterface& e);

	/// Adds an alias to the database.
	/// \param[in] key The alias name.
	/// \param[in] compound The compound which is identified by the alias.
	void addAlias(const KeyType& key, const cfp::Compound& compound);

	/// Retrieves a compound by the specified alias from the database.
	const cfp::Compound getAlias(const KeyType& key) const;

	/// Retrieves a compound by the specified chemical
	/// element signature from the database.
	const cfp::Compound getAlias(const cfp::ChemicalElementInterface& e) const;

	friend std::ostream& operator<<(std::ostream& o, const ElementDatabase& db);
private:
	ElementHash mElementHash; //!< Hash table for chemical element datasets.
	AliasHash   mAliasHash;   //!< Hash table for compound aliases.
};

#endif
