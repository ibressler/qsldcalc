/*
 * src/inputdata.cpp
 *
 * Copyright (c) 2010-2011, Ingo Bressler (qsldcalc at ingobressler.net)
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

#include <numeric>
#include <functional>
#include "utils.h"
#include "inputdata.h"
 
InputData::InputData(ElementDatabase& db)
	: mDB(&db)
{
}

void 
InputData::set(QHashType& h, const QString& key, const QVariant& var) { h.insert(key, var); }

QVariant 
InputData::get(const QHashType& h, const QString& key) const { return h.value(key); }

void 
InputData::set(const QString& key, const QVariant& var) { set(mData, key, var); }

void 
InputData::setInit(const QString& key, const QVariant& var) { set(mInitData, key, var); }

QVariant 
InputData::get(const QString& key) const { return get(mData, key); }

QVariant 
InputData::getInit(const QString& key) const { return get(mInitData, key); }

const cfp::Compound& 
InputData::empiricalFormula() const
{
	return mFormulaParser.empirical();
}

void 
InputData::addAlias(const QString& name)
{
	mDB->addAlias(name, empiricalFormula());
//	Element newElem;
//	newElem.setProperty<int>(Element::ELECTRONS_PROPERTY,
//	                         get("number of electrons").toInt());
}

void 
InputData::interpretFormula(const QString& formulaKey)
{
	QVariant var(mData.value(formulaKey));
#ifdef DEBUG
	if (!var.isValid() || !var.canConvert(QVariant::ByteArray)) {
		std::cerr << "InputData: Can't retrieve formula !" << std::endl;
	}
#endif
	QByteArray formula(var.toByteArray());
	// may throw an exception
	mFormulaParser.process(formula.data(), formula.length());

	std::stringstream ss;
	ss << mFormulaParser.empirical();
	set(formulaKey, QVariant(QString::fromStdString(ss.str())));

	CompleteList elemList;
	buildCompleteList(elemList, mFormulaParser.empirical());
}

void 
InputData::buildCompleteList(CompleteList& list, const cfp::Compound& comp)
{
	foreach(cfp::CompoundElement e, comp) 
	{
		Element::Ptr ep = mDB->getElement(e);
		if (!ep.isNull()) {
			list.append(ElemPair(e, ep));
		} else {
			const cfp::Compound subComp = mDB->getAlias(e);
			if (subComp.size() > 0) {
				buildCompleteList(list, subComp);
			} else {
				throw ErrorUnknownElement(e);
			}
		}
	}
	calcData(list);
}

void 
addComplex2Map(QVariantMap& map, const char * key, complex c)
{
	QVariantList list;
	list.append(QVariant(c.real()));
	list.append(QVariant(c.imag()));
	map.insert(key, QVariant(list));
}

double 
InputData::calcElectrons(const CompleteList& cl,
                         QVariantMap&        partialElectrons)
{
	double totalElectrons = 0.0;
	CompleteList::const_iterator it = cl.begin();
	for(;it != cl.end(); it++) {
		QString name(it->first.uniqueName().c_str());
		double electrons = 
			(it->first.coefficient() * it->second->electrons());
		partialElectrons.insert(name, QVariant(electrons));
		totalElectrons += electrons;
	}
	partialElectrons.insert("value", QVariant(totalElectrons));
	set("number of electrons", QVariant(partialElectrons));
	return totalElectrons;
}

void 
InputData::calcMassAndVolume(const CompleteList& cl,
                             QVariantMap&        partialVolume,
                             double&             totalMass,
                             double&             totalVolume)
{
	QVariantMap partialMass;
	double compoundDensity = get("ntrDensity").toDouble();
	totalMass = 0.0;
	totalVolume = 0.0;
	CompleteList::const_iterator it = cl.begin();
	for(;it != cl.end(); it++) {
		// calculate partial values
		double weight = (it->first.coefficient() * it->second->atomicMass());
		double volume = 1e-2 * weight / (avogadro() * compoundDensity);

		// create element name from nucleon number and symbol
		QString name(it->first.uniqueName().c_str());

		// add partial values to result and calculate total value
		partialMass.insert(name, QVariant(weight));
		totalMass += weight;
		partialVolume.insert(name, QVariant(volume));
		totalVolume += volume;
	}

	// volume ratios do not make sense here, as the density is entered manually

	// calculate mass percentages
	QVariantMap massComposition;
	QVariantMap::const_iterator mit = partialMass.constBegin();
	while(mit != partialMass.constEnd()) {
		double percentage = 100.0 * mit.value().toDouble()/totalMass;
		massComposition.insert(mit.key(), QVariant(percentage));
		mit++;
	}

	QVariantMap masses;
	masses.insert("value", QVariant(totalMass));
	masses.insert("partial masses g/mol", QVariant(partialMass));
	masses.insert("mass ratios %", QVariant(massComposition));
	set("molecular mass g/mol", QVariant(masses));

	partialVolume.insert("value", QVariant(totalVolume));
	set("molecular volume nm^3", QVariant(partialVolume));
}

void 
InputData::calcNSL(const CompleteList& cl, 
                   double              totalVolume,
                   QVariantMap&        neutron,
                   complex (Element::*func)(void) const,
                   const char        * slText,
                   const char        * sldText)
{
	complex totalSL(0.0, 0.0);
	complex totalSLD(0.0, 0.0);
	QVariantMap partialSLD;
	CompleteList::const_iterator it = cl.begin();
	for(;it != cl.end(); it++) {
		complex sl = (it->first.coefficient() * ((*it->second).*func)());
		complex sld = 1e8 * (sl / totalVolume);
		addComplex2Map(partialSLD, it->first.uniqueName().c_str(), sld);
		totalSL += sl;
		totalSLD += sld;
	}

	addComplex2Map(partialSLD, "value", totalSLD);
	addComplex2Map(neutron, slText, totalSL);
	neutron.insert(sldText, partialSLD);
}

double 
interpolate(double x1, double y1, double x2, double y2, double x3)
{
	return (x3-x1) * (y2-y1)/(x2-x1) + y1;
}

complex 
sldXray(double electrons, double fp, double fpp, double vol)
{
	complex sld(
		(electrons - pow(electrons/82.5, 2.37) + fp)
			* electronRadius() / vol,
		fpp * electronRadius() / vol);
	return sld;
}

bool 
InputData::calcXrayCoefficients(double&           fp, 
                                double&           fpp,
                                Element::Ptr ep,
                                double            coeff,
                                double            energy)
{
	typedef MapTriple::const_iterator Iter;
	Iter end = ep->xrayCoefficients().end();
	Iter begin = ep->xrayCoefficients().begin();
	Iter it = ep->xrayCoefficients().find(energy);
	fp = 0.0, fpp = 0.0;
	// energy already in the map
	if (it != end) {
		fp = (coeff * it->second.first);
		fpp = (coeff * it->second.second);
	} else {
		// interpolate missing energy
		ep->addXrayCoefficient(energy, 0.0, 0.0);
		it = ep->xrayCoefficients().find(energy);
		Iter prev = it; prev--;
		Iter next = it; next++;
		if (it != end && it != begin && next != end) {
			double prevEn = prev->first;
			double nextEn = next->first;

			double prevFp = prev->second.first;
			double nextFp = next->second.first;
			fp  = interpolate(prevEn, prevFp, 
					  nextEn, nextFp, energy);
			double prevFpp = prev->second.second;
			double nextFpp = next->second.second;
			fpp = interpolate(prevEn, prevFpp,
					  nextEn, nextFpp, energy);
		} else {
			// given xray energy is out of range
			// (of values in the map)
			return false;
		}
	}
	return true;
}

void 
InputData::calcXrayEnergies(const CompleteList& cl,
                            const QVariantMap& partialVolumes,
                            const QVariantMap& partialElectrons)
{
	QVariantMap xray, partialFp, partialFpp, partialSLD;
	double energy = 1000.0 * get("ntrXrayEn").toDouble();
	double totalFp = 0.0, totalFpp = 0.0;

	if (!partialVolumes.contains("value")) return;
	if (!partialElectrons.contains("value")) return;
	double totalVolume = 1e-8 * partialVolumes.value("value").toDouble();
	double totalElectrons = partialElectrons.value("value").toDouble();

	CompleteList::const_iterator elemIt = cl.begin();
	for(;elemIt != cl.end(); elemIt++) 
	{
		Element::Ptr ep = elemIt->second;
		if (ep->isIsotope()) {
			ep = mDB->getElement(ElementDatabase::KeyType(ep->symbol().c_str()));
			if (ep.isNull())
			{
#ifdef DEBUG
				std::cerr << "InputData::calcXrayEnergies: '"
					<< ep->symbol().c_str()
					<<"' not found in Database !" << std::endl;
#endif
				return; // avoid invalid value output
			}
		}
		if (ep->xrayCoefficients().size() < 2)
		{
#ifdef DEBUG
			std::cerr << "InputData::calcXrayEnergies: "
				<< "Not enough X-ray energies found !"
				<< std::endl;
#endif
			continue;
		}
		double fp = 0.0, fpp = 0.0;
		bool ok = calcXrayCoefficients(fp, fpp, ep, 
		                        elemIt->first.coefficient(), energy);
		if (!ok) {
#ifdef DEBUG
			std::cerr << "InputData::calcXrayEnergies, "
				<< "Specified energy is out of bounds!"
				<< std::endl;
#endif
			return;
		}

		// create element name from nucleon number and symbol
		std::string nameStd = elemIt->first.uniqueName();
		QString name = QString::fromStdString(nameStd);
		// store partial xray scattering coefficients
		partialFp.insert(name, QVariant(fp));
		totalFp += fp;
		partialFpp.insert(name, QVariant(fpp));
		totalFpp += fpp;

		// calculate partial SLDs
		if (partialVolumes.contains(name)) {
			double volume = 1e-8 * partialVolumes.value(name).toDouble();
			double electrons = elemIt->first.coefficient() * ep->electrons();
			complex sld = sldXray(electrons, fp, fpp, volume);
			addComplex2Map(partialSLD, nameStd.c_str(), sld);
		} else {
#ifdef DEBUG
			std::cerr << "partialVolumes empty!" << std::endl;
#endif
		}

	} // end for each element in list
	partialFp.insert("value", QVariant(totalFp));
	partialFpp.insert("value", QVariant(totalFpp));
	xray.insert("f'", QVariant(partialFp));
	xray.insert("f''", QVariant(partialFpp));
	// calculate SLD
	complex totalSLD = sldXray(totalElectrons, totalFp, totalFpp, totalVolume);
	addComplex2Map(partialSLD, "value", totalSLD);
	xray.insert("SLD cm^-2", QVariant(partialSLD));
	set("xray scattering", QVariant(xray));
}

void 
InputData::calcData(const CompleteList& cl)
{
	QVariantMap partialElectrons;
	calcElectrons(cl, partialElectrons);

	double totalMass = 0.0;
	double totalVolume = 0.0;
	QVariantMap partialVolumes;
	calcMassAndVolume(cl, partialVolumes, totalMass, totalVolume);

	QVariantMap neutron;
	calcNSL(cl, totalVolume, neutron, &Element::nslCoherent, "coherent scattering length 1e-15m", "SLD coherent 1/cm^2");
	calcNSL(cl, totalVolume, neutron, &Element::nslIncoherent, "incoherent scattering length 1e-15m", "SLD incoherent 1/cm^2");
	set("neutron scattering", QVariant(neutron));

	calcXrayEnergies(cl, partialVolumes, partialElectrons);
}

std::ostream& 
std::operator<<(std::ostream& o, const InputData& ind)
{
	InputData::QHashType::const_iterator it = ind.mData.begin();
	while (it != ind.mData.end() ) {
		o << it.key().toLatin1().data();
		o << ": ";
		switch(it.value().type()) {
			case QVariant::Double: 
				o << it.value().toDouble();
				break;
			case QVariant::String: 
				o << it.value().toString().toLatin1().data();
				break;
			default:
				o << "n/a";
				break;
		}
		o << std::endl;
		it++;
	}
	return o;
}

ErrorUnknownElement::ErrorUnknownElement(const cfp::CompoundElement& e)
	: cfp::Error(0, 0)
{
	std::string msg;
	msg.append("Unknown chemical element or isotope entered: '");
	msg.append(e.uniqueName());
	msg.append("' !");
	setMessage(msg);
}

