/*
 * Compartment.cpp
 *
 *  Created on: Apr 16, 2009
 *      Author: kstanton
 */

#include "Compartment.h"

Compartment::Compartment(
		unsigned int compId,
		double dbBaseRate,
		TransformationSet* transformationSet,
		unsigned int nReactants,
		CompartmentReaction* parentReaction)
{
	this->name = "No name assigned to compartment";
	reactantLists = new ReactantList *[nReactants];
	n_reactants = nReactants;
	m_pParentReaction = parentReaction;
	m_iCompId = compId;
	active = true;
		//Set up the reactantLists
	for(unsigned int r=0; r<nReactants; r++)
		reactantLists[r]=(new ReactantList(r,transformationSet,25));
}
Compartment::~Compartment()
{
	if(DEBUG) cout<<"Destorying rxn Compartment: "<<name<<endl;

	for(unsigned int r=0; r<n_reactants; r++)
	{
		//delete reactantTemplates[r]; DO NOT DELETE HERE (MoleculeType has responsibility of
		//deleting all template molecules of its type now.
		delete reactantLists[r];
	}
	delete [] reactantLists;
}
unsigned int Compartment::getReactantCount(unsigned int reactantIndex) const
{
	return reactantLists[reactantIndex]->size();
}
void Compartment::printDetails() const
{
	for(unsigned int i=0; i<n_reactants; i++)
		reactantLists[i]->printDetails();
}
void Compartment::pickMappingSets(MappingSet** mappingSet, double random_A_number) const
{
	//Note here that we completely ignore the argument.  The argument is only
	//used for DOR reactions because we need that number to select the reactant to fire

	//So, we shall loop through the lists and extract out the MappingSets and
	//the molecule ID that the mappingSet was generated from
	//unsigned int *moleculeIDs = new unsigned int [n_reactants];
	for(unsigned int ii=0; ii<n_reactants; ii++)
	{
		reactantLists[ii]->pickRandom(mappingSet[ii]);
	}
}

double Compartment::update_a()
{
	// We return 0 propensity for inactive compartments to fire
	if(!active) return 0;
	a=1;
	for(unsigned int i=0; i<n_reactants; i++)
		a*=reactantLists[i]->size();
	a*=m_pParentReaction->baseRate;
	return a;
}

void Compartment::init(CompartmentReaction* thisReactionClass)
{
	for(unsigned int r=0; r<n_reactants; r++)
	{
		thisReactionClass->reactantTemplates[r]->getMoleculeType()->addReactionClass(thisReactionClass,r);
	}
}

inline bool Compartment::tryToAdd(Molecule *m, unsigned int reactantPos, int rxnIndex)
{
	//Try to map it!
	//Get the specified reactantList
	ReactantList *rl = reactantLists[reactantPos];
	ms = rl->pushNextAvailableMappingSet();
	if(!m_pParentReaction->reactantTemplates[reactantPos]->compare(m,rl,ms)) {
		//we must remove, if we did not match.  This will also remove
		//everything that was cloned off of the mapping set
		rl->removeMappingSet(ms->getId());
	} else {
		m->setRxnListMappingId(rxnIndex,ms->getId());
		m->setRxnListCompartmentMappingId(rxnIndex,m->getCompartmentId());
	}

	return true;
}
void Compartment::tryToRemove(Molecule *m, unsigned int reactantPos, int rxnIndex)
{
	//Get the specified reactantList
	ReactantList *rl = reactantLists[reactantPos];
	if(!m_pParentReaction->reactantTemplates[reactantPos]->compare(m))
	{
		rl->removeMappingSet(m->getRxnListMappingId(rxnIndex));
		m->setRxnListMappingId(rxnIndex,Molecule::NOT_IN_RXN);
		m->setRxnListCompartmentMappingId(rxnIndex,Molecule::NOT_IN_RXN);
	}
}

void Compartment::removeClones(Molecule *m, unsigned int reactantPos, int rxnIndex)
{
	//Get the specified reactantList
	ReactantList *rl = reactantLists[reactantPos];
	//If this reaction has multiple instances, we always remove them all!
	// then we remap because other mappings may have changed.  Yes, this may
	// be more ineffecient, but it is the fast implementation
	if(rl->getHasClonedMappings()) {
		if(m->getRxnListMappingId(rxnIndex)>=0) {
			rl->removeMappingSet(m->getRxnListMappingId(rxnIndex));
			m->setRxnListMappingId(rxnIndex,Molecule::NOT_IN_RXN);
			m->setRxnListCompartmentMappingId(rxnIndex,Molecule::NOT_IN_RXN);
		}
	}
}
