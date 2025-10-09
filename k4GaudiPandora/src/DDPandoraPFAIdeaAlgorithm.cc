#include "DDPandoraPFAIdeaAlgorithm.h"
#include "DDExternalClusterCreator.h"

#include "DD4hep/DD4hepUnits.h"
#include "DD4hep/DetType.h"
#include "DD4hep/Detector.h"
#include "DD4hep/DetectorSelector.h"

#include "LCTrackClusterAssociation/TrackClusterAssociationAlgorithm.h"
#include "PfoCreationAlgorithmIdea.h"
#include "DDPandoraPFANewAlgorithm.h"
#include "DDGeometryCreatorIDEA.h"
#include "BremRecoveryAlgorithm.h"

namespace lc_content {
class TrackClusterAssociationAlgorithmFactory : public pandora::AlgorithmFactory {
public:
    pandora::Algorithm *CreateAlgorithm() const { return new TrackClusterAssociationAlgorithm(); };
};

class PfoCreationAlgorithmIdeaFactory : public pandora::AlgorithmFactory {
public:
    pandora::Algorithm *CreateAlgorithm() const { return new PfoCreationAlgorithmIdea(); };
};
} // namespace lc_content

DDPandoraPFAIdeaAlgorithm::DDPandoraPFAIdeaAlgorithm(const std::string& name, ISvcLocator* svcLoc)
    : Gaudi::Algorithm(name, svcLoc), m_pandora() {}

// DDPandoraPFAIdeaAlgorithm::DDPandoraPFAIdeaAlgorithm(const std::string& name, ISvcLocator* svcLoc)
//     : MultiTransformer(name, svcLoc,
//                        {
//                           KeyValues("trackCollections", {""}),
//                           KeyValues("caloHitCollections", {""})
//                        },
//                        {
//                           KeyValues("PFOCollectionName", {"PandoraPFANewPFOs"})
//                        }),
//       m_pPandora() {}

StatusCode DDPandoraPFAIdeaAlgorithm::initialize() {
  m_geoSvc = serviceLocator()->service("GeoSvc"); // important to initialize m_geoSvc
  if (!m_geoSvc) {
    error() << "Unable to retrieve the GeoSvc" << endmsg;
    return StatusCode::FAILURE;
  }

  auto getFieldFromCompact = []() -> double {
    dd4hep::Detector& mainDetector = dd4hep::Detector::getInstance();
    const double position[3] = {0, 0, 0};      // position to calculate magnetic field at (the origin in this case)
    double magneticFieldVector[3] = {0, 0, 0}; // initialise object to hold magnetic field
    mainDetector.field().magneticField(position, magneticFieldVector); // get the magnetic field vector from DD4hep

    return magneticFieldVector[2] / dd4hep::tesla; // z component at (0,0,0)
  };

  const dd4hep::rec::LayeredCalorimeterData* drcExtension =
      getExtension((dd4hep::DetType::CALORIMETER | dd4hep::DetType::ELECTROMAGNETIC | dd4hep::DetType::HADRONIC),
                   (dd4hep::DetType::AUXILIARY | dd4hep::DetType::FORWARD));

  m_trackCreatorSettings.m_bField = getFieldFromCompact();
  m_trackCreatorSettings.m_endcapInnerZ = drcExtension->extent[2] / dd4hep::mm;

  m_geometryCreator = std::make_unique<DDGeometryCreatorIDEA>(m_geometryCreatorSettings, m_pandora, this);
  m_caloHitCreator = std::make_unique<DualReadoutCaloHitCreator>(m_caloHitCreatorSettings, m_pandora, this);
  m_trackCreator = std::make_unique<DDTrackCreatorIDEA>(m_trackCreatorSettings, m_pandora, this);
  m_pfoCreator = std::make_unique<DDPfoCreatorIdea>(m_pfoCreatorSettings, m_pandora, this);

  try {
    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            PandoraApi::RegisterAlgorithmFactory(m_pandora, "ExternalClustering",
                                                                 new DDExternalClusterCreator::Factory));

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            PandoraApi::RegisterAlgorithmFactory(m_pandora, "TrackClusterAssociation",
                                                                 new lc_content::TrackClusterAssociationAlgorithmFactory));

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            PandoraApi::RegisterAlgorithmFactory(m_pandora, "BremRecovery",
                                                                 new BremRecoveryAlgorithm::Factory));

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            PandoraApi::RegisterAlgorithmFactory(m_pandora, "CreatePfo",
                                                                 new lc_content::PfoCreationAlgorithmIdeaFactory));

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, m_geometryCreator->CreateGeometry())

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            PandoraApi::ReadSettings(m_pandora, m_pandoraSettingsXmlFile))
  } catch (pandora::StatusCodeException& statusCodeException) {
    error() << "Pandora failed to initialize DDPandoraPFAIdeaAlgorithm: " << statusCodeException.ToString() << endmsg;
    throw;
  } catch (std::exception& exception) {
    error() << "DDPandoraPFAIdeaAlgorithm failure: " << exception.what() << endmsg;
    return StatusCode::FAILURE;
  }

  return StatusCode::SUCCESS;
}

