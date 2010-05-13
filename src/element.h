/*
 * src/element.h
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

#ifndef EDB_ELEMENT_H
#define EDB_ELEMENT_H

#include <complex>
#include <vector>
#include <map>
#include <iostream> // remove me
#include <QPointer>
#include <cfp/cfp.h>
#include <boost/variant.hpp>

/// Complex numbers in floating point representation.
typedef std::complex<double> complex;

/// A tuple of floating point numbers
typedef std::pair<double, double> DoublePair;

/// A Map of floating point numbers used to store the Xray
/// scattering factors indexed by their energy.
/// \sa Element::xrayCoefficients()
typedef std::map<double, DoublePair> MapTriple;

/**
 * A real-world chemical element. It is used to store all relevant
 * characteristics and provide dynamic, i.e. automated, access to 
 * them. It is compatible to cfp::ChemicalElement but stores its data
 * in a different manner: a container of variants (PropertyVariant).
 * boost::variant is used to accomplish this.
 *
 * <b>Data sources</b>:
 * - For neutron scattering length and cross sections, the data is
 *   taken from http://www.ncnr.nist.gov/resources/n-lengths/list.html
 *   which references <em>the Special Feature section of neutron
 *   scattering lengths and cross sections of the elements and their
 *   isotopes in Neutron News, Vol. 3, No. 3, 1992, pp. 29-37</em>.
 * - \anchor xrayFactors For X-Ray scattering factors, data is taken from 
 *   http://skuld.bmsc.washington.edu/scatter/AS_periodic.html
 *   Which states: <em>
 *   The scattering factor data files indexed through the table above
 *   were calculated using the subroutine library by Brennan and Cowan
 *   [...]. The values for f' and f" are derived using the
 *   theoretical approximation developed by Cromer and Liberman. This
 *   theory gives accurate values far from an absorption edge but does
 *   not account for the effects of neighboring atoms, which can be
 *   very substantial near an absorption edge. Before conducting an
 *   anomalous scattering experiment close to an absorption edge it is
 *   therefore advisable to determine the actual scattering behaviour
 *   of your sample [...].</em>
 *
 * \todo An alternative for internal value handling is QVariant. 
 * Possible advantage: More convenient use, especially regarding value
 * retrieval. The visitor concept of boost::variant requires to 
 * introduce funtional objects for accessing or modifying its data. 
 * That increases code complexity slightly and is more difficult to 
 * read and understand afterwards. The core motivation for
 * boost::variant was to maintain independence from Qt to allow for
 * outsourcing the database code into a separate library (may be
 * useful for other programs too).
 * \todo Wrap the internal data representation (Element::PropertyVariant) into
 * a separate class. Eventually, the type identification code too
 * (Element::Property and Element::PropertyType).
 */
class Element: public cfp::ChemicalElementInterface, public QObject
{
public:
	/// A guarded Pointer to such an Element.
	typedef QPointer<Element> Ptr;

	/// All properties of an Element in the database.
	typedef enum {
		SYMBOL_PROPERTY,          //!< Chemical symbol.
		NAME_PROPERTY,            //!< Full name.
		/// Number of nucleons (aka atomic mass number \e A).
		NUCLEONS_PROPERTY,
		/// Number of electrons (aka atomic number \e Z).
		ELECTRONS_PROPERTY,
		/// The atomic mass.
		ATOMIC_MASS_PROPERTY,
		/// The natural abundance in the local environment of 
		/// the Earth's crust and atmosphere.
		ABUNDANCE_PROPERTY,
		/// Bound coherent neutron scattering length.
		NS_L_COHERENT_PROPERTY,
		/// Bound incoherent neutron scattering length.
		NS_L_INCOHERENT_PROPERTY,
		/// Bound coherent neutron scattering cross section.
		NS_CS_COHERENT_PROPERTY,
		/// Bound incoherent neutron scattering cross section.
		NS_CS_INCOHERENT_PROPERTY,
		/// Total bound scattering cross section.
		NS_CS_TOTAL_PROPERTY,
		/// Absorption cross section for 2200 m/s neutrons
		NS_CS_ABSORPTION_PROPERTY,
		/// X-Ray Scattering factors as a function of energy.
		/// <em>Not used at the moment.</em>
		XRAY_ENERGY_PROPERTY,
		/// Invalid property.
		INVALID_PROPERTY
	} Property;
	/// The data type of a property to allow for convenient
	/// testing at runtime.
	typedef enum {
		INT_TYPE,     //!< Integer.
		STRING_TYPE,  //!< Character string.
		DOUBLE_TYPE,  //!< Floating point number.
		COMPLEX_TYPE, //!< Complex number with floating point.
		INVALID_TYPE  //!< Invalid.
	} PropertyType; // has to have to same order as PropertyVariant, see propertyType()
	/// Data type of a single property value.
	typedef boost::variant<int, std::string, double, complex> PropertyVariant;
	/// Container for all defined properties.
	typedef std::vector<Property> PropertyVector;
	/// Container for all defined property values, the data.
	typedef std::vector<PropertyVariant> PropertyVariantVector;
	typedef PropertyVariantVector::const_iterator PropertyValueIteratorConst;
	typedef PropertyVariantVector::iterator PropertyValueIterator;
	/// Default value for properties with invalid type.
	/// \sa PropertyType
	static const int INVALID_PROPERTY_VALUE;
public:
	/// Initializes all properties of this Element.
	/// Sets all properties to valid types and values accordingly.
	/// The values are valid regarding their Element::PropertyType but may
	/// not be valid from a physical point of view.
	/// \sa isValid()
	Element();

