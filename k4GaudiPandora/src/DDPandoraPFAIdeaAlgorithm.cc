#include "DDPandoraPFAIdeaAlgorithm.h"
#include "DDExternalClusteringAlgorithm.h"

#include "DD4hep/DD4hepUnits.h"
#include "DD4hep/DetType.h"
#include "DD4hep/Detector.h"
#include "DD4hep/DetectorSelector.h"

#include "LCMonitoring/VisualMonitoringAlgorithm.h"
#include "LCTrackClusterAssociation/TrackClusterAssociationAlgorithm.h"

#include "LCPlugins/DualReadoutCorrection.h"
#include "MLInference/ClusterNeutralPidAlgorithm.h"
#include "LCParticleId/ForwardPhotonIdAlgorithm.h"
#include "LCClustering/EcalSeededClusteringAlgorithm.h"
#include "MLInference/SatelliteAssignmentOnnxAlgorithm.h"
#include "LCUtility/IsolatedHitPreparationAlgorithm.h"
#include "LCTopologicalAssociation/IsolatedHitMergingAlgorithm.h"
#include "LCPfoConstruction/IdeaPfoCreationAlgorithm.h"

#include "DDPandoraPFANewAlgorithm.h"
#include "DDBFieldPlugin.h"
#include "DDGeometryCreatorIdea.h"

namespace lc_content {
class TrackClusterAssociationAlgorithmFactory : public pandora::AlgorithmFactory {
public:
  pandora::Algorithm *CreateAlgorithm() const { return new TrackClusterAssociationAlgorithm(); };
};

class IsolatedHitMergingAlgorithmFactory : public pandora::AlgorithmFactory {
public:
  pandora::Algorithm *CreateAlgorithm() const { return new IsolatedHitMergingAlgorithm(); };
};

class VisualMonitoringAlgorithmFactory : public pandora::AlgorithmFactory {
public:
  pandora::Algorithm *CreateAlgorithm() const { return new VisualMonitoringAlgorithm(); };
};
} // namespace lc_content

DDPandoraPFAIdeaAlgorithm::DDPandoraPFAIdeaAlgorithm(const std::string& name, ISvcLocator* svcLoc)
    : MultiTransformer(name, svcLoc,
                       {
                           KeyValue("inputTrackCollection", "TracksFromGenParticles"),
                           KeyValues("inputCaloHitCollections", {}),
                           KeyValues("inputClusterCollections", {}),
                       },
                       {
                           KeyValue("outputClusterCollection", "PandoraClusters"),
                           KeyValue("outputPfoCollection", "PandoraPfaIdea")
                       }),
      m_pandora() {}

