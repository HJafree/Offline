//
// Stereo hit diagnostics.  Split out of MakeComboHits
//
// Original author D. Brown
//

// framework
#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "Offline/GeometryService/inc/GeomHandle.hh"
#include "art/Framework/Core/EDAnalyzer.h"
#include "Offline/GeometryService/inc/DetectorSystem.hh"
#include "art_root_io/TFileService.h"
#include "Offline/ProditionsService/inc/ProditionsHandle.hh"
#include "Offline/TrackerConditions/inc/StrawElectronics.hh"
// root
#include "TMath.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TLine.h"
#include "TMarker.h"
#include "TList.h"
#include "TLegend.h"
#include "TTree.h"
// data
#include "Offline/RecoDataProducts/inc/StrawHit.hh"
#include "Offline/RecoDataProducts/inc/StrawHitFlag.hh"
#include "Offline/RecoDataProducts/inc/ComboHit.hh"
#include "Offline/RecoDataProducts/inc/StrawDigi.hh"
#include "Offline/MCDataProducts/inc/StrawDigiMC.hh"
#include "Offline/MCDataProducts/inc/MCRelationship.hh"
// Utilities
#include "Offline/Mu2eUtilities/inc/SimParticleTimeOffset.hh"
#include "Offline/TrkDiag/inc/TrkMCTools.hh"
// diagnostics
#include "Offline/TrkDiag/inc/ComboHitInfo.hh"
using namespace std;

namespace mu2e
{
  class ComboHitDiag : public art::EDAnalyzer {
    public:
      explicit ComboHitDiag(fhicl::ParameterSet const&);
      virtual ~ComboHitDiag();
      virtual void beginJob();
      virtual void analyze(const art::Event& e);
    private:
      // configuration
      int _diag;
      bool _mcdiag, _digidiag, _useshfcol;
      // event object Tags
      art::InputTag   _chtag;
      art::InputTag   _shftag;
      art::InputTag   _mcdigistag;
      art::InputTag   _digistag;
      // event data cache
      const ComboHitCollection* _chcol;
      const StrawHitFlagCollection* _shfcol;
      const StrawDigiMCCollection *_mcdigis;
      const StrawDigiCollection *_digis;
      const StrawDigiADCWaveformCollection *_digiadcs;
      // time offset
      SimParticleTimeOffset _toff;
      // diagnostics
      TTree *_chdiag;
      Int_t _evt; // add event id
      XYZVectorF _pos; // average position
      XYZVectorF _wdir; // direction at this position (typically the wire direction)
      Float_t _wdist; // distance from wire center along this direction
      Float_t _wres; // estimated error along this direction
      Float_t _tres; // estimated error perpendicular to this direction
      Float_t _time; // Average time for these
      Float_t _correcttime, _dtime, _ptime;
      Float_t _edep; // average energy deposit for these
      Float_t _qual; // quality of combination
      Float_t _dz; // z extent
      Int_t _nsh, _nch; // number of associated straw hits
      Int_t _strawid; // strawid info
      Int_t _esel,_rsel, _tsel,  _bkgclust, _bkg, _stereo, _tdiv, _isolated, _strawxtalk, _elecxtalk, _calosel;
      // mc diag
      XYZVectorF _mcpos; // average MC hit position

      Float_t _mctime, _mcdist;
      Int_t _mcpdg, _mcproc, _mcgen;

      Float_t _delay[2], _threshold[2], _adcgain;
      std::vector<short unsigned> _digiadc;
      Int_t _digifwpmp;
      Int_t _digipeak;
      Float_t _digipedestal;
      Int_t _digitdc[2], _digitot[2];
      ProditionsHandle<StrawElectronics> _strawele_h;

      // per-hit diagnostics
      vector<ComboHitInfo> _chinfo;
      vector<ComboHitInfoMC> _chinfomc;
      // helper functions
      bool findData(const art::Event& evt);
  };

  ComboHitDiag::ComboHitDiag(fhicl::ParameterSet const& pset) :
    art::EDAnalyzer(pset),
    _diag   (pset.get<int>("diagLevel",1)),
    _mcdiag   (pset.get<bool>("MCdiag",true)),
    _digidiag           (pset.get<bool>("digidiag",false)),
    _useshfcol    (pset.get<bool>("UseStrawHitFlagCollection",false)),
    _chtag    (pset.get<art::InputTag>("ComboHitCollection")),
    _shftag   (pset.get<art::InputTag>("StrawHitFlagCollection","none")),
    _mcdigistag   (pset.get<art::InputTag>("StrawDigiMCCollection","makeSD")),
    _digistag           (pset.get<art::InputTag>("StrawDigiCollection","makeSD")),
    _toff(pset.get<fhicl::ParameterSet>("TimeOffsets"))
  {}

