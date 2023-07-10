// Generates particles in a flat shaped spectrum, size determined by FCL params these will be attached to a mu- in
// the input SimParticleCollection.
// This module throws an exception if no suitable muon is found.
//
// S Middleton, 2021

#include <iostream>
#include <string>
#include <cmath>
#include <memory>

#include "CLHEP/Vector/ThreeVector.h"
#include "CLHEP/Vector/LorentzVector.h"
#include "CLHEP/Random/RandomEngine.h"
#include "CLHEP/Random/RandExponential.h"
#include "CLHEP/Random/RandFlat.h"
#include "CLHEP/Units/PhysicalConstants.h"

#include "fhiclcpp/types/Atom.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art_root_io/TFileService.h"
#include "Offline/SeedService/inc/SeedService.hh"
#include "Offline/GlobalConstantsService/inc/GlobalConstantsHandle.hh"
#include "Offline/GlobalConstantsService/inc/ParticleDataList.hh"
#include "Offline/GlobalConstantsService/inc/PhysicsParams.hh"
#include "Offline/Mu2eUtilities/inc/RandomUnitSphere.hh"
#include "Offline/DataProducts/inc/PDGCode.hh"
#include "Offline/MCDataProducts/inc/StageParticle.hh"
#include "Offline/Mu2eUtilities/inc/simParticleList.hh"

