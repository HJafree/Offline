//
// TODO: explain purpose
// Original author S. Middleton and H. Jafree
//

#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Core/EDProducer.h"
#include "art_root_io/TFileService.h"
#include "art/Utilities/make_tool.h"
#include "canvas/Persistency/Common/Ptr.h"

#include "Offline/ProditionsService/inc/ProditionsHandle.hh"

#include "Offline/GeometryService/inc/GeomHandle.hh"
#include "Offline/GeometryService/inc/DetectorSystem.hh"
#include "Offline/TrackerGeom/inc/Tracker.hh"

#include "Offline/RecoDataProducts/inc/ComboHit.hh"
#include "Offline/RecoDataProducts/inc/TimeCluster.hh"
#include "Offline/RecoDataProducts/inc/HelixSeed.hh" 

#include "CLHEP/Units/PhysicalConstants.h"
#include "CLHEP/Matrix/Vector.h"
#include "CLHEP/Matrix/SymMatrix.h"

#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <utility>
#include <functional>
#include <float.h>
#include <vector>
#include <map>

//ROOT
#include "TStyle.h"
#include "Rtypes.h"
#include "TH1F.h"
#include "TCanvas.h"
#include "TH2F.h"
#include "TLegend.h"
#include "TTree.h"
#include "TLatex.h"
#include "TGraph.h"
#include "TProfile.h"

namespace mu2e{

  class PhotonConversionFinder : public art::EDProducer {
    public:
      struct Config{
        using Name=fhicl::Name;
        using Comment=fhicl::Comment;
        fhicl::Atom<int> diag{Name("diag"), Comment("Create diag histograms"),0};
        fhicl::Atom<art::InputTag> chToken{Name("ComboHitCollection"),Comment("tag for straw hit collection")};
        fhicl::Atom<art::InputTag> tcToken{Name("TimeClusterCollection"),Comment("tag for time cluster collection")};
      };
      typedef art::EDProducer::Table<Config> Parameters;
      explicit PhotonConversionFinder(const Parameters& conf);
      virtual ~PhotonConversionFinder(){};
      virtual void beginRun(art::Run& ) override;
      virtual void beginJob() override;
      virtual void produce(art::Event& event ) override;

    private:

      Config _conf;

      //config parameters:
      int _diag;
      art::InputTag  _chToken;
      art::InputTag  _tcToken;
      const ComboHitCollection* _chcol;

  };


 PhotonConversionFinder::PhotonConversionFinder(const Parameters& conf) :
  art::EDProducer(conf),
  _diag (conf().diag()),
  _chToken (conf().chToken()),
  _tcToken (conf().tcToken())
  {
    consumes<ComboHitCollection>(_chToken);
    consumes<TimeClusterCollection>(_tcToken);
    produces<HelixSeedCollection>();
  }
 
  void PhotonConversionFinder::beginRun(art::Run& )  {//TODO 
  }
 
  TTree* _photon_conversion;
  //Numbers:
  Int_t _nsh, _nch; // # associated straw hits / event
  Int_t _ntc; // # clusters/event
  Int_t _nhits, _nused; // # hits used
  Int_t _n_panels; // # panels
  Int_t _n_stations; // # stations
  Int_t _n_planes; // # stations
  void PhotonConversionFinder::beginJob() { //TODO - can add TTree and THistograms here if required
    if(_diag > 0){
     art::ServiceHandle<art::TFileService> tfs;
     _photon_conversion=tfs->make<TTree>("photon_conversion"," Diagnostics for Photon Conversion Track Fitting"); 
     _photon_conversion->Branch("StrawHitsInEvent", &_nsh, "StrawHitsInEvent/I");
       for(size_t ich = 0;ich < _chcol->size(); ++ich){
                ComboHit const& chit =(*_chcol)[ich];   
                 _nsh = chit.nStrawHits();
		 std::cout  << " Number of Straws Hit " << _nsh << std::endl;
						      }
     _photon_conversion->Fill();
  }
 }

  void PhotonConversionFinder::produce(art::Event& event ) {
    /*auto const& chH = event.getValidHandle<ComboHitCollection>(_chToken);
    const ComboHitCollection& chcol(*chH);
    auto  const& tcH = event.getValidHandle<TimeClusterCollection>(_tcToken);
    const TimeClusterCollection& tccol(*tcH);*/
    std::unique_ptr<HelixSeedCollection> seed_col(new HelixSeedCollection());
    //TODO - this is where your algorithm will be...
    event.put(std::move(seed_col)); //TODO - one per event
  }


}//end mu2e namespace
using mu2e::PhotonConversionFinder;
DEFINE_ART_MODULE(PhotonConversionFinder)
