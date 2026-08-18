#ifndef DDTrackCreatorIdea_h
#define DDTrackCreatorIdea_h 1

#include "DDTrackCreatorBase.h"

/**
 *  @brief  Track creator for the IDEA detector.
 *
 *  The tracks arrive from TracksFromGenParticles already selected (calo-reaching, no ghost helices)
 *  and already extrapolated to the calorimeter face, so this creator neither applies quality cuts
 *  nor builds the DDKalTest tracking system: only CreateTracks differs from the base.
 */
class DDTrackCreatorIdea : public DDTrackCreatorBase {
public:
  DDTrackCreatorIdea(const Settings& settings, pandora::Pandora& pandora, const Gaudi::Algorithm* alg);
  ~DDTrackCreatorIdea() override = default;

  pandora::StatusCode CreateTracks(const std::vector<edm4hep::Track>& tracks) override;
};

#endif
