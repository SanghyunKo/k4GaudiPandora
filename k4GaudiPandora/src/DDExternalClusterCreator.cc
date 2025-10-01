#include "DDExternalClusterCreator.h"
#include "DDPandoraPFAIdeaAlgorithm.h"

// EDM4hep
#include "edm4hep/CalorimeterHitCollection.h"
#include "edm4hep/ClusterCollection.h"

// Pandora
#include "Api/PandoraContentApi.h"
#include "Pandora/PdgTable.h"

DDExternalClusterCreator::DDExternalClusterCreator() : m_flagClustersAsPhotons(false) {}

pandora::StatusCode DDExternalClusterCreator::Run() {
  try {
    const pandora::CaloHitList* pCaloHitList = NULL;
    PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, pCaloHitList));

    if (pCaloHitList->empty())
      return pandora::STATUS_CODE_SUCCESS;

    // Get external photon cluster collection
    const std::vector<edm4hep::Cluster>* pExtClusters = DDPandoraPFAIdeaAlgorithm::GetClusterVector();

    if (pExtClusters->size() == 0)
      return pandora::STATUS_CODE_SUCCESS;

    // Populate pandora parent address to calo hit map
    ParentAddressToCaloHitMap parentAddressToCaloHitMap;
    std::hash<podio::ObjectID> hasher;

    for (pandora::CaloHitList::const_iterator hitIter = pCaloHitList->begin(), hitIterEnd = pCaloHitList->end();
         hitIter != hitIterEnd; ++hitIter) {
      const pandora::CaloHit* const pCaloHit = *hitIter;

      // FIXME are we sure about this
      const edm4hep::CalorimeterHit* edmCaloHit = static_cast<const edm4hep::CalorimeterHit*>(pCaloHit->GetParentAddress());
      parentAddressToCaloHitMap.insert(ParentAddressToCaloHitMap::value_type((void*)hasher(edmCaloHit->id()), pCaloHit));
    }

    // without the following pandora throws STATUS_CODE_NOT_ALLOWED
    // for ClusterManager->Create(parameters, pCluster, factory)
    const pandora::ClusterList* pClusterList = nullptr;
    std::string clusterListNameTmp;
    std::string clusterListNameFinal = "ExternalClusters";

    PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraContentApi::CreateTemporaryListAndSetCurrent(*this, pClusterList, clusterListNameTmp));

    // Recreate external clusters within the pandora framework
    for (unsigned int iCluster = 0; iCluster < pExtClusters->size(); ++iCluster) {
      const auto& aClus = pExtClusters->at(iCluster);

      const auto& calorimeterHitVec = aClus.getHits();
      const pandora::Cluster* pPandoraCluster = nullptr;

      for (auto iter = calorimeterHitVec.begin(); iter != calorimeterHitVec.end(); ++iter) {
        // FIXME podio::ObjectID -> std::hash -> std::size_t -> void*
        // this looks horrible and should be temporary

        ParentAddressToCaloHitMap::const_iterator pandoraCaloHitIter = parentAddressToCaloHitMap.find((void*)hasher(iter->id()));

        if (pandoraCaloHitIter == parentAddressToCaloHitMap.end())
          continue;

        const pandora::CaloHit* const pPandoraCaloHit = pandoraCaloHitIter->second;

        if (pPandoraCluster == nullptr) {
          PandoraContentApi::Cluster::Parameters parameters;
          parameters.m_caloHitList.push_back(pPandoraCaloHit);
          PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                                   PandoraContentApi::Cluster::Create(*this, parameters, pPandoraCluster));

          if (m_flagClustersAsPhotons) {
            PandoraContentApi::Cluster::Metadata metadata;
            metadata.m_particleId = pandora::PHOTON;
            PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                                     PandoraContentApi::Cluster::AlterMetadata(*this, pPandoraCluster, metadata));
          }
        } else {
          PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                                   PandoraContentApi::AddToCluster(*this, pPandoraCluster, pPandoraCaloHit));
        }
      } // calo hit s
    } // clusters

    if (!pClusterList->empty()) {
      PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraContentApi::SaveList<pandora::Cluster>(*this, clusterListNameFinal));
      PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraContentApi::ReplaceCurrentList<pandora::Cluster>(*this, clusterListNameFinal));
    }
  } catch (pandora::StatusCodeException& statusCodeException) {
    return statusCodeException.GetStatusCode();
  } catch (std::exception& exception) {
    std::cout << "DDExternalClusteringAlgorithm failure: " << exception.what() << std::endl;
    return pandora::STATUS_CODE_FAILURE;
  }

  return pandora::STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