	~Element(); //!< Releases all memory.

	/// Returns the overall number of available properties.
	static int propertyCount();

	/// Returns the Property with the specified index.
	static Property getProperty(int index);

	/// Tests a given property value for invalid type.
	/// \sa INVALID_TYPE, INVALID_PROPERTY_VALUE
	static bool isValidVariant(const PropertyVariant& var);

	/// Returns the name of a given property. It is used to 
	/// display a name in the GUI (after translation by Qt).
	static const char * propertyName(Property p);

	/// Returns the data type of a given property.
	static PropertyType propertyType(Property p);

	/// Tests if the given property value matches the given property.
	static bool isValidType(Property p, const PropertyVariant& var);

	/// Returns a read-only iterator to the first of all property values.
	PropertyValueIteratorConst beginConst() const;

	/// Returns a read-only iterator which points behind the last
	/// property value.
	PropertyValueIteratorConst endConst() const;

	/// Returns the Property of the specified property value
	/// iterator created with beginConst() or endConst();
	Property getProperty(const PropertyValueIteratorConst& iter) const;

	/// Returns the property value associated with the specified property.
	const PropertyVariant& propertyConst(Property p) const;

	/// Sets the specified property to the provided value.
	template<typename T>
	bool setProperty(Property p, const T& value);

	double electrons() const;     //!< \conv \see ELECTRONS_PROPERTY
	double atomicMass() const;    //!< \conv \see ATOMIC_MASS_PROPERTY
	complex nslCoherent() const;  //!< \conv \see NS_L_COHERENT_PROPERTY
	complex nslIncoherent() const;//!< \conv \see NS_L_INCOHERENT_PROPERTY
	
	/// Tests this element for (physical) valid property values.
	bool isValid() const;

	/// Provides read-only access to the X-Ray scattering factors.
	/// See the \ref xrayFactors "description above".
	const MapTriple& xrayCoefficients() const;

	/// Adds a new triple \f$ [ E, f'(E), f''(E) ] \f$ to the X-Ray
	/// scattering factors. Overwrites existing triples with the
	/// same energy value.
	void addXrayCoefficient(double energy, double fp, double fpp);

private:
	/// Returns a read/write iterator to the first of all property values.
	PropertyValueIterator begin();

	/// Returns a read/write iterator which points behind the last
	/// property value.
	PropertyValueIterator end();

	/// Returns the Property of the specified property value
	/// iterator created with begin() or end();
	Property getProperty(const PropertyValueIterator& iter) const;

	/// Returns the value of given property p. The chosen type
	/// must match that of the given property.
	template<typename T>
	T getValue(Property p) const;

	/// Implementation of data access for cfp::ChemicalElementInterface.
	virtual std::string doSymbol() const;
	/// Implementation of data access for cfp::ChemicalElementInterface.
	virtual void doSetSymbol(const std::string& s);
	/// Implementation of data access for cfp::ChemicalElementInterface.
	virtual int doNucleons() const;
	/// Implementation of data access for cfp::ChemicalElementInterface.
	virtual void doSetNucleons(int n);

	/// Explicitly typified default value for property \e p.
	static int defaultValue(int a, Element::Property p);
	/// Explicitly typified default value for property \e p.
	static double defaultValue(double a, Element::Property p);
	/// Explicitly typified default value for property \e p.
	static complex defaultValue(complex a, Element::Property p);
	/// Explicitly typified default value for property \e p.
	static std::string defaultValue(std::string a, Element::Property p);
private:
	/// Container for all property values.
	PropertyVariantVector mProperties;
	/// Points to the names of all properties.
	static const char * mPropertyNames[INVALID_PROPERTY];
	/// Container for \ref xrayFactors "X-Ray scattering factors".
	MapTriple mXrayCoefficients;
};

template<typename T>
T Element::getValue(Property p) const 
{
	try {	return boost::get<T>(propertyConst(p));
	} catch(boost::bad_get e) {
		std::cerr << "exec " << propertyName(p) << std::endl;
		return defaultValue(T(), p);
	}
}

template<typename T>
bool Element::setProperty(Property p, const T& value)
{
	PropertyVariant var(value);
	// type check
	if (p == INVALID_PROPERTY ||
	    !isValidType(p, var)) return false;
	// input data verification
	if (p == ELECTRONS_PROPERTY ||
	    p == ABUNDANCE_PROPERTY ||
	    p == ATOMIC_MASS_PROPERTY)
	{
		if (value < T(0)) var = T(0);
	}

	mProperties.at(p) = var;
	return true;
}

/// Feed the string representation of a database element to a std::stream.
std::ostream& operator<<(std::ostream& o, const Element& e);
namespace std {
/// Comparison of complex numbers by length (Euclidian Norm).
bool operator<(const ::complex& a, const ::complex& b);
}

#endif // this file

