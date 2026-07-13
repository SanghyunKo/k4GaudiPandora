#ifndef DDPandoraPFAIdeaAlgorithm_h
#define DDPandoraPFAIdeaAlgorithm_h 1

#include "DualReadoutCaloHitCreator.h"
#include "DDTrackCreatorIDEA.h"
#include "DDPfoCreatorIdea.h"
#include "DDGeometryCreatorIDEA.h"

#include "edm4hep/CalorimeterHitCollection.h"
#include "edm4hep/ClusterCollection.h"
#include "edm4hep/TrackCollection.h"
#include "edm4hep/ReconstructedParticleCollection.h"

#include "k4FWCore/Transformer.h"
#include "k4Interface/IGeoSvc.h"

#include "Gaudi/Property.h"
#include "DDRec/DetectorData.h"

#include <vector>
#include <string>

namespace {
  class Pandora;
}

// forward declarations for the external clustering algorithm
class ExternalEventParameter;
class ExternalClusterHolder;

struct DDPandoraPFAIdeaAlgorithm final
    : k4FWCore::MultiTransformer<std::tuple<edm4hep::ClusterCollection,
                                            edm4hep::ReconstructedParticleCollection>(
          const edm4hep::TrackCollection&,
          const std::vector<const edm4hep::CalorimeterHitCollection*>&,
          const std::vector<const edm4hep::ClusterCollection*>&)> {
public:
  DDPandoraPFAIdeaAlgorithm(const std::string& name, ISvcLocator* svcLoc);
  ~DDPandoraPFAIdeaAlgorithm()=default;

  StatusCode initialize() override;
  StatusCode finalize() override { return StatusCode::SUCCESS; }

  std::tuple<edm4hep::ClusterCollection, edm4hep::ReconstructedParticleCollection>
  operator()(const edm4hep::TrackCollection& trackColl,
             const std::vector<const edm4hep::CalorimeterHitCollection*>& caloHitColls,
             const std::vector<const edm4hep::ClusterCollection*>& clusterColls) const override;

  const pandora::Pandora* GetPandora() const;

private:
  void finaliseSteeringParameters();

  SmartIF<IGeoSvc> m_geoSvc;
  std::unique_ptr<ExternalEventParameter> m_extEvtParam;
  std::unique_ptr<ExternalClusterHolder> m_extClusterHolder;

  pandora::Pandora m_pandora;
  std::unique_ptr<DDGeometryCreatorIDEA> m_geometryCreator;
  std::unique_ptr<DualReadoutCaloHitCreator> m_caloHitCreator;
  std::unique_ptr<DDTrackCreatorIDEA> m_trackCreator;
  std::unique_ptr<DDPfoCreatorIdea> m_pfoCreator;

  DDGeometryCreatorIDEA::Settings m_geometryCreatorSettings;
  DualReadoutCaloHitCreator::Settings m_caloHitCreatorSettings;
  DDTrackCreatorIDEA::Settings m_trackCreatorSettings;
  DDPfoCreatorIdea::Settings m_pfoCreatorSettings;

  Gaudi::Property<std::string> m_pandoraSettingsXmlFile{this, "PandoraSettingsXmlFile", "",
                                                        "The pandora settings xml file"};

  // Geometry settings
  Gaudi::Property<bool> m_isOption2{this, "IsOption2", true,
      "Flag for the IDEA option: true for option 2 (crystal DRC), false for option 1 (fiber DRC)"};

  // calo hit creator settings
  Gaudi::Property<std::string> m_cherenkovFieldName{this, "CherenkovFieldName", "cherenkov",
      "Name of the cherenkov field in the cellID encoding"};
  // for each detector
  Gaudi::Property<std::vector<uint64_t>> m_systemIDs{this, "CaloSystemIDs", {},
      "User-given system IDs for the different calorimeters"};
  Gaudi::Property<std::vector<std::string>> m_collectionTypes{this, "CaloCollectionTypes", {},
      "Types of the calo hit collections, either ECAL or HCAL (use ECAL for the monolithic DRC)"};
  Gaudi::Property<std::vector<std::string>> m_layerFieldNames{this, "CaloLayerFieldNames", {},
      "Names of the layer field in the cellID encoding (leave empty if longitudinally unsegmented)"};
  Gaudi::Property<std::vector<std::string>> m_encodingStrings{this, "CaloEncodingStrings", {},
      "User-given cellID encoding strings (temporary solution)"};
  Gaudi::Property<std::vector<float>> m_cellSizes{this, "CaloCellSizes", {},
      "Cell sizes in mm (only used for PandoraMonitoring)"};
};

#endif
