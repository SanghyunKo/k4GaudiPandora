#ifndef k4GaudiPandora_DDPfoCreatorIdea_h
#define k4GaudiPandora_DDPfoCreatorIdea_h 1

#include "Api/PandoraApi.h"

#include "Gaudi/Algorithm.h"

#include "edm4hep/ReconstructedParticleCollection.h"
#include "edm4hep/ClusterCollection.h"

// copy-paste of DDPfoCreator
// TODO can't we use only one?
class DDPfoCreatorIdea {
public:
  class Settings {
  public:
    Settings()=default;
    ~Settings()=default;

    // TODO empty settings for now
  };

public:
  DDPfoCreatorIdea(const Settings& settings, pandora::Pandora& pandora, const Gaudi::Algorithm* algorithm);
  ~DDPfoCreatorIdea()=default;

  pandora::StatusCode CreatePFOs(const edm4hep::ClusterCollection& inputClusterColl,
                                 edm4hep::ClusterCollection& clusterColl,
                                 edm4hep::ReconstructedParticleCollection& aPfoColl) const;

private:
  void AddTracksToRecoParticle(const pandora::ParticleFlowObject* const pPandoraPfo,
                               edm4hep::MutableReconstructedParticle& pReconstructedParticle) const;
  void AddClustersToRecoParticle(const pandora::ParticleFlowObject* const pPandoraPfo,
                                 const edm4hep::ClusterCollection& inputClusterColl,
                                 edm4hep::ClusterCollection& clusterColl,
                                 edm4hep::MutableReconstructedParticle& reconstructedParticle) const;
  void SetRecoParticlePropertiesFromPFO(const pandora::ParticleFlowObject* const pPandoraPfo,
                                        edm4hep::MutableReconstructedParticle& reconstructedParticle) const;

  const Settings m_settings;   ///< The pfo creator settings
  pandora::Pandora& m_pandora; ///< Reference to the pandora object from which to extract the pfos
  const Gaudi::Algorithm& m_algorithm; ///< Reference to the Gaudi algorithm for message streaming
};

#endif
