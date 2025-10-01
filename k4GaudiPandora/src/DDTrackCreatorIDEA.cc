#include "DDTrackCreatorIDEA.h"

#include "Pandora/PandoraEnumeratedTypes.h"
#include "Pandora/PandoraInputTypes.h"

DDTrackCreatorIDEA::DDTrackCreatorIDEA(const Settings& settings, pandora::Pandora& pandora, const Gaudi::Algorithm* algorithm)
    : m_settings(settings), m_pandora(pandora), m_algorithm(*algorithm) {}

void DDTrackCreatorIDEA::CopyTrackState(const edm4hep::TrackState& pTrackState,
                                        pandora::InputTrackState& inputTrackState) const {
  // copy-paste of DDTrackCreatorBase
  const double pt(m_settings.m_bField * 2.99792e-4 / std::fabs(pTrackState.omega));

  const double px(pt * std::cos(pTrackState.phi));
  const double py(pt * std::sin(pTrackState.phi));
  const double pz(pt * pTrackState.tanLambda);

  const double xs(pTrackState.referencePoint[0]);
  const double ys(pTrackState.referencePoint[1]);
  const double zs(pTrackState.referencePoint[2]);

  inputTrackState = pandora::TrackState(xs, ys, zs, px, py, pz);
}

void DDTrackCreatorIDEA::GetTrackStates(const edm4hep::Track& pTrack,
                                        PandoraApi::Track::Parameters& trackParameters) const {
  // copy-paste of DDTrackCreatorBase
  // local lambda function
  auto edm4hepTrackState = [](const edm4hep::Track& track, int location) -> edm4hep::TrackState {
    for (const auto& ts : track.getTrackStates()) {
      if (ts.location == location)
        return ts;
    }
    throw pandora::StatusCodeException(pandora::STATUS_CODE_NOT_FOUND);
  };

  const auto& pTrackState = edm4hepTrackState(pTrack, edm4hep::TrackState::AtIP);

  const double pt(m_settings.m_bField * 2.99792e-4 / std::fabs(pTrackState.omega));
  trackParameters.m_momentumAtDca =
      pandora::CartesianVector(std::cos(pTrackState.phi), std::sin(pTrackState.phi), pTrackState.tanLambda) * pt;

  this->CopyTrackState(edm4hepTrackState(pTrack, edm4hep::TrackState::AtFirstHit),
                 trackParameters.m_trackStateAtStart);

  this->CopyTrackState(edm4hepTrackState(pTrack, edm4hep::TrackState::AtLastHit),
                       trackParameters.m_trackStateAtEnd);
  // FIXME Doesn't this throw if the track doesn't reach calo?
  this->CopyTrackState(edm4hepTrackState(pTrack, edm4hep::TrackState::AtCalorimeter),
                       trackParameters.m_trackStateAtCalorimeter);

  trackParameters.m_isProjectedToEndCap =
      ((std::fabs(trackParameters.m_trackStateAtCalorimeter.Get().GetPosition().GetZ()) < m_settings.m_endcapInnerZ)
           ? false
           : true);

  // FIXME ignore timing for the moment
  trackParameters.m_timeAtCalorimeter = -1;
}

pandora::StatusCode DDTrackCreatorIDEA::CreateTracks(const std::vector<edm4hep::Track>& tracks) const {
  for (const auto& pTrack : tracks) {
    // Note: copy-paste of the DDTrackCreatorCLIC

    // Take the first track state for the parameters
    const auto& trackState = pTrack.getTrackStates()[0];

    // Proceed to create the pandora track
    object_creation::TrackParameters trackParameters;
    trackParameters.m_d0 = trackState.D0;
    trackParameters.m_z0 = trackState.Z0;
    trackParameters.m_pParentAddress = &pTrack;

    const float signedCurvature = trackState.omega;
    trackParameters.m_particleId = 0; // no PID at this stage
    trackParameters.m_mass = 0.; // no mass at this stage

    if (signedCurvature != 0.f)
      trackParameters.m_charge = static_cast<int>(signedCurvature / std::fabs(signedCurvature));

    try {
      GetTrackStates(pTrack, trackParameters);
      // FIXME Assume all track reach the calo for now
      trackParameters.m_reachesCalorimeter = true;
      // FIXME isn't this already done in GetTrackStates?
      // GetTrackStatesAtCalo(pTrack, trackParameters);

      // FIXME ignore low-pt or MIP particles for now
      trackParameters.m_canFormPfo = true;
      trackParameters.m_canFormClusterlessPfo = false;

      PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                              PandoraApi::Track::Create(m_pandora, trackParameters))
    } catch (pandora::StatusCodeException& statusCodeException) {
      m_algorithm.error() << "Failed to extract a track: " << statusCodeException.ToString() << endmsg;
      m_algorithm.debug() << " failed track : " << pTrack << endmsg;
    }
  }

  return pandora::STATUS_CODE_SUCCESS;
}

// void DDTrackCreatorIDEA::GetTrackStatesAtCalo(const edm4hep::Track& track,
//                                               object_creation::TrackParameters& trackParameters) const {
//   if (!trackParameters.m_reachesCalorimeter.Get()) {
//     m_algorithm.debug() << "Track does not reach the ECal" << endmsg;
//     return;
//   }
//
//   size_t i = static_cast<size_t>(-1);
//   for (size_t j = 0; j < track.getTrackStates().size(); ++j) {
//     if (track.getTrackStates()[j].location == edm4hep::TrackState::AtCalorimeter) {
//       i = j;
//       break;
//     }
//   }
//
//   if (i == static_cast<size_t>(-1)) {
//     m_algorithm.verbose() << "Track does not have a trackState at calorimeter" << endmsg;
//     return;
//   }
//
//   const auto& trackAtCalo = track.getTrackStates(i);
//   const auto& tsPosition = trackAtCalo.referencePoint;
//
//   // WARNING Assume extrapolation had already been done out of the box
//   pandora::InputTrackState pandoraTrackState;
//   this->CopyTrackState(trackAtCalo, pandoraTrackState);
//   trackParameters.m_trackStates.push_back(pandoraTrackState);
//
//   return;
// }