const pandora::Pandora* DDPandoraPFAIdeaAlgorithm::GetPandora() const {
  return &m_pandora;
}

// const std::vector<edm4hep::Cluster>* DDPandoraPFAIdeaAlgorithm::m_clusterMember;

const std::vector<edm4hep::Cluster>* DDPandoraPFAIdeaAlgorithm::GetClusterVector() {
  return &m_clusterMember;
}

StatusCode DDPandoraPFAIdeaAlgorithm::execute(const EventContext&) const {
  // input collection
  const edm4hep::TrackCollection* trackColl = m_trackColl.get();
  const edm4hep::CalorimeterHitCollection* caloHitColl = m_caloHitColl.get();
  const edm4hep::ClusterCollection* clusterColl = m_clusterColl.get();

  // output collection
  edm4hep::ReconstructedParticleCollection* pfoColl = m_pfoColl.createAndPut();

  try {
    // track
    std::vector<edm4hep::Track> tracksVector;

    for (const auto& aTrk : *trackColl)
      tracksVector.push_back(aTrk);

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, m_trackCreator->CreateTracks(tracksVector));

    // calo hit
    std::vector<edm4hep::CalorimeterHit> caloHitVector;

    for (const auto& aCaloHit : *caloHitColl)
      caloHitVector.push_back(aCaloHit);

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            m_caloHitCreator->createCaloHits(caloHitVector));

    for (const auto& aClus : *clusterColl)
      m_clusterMember.push_back(aClus);

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::ProcessEvent(m_pandora));

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            m_pfoCreator->CreatePFOs(*pfoColl))

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::Reset(m_pandora))
  } catch (std::exception& e) {
    error() << "Pandora failed to process event: std::exception " << e.what() << endmsg;
    throw;
  } catch (...) {
    error() << "Pandora failed to process event: unrecognized exception" << endmsg;
    throw;
  }

  // FIXME temporary workaround to access calo hits in pandora algorithm
  m_clusterMember.clear();

  return StatusCode::SUCCESS;
}

// std::tuple<edm4hep::ReconstructedParticleCollection>
// DDPandoraPFAIdeaAlgorithm::operator()(
//     const std::vector<const edm4hep::TrackCollection*>& trackCollections,
//     const std::vector<const edm4hep::CalorimeterHitCollection*>& caloCollections) const {
//   try {
//     // track
//     std::vector<edm4hep::Track> tracksVector;
//
//     for (const auto& trackCollection : trackCollections)
//       tracksVector.insert(tracksVector.end(), trackCollection->begin(), trackCollection->end());
//
//     PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, m_trackCreator->CreateTracks(tracksVector));
//
//     // calo hit
//     const auto caloHitCollectionNames = inputLocations("caloHitCollections");
//     std::map<std::string, std::vector<edm4hep::CalorimeterHit>> caloCollectionMap;
//
//     for (size_t i = 0; i < caloHitCollectionNames.size(); i++) {
//       const auto& coll = caloCollections[i];
//       caloCollectionMap[caloHitCollectionNames[i]].reserve(coll->size());
//       auto itr = caloCollectionMap.find(caloHitCollectionNames[i]);
//       itr->second.insert(itr->second.end(), coll->begin(), coll->end());
//     }
//
//     PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
//                             m_caloHitCreator->createCaloHits(caloCollectionMap));
//
//     // PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::ProcessEvent(m_pPandora));
//
//     edm4hep::ReconstructedParticleCollection pReconstructedParticleCollection;
//
//     return std::make_tuple(std::move(pReconstructedParticleCollection));
//   } catch (std::exception& e) {
//     error() << "Pandora failed to process event: std::exception " << e.what() << endmsg;
//     throw;
//   } catch (...) {
//     error() << "Pandora failed to process event: unrecognized exception" << endmsg;
//     throw;
//   }
// }

DECLARE_COMPONENT(DDPandoraPFAIdeaAlgorithm)
