/*
 * src/inputdata.h
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

#ifndef INPUT_DATA_H
#define INPUT_DATA_H

#include <iostream>
#include <QVariant>
#include <QValidator>
#include <cfp/cfp.h>
#include "elementdatabase.h"


class InputData;
namespace std {
	/// Outputs the string representation of the input and calculated data
	/// to an output stream.
	/// \param[out] o Stream to write to.
	/// \param[in] ind Input and calculation data to stringify.
	ostream& operator<<(ostream& o, const InputData& ind);
}

/**
 * Stores GUI input data and performs compound calculations. User input from 
 * the GUI is stored in hash tables with QVariant data type. It is parsed on 
 * request by interpretFormula() which gets detailed chemical element 
 * characteristics from the associated ElementDatabase, calculates all 
 * total and partial compound characteristics and stores the results in the 
 * hash table where it can be retrieved on request by get().
 *
 * Currently, the results are stored with the following keys in a hierarchical
 * structure by use of \e QVariantMap as a container for further \e QVariant types:
 * \code
 * - "number of electrons"                     QVariantMap
 *     - "value"                               total number: QVariant(double)
 *     <one entry per element>                 [chem. symbol, QVariant(double)]
 *
 * - "molecular mass g/mol"                    QVariantMap
 *     - "value"                               total mass: QVariant(double)
 *     - "partial masses g/mol"                QVariantMap
 *         <one entry per element>             [chem. symbol, QVariant(double)]
 *     - "mass ratios %"                       QVariantMap
 *         <one entry per element>             [chem. symbol, QVariant(double)]
 *
 * - "molecular volume nm^3"                   QVariantMap
 *     - "value"                               total volume: QVariant(double)
 *     <one entry per element>                 [chem. symbol, QVariant(double)]
 *
 * - "xray scattering"                         QVariantMap
 *     - "f'"                                  QVariantMap
 *       - "value"                             total 1.deriv scattering coeff.: QVariant(double)
 *       <one entry per element>               [chem. symbol, QVariant(double)]
 *     - "f''"                                 QVariantMap
 *       - "value"                             total 2.deriv scattering coeff.: QVariant(double)
 *       <one entry per element>               [chem. symbol, QVariant(double)]
 *     - "SLD cm^-2"                           QVariantMap
 *       - "value"                             total complex X-Ray SLD: QVariant(complex)
 *       <one entry per element>               [chem. symbol, QVariant(complex)]
 *       
 * - "neutron scattering"                      QVariantMap
 *     - "coherent scattering length 1e-15m"   total complex value: QVariant(complex)
 *     - "SLD coherent 1/cm^2"                 QVariantMap
 *         - "value"                           total complex value: QVariant(complex)
 *         <one entry per element>             [chem. symbol, QVariant(complex)]
 *     - "incoherent scattering length 1e-15m" total complex value: QVariant(complex)
 *     - "SLD incoherent 1/cm^2"               QVariantMap
 *         - "value"                           total complex value: QVariant(complex)
 *         <one entry per element>             [chem. symbol, QVariant(complex)]
 *
 * QVariant(complex) ==> QVariant( QVariantList( QVariant(double), QVariant(double) ))
 * \endcode
 */
class InputData
{
	/// Hash table type for internal data organization.
	typedef QHash<QString, QVariant> QHashType;

	/// Combined type for the chemical element signature from a formula and
	/// the element dataset from the database.
	typedef QPair<cfp::CompoundElement, Element::Ptr> ElemPair;

	/// A List of combined element types. Can be interpreted as a compound 
	/// (from formula parsing) extended by pointers to the according 
	/// database elements.
	typedef QList<ElemPair> CompleteList;
public:
	/// Constructor.
	InputData(ElementDatabase& db);

	/// Adds data.
	/// \param[in] key Name of the entry.
	/// \param[in] var Data to add.
	void set(const QString& key, const QVariant& var);

	/// Adds data to the initial set which is queried at GUI reset.
	/// \param[in] key Name of the entry.
	/// \param[in] var Data to add.
	void setInit(const QString& key, const QVariant& var);

	/// Returns data with the specified key.
	/// \param[in] key Name of the entry.
	/// \returns Data associated to the specified key.
	QVariant get(const QString& key) const;

	/// Returns data with the specified key from the initial set which is
	/// queried at GUI reset.
	/// \param[in] key Name of the entry.
	/// \returns Data associated to the specified key.
	QVariant getInit(const QString& key) const;

	/// Parses the given formula character string. Stores the resulting
	/// compound in the database with the supplied formula as key.
	/// \param[in] formulaKey The formula to interpret.
	void interpretFormula(const QString& formulaKey);

