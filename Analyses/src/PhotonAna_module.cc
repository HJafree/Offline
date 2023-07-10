//
// TODO:  This file will analyze the outgoing photons energy, momentum and position by looking at the indicent electron/positron
// Original author S. Middleton and H. Jafree
//

#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art_root_io/TFileService.h"
#include "canvas/Utilities/InputTag.h"

#include "Offline/ProditionsService/inc/ProditionsHandle.hh"

#include "Offline/GeometryService/inc/GeomHandle.hh"
#include "Offline/GeometryService/inc/DetectorSystem.hh"
#include "Offline/TrackerGeom/inc/Tracker.hh"
#include "Offline/RecoDataProducts/inc/KalSeed.hh"
#include "Offline/MCDataProducts/inc/SimParticle.hh"
#include "Offline/MCDataProducts/inc/MCTrajectoryPoint.hh"
#include "Offline/MCDataProducts/inc/MCTrajectoryCollection.hh"

#include "CLHEP/Units/PhysicalConstants.h"
#include "CLHEP/Matrix/Vector.h"
#include "CLHEP/Matrix/SymMatrix.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
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
#include "TH3F.h"
#include "TLegend.h"
#include "TTree.h"
#include "TLatex.h"
#include "TGraph.h"
#include "TProfile.h"
using namespace std;
namespace mu2e{

class PhotonAna : public art::EDAnalyzer {
	public:
		struct Config {
		using Name=fhicl::Name;
		using Comment=fhicl::Comment;
		fhicl::Atom<int> diag{Name("diag"), Comment("Create diag histograms"),0};
		fhicl::Atom<art::InputTag> KalToken{Name("KalSeedCollection"),Comment("tag for kal seed collection")};
		fhicl::Atom<art::InputTag> SimToken{Name("MCTrajectoryCollection"),Comment("tag for MC trajectory collection")};
};
	typedef art::EDAnalyzer::Table<Config> Parameters;
	explicit PhotonAna(const Parameters& conf);
	virtual void beginJob() override;
	virtual void analyze(const art::Event& e);

private:
	Config _conf;
	art::InputTag _KalToken;
	art::InputTag _SimToken;
	const KalSeedCollection* _KalCol;
	const MCTrajectoryCollection* _SimCol;
	TTree* _photon_analyzer; 
	TH3F* _3Dphoton_position;
	TH2F* _2Dphoton_posxy;
	TH2F* _2Dphoton_posrz;
	TH2F* _2Dphoton_timez;
	TH2F* _2Dphoton_momz;
	Float_t _pdgid;
	Float_t startmomentum; 
	Float_t endmomentum;
	//for position: need x y and x-- maybe can yse brackets with , to seperate-- for not only plot start pos
	Float_t time;
	Float_t startposx;
	Float_t startposy;
	Float_t startposz;
	Float_t startposr;
	Float_t endposx;
	Float_t endposy;
	Float_t endposz;
	//Float_t _y;
	//Float_t _z;

	};

PhotonAna::PhotonAna(const Parameters& conf) :
art::EDAnalyzer(conf),
_KalToken(conf().KalToken()),
_SimToken(conf().SimToken())
{}

void PhotonAna::beginJob() { //TODO - can add TTree and THistograms here if required
	art::ServiceHandle<art::TFileService> tfs;
	_photon_analyzer=tfs->make<TTree>("photon_analyzer"," Diagnostics for Photon Conversion Track Fitting");
	_photon_analyzer->Branch("pdgid", &_pdgid, "pdgid/F"); 
	_photon_analyzer->Branch("StartMom", &startmomentum, "StartMom/F"); 
	_photon_analyzer->Branch("EndMom", &endmomentum, "End Mom/F");
	_photon_analyzer->Branch("StartPositionx", &startposx, "StartPositionx/F");
	//_photon_analyzer->GetXaxis()->SetTitle("Position in x [mm]");
	_photon_analyzer->Branch("StartPositiony", &startposy, "StartPositiony/F");
	//_photon_analyzer->GetXaxis()->SetTitle("Position in y [mm]");
	_photon_analyzer->Branch("StartPositionz", &startposz, "StartPositionz/F");
	//_photon_analyzer->GetXaxis()->SetTitle("Position in z [mm]");
	_photon_analyzer->Branch("EndPositionx", &endposx, "EndPositionx/F");
	_photon_analyzer->Branch("EndPositiony", &endposx, "EndPositiony/F");
	_photon_analyzer->Branch("EndPositionz", &endposx, "EndPositionz/F"); 
	_photon_analyzer->Branch("Time", &time, "Time/F"); 
	//_photon_analyzer->GetXaxis()->SetTitle("Time[ns]");
	_3Dphoton_position=tfs->make<TH3F>("3D photon generation","Position of e+e- hits in X-Y-Z plane " ,100,-3980,-3820,100,-80, 80,150,5400, 6300);
	_3Dphoton_position->GetXaxis()->SetTitle("Position in x [mm]");
	_3Dphoton_position->GetYaxis()->SetTitle("Position in y [mm]");
	_3Dphoton_position->GetZaxis()->SetTitle("Position in z [mm]");
	_2Dphoton_posxy=tfs->make<TH2F>("2D photon analyzer XY","Position of e+e- hits in X-Y plane " ,100,-3980,-3820,100,-80, 80);
	_2Dphoton_posxy->GetXaxis()->SetTitle("Position in x [mm]");
	_2Dphoton_posxy->GetYaxis()->SetTitle("Position in y [mm]");
	_2Dphoton_posrz=tfs->make<TH2F>("2D photon generation RZ","Position of e+e- hits in R-Z plane" ,100, 3980,3820,150,5400, 6300);
	_2Dphoton_posrz->GetXaxis()->SetTitle("Position in r [mm]");
	_2Dphoton_posrz->GetYaxis()->SetTitle("Position in z [mm]");
	_2Dphoton_timez=tfs->make<TH2F>("2D photon generation","Time and Position of e+e- Z plane" ,150,5400, 6300,100,0,6000);
	_2Dphoton_timez->GetXaxis()->SetTitle("Position in z [mm]");
	_2Dphoton_timez->GetYaxis()->SetTitle("Time [ns]");
	_2Dphoton_momz=tfs->make<TH2F>("2D photon generation","Momentum and Position of e+e- Z plane" ,150,5400, 6300,100,-1,1);
	_2Dphoton_momz->GetXaxis()->SetTitle("Position in z [mm]");
	_2Dphoton_momz->GetYaxis()->SetTitle("Start Momentum [MeV/c]");
}
void PhotonAna::analyze(const art::Event& event) {
	auto chH = event.getValidHandle<mu2e::MCTrajectoryCollection>(_SimToken);
	_SimCol = chH.product();
	std::map<art::Ptr<mu2e::SimParticle>,mu2e::MCTrajectory>::const_iterator trajectoryIter;
	for(unsigned int k = 0; k < _SimCol->size(); k++){ 
		for(trajectoryIter=_SimCol->begin(); trajectoryIter!=_SimCol->end(); trajectoryIter++){
		_pdgid = trajectoryIter->first->pdgId();
		startmomentum = trajectoryIter->first->startMomentum().mag();
		endmomentum = trajectoryIter->first->endMomentum().mag();
		startposx = trajectoryIter->first->startPosition().x();
		startposy = trajectoryIter->first->startPosition().y();
		startposz = trajectoryIter->first->startPosition().z();
		startposr = sqrt(startposx*startposx+startposy*startposy);
		endposx = trajectoryIter->first->startPosition().x();
		endposy = trajectoryIter->first->startPosition().y();
		endposz = trajectoryIter->first->startPosition().z();
		time = trajectoryIter->first->startGlobalTime();
	  _photon_analyzer->Fill(); 
	  _3Dphoton_position->Fill(startposx,startposy,startposz); 
	  _2Dphoton_posxy->Fill(startposx,startposy); 
	  _2Dphoton_posrz->Fill(startposr,startposz); 
	  _2Dphoton_timez->Fill(startposz,time); 
	  _2Dphoton_momz->Fill(startmomentum,time); 
		}
	}
}
 /* using LHPT = KinKal::PiecewiseTrajectory<KinKal::LoopHelix>;
  void PhotonAna::analyze(const art::Event& event) {
    auto kalH = event.getValidHandle<KalSeedCollection>(_KalToken);
    _KalCol = kalH.product();
  cout << "Kal Col: " << _KalCol << endl;
    for(unsigned int k = 0; k < _KalCol->size(); k++){
      KalSeed kseed = (*_KalCol)[k];
  cout << "Helix: " << kseed.loopHelixFit() << endl;
      if(kseed.loopHelixFit()){
        std::unique_ptr<LHPT> trajectory = kseed.loopHelixFitTrajectory();
 cout << "Line 99"  << endl;
        double t1 = trajectory->range().begin();
 cout << "Line 101 " << endl;
        double x1 = trajectory->position3(t1).x();
        double y1 = trajectory->position3(t1).y();
        double z1 = trajectory->position3(t1).z();
        _pathlength = sqrt((x1)*(x1)+(y1)*(y1)+(z1)*(z1));
        _photon_analyzer->Fill(); 

        }
// cout << "Traj Range: " << x1 << endl;
      }
    } */ 

}//end mu2e namespace
using mu2e::PhotonAna;
DEFINE_ART_MODULE(PhotonAna);