StatusCode DDPandoraPFAIdeaAlgorithm::initialize() {
  m_geoSvc = serviceLocator()->service("GeoSvc"); // important to initialize m_geoSvc
  if (!m_geoSvc) {
    error() << "Unable to retrieve the GeoSvc" << endmsg;
    return StatusCode::FAILURE;
  }

  finaliseSteeringParameters();

  m_geometryCreator = std::make_unique<DDGeometryCreatorIdea>(m_geometryCreatorSettings, m_pandora, this);
  m_caloHitCreator = std::make_unique<DualReadoutCaloHitCreator>(m_caloHitCreatorSettings, m_pandora, this);
  m_trackCreator = std::make_unique<DDTrackCreatorIdea>(m_trackCreatorSettings, m_pandora, this);
  m_pfoCreator = std::make_unique<DDPfoCreatorIdea>(m_pfoCreatorSettings, m_pandora, this);

  try {
    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            PandoraApi::RegisterEnergyCorrectionPlugin(m_pandora, "DualReadoutCorrection", pandora::EnergyCorrectionType::HADRONIC,
                                                                       new lc_content::DualReadoutCorrection));

    // Magnetic field from the dd4hep field map: algorithms retrieve it via the plugin (position
    // dependent) instead of a hardcoded XML value.
    dd4hep::Detector& mainDetector = dd4hep::Detector::getInstance();
    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            PandoraApi::SetBFieldPlugin(m_pandora, new DDBFieldPlugin(mainDetector)));

    // Register algorithms
    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            PandoraApi::RegisterAlgorithmFactory(m_pandora, "DDExternalClustering",
                                                                 new DDExternalClusteringAlgorithm::Factory));

    // Set external parameters for DDExternalClusteringAlgorithm
    // everything is owned by this algorithm
    m_extEvtParam = std::make_unique<ExternalEventParameter>();
    m_extClusterHolder = std::make_unique<ExternalClusterHolder>();
    m_extEvtParam->m_externalClusterHolder = m_extClusterHolder.get();

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            PandoraApi::SetExternalParameters(m_pandora, "DDExternalClustering", m_extEvtParam.get()))

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            PandoraApi::RegisterAlgorithmFactory(m_pandora, "TrackClusterAssociation",
                                                                 new lc_content::TrackClusterAssociationAlgorithmFactory));

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            PandoraApi::RegisterAlgorithmFactory(m_pandora, "ClusterNeutralPid",
                                                                 new lc_content::ClusterNeutralPidAlgorithm::Factory));

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            PandoraApi::RegisterAlgorithmFactory(m_pandora, "ForwardPhotonId",
                                                                 new lc_content::ForwardPhotonIdAlgorithm::Factory));

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            PandoraApi::RegisterAlgorithmFactory(m_pandora, "IsolatedHitPreparation",
                                                                 new lc_content::IsolatedHitPreparationAlgorithm::Factory));

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            PandoraApi::RegisterAlgorithmFactory(m_pandora, "IsolatedHitMerging",
                                                                 new lc_content::IsolatedHitMergingAlgorithmFactory));

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            PandoraApi::RegisterAlgorithmFactory(m_pandora, "EcalSeededClustering",
                                                                 new lc_content::EcalSeededClusteringAlgorithm::Factory));

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            PandoraApi::RegisterAlgorithmFactory(m_pandora, "SatelliteAssignmentOnnx",
                                                                 new lc_content::SatelliteAssignmentOnnxAlgorithm::Factory));

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            PandoraApi::RegisterAlgorithmFactory(m_pandora, "CreatePfo",
                                                                 new lc_content::IdeaPfoCreationAlgorithm::Factory));

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            PandoraApi::RegisterAlgorithmFactory(m_pandora, "VisualMonitoring",
                                                                 new lc_content::VisualMonitoringAlgorithmFactory));

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

std::tuple<edm4hep::ClusterCollection, edm4hep::ReconstructedParticleCollection>
DDPandoraPFAIdeaAlgorithm::operator()(
    const edm4hep::TrackCollection& trackColl,
    const std::vector<const edm4hep::CalorimeterHitCollection*>& caloHitColls,
    const std::vector<const edm4hep::ClusterCollection*>& clusterColls) const {

  try {
    // Create output collections
    edm4hep::ClusterCollection outClusterColl;
    edm4hep::ReconstructedParticleCollection pfoColl;

    // track
    std::vector<edm4hep::Track> tracksVector;

    for (const auto& aTrk : trackColl)
      tracksVector.push_back(aTrk);

    // TODO: track creator is working in progress
    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, m_trackCreator->CreateTracks(tracksVector));

    // calo hits - create vectors to ensure stable addresses
    std::vector<std::vector<edm4hep::CalorimeterHit>> caloHitVectors(caloHitColls.size());
    for (size_t i = 0; i < caloHitColls.size(); ++i) {
      caloHitVectors[i].reserve(caloHitColls[i]->size());

      for (const auto& hit : *caloHitColls[i])
        caloHitVectors[i].push_back(hit);
    }

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            m_caloHitCreator->createCaloHits(caloHitVectors));

    // host edm4hep clusters for the external clustering algorithm
    std::unique_ptr<std::vector<std::vector<edm4hep::Cluster>>> externalClustersPtr =
        std::make_unique<std::vector<std::vector<edm4hep::Cluster>>>();
    externalClustersPtr->reserve(clusterColls.size());

    // loop over the input cluster collections and fill the external clusters vector
    for (const auto* clusterCollection : clusterColls) {
      std::vector<edm4hep::Cluster> clusterValues;
      clusterValues.reserve(clusterCollection->size());

      for (const auto& cluster : *clusterCollection) {
        clusterValues.push_back(cluster);
      }

      externalClustersPtr->push_back(std::move(clusterValues));
    }

    m_extClusterHolder->setExternalClusters(externalClustersPtr.get());

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::ProcessEvent(m_pandora));

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                            m_pfoCreator->CreatePFOs(outClusterColl, pfoColl))

    PANDORA_THROW_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::Reset(m_pandora))

    return std::make_tuple(std::move(outClusterColl), std::move(pfoColl));
  } catch (const pandora::StatusCodeException& statusCodeException) {
    // pandora::StatusCodeException does NOT derive from std::exception, so it
    // must be caught explicitly (otherwise it falls through to catch(...)).
    error() << "Pandora failed to process event: pandora::StatusCodeException "
            << statusCodeException.ToString() << endmsg;
    error() << statusCodeException.GetBackTrace() << endmsg;
    throw;
  } catch (std::exception& e) {
    error() << "Pandora failed to process event: std::exception " << e.what() << endmsg;
    throw;
  } catch (...) {
    error() << "Pandora failed to process event: unrecognized exception" << endmsg;
    throw;
  }
}