	/// Returns the compound from recent formula parsing.
	const cfp::Compound& empiricalFormula() const;

	/// Adds a new alias to the element database of type 
	/// ElementDatabase.
	/// \param[in] name Alias name.
	void addAlias(const QString& name);

	friend std::ostream& std::operator<<(std::ostream& o, const InputData& ind);
private:
	/// Adds data.
	/// \param[in,out] h A hash table to use for storing data.
	/// \param[in] key Name of the entry.
	/// \param[in] var Data to add.
	void set(QHashType& h, const QString& key, const QVariant& var);

	/// Returns data with the specified key.
	/// \param[in] h A hash table to use for storing data.
	/// \param[in] key Name of the entry.
	/// \returns Data associated to the specified key.
	QVariant get(const QHashType& h, const QString& key) const;

	/// Adds references to database elements to a given compound (from
	/// parsing). For every element in the compound \e comp it queries 
	/// the element database and adds a reference to it in \e list.
	/// If not found, throws ErrorUnknownElement.
	/// \param[out] list List of chemical element signatures combined with
	///             element database references.
	/// \param[in] comp A list of chemical element signatures from parsing
	///            a formula.
	void buildCompleteList(CompleteList& list, const cfp::Compound& comp);

	/// Calculates all information which shall be displayed in the 
	/// main window for a given formula.
	/// \param[in] cl The completely defined formula.
	void calcData(const CompleteList& cl);

	/// Calculates total and partial number of electrons.
	/// \param[in] cl Complete formula.
	/// \param[out] partialElectrons Map with the number of electrons for
	///             each chemical element (its symbol) in the formula.
	double calcElectrons(const CompleteList& cl, 
	                     QVariantMap&        partialElectrons);

	/// Calculates total and partial mass and volume.
	/// \param[in] cl Complete formula.
	/// \param[out] partialVolume Map with the volume fraction for each
	///             chemical element (its symbol) in the formula.
	/// \param[out] totalMass Total mass of the compound.
	/// \param[out] totalVolume Total Volume of the compound.
	void calcMassAndVolume(const CompleteList& cl, 
	                       QVariantMap&        partialVolume,
	                       double&             totalMass,
	                       double&             totalVolume);

	/// Calculates total and partial neutron scattering lengths and densities.
	/// \param[in] cl Complete formula.
	/// \param[in] totalVolume The total volume of the compound.
	/// \param[in,out] neutron A Map which contains all neutron scattering
	///                related data finally.
	/// \param[in] func Member function of Element to retrieve the 
	///            neutron scattering length (coherent or incoherent).
	/// \param[in] slText Key for storing the total neutron scattering length.
	/// \param[in] sldText Key for storing the partial neutron scattering densities.
	void calcNSL(const CompleteList& cl, 
		     double              totalVolume,
	             QVariantMap&        neutron,
	             complex (Element::*func)(void) const,
	             const char        * slText,
                     const char        * sldText);

	/// Calculates total and partial X-Ray scattering coefficients as well
	/// as total and partial X-Ray scattering length densities.
	/// \param[in] cl Complete formula.
	/// \param[in] partialVolumes Partial volume fractions of the compound.
	/// \param[in] partialElectrons Partial number of electrons of the 
	///            compound.
	void calcXrayEnergies(const CompleteList& cl,
	                      const QVariantMap&  partialVolumes,
	                      const QVariantMap&  partialElectrons);

	/// Helper of calcXrayEnergies()
	/// \param[out] fp Xray scattering coefficient (first derivation)
	/// \param[out] fpp Xray scattering coefficient (second derivation)
	/// \param[in] elemIt Iterator to an element in a CompleteList
	///            (to calculate the coefficients for).
	/// \param[in] energy User specified Xray energy.
	/// \returns True, on success. False, if the given energy value is out
	///          of range.
	bool calcXrayCoefficients(double&           fp, 
	                          double&           fpp,
	                          Element::Ptr ep,
	                          double            coeff,
	                          double            energy);

private:
	/// A reference to the element database.
	ElementDatabase::Ptr mDB;
	/// Initial input data (used for reset).
	QHashType                 mInitData;
	/// Input data and intermediate calculation results (buffer).
	QHashType                 mData;
	/// Chemical formula parser.
	cfp::Parser               mFormulaParser;
};

/// The element entered by the user is not found in the database.
class ErrorUnknownElement: public cfp::Error
{
public:
	explicit ErrorUnknownElement(const cfp::CompoundElement& e);
};

#endif