#include <TTree.h>
#include <TH2F.h>
#include <TH3F.h>
namespace mu2e {

//================================================================
class FlatMuonDaughterGenerator : public art::EDProducer {
public:
struct Config {
using Name=fhicl::Name;
using Comment=fhicl::Comment;
fhicl::Atom<double> startMom{Name("startMom"),0};
fhicl::Atom<double> endMom{Name("endMom"),105};
fhicl::Atom<art::InputTag> inputSimParticles{Name("inputSimParticles"),Comment("A SimParticleCollection with input stopped muons.")};
fhicl::Atom<std::string> stoppingTargetMaterial{Name("stoppingTargetMaterial"),Comment("material")};
fhicl::Atom<unsigned> verbosity{Name("verbosity")};
fhicl::Atom<int> pdgId{Name("pdgId"),Comment("pdg id of daughter particle")};
};

using Parameters= art::EDProducer::Table<Config>;
explicit FlatMuonDaughterGenerator(const Parameters& conf);
virtual void beginJob() override;
virtual void produce(art::Event& event) override;

//----------------------------------------------------------------
private:

double particleMass_;
double startMom_;
double endMom_;
double muonLifeTime_;

art::ProductToken<SimParticleCollection> const simsToken_;

unsigned verbosity_;

art::RandomNumberGenerator::base_engine_t& eng_;
CLHEP::RandFlat randFlat_;
CLHEP::RandExponential randExp_;
RandomUnitSphere   randomUnitSphere_;
ProcessCode process;
int pdgId_;
PDGCode::type pid;
TTree* _photon_generator;
TH2F* _2Dphoton_genxy;
TH2F* _2Dphoton_genrz;
TH2F* _2Dphoton_zmom;
TH2F* _2Dphoton_ztime;
TH3F* _3Dphoton_genxyz;
Float_t _momentum;
Float_t _energy;
Float_t _time_hist;
Float_t _x;
Float_t _y;
Float_t _z;
Float_t _r;


};

//================================================================
FlatMuonDaughterGenerator::FlatMuonDaughterGenerator(const Parameters& conf)
: EDProducer{conf}
, particleMass_(GlobalConstantsHandle<ParticleDataList>()->particle(static_cast<PDGCode::type>(conf().pdgId())).mass())
, startMom_(conf().startMom())
, endMom_(conf().endMom())
, muonLifeTime_{GlobalConstantsHandle<PhysicsParams>()->getDecayTime(conf().stoppingTargetMaterial())}
, simsToken_{consumes<SimParticleCollection>(conf().inputSimParticles())}
, verbosity_{conf().verbosity()}
, eng_{createEngine(art::ServiceHandle<SeedService>()->getSeed())}
, randFlat_{eng_}
, randExp_{eng_}
, randomUnitSphere_{eng_}
, pdgId_(conf().pdgId())

{
produces<mu2e::StageParticleCollection>();
pid = static_cast<PDGCode::type>(pdgId_);

if (pid == PDGCode::e_minus) { process = ProcessCode::mu2eFlateMinus; }
else if (pid == PDGCode::e_plus) { process = ProcessCode::mu2eFlatePlus; }
else if (pid == PDGCode::gamma) { process = ProcessCode::mu2eFlatPhoton; }
else {
throw   cet::exception("BADINPUT")
<<"FlatMuonDaughterGenerator::produce(): No process associated with chosen PDG id\n";
}

}

void FlatMuonDaughterGenerator::beginJob() { //TODO - can add TTree and THistograms here if required
art::ServiceHandle<art::TFileService> tfs;
_photon_generator=tfs->make<TTree>("photon_generation"," Diagnostics for Photon Generation Track Fitting");
_2Dphoton_genxy = tfs->make<TH2F>("2D photon generation XY","Position of muons hits in X-Y plane " ,100,-3980,-3820,100,-80, 80);
_2Dphoton_genxy->GetXaxis()->SetTitle("Position in x [mm]");
_2Dphoton_genxy->GetYaxis()->SetTitle("Position in y [mm]");
_2Dphoton_genrz = tfs->make<TH2F>("2D photon generation RZ","Position of muon hits in R-Z plane" ,100, 3980,3820,150,5400, 6300);
_2Dphoton_genrz->GetXaxis()->SetTitle("Position in r [mm]");
_2Dphoton_genrz->GetYaxis()->SetTitle("Position in z [mm]");
_3Dphoton_genxyz = tfs->make<TH3F>("3D photon generation","Position of muon hits in X-Y-Z plane " ,100,-3980,-3820,150,5400,6300,100,-80,80);
_3Dphoton_genxyz->GetXaxis()->SetTitle("Position in x [mm]");
_3Dphoton_genxyz->GetYaxis()->SetTitle("Position in z [mm]");
_3Dphoton_genxyz->GetZaxis()->SetTitle("Position in y [mm]");
_2Dphoton_zmom = tfs->make<TH2F>("2D photon generation Mom-Z","Momentum and position in z axis" ,150,5400, 6300,100,-500,500);
_2Dphoton_zmom->GetXaxis()->SetTitle("Position in z [mm]");
_2Dphoton_zmom->GetYaxis()->SetTitle("Momentum [MeV/c]");
_2Dphoton_ztime = tfs->make<TH2F>("2D photon generation Time-Z","Time and position in z axis" ,150,5400, 6300,100,0,6000);
_2Dphoton_ztime->GetXaxis()->SetTitle("Position in z [mm]");
_2Dphoton_ztime->GetYaxis()->SetTitle("Time [ns]");
_photon_generator->Branch("Momentum", &_momentum, "Momentum/F");
_photon_generator->Branch("Energy", &_energy, "Energy/F");
_photon_generator->Branch("Time", &_time_hist, "Time/F");
_photon_generator->Branch("x", &_x, "x/F");
_photon_generator->Branch("y", &_y, "y/F");
_photon_generator->Branch("z", &_z, "z/F");
}

//================================================================
void FlatMuonDaughterGenerator::produce(art::Event& event) {
auto output{std::make_unique<StageParticleCollection>()};

const auto simh = event.getValidHandle<SimParticleCollection>(simsToken_);
const auto mus = stoppedMuMinusList(simh);

if(mus.empty()) {
throw   cet::exception("BADINPUT")
<<"FlatMuonDaughterGenerator::produce(): no suitable stopped mu- in the input SimParticleCollection\n";

}

const auto mustop = mus.at(eng_.operator unsigned int() % mus.size());
double randomMom = randFlat_.fire(startMom_, endMom_);
double randomE = sqrt(particleMass_*particleMass_ + randomMom*randomMom);
double time = mustop->endGlobalTime() + randExp_.fire(muonLifeTime_);


output->emplace_back(mustop,
process,
pid,
mustop->endPosition(),
CLHEP::HepLorentzVector{randomUnitSphere_.fire(randomMom), randomE},
time
);

event.put(std::move(output));

_momentum=randomMom;
_energy=randomE;
_time_hist=time;
_x = mustop->endPosition().x();
_y = mustop->endPosition().y();
_z = mustop->endPosition().z();
_r = (sqrt(_x*_x+_y*_y));
_photon_generator->Fill();
_2Dphoton_genxy->Fill(_x,_y);
_2Dphoton_genrz->Fill(_r,_z);
_2Dphoton_zmom->Fill(_z,_momentum);
_2Dphoton_ztime->Fill(_z,_time_hist);
_3Dphoton_genxyz->Fill(_x,_z,_y);
}



//================================================================
} // namespace mu2e

DEFINE_ART_MODULE(mu2e::FlatMuonDaughterGenerator);