void DDPandoraPFAIdeaAlgorithm::finaliseSteeringParameters() {
  // copy steering parameters to the settings objects
  m_geometryCreatorSettings.m_isOption2 = m_isOption2;

  // TODO avoid duplication with DDPandoraPFANewAlgorithm
  auto getFieldFromCompact = []() -> double {
    dd4hep::Detector& mainDetector = dd4hep::Detector::getInstance();
    const double position[3] = {0, 0, 0};      // position to calculate magnetic field at (the origin in this case)
    double magneticFieldVector[3] = {0, 0, 0}; // initialise object to hold magnetic field
    mainDetector.field().magneticField(position, magneticFieldVector); // get the magnetic field vector from DD4hep

    return magneticFieldVector[2] / dd4hep::tesla; // z component at (0,0,0)
  };

  const dd4hep::rec::LayeredCalorimeterData* drcExtension =
      getExtension((dd4hep::DetType::CALORIMETER | dd4hep::DetType::BARREL | dd4hep::DetType::ENDCAP),
                   (dd4hep::DetType::AUXILIARY | dd4hep::DetType::FORWARD));

  // track creator settings
  m_trackCreatorSettings.m_bField = getFieldFromCompact();
  m_trackCreatorSettings.m_endcapInnerZ = drcExtension->extent[2] / dd4hep::mm;

  // calo hit creator settings
  m_caloHitCreatorSettings.m_cherenkovFieldName = m_cherenkovFieldName;
  m_caloHitCreatorSettings.m_theta = std::atan2(drcExtension->extent[0], drcExtension->extent[2]);
  m_caloHitCreatorSettings.m_subDetectorSettings.resize(m_systemIDs.value().size());

  for (size_t icol = 0; icol < m_systemIDs.value().size(); ++icol) {
    auto& subdetectorSetting = m_caloHitCreatorSettings.m_subDetectorSettings.at(icol);
    subdetectorSetting.m_systemID = m_systemIDs.value().at(icol);
    subdetectorSetting.m_encodingString = m_encodingStrings.value().at(icol);
    subdetectorSetting.m_layerFieldName = m_layerFieldNames.value().at(icol);
    subdetectorSetting.m_collectionType = m_collectionTypes.value().at(icol);
    subdetectorSetting.m_cellSize = m_cellSizes.value().at(icol);

    for (const auto& layer : drcExtension->layers)
      subdetectorSetting.m_layerThicknesses.push_back(layer.sensitive_thickness);
  }
}

DECLARE_COMPONENT(DDPandoraPFAIdeaAlgorithm)
