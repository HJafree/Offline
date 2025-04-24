// Purpose: Event filter for pair converions that make it through the tracker
// author: S Middleton and H Jafree, 2023
#include "art/Framework/Core/EDFilter.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "fhiclcpp/ParameterSet.h"

#include "art/Framework/Principal/Handle.h"

// Mu2e includes.
#include "Offline/MCDataProducts/inc/StrawGasStep.hh"
#include "Offline/MCDataProducts/inc/SimParticle.hh"
#include <iostream>
#include <string>

using namespace std;
namespace mu2e {

  class PairConvFilter : public art::EDFilter {
    public:
		  struct Config {
		  using Name=fhicl::Name;
		  using Comment=fhicl::Comment;
		  fhicl::Atom<int> diag{Name("diag"), Comment("Create diag histograms"),0};
	    fhicl::Atom<art::InputTag> StrawToken{Name("StrawGasStepCollection"),Comment("tag for straw gas step collection")};
 			fhicl::Atom<art::InputTag> SimToken{Name("SimParticleCollection"),Comment("tag for Sim collection")};
};
	explicit PairConvFilter(const art::EDFilter::Table<Config>& config);
   		 virtual bool filter(art::Event& event) override;
    private:
	  	Config _conf;
	 		int _diagLevel;
	  	art::InputTag _StrawToken;
	 		art::InputTag _SimToken;
	  	const StrawGasStepCollection* _StrawCol;
	 		const SimParticleCollection* _SimCol;
};

  PairConvFilter::PairConvFilter(const art::EDFilter::Table<Config>& config) :
     EDFilter{config},
   _diagLevel(config().diag()),
   _StrawToken(config().StrawToken()),
   _SimToken(config().SimToken())
  {}

  bool PairConvFilter::filter(art::Event& event) {
    
    bool pass = false;
          //------------ Straw Gas Collection  --------------------//
	auto strawH = event.getValidHandle<StrawGasStepCollection>(_StrawToken);
	_StrawCol = strawH.product();
	unsigned int nElectronSteps = 0;
	unsigned int nPositronSteps = 0;
	for (size_t k = 0; k < _StrawCol->size(); k++){
  	StrawGasStep strawgas = (*_StrawCol)[k];
  	art::Ptr<SimParticle> const& simpart = strawgas.simParticle();
  		if      (simpart->pdgId()==11)  ++nElectronSteps;
  		else if (simpart->pdgId()==-11) ++nPositronSteps;
		}
   			 //------------ Sim Particles --------------------//
	 auto sH = event.getValidHandle<mu2e::SimParticleCollection>(_SimToken);
    _SimCol = sH.product();
    unsigned int nConvParticles = 0;
	  unsigned int nConv = 0;
	  unsigned int nBoth = 0;
	  unsigned int nSameMother = 0;
	  unsigned int nSameMotherPhoton = 0;
    bool isConv = false;
    bool hasElectron = false;
    bool hasPositron = false;
    bool hasMotherPhoton = false;
		bool hasBoth = false;
		bool validPairConv = false;	
		int eventid_ = event.id().event(); 
    int runid_ = event.run();
    int subrunid_ = event.subRun();
    std::vector<double> ElecMotherPos;
    std::vector<double> PosMotherPos;
    for ( SimParticleCollection::const_iterator i=_SimCol->begin(); i!=_SimCol->end(); ++i ){
      SimParticle const& sim = i->second;
      art::Ptr<SimParticle> const& parent = sim.parent();
			if(abs(sim.pdgId()) == 11 and sim.creationCode()==13){
 				if(isConv ==false) nConv ++;
          isConv = true;
          nConvParticles ++;
				 if(parent->pdgId() == 22 and parent->creationCode()==175) 
						hasMotherPhoton = true;
				 if(sim.pdgId() == 11) {
            hasElectron  = true;
            ElecMotherPos.push_back(parent->endPosition().x());
            ElecMotherPos.push_back(parent->endPosition().y());
            ElecMotherPos.push_back(parent->endPosition().z());
						}
				 if(sim.pdgId() == -11){
            hasPositron  = true;
            PosMotherPos.push_back(parent->endPosition().x());
            PosMotherPos.push_back(parent->endPosition().y());
            PosMotherPos.push_back(parent->endPosition().z());
						}
  	}
  }
    if(hasElectron and hasPositron) hasBoth = true;
    if(isConv and hasBoth) nBoth ++;
    if(ElecMotherPos.size() !=0 and PosMotherPos.size() != 0 and ElecMotherPos[0] == PosMotherPos[0] and ElecMotherPos[1] == PosMotherPos[1] and ElecMotherPos[2] == PosMotherPos[2]) nSameMother++;
    if(ElecMotherPos.size() !=0 and PosMotherPos.size() != 0 and ElecMotherPos[0] == PosMotherPos[0] and ElecMotherPos[1] == PosMotherPos[1] and ElecMotherPos[2] == PosMotherPos[2] and hasMotherPhoton) nSameMotherPhoton++;
    if(isConv and hasBoth and ElecMotherPos.size() !=0 and PosMotherPos.size() != 0 and ElecMotherPos[0] == PosMotherPos[0] and ElecMotherPos[1] == PosMotherPos[1] and ElecMotherPos[2] == PosMotherPos[2] and hasMotherPhoton and nConvParticles==2) validPairConv = true;
				        if(_diagLevel == 1){ std::cout<<"+=======================================================+"<<std::endl;
	std::cout<<"event : "<<eventid_<<" subrun : "<<subrunid_<<" run : "<<runid_<<std::endl;}
        std::cout<<"number of conversion particles in this event "<<nConvParticles<<std::endl;
        std::cout<<"number of total conversions "<<nConv<<std::endl;
        std::cout<<"number of events with both e+/e-s conversions "<<nBoth<<std::endl;
        std::cout<<"number of events with both e+/e-s conversions from same mother "<<nSameMother<<std::endl;
        std::cout<<"number of events with both e+/e-s conversions from same mother and its a photon "<<nSameMotherPhoton<<std::endl;


    if (nElectronSteps>=10 and nPositronSteps>=10 and (validPairConv)) pass=true;
    return pass;
  }
}

using mu2e::PairConvFilter;
DEFINE_ART_MODULE(PairConvFilter)
