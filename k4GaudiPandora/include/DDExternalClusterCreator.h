#ifndef DDExternalClusterCreator_h
#define DDExternalClusterCreator_h 1

// Pandora
#include "Pandora/Algorithm.h"
#include "Objects/CaloHit.h"
#include "Helpers/XmlHelper.h"

// c++
#include <map>

// copy-paste of DDExternalClusteringAlgorithm, but with Gaudi instead of Marlin
class DDExternalClusterCreator : public pandora::Algorithm {
public:
  class Factory : public pandora::AlgorithmFactory {
  public:
    pandora::Algorithm* CreateAlgorithm() const;
  };

  DDExternalClusterCreator();
  ~DDExternalClusterCreator()=default;

private:
  pandora::StatusCode Run();
  pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

  typedef std::map<const void*, const pandora::CaloHit*> ParentAddressToCaloHitMap;

  // std::string m_externalClusterCollectionName = "";    ///< The collection name for the external clusters
  bool m_flagClustersAsPhotons = false;  ///< Whether to automatically flag new clusters as fixed photons
};

//------------------------------------------------------------------------------------------------------------------------------------------

inline pandora::Algorithm* DDExternalClusterCreator::Factory::CreateAlgorithm() const {
  return new DDExternalClusterCreator();
}

#endif
