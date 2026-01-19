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

// #include "k4FWCore/Transformer.h"
#include "k4Interface/IGeoSvc.h"

#include "Gaudi/Property.h"

// headers for plain Gaudi alg (TODO deprecate it if possible)
#include "Gaudi/Algorithm.h"
#include "k4FWCore/DataHandle.h"

#include "DDRec/DetectorData.h"

#include <vector>
#include <string>

namespace {
  class Pandora;
}

class DDPandoraPFAIdeaAlgorithm : public Gaudi::Algorithm {
public:
  DDPandoraPFAIdeaAlgorithm(const std::string& name, ISvcLocator* svcLoc);
  ~DDPandoraPFAIdeaAlgorithm()=default;

  StatusCode initialize() override;
  StatusCode execute(const EventContext&) const override;
  StatusCode finalize() override { return StatusCode::SUCCESS; }

  const pandora::Pandora* GetPandora() const;

  // FIXME temporary workaround to access clusters in pandora algorithm
  static const std::vector<edm4hep::Cluster>* GetClusterVector();

private:
  SmartIF<IGeoSvc> m_geoSvc;
  pandora::Pandora m_pandora;
  std::unique_ptr<DDGeometryCreatorIDEA> m_geometryCreator;
  std::unique_ptr<DualReadoutCaloHitCreator> m_caloHitCreator;
  std::unique_ptr<DDTrackCreatorIDEA> m_trackCreator;
  std::unique_ptr<DDPfoCreatorIdea> m_pfoCreator;

  DDGeometryCreator::Settings m_geometryCreatorSettings;
  DualReadoutCaloHitCreator::Settings m_caloHitCreatorSettings;
  DDTrackCreatorIDEA::Settings m_trackCreatorSettings;
  DDPfoCreatorIdea::Settings m_pfoCreatorSettings;

  Gaudi::Property<std::string> m_pandoraSettingsXmlFile{this, "PandoraSettingsXmlFile", "",
                                                        "The pandora settings xml file"};

  // collection names
  Gaudi::Property<std::string> m_caloHitCollName{this, "inputCaloHitCollection", "TopoClusterAllCells",
                                                 "input calo collection name"};
  Gaudi::Property<std::string> m_trackCollName{this, "inputTrackCollection", "TracksFromGenParticles",
                                               "input track collection name"};
  Gaudi::Property<std::string> m_clusterCollName{this, "inputClusterCollection", "TopoClusterAll",
                                               "input cluster collection name"};
  Gaudi::Property<std::string> m_pfoCollName{this, "outputPfoCollection", "PandoraPfaIdea",
                                               "output PFO collection name"};
  Gaudi::Property<std::string> m_outClusterCollName{this, "outputClusterCollection", "PandoraClusters",
                                                    "output cluster collection name"};

  // Input collections
  mutable k4FWCore::DataHandle<edm4hep::CalorimeterHitCollection> m_caloHitColl{m_caloHitCollName, Gaudi::DataHandle::Reader,
                                                                                this};
  mutable k4FWCore::DataHandle<edm4hep::TrackCollection> m_trackColl{m_trackCollName, Gaudi::DataHandle::Reader,
                                                                     this};
  mutable k4FWCore::DataHandle<edm4hep::ClusterCollection> m_clusterColl{m_clusterCollName,
                                                                         Gaudi::DataHandle::Reader, this};

  // Output collections
  mutable k4FWCore::DataHandle<edm4hep::ReconstructedParticleCollection> m_pfoColl{m_pfoCollName, Gaudi::DataHandle::Writer,
                                                                                   this};
  mutable k4FWCore::DataHandle<edm4hep::ClusterCollection> m_outClusterColl{m_outClusterCollName, Gaudi::DataHandle::Writer, this};

  inline static std::vector<edm4hep::Cluster> m_clusterMember; // FIXME temporary workaround to access clusters in pandora algorithm
};

// class DDPandoraPFAIdeaAlgorithm : public k4FWCore::MultiTransformer<std::tuple<edm4hep::ReconstructedParticleCollection>(
//     const std::vector<const edm4hep::TrackCollection*>&,
//     const std::vector<const edm4hep::CalorimeterHitCollection*>&)> {
// public:
//   DDPandoraPFAIdeaAlgorithm(const std::string& name, ISvcLocator* svcLoc);
//
//   StatusCode initialize() override;
//   StatusCode finalize() override { return StatusCode::SUCCESS; }
//
//   std::tuple<edm4hep::ReconstructedParticleCollection>
//   operator()(const std::vector<const edm4hep::TrackCollection*>& trackCollections,
//              const std::vector<const edm4hep::CalorimeterHitCollection*>& caloCollections) const override;
//
//   const pandora::Pandora* GetPandora() const;
//
// private:
//   SmartIF<IGeoSvc> m_geoSvc;
//   pandora::Pandora m_pPandora;
//   std::unique_ptr<DualReadoutCaloHitCreator> m_caloHitCreator;
//   std::unique_ptr<DDTrackCreatorIDEA> m_trackCreator;
//
//   DualReadoutCaloHitCreator::Settings m_caloHitCreatorSettings;
//   DDTrackCreatorIDEA::Settings m_trackCreatorSettings;
//
//   Gaudi::Property<std::string> m_pandoraSettingsXmlFile{this, "PandoraSettingsXmlFile", "",
//                                                         "The pandora settings xml file"};
// };

#endif
