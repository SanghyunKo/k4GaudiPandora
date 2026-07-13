#ifndef K4GAUDIPANDORA_DUALREADOUTCALOHITCREATOR_H
#define K4GAUDIPANDORA_DUALREADOUTCALOHITCREATOR_H 1

// Pandora
#include "Api/PandoraApi.h"

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

    // shared parameters
    float m_theta; // barrel-endcap transition theta
    std::string m_cherenkovFieldName; // name of the cherenkov field in the cellID encoding

    // collection-specific settings
    class SubDetectorSettings {
    public:
      SubDetectorSettings()=default;
      ~SubDetectorSettings()=default;

      uint64_t m_systemID; // user-given system ID for this subdetector
      std::string m_layerFieldName; // name of the layer field in the cellID encoding (leave empty if longitudinally unsegmented)
      std::string m_collectionType; // either "ECAL" or "HCAL"
      std::string m_encodingString; // user-given cellID encoding string (temporary solution)
      float m_cellSize; // in mm (only used for PandoraMonitoring)
      std::vector<float> m_layerThicknesses; // in mm (edm4hep unit)
    };

    std::vector<SubDetectorSettings> m_subDetectorSettings; // settings for each subdetector
  };

  DualReadoutCaloHitCreator(const Settings& settings, pandora::Pandora& pandora, const Gaudi::Algorithm* algorithm);
  virtual ~DualReadoutCaloHitCreator()=default;

  // create calo hits in Pandora from provided hit vectors
  pandora::StatusCode createCaloHits(const std::vector<std::vector<edm4hep::CalorimeterHit>>& caloHitVectors) const;

  // reset calo hit vector
  void Reset();

private:
  // settings
  const Settings m_settings;

  // interfaces
  pandora::Pandora& m_pandora;
  const Gaudi::Algorithm& m_algorithm;
};

#endif