  ComboHitDiag::~ComboHitDiag(){}

  void ComboHitDiag::beginJob() {
    // create diagnostics if requested
    if(_diag > 0){
      art::ServiceHandle<art::TFileService> tfs;
      // detailed diagnostics
      _chdiag=tfs->make<TTree>("chdiag","combo hit diagnostics");
      _chdiag->Branch("evt",&_evt,"evt/I");  // add event id
      _chdiag->Branch("wdist",&_wdist,"wdist/F");
      _chdiag->Branch("pos.",&_pos);
      _chdiag->Branch("wdir.",&_wdir);
      _chdiag->Branch("wdist",&_wdist,"wdist/F");
      _chdiag->Branch("wres",&_wres,"wres/F");
      _chdiag->Branch("tres",&_tres,"tres/F");
      _chdiag->Branch("time",&_time,"time/F");
      _chdiag->Branch("correcttime",&_correcttime,"correcttime/F");
      _chdiag->Branch("dtime",&_time,"dtime/F");
      _chdiag->Branch("ptime",&_time,"ptime/F");
      _chdiag->Branch("edep",&_edep,"edep/F");
      _chdiag->Branch("qual",&_qual,"qual/F");
      _chdiag->Branch("dz",&_dz,"dz/F");
      _chdiag->Branch("nsh",&_nsh,"nsh/I");
      _chdiag->Branch("nch",&_nch,"nch/I");
      _chdiag->Branch("esel",&_esel,"esel/I");
      _chdiag->Branch("rsel",&_rsel,"rsel/I");
      _chdiag->Branch("tsel",&_tsel,"tsel/I");
      _chdiag->Branch("bkgclust",&_bkgclust,"bkgclust/I");
      _chdiag->Branch("bkg",&_bkg,"bkg/I");
      _chdiag->Branch("stereo",&_stereo,"stereo/I");
      _chdiag->Branch("tdiv",&_tdiv,"tdiv/I");
      _chdiag->Branch("strawxtalk",&_strawxtalk,"strawxtalk/I");
      _chdiag->Branch("elecxtalk",&_elecxtalk,"elecxtalk/I");
      _chdiag->Branch("isolated",&_isolated,"isolated/I");
      _chdiag->Branch("calosel",&_calosel,"calosel/I");
      _chdiag->Branch("strawid",&_strawid,"strawid/I");
      if(_diag > 1)
        _chdiag->Branch("chinfo",&_chinfo);
      if(_mcdiag){
        _chdiag->Branch("mcpos.",&_mcpos);
        _chdiag->Branch("mctime",&_mctime,"mctime/F");
        _chdiag->Branch("mcdist",&_mcdist,"mcdist/F");
        _chdiag->Branch("mcpdg",&_mcpdg,"mcpdg/I");
        _chdiag->Branch("mcproc",&_mcproc,"mcproc/I");
        _chdiag->Branch("mcgen",&_mcgen,"mcgen/I");
        if(_diag > 1)
          _chdiag->Branch("chinfomc",&_chinfomc);
      }
      if (_digidiag){
        _chdiag->Branch("digitdc",&_digitdc,"digitdccal/I:digitdchv/I");
        _chdiag->Branch("digitot",&_digitot,"digitotcal/I:digitot/I");
        _chdiag->Branch("digifwpmp",&_digifwpmp,"digifwpmp/I");
        _chdiag->Branch("digipeak",&_digipeak,"digipeak/I");
        _chdiag->Branch("digipedestal",&_digipedestal,"digipedestal/F");
        _chdiag->Branch("digiadc",&_digiadc);
        _chdiag->Branch("delay",&_delay,"delaycal/F:delayhv/F");
        _chdiag->Branch("threshold",&_threshold,"thresholdcal/F:thresholdhv/F");
        _chdiag->Branch("adcgain",&_adcgain,"adcgain/F");
      }
    }
  }

  void ComboHitDiag::analyze(const art::Event& evt ) {
    // find data in event
    findData(evt);
    _evt = evt.id().event();  // add event id
    StrawElectronics const& strawele = _strawele_h.get(evt.id());
    // loop over combo hits
    for(size_t ich = 0;ich < _chcol->size(); ++ich){
      ComboHit const& ch =(*_chcol)[ich];
      // general information
      _nsh = ch.nStrawHits();
      _nch = ch.nCombo();
      _strawid = ch.strawId().asUint16();
      _pos = ch.pos();

      _wdir = ch.wdir();
      _wdist = ch.wireDist();
      _wres = ch.wireRes();
      _tres = ch.transRes();
      _time = ch.time();
      _correcttime = ch.correctedTime();
      _dtime = ch.driftTime();
      _ptime = ch.propTime();
      _edep = ch.energyDep();
      _qual = ch.qual();
      StrawHitFlag flag;
      if(_useshfcol)
        flag = _shfcol->at(ich);
      else
        flag = ch.flag();
      _stereo = flag.hasAllProperties(StrawHitFlag::stereo);
      _tdiv = flag.hasAllProperties(StrawHitFlag::tdiv);
      _esel = flag.hasAllProperties(StrawHitFlag::energysel);
      _rsel = flag.hasAllProperties(StrawHitFlag::radsel);
      _tsel = flag.hasAllProperties(StrawHitFlag::timesel);
      _calosel = flag.hasAllProperties(StrawHitFlag::calosel);
      _strawxtalk = flag.hasAllProperties(StrawHitFlag::strawxtalk);
      _elecxtalk = flag.hasAllProperties(StrawHitFlag::elecxtalk);
      _isolated = flag.hasAllProperties(StrawHitFlag::isolated);
      _bkg = flag.hasAllProperties(StrawHitFlag::bkg);
      _bkgclust = flag.hasAllProperties(StrawHitFlag::bkgclust);
      _dz = 0.0;
      // center of this wire
      XYZVectorF cpos = _pos - _wdist*_wdir;
      // now hit-by-hit info
      _chinfo.clear();
      if(_diag > 1){
        // loop over comopnents (also ComboHits)
        ComboHitCollection::CHCIter compis;
        _chcol->fillComboHits(evt,ich,compis);
        float minz(1.0e6),maxz(-1.0);
        for(auto compi : compis) {
          ComboHit const& comp = *compi;
          if(comp.pos().z() > maxz) maxz = comp.pos().z();
          if(comp.pos().z() < minz) minz = comp.pos().z();
          ComboHitInfo chi;

          chi._pos= comp.pos();
          chi._wdist = comp.wireDist();
          chi._wres = comp.wireRes();
          chi._tres = comp.transRes();
          chi._tdrift = comp.driftTime();
          chi._thit = comp.time();
          chi._strawid = comp.strawId().straw();
          chi._panelid = comp.strawId().panel();

          XYZVectorF dpos = comp.pos()-ch.pos();
          chi._dwire = dpos.Dot(ch.wdir());
          chi._dwerr = comp.wireRes();
          chi._dterr = comp.transRes();
          chi._dperp = sqrt(dpos.mag2() - chi._dwire*chi._dwire);
          chi._dtime = comp.time()- ch.time();
          chi._dedep = comp.energyDep() - ch.energyDep();
          chi._ds = comp.strawId().straw() - compis.front()->strawId().straw();
          chi._dp = comp.strawId().panel() - compis.front()->strawId().panel();
          chi._nch = comp.nCombo();
          chi._nsh = comp.nStrawHits();
          _chinfo.push_back(chi);
        }
        _dz = maxz-minz;
      }
      if(_mcdiag){
        _chinfomc.clear();
        // get the StrawDigi indices associated with this ComboHit
        std::vector<StrawDigiIndex> shids;
        _chcol->fillStrawDigiIndices(evt,ich,shids);
        if(shids.size() != ch.nStrawHits())
          throw cet::exception("DIAG")<<"mu2e::ComboHitDiag: invalid ComboHit" << std::endl;
        // use the 1st hit to define the MC match; this is arbitrary should be an average FIXME!
        StrawDigiMC const& mcd1 = _mcdigis->at(shids[0]);
        auto const& spmcp = mcd1.strawGasStep(StrawEnd::cal);
        art::Ptr<SimParticle> spp = spmcp->simParticle();
        _mctime = _toff.timeWithOffsetsApplied(*spmcp);
        _mcpdg = spp->pdgId();
        _mcproc = spp->creationCode();
        if(spp->genParticle().isNonnull())
          _mcgen = spp->genParticle()->generatorId().id();
        else
          _mcgen = -1;
        // find the relation with each hit
        _mcpos = XYZVectorF(0.0,0.0,0.0);
        for(auto shi : shids) {
          ComboHitInfoMC chimc;
          StrawDigiMC const& mcd = _mcdigis->at(shi);
          //chimc._rel = MCRelationship::relationship(mcd,mcd1);
          chimc._mcpos = XYZVectorF(spmcp->position().x(),spmcp->position().y(), spmcp->position().z() );

          MCRelationship rel(mcd,mcd1);
          chimc._rel = rel.relationship();
          _chinfomc.push_back(chimc);
          // find average MC properties
          _mcpos += XYZVectorF(spmcp->position().x(), spmcp->position().y(), spmcp->position().z());
        }
        _mcpos /= shids.size();
        _mcdist = (_mcpos - cpos).Dot(_wdir);


      }
      if (_digis != 0){
        std::vector<StrawDigiIndex> shids;
        _chcol->fillStrawDigiIndices(evt,ich,shids);
        // use the 1st hit to define the MC match; this is arbitrary should be an average FIXME!
        auto digi = _digis->at(shids[0]);
        for(size_t iend=0;iend<2;++iend){
          _digitdc[iend] = digi.TDC(static_cast<StrawEnd::End>(iend));
          _digitot[iend] = digi.TOT(static_cast<StrawEnd::End>(iend));
        }
        _digifwpmp = digi.PMP();

        _delay[StrawEnd::cal] = strawele.getTimeOffsetStrawCal(ch.strawId().uniqueStraw());
        _delay[StrawEnd::hv] = strawele.getTimeOffsetStrawHV(ch.strawId().uniqueStraw());
        for (size_t iend=0;iend<2;++iend){
          _threshold[iend] = strawele.threshold(ch.strawId(), static_cast<StrawEnd::End>(iend));
        }
        _adcgain = strawele.currentToVoltage(ch.strawId(),StrawElectronics::adc);
      }
      if (_digiadcs != 0){
        std::vector<StrawDigiIndex> shids;
        _chcol->fillStrawDigiIndices(evt,ich,shids);
        // use the 1st hit to define the MC match; this is arbitrary should be an average FIXME!
        auto digiadc = _digiadcs->at(shids[0]);
        _digiadc = digiadc.samples();
        _digipedestal = 0;
        _digipeak = 0;
        for (size_t i=0;i<_digiadc.size();++i){
          if (i<strawele.nADCPreSamples()){
            _digipedestal += _digiadc[i];
          }
          if (_digiadc[i] > _digipeak)
            _digipeak = _digiadc[i];
        }
        _digipedestal /= (float) strawele.nADCPreSamples();
      }

      _chdiag->Fill();
    }
  }