pandora::StatusCode DDExternalClusterCreator::ReadSettings(const pandora::TiXmlHandle xmlHandle) {
  // PANDORA_RETURN_RESULT_IF(
  //     pandora::STATUS_CODE_SUCCESS, !=,
  //     pandora::XmlHelper::ReadValue(xmlHandle, "ExternalClusterCollectionName", m_externalClusterCollectionName));

  PANDORA_RETURN_RESULT_IF_AND_IF(pandora::STATUS_CODE_SUCCESS, pandora::STATUS_CODE_NOT_FOUND, !=,
                                  pandora::XmlHelper::ReadValue(xmlHandle, "FlagClustersAsPhotons", m_flagClustersAsPhotons));

  return pandora::STATUS_CODE_SUCCESS;
}

// DDExternalClusterCreator::DDExternalClusterCreator(const Settings& settings)
//     : m_settings(settings) {}
//
// pandora::StatusCode DDExternalClusterCreator::createClusters(const std::vector<edm4hep::Cluster>& inputClusters) const {
//   // loop over input collections
//
//   try {
//     const pandora::CaloHitList* pCaloHitList = NULL;
//     // FIXME is there any other way to get a pandora list?
//     PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, pCaloHitList));
//
//     if (pCaloHitList->empty())
//       return pandora::STATUS_CODE_SUCCESS;
//
//     // Get external photon cluster collection
//     const unsigned int nExternalClusters(inputClusters.size());
//
//     if (0 == nExternalClusters)
//       return pandora::STATUS_CODE_SUCCESS;
//
//     // Populate pandora parent address to calo hit map
//     ParentAddressToCaloHitMap parentAddressToCaloHitMap;
//
//     for (pandora::CaloHitList::const_iterator hitIter = pCaloHitList->begin(), hitIterEnd = pCaloHitList->end();
//          hitIter != hitIterEnd; ++hitIter) {
//       const pandora::CaloHit* const pCaloHit = *hitIter;
//       parentAddressToCaloHitMap.insert(ParentAddressToCaloHitMap::value_type(pCaloHit->GetParentAddress(), pCaloHit));
//     }
//
//     // Recreate external clusters within the pandora framework
//     for (unsigned int iCluster = 0; iCluster < nExternalClusters; ++iCluster) {
//       const auto& externalCluster = inputClusters.at(iCluster);
//
//       const pandora::Cluster* pPandoraCluster = NULL;
//
//       for (auto iter = externalCluster.hits_begin(), iterEnd = externalCluster.hits_end(); iter != iterEnd; ++iter) {
//         ParentAddressToCaloHitMap::const_iterator pandoraCaloHitIter = parentAddressToCaloHitMap.find(&(*iter));
//
//         if (parentAddressToCaloHitMap.end() == pandoraCaloHitIter)
//           continue;
//
//         const pandora::CaloHit* const pPandoraCaloHit = pandoraCaloHitIter->second;
//
//         if (NULL == pPandoraCluster) {
//           PandoraContentApi::Cluster::Parameters parameters;
//           parameters.m_caloHitList.push_back(pPandoraCaloHit);
//           PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
//                                    PandoraContentApi::Cluster::Create(*this, parameters, pPandoraCluster));
//
//           if (m_settings.m_flagClustersAsPhotons) {
//             PandoraContentApi::Cluster::Metadata metadata;
//             metadata.m_particleId = pandora::PHOTON;
//             PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
//                                      PandoraContentApi::Cluster::AlterMetadata(*this, pPandoraCluster, metadata));
//           }
//         } else {
//           PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
//                                    PandoraContentApi::AddToCluster(*this, pPandoraCluster, pPandoraCaloHit));
//         }
//       }
//     }
//   } catch (pandora::StatusCodeException& statusCodeException) {
//     return statusCodeException.GetStatusCode();
//   } catch (std::exception& exception) {
//     std::cout << "DDExternalClusterCreator failure: " << exception.what() << std::endl;
//     return pandora::STATUS_CODE_FAILURE;
//   }
//
//   return pandora::STATUS_CODE_SUCCESS;
// }
