////////////////////////////////////////////////////////////////////////
// Class:       RecoInTpcAna
// Module Type: analyzer
// File:        RecoInTpcStudy_module.cc
//
// Generated at Thu Aug  1 12:12:14 2019 by Hunter Sullivan using artmod
// from cetpkgsupport v1_10_02.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

namespace piinelastic
{

class RecoInTpcAna;

class RecoInTpcAna : public art::EDAnalyzer {
public:
  explicit RecoInTpcAna(fhicl::ParameterSet const & p);
  // The destructor generated by the compiler is fine for classes
  // without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  RecoInTpcAna(RecoInTpcAna const &) = delete;
  RecoInTpcAna(RecoInTpcAna &&) = delete;
  RecoInTpcAna & operator = (RecoInTpcAna const &) = delete;
  RecoInTpcAna & operator = (RecoInTpcAna &&) = delete;

  // Required functions.
  void analyze(art::Event const & e) override;

  // Selected optional functions.
  void beginJob() override;
  void endJob() override;
  void reconfigure(fhicl::ParameterSet const & p) override;

private:

  TH1D* hRecoPitch       = nullptr;
  TH1D* hRecoDeDx        = nullptr;
  TH1D* hRecoEnDep       = nullptr;
  TH1D* hRecoResR        = nullptr;
  TH1D* hRecoZPos        = nullptr;
  TH1D* hRecoEnLossInTpc = nullptr;

};


//------------------------------------------------------------------------------
RecoInTpcAna::RecoInTpcAna(fhicl::ParameterSet const & p)
  : EDAnalyzer(p)
{}

//------------------------------------------------------------------------------
void RecoInTpcAna::reconfigure(fhicl::ParameterSet const & p)
{
  fTrackModuleLabel         = pset.get< std::string >("TrackModuleLabel"      , "pmtrack");
  fCalorimetryModuleLabel   = pset.get< std::string >("CalorimetryModuleLabel", "calo"     );
  fWCTrackLabel 		        = pset.get< std::string >("WCTrackLabel"          , "wctrack"  );
  fWC2TPCModuleLabel      	= pset.get< std::string >("WC2TPCModuleLabel"     , "wctracktpctrackmatch");
}

//------------------------------------------------------------------------------
void RecoInTpcAna::analyze(art::Event const & e)
{
  // Let's get the reco and wc stuff
  art::Handle< std::vector<ldp::WCTrack> > wctrackHandle;   // Container of wc tracks  
  std::vector<art::Ptr<ldp::WCTrack> >     wctrack;         // Vector of wc tracks  
  art::Handle< std::vector<recob::Track> > trackListHandle; // Container of reco tracks
  std::vector<art::Ptr<recob::Track> >     tracklist;       // Vector of wc tracks  

  // Find which recontructed track is associated with the WC
  if(!evt.getByLabel(fWCTrackLabel, wctrackHandle)) return;
  if(!evt.getByLabel(fTrackModuleLabel,trackListHandle)) return;
  nEvts++;

  // Fill the container of reco tracks
  art::fill_ptr_vector(wctrack, wctrackHandle);
  art::fill_ptr_vector(tracklist, trackListHandle);

  art::FindOneP<recob::Track> fWC2TPC(wctrackHandle, evt, fWC2TPCModuleLabel);
  int matchedRecoTrkKey = -99999;
  if (fWC2TPC.isValid())
  {
    for (unsigned int indexAssn = 0; indexAssn < fWC2TPC.size(); ++indexAssn)
    {
      cet::maybe_ref<recob::Track const> trackWC2TPC(*fWC2TPC.at(indexAssn));
      if (!trackWC2TPC) continue;
      recob::Track const &aTrack(trackWC2TPC.ref());
      matchedRecoTrkKey = aTrack.ID(); // This checks out OK
    }
  } // if there's an associated track
  // Now we know what ID identifies the reco track we're interested in.

  // If we didn't get a track :,(
  if (matchedRecoTrkKey == -99999) return;
  nMatchedTracks++;

  // Let's define the containers of the important information about the track 
  std::vector<double> vRecoPitch;
  std::vector<double> vRecoDeDx;
  std::vector<double> vRecoEnDep;
  std::vector<double> vRecoResR;
  std::vector<double> vRecoZPos;
  std::vector<double> vRecoIncidentKe;

  // Let's get the calorimetry of the matched track
  art::FindManyP<anab::Calorimetry> fmcal(trackListHandle, evt, fCalorimetryModuleLabel);
  art::Ptr<recob::Track> theRecoTrk; 

  // Look for matched tracks
  int nIdMatch = 0;                
  for (auto recoTrk : tracklist)
  {
    // We keep only the matched one
    if (recoTrk->ID() != matchedRecoTrkKey) continue;

    // We got it!
    theRecoTrk = recoTrk; 
    nIdMatch++;
  }//<-- End looking for matched tracks
  if (nIdMatch > 1) throw cet::exception("RecoInTpcAna") << "More than one matched track...\n";

  // Let's make sure the track first point is actually it (directionality problem).
  auto realFirstValidPos = (theRecoTrk->TrajectoryPoint(theRecoTrk->FirstValidPoint())).position;
  auto realLastValidPos  = (theRecoTrk->TrajectoryPoint(theRecoTrk->LastValidPoint())).position;
  bool isInvertedTracking = false; // Is the tracking inverted? If it is, we need to flip all the vectors later...
    
  // Check for track inversion
  if (realLastValidPos.Z() < realFirstValidPos.Z())
  {
      isInvertedTracking = true;
      std::cout << "point " << realFirstValidPos.Z() << " " << realLastValidPos.Z() << " " << isInvertedTracking << "\n";
      auto bogusFirst = realFirstValidPos;
      realFirstValidPos = realLastValidPos;
      realLastValidPos = bogusFirst;
  }

  recoStaPos = realFirstValidPos.Vect();
  recoEndPos = realFirstValidPos.Vect();
  std::cout << "\n\n------------------>" << run << " " << subrun << " " << eventN << "\n"
            << "Track " << recoStaPos.X() << " " << recoStaPos.Y() << " " << recoStaPos.Z() << " --- " << recoEndPos.X() << " " << recoEndPos.Y() << " " << recoEndPos.Z() << "\n";

  // Pileup check
  for (auto recoTrkPileUp : tracklist)
  {
    if (recoTrkPileUp->ID() == matchedRecoTrkKey) continue; // Skip matched track

    auto recoTrkPileUpFirstPos = (recoTrkPileUp->TrajectoryPoint(recoTrkPileUp->FirstValidPoint())).position;
    auto recoTrkPileUpLastPos  = (recoTrkPileUp->TrajectoryPoint(recoTrkPileUp->LastValidPoint())).position;

    if (recoTrkPileUpLastPos.Z() < recoTrkPileUpFirstPos.Z())
    {
      std::cout << "point " << recoTrkPileUpFirstPos.Z() << " " << recoTrkPileUpLastPos.Z() << " is inverted!\n";
      auto bogusFirst = recoTrkPileUpFirstPos;
      realFirstValidPos = recoTrkPileUpLastPos;
      recoTrkPileUpLastPos = bogusFirst;
    }

    double tracksDistance = (recoEndPos - recoTrkPileUpFirstPos).Mag();
    if (recoTrkPileUpFirstPos.Z() < RANGE_TRKS_FF)
    {
      std::cout << "          firstPos "; recoTrkPileUpFirstPos.Print();
      std::cout << "           lastPos "; recoTrkPileUpLastPos.Print();
      std::cout << "    tracksDistance " << tracksDistance << std::endl;                                                                                                                                                                                                                               nTracksTPCFF++;
    }
  } //<-- End PileUp Check
  std::cout << "nTracksTPCFF: " << nTracksTPCFF << "\n";

  // Start tracking if the calo for my reco track is valid
  size_t furtherstInZCaloPointIndex = 0;
  double furtherstInZCaloPointZ     = 0.;
  if (fmcal.isValid())
  {
    std::vector<art::Ptr<anab::Calorimetry>> calos = fmcal.at(theRecoTrk.key());

    for (size_t iC = 0; iC < calos.size(); ++iC)
    {
      if (!calos[iC]->PlaneID().isValid)continue;
      if (calos[iC]->PlaneID().Plane == 0)continue; // Skip Induction Plane
      nValidCaloOnColl++;

      // Loop over calorimetry points
      for (size_t iE = 0; iE < calos[iC]->dEdx().size(); ++iE)
      {
        if (calos[iC]->dEdx()[iE] > HIT_DEDX_THRESHOLD)continue;
        
        //If the following happens, there's a mess in the calorimetry module, skip
        if (calos[iC]->XYZ()[iE].Z() < 0 || calos[iC]->XYZ()[iE].Z() > 90.)continue;
        // Let's register the furtherst point in Z to determine if the track interacted or not.
        if (calos[iC]->XYZ()[iE].Z() > furtherstInZCaloPointZ) furtherstInZCaloPointIndex = iE;
        if (!inActiveVolume(calos[iC]->XYZ()[iE].Vect()))continue;
        
        vRecoPitch.push_back(calos[iC]->TrkPitchVec()[iE]);
        vRecoDeDx.push_back(calos[iC]->dEdx()[iE]);
        vRecoEnDep.push_back(calos[iC]->dEdx()[iE] * calos[iC]->TrkPitchVec()[iE]);
        vRecoResR.push_back(calos[iC]->ResidualRange()[iE]);
        vRecoZPos.push_back(calos[iC]->XYZ()[iE].Z());
      } //<-- End loop on calo points

      if (!furtherstInZCaloPointIndex)
      { 
        vRecoPitch.clear();
        vRecoPitch.clear();
        vRecoDeDx.clear();
        vRecoEnDep.clear();
        vRecoResR.clear();
        vRecoZPos.clear();
      }

      // Let's check if this track is interacting
      if (furtherstInZCaloPointIndex) isTrackInteracting = inActiveVolume(calos[iC]->XYZ()[furtherstInZCaloPointIndex]);
    } // Loop on Collection Vs Induction
  }   //<-- End is calorimetry valid

  WCMom = wctrack[0]->Momentum();
  WC4X  = wctrack[0]->HitPosition(3, 0);
  WC4Y  = wctrack[0]->HitPosition(3, 1);

// ###############
  // Sanity checks
  if (!vRecoDeDx.size())
    return; // If there are no points, return
  if (nIdMatch != 1)
    return; // If for some reason I couldn't find the match track, I should return
  if (recoEndX < -100. || recoEndY < -100. || recoEndZ < -100.)
    throw cet::exception("RecoVsMcInTpc") << "Weird End of track...\n";
  if (WCMom < 0.)
    throw cet::exception("RecoVsMcInTpc") << "WCMom < 0...\n";
  if (mass < 0.)
    throw cet::exception("RecoVsMcInTpc") << "Mass < 0...\n";
  if (vRecoPitch.size() != vRecoDeDx.size() != 
      vRecoEnDep.size() != vRecoResR.size() !=
      vRecoPitch.size() != vRecoZPos.size() !=
      vRecoPitch.size() != vRecoEnDep.size())
    throw cet::exception("RecoVsMcInTpc")
        << "Different dimensions... \n";
// ###############

  // Remember? If the z are inverted, flip the vectors
  if (isInvertedTracking)
  {
    //Is track inverted
    std::reverse(vRecoPitch.begin(), vRecoPitch.end());
    std::reverse(vRecoDeDx.begin(), vRecoDeDx.end());
    std::reverse(vRecoEnDep.begin(), vRecoEnDep.end());
    std::reverse(vRecoResR.begin(), vRecoResR.end());
    std::reverse(vRecoZPos.begin(), vRecoZPos.end());
  }

  // Start filling plots
  double energyDepositedThusFar = 0.;
  for (size_t iDep = 0; iDep < vRecoEnDep.size(); iDep++)
  {
    energyDepositedThusFar += vRecoEnDep[iDep];
  }
  hRecoEnLossInTpc->Fill(energyDepositedThusFar);

  for (int iPt = 0; iPt < vRecoPitch.size(); iPt++)
  {
    hRecoPitch->Fill(vRecoPitch[iPt]);
    hRecoDeDx->Fill(vRecoDeDx[iPt]);
    hRecoEnDep->Fill(vRecoEnDep[iPt]);
    hRecoResR->Fill(vRecoResR[iPt]);
    hRecoZPos->Fill(hRecoZPos[iPt]);
  }
}

//------------------------------------------------------------------------------
void RecoInTpcAna::beginJob()
{
  hRecoPitch = tfs->make<TH1D>("hRecoPitch", "RecoPitch", 1000, 0, 10);
  hRecoDeDx  = tfs->make<TH1D>("hRecoDeDx",  "RecoDeDx",  1000, 0, 10);
  hRecoEnDep = tfs->make<TH1D>("hRecoEnDep", "RecoEnDep", 1000, 0, 10);
  hRecoResR  = tfs->make<TH1D>("hRecoResR",  "RecoResR",  1000, 0, 100);
  hRecoZPos  = tfs->make<TH1D>("hRecoZPos",  "RecoZPos",  1000, 0, 100);
  hRecoEnLossInTpc = tfs->make<TH1D>("hRecoEnLossInTpc", "RecoEnLossInTpc", 1000, 0, 1000);
}

//------------------------------------------------------------------------------
void RecoInTpcAna::endJob()
{
  // Implementation of optional member function here.
}

DEFINE_ART_MODULE(RecoInTpcAna)
}