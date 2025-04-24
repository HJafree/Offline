// author : Huma Jafree & Sophie Middleton 
// purpose : This file will analyse reconstructed tracks to find point of origin (vertex finder)

#include "Offline/DataProducts/inc/GenVector.hh"
#include "Offline/Mu2eKinKal/inc/WireHitState.hh"
#include "Offline/RecoDataProducts/inc/KalSeed.hh"

#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art_root_io/TFileService.h"
#include "canvas/Utilities/InputTag.h"
namespace mu2e{

  class PairConvAna : public art::EDAnalyzer {
	  public:
		  struct Config {
		  using Name=fhicl::Name;
		  using Comment=fhicl::Comment;
		  fhicl::Atom<int> diag{Name("diag"), Comment("Create diag histograms"),0};
	    fhicl::Atom<art::InputTag> KalToken{Name("KalSeedCollection"),Comment("tag for kal seed collection")};
  };
	  typedef art::EDAnalyzer::Table<Config> Parameters;
	  explicit PairConvAna(const Parameters& conf);
	  virtual void beginJob() override;
	  virtual void analyze(const art::Event& e);

  private:
	  Config _conf;
	  int _diagLevel;
	  art::InputTag _KalToken;
	  const KalSeedCollection* _KalCol;
	};
 void PairConvAna::beginJob() {//TODO: add histograms here 
}
 void PairConvAna::analyze(const art::Event& event) { 
    auto sH = event.getValidHandle<mu2e::KalSeedCollection>(_KalToken);
    _KalCol = sH.product();
			for (size_t k = 0; k < _KalCol->size(); k++){
  		KalSeed kalseed = (*_KalCol)[k];//TODO: print statments go here
				std::cout << "Loop Helix Bool " << kalseed.loopHelixFit() << std::endl;
		}
}
 //put in mu2e offline, analyses or anywhere alse
//set up analyser but impot kal seed collectoon
/*template<class KTRAJ> void AddKinKalTrajectory(std::unique_ptr<KTRAJ> &trajectory){
	double t1;
  double t2;
  t1=trajectory->range().begin();
  t2=trajectory->range().end();

  double x1=trajectory->position3(t1).x();
  double y1=trajectory->position3(t1).y();
  double z1=trajectory->position3(t1).z();

	std::cout << "Range Begin: " << t1 << endl;
	std::cout << "Range End: " << t2 << endl;

	std::cout << "Position in x: " << x1 << endl;
	std::cout << "Position in y: " << y1 << endl;
	std::cout << "Position in z: " << z1 << endl;
*/

}
