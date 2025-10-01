#ifndef K4GAUDIPANDORA_DUALREADOUTCALOHITCREATOR_H
#define K4GAUDIPANDORA_DUALREADOUTCALOHITCREATOR_H 1

// Pandora
#include "Api/PandoraApi.h"

// DD4hep
// #include "DD4hep/Detector.h"

// EDM4hep
#include "edm4hep/CalorimeterHitCollection.h"

// c++
#include <string>
#include <vector>
#include <algorithm>

// Gaudi
#include "GaudiKernel/Algorithm.h"

class DualReadoutCaloHitCreator {
public:

  // (internal) config settings
  class Settings {
  public:
    Settings()=default;
    ~Settings()=default;

    // std::vector<std::string> m_collections; // FIXME currently not used at all
  };

  DualReadoutCaloHitCreator(const Settings& settings, pandora::Pandora& pandora, const Gaudi::Algorithm* algorithm);
  virtual ~DualReadoutCaloHitCreator()=default;

  // create calo hits in Pandora
  pandora::StatusCode createCaloHits(const std::vector<edm4hep::CalorimeterHit>& inputCaloHits) const;

  // reset calo hit vector
  void Reset();

private:
  // void getCaloHitProperties(const edm4hep::CalorimeterHit& hit,
  //                           PandoraApi::CaloHit::Parameters& caloHitParameters) const;

  // settings
  const Settings m_settings;

  // interfaces
  pandora::Pandora& m_pandora;
  // dd4hep::VolumeManager m_volumeManager;
  const Gaudi::Algorithm& m_algorithm;
};

#endif
