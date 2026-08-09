#ifndef DDTrackCreatorIdea_h
#define DDTrackCreatorIdea_h 1

#include "Api/PandoraApi.h"
#include "Pandora/ObjectCreation.h"

#include "edm4hep/ReconstructedParticleCollection.h"
#include "edm4hep/Track.h"

// Gaudi
#include "GaudiKernel/Algorithm.h"

class DDTrackCreatorIdea {
public:
  class Settings {
  public:
    Settings()=default;
    ~Settings()=default;

    float m_bField;
    float m_endcapInnerZ;
  };

  DDTrackCreatorIdea(const Settings& settings, pandora::Pandora& pandora, const Gaudi::Algorithm* alg);
  ~DDTrackCreatorIdea()=default;

  pandora::StatusCode CreateTracks(const std::vector<edm4hep::Track>& tracks) const;
  // void GetTrackStatesAtCalo(const edm4hep::Track& track, object_creation::TrackParameters& params) const;

  void GetTrackStates(const edm4hep::Track& pTrack, PandoraApi::Track::Parameters& trackParameters) const;
  void CopyTrackState(const edm4hep::TrackState& pTrackState, pandora::InputTrackState& inputTrackState) const;

private:
  const Settings m_settings;
  pandora::Pandora& m_pandora;
  const Gaudi::Algorithm& m_algorithm;
};

#endif
