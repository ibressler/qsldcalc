/*
 * src/element.cpp
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

#include "element.h"

using namespace edb;

const int    Element::INVALID_PROPERTY_VALUE = -2;

const char * Element::mPropertyNames[] = {
	"symbol", // SYMBOL_PROPERTY
	"name", // NAME_PROPERTY
	"nucleons", // NUCLEONS_PROPERTY
	"electrons", // ELECTRONS_PROPERTY
	"atomic mass", // ELECTRONS_PROPERTY
	"abundance", // ABUNDANCE_PROPERTY
	"coherent length", // NEUTRON_SCATTERING_LENGTH_COHERENT_PROPERTY
	"incoherent length", // NEUTRON_SCATTERING_LENGTH_INCOHERENT_PROPERTY
	"coherent cross section", // NEUTRON_SCATTERING_CROSS_SECTION_COHERENT_PROPERTY
	"incoherent cross section", // NEUTRON_SCATTERING_CROSS_SECTION_INCOHERENT_PROPERTY
	"total cross section", // NEUTRON_SCATTERING_CROSS_SECTION_TOTAL_PROPERTY
	"absorption cross section", // NEUTRON_SCATTERING_CROSS_SECTION_ABSORPTION_PROPERTY
	"xray energies" // XRAY_SCATTERING_ENERGY_MAP_PROPERTY
};

Element::Element()
	: cfp::ChemicalElementInterface(),
	  // init properties (max+1 to avoid out-of-range, see propertyConst())
	  mProperties(INVALID_PROPERTY+1)
{
	for(int i=0; i < propertyCount(); i++) {
		Property p = getProperty(i);
		switch(p) {
			case STRING_TYPE:
				mProperties.at(p) = defaultValue(std::string(), p); break;
			case INT_TYPE:
				mProperties.at(p) = defaultValue(int(), p); break;
			case DOUBLE_TYPE:
				mProperties.at(p) = defaultValue(double(), p); break;
			case COMPLEX_TYPE:
				mProperties.at(p) = defaultValue(complex(), p); break;
			default:
				mProperties.at(p) = INVALID_PROPERTY_VALUE; break;
		}
	}
}

Element::~Element()
{
}

const char * 
Element::propertyName(Property p)
{
	if (p < 0 || p >= INVALID_PROPERTY) return "";
	return mPropertyNames[p];
}

int 
Element::propertyCount()
{
	return static_cast<int>(INVALID_PROPERTY);
}

Element::Property 
Element::getProperty(int index) 
{
	if (index < 0 || index > static_cast<int>(INVALID_PROPERTY)) 
		return INVALID_PROPERTY;
	return static_cast<Property>(index);
}

bool 
Element::isValidVariant(const PropertyVariant& var) 
{
	return !(var.which() == 0 && 
		boost::get<int>(var) == INVALID_PROPERTY_VALUE);
}

Element::PropertyType 
Element::propertyType(Element::Property p)
{
	switch (p) {
		case SYMBOL_PROPERTY:
		case NAME_PROPERTY:
			return STRING_TYPE; break;
		case NUCLEONS_PROPERTY:
		case ELECTRONS_PROPERTY:
			return INT_TYPE; break;
		case ATOMIC_MASS_PROPERTY:
		case ABUNDANCE_PROPERTY:
		case NS_CS_COHERENT_PROPERTY:
		case NS_CS_INCOHERENT_PROPERTY:
		case NS_CS_TOTAL_PROPERTY:
		case NS_CS_ABSORPTION_PROPERTY:
			return DOUBLE_TYPE; break;
		case NS_L_COHERENT_PROPERTY:
		case NS_L_INCOHERENT_PROPERTY:
			return COMPLEX_TYPE; break;
		default:	// INVALID_PROPERTY
			return INVALID_TYPE; break;
	}
}

bool 
Element::isValidType(Property p, const PropertyVariant& var)
{
	return (var.which() == propertyType(p));
}

bool 
Element::isValid() const 
{
	try {
		const std::string& sym  = boost::get<std::string>(propertyConst(SYMBOL_PROPERTY));
		const std::string& name = boost::get<std::string>(propertyConst(NAME_PROPERTY));
		const int& nuc   = boost::get<int>(propertyConst(NUCLEONS_PROPERTY));
		const int& elec  = boost::get<int>(propertyConst(ELECTRONS_PROPERTY));
		const double& ab = boost::get<double>(propertyConst(ABUNDANCE_PROPERTY));
		const double& am = boost::get<double>(propertyConst(ATOMIC_MASS_PROPERTY));
		return !sym.empty() &&
		       !name.empty() &&
		       elec > 0 &&
		       nuc != 0 &&
		       ab >= 0.0 &&
		       am > 0.0;
	} catch(boost::bad_get e) { return false; }
}

const MapTriple& 
Element::xrayCoefficients() const 
{
	return mXrayCoefficients;
}

void 
Element::addXrayCoefficient(double energy, double fp, double fpp)
{
	size_t num = mXrayCoefficients.erase(energy);
	if (num > 0) {
		std::cerr << "Element::addXrayCoefficient: "
			<< "Found duplicate: " << symbol() 
			<< " (" << energy << ", " << fp << ", " << fpp << ")"
			<< " -> overwritten !"
			<< std::endl;
	}
	mXrayCoefficients.insert(std::make_pair(energy,
	                         std::make_pair(fp, fpp)));
}

Element::PropertyValueIteratorConst 
Element::beginConst() const { return mProperties.begin(); }

Element::PropertyValueIteratorConst 
Element::endConst() const { return mProperties.end(); }

Element::Property 
Element::getProperty(const PropertyValueIteratorConst& iter) const 
{
	return getProperty(iter - beginConst());
}

double 
Element::electrons() const 
{ 
	return getValue<int>(ELECTRONS_PROPERTY);
}

double 
Element::atomicMass() const
{
	return getValue<double>(ATOMIC_MASS_PROPERTY);
}

complex 
Element::nslCoherent() const
{
	return getValue<complex>(NS_L_COHERENT_PROPERTY);
}

complex 
Element::nslIncoherent() const
{
	return getValue<complex>(NS_L_INCOHERENT_PROPERTY);
}

std::ostream& 
std::operator<<(std::ostream& o, const Element& e)
{
	edb::Element::PropertyValueIteratorConst pit = e.beginConst();
	for(; pit != e.endConst(); pit++)
	{
		o << *pit;
	}
	o << std::endl;
	const edb::MapTriple& map = e.xrayCoefficients();
	edb::MapTriple::const_iterator it = map.begin();
	while (it != map.end() )
	{
		o << it->first << "\t" << it->second.first << "\t" << it->second.second
			<< std::endl;
		it++;
	}
	return o;
}

bool 
std::operator<(const edb::complex& a, const edb::complex& b)
{
	return
	(a.real()*a.real() + a.imag()*a.imag()) < 
	(b.real()*b.real() + b.imag()*b.imag());
}

// private //

Element::PropertyValueIterator 
Element::begin()
{
	return mProperties.begin();
}

Element::PropertyValueIterator
Element::end()
{
	return mProperties.end();
}

Element::Property 
Element::getProperty(const PropertyValueIterator& iter) const 
{
	return getProperty(iter - beginConst());
}

std::string 
Element::doSymbol() const 
{
	return getValue<std::string>(SYMBOL_PROPERTY);
}

void 
Element::doSetSymbol(const std::string& s)
{
	setProperty(SYMBOL_PROPERTY, s);
}

int 
Element::doNucleons() const
{
	return getValue<int>(NUCLEONS_PROPERTY);
}

void 
Element::doSetNucleons(int n) 
{
	setProperty(NUCLEONS_PROPERTY, n);
}

int
Element::defaultValue(int a, Element::Property p) { return 0; }

double 
Element::defaultValue(double a, Element::Property p) 
{
	if (p == ABUNDANCE_PROPERTY) return -1.0;
	else                         return 0.0;
}

complex 
Element::defaultValue(complex a, Element::Property p)
{
	return complex(0.0, 0.0);
}

std::string 
Element::defaultValue(std::string a, Element::Property p) { return ""; }

const Element::PropertyVariant& 
Element::propertyConst(Element::Property p) const { return mProperties.at(p); }