  bool ComboHitDiag::findData(const art::Event& evt){
    _chcol = 0;  _shfcol = 0; _mcdigis = 0; _digis = 0; _digiadcs = 0;
    auto chH = evt.getValidHandle<ComboHitCollection>(_chtag);
    _chcol = chH.product();
    if(_useshfcol){
      auto shfH = evt.getValidHandle<StrawHitFlagCollection>(_shftag);
      _shfcol = shfH.product();
    }
    if(_mcdiag){
      auto mcdH = evt.getValidHandle<StrawDigiMCCollection>(_mcdigistag);
      _mcdigis = mcdH.product();
      // update time offsets
      _toff.updateMap(evt);
    }
    if (_digidiag){
      auto dH = evt.getValidHandle<StrawDigiCollection>(_digistag);
      _digis = dH.product();
      auto daH = evt.getValidHandle<StrawDigiADCWaveformCollection>(_digistag);
      _digiadcs = daH.product();
    }
    return _chcol != 0 && (!_useshfcol || _shfcol != 0) && (_mcdigis != 0 || !_mcdiag) && ((_digis != 0 && _digiadcs != 0) || !_digidiag);
  }
}
// Part of the magic that makes this class a module.
using mu2e::ComboHitDiag;
DEFINE_ART_MODULE(ComboHitDiag);

