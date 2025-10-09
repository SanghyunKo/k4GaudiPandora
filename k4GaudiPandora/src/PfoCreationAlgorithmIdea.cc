#include "PfoCreationAlgorithmIdea.h"
#include "BremRecoveryAlgorithm.h" // FIXME only for sortByDistance

#include "Objects/Cluster.h"
#include "Objects/Track.h"

#include "Pandora/PdgTable.h"

namespace lc_content {

PfoCreationAlgorithmIdea::PfoCreationAlgorithmIdea() {}

pandora::StatusCode PfoCreationAlgorithmIdea::Run() {
  const pandora::PfoList* pPfoList = nullptr;
  std::string pfoListName;

  PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraContentApi::CreateTemporaryListAndSetCurrent(*this, pPfoList, pfoListName));

  PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, this->CreateElectronCandidates());

  if (!pPfoList->empty()) {
    PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraContentApi::SaveList<pandora::ParticleFlowObject>(*this, m_outputPfoListName));
    PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraContentApi::ReplaceCurrentList<pandora::ParticleFlowObject>(*this, m_outputPfoListName));
  }

  return pandora::STATUS_CODE_SUCCESS;
}

pandora::StatusCode PfoCreationAlgorithmIdea::CreateElectronCandidates() const {
  // for now rely on the "Calo-driven" way
  const pandora::ClusterList* clusterList = nullptr;

  PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, clusterList));

  // loop over clusters
  for (auto iter = clusterList->begin(); iter != clusterList->end(); ++ iter) {
    const pandora::Cluster* const aClus = *iter;
    const auto& associatedTrackList = aClus->GetAssociatedTrackList();

    // check track-cluster association
    if (!associatedTrackList.empty()) {
      // TODO apply some loose ID criteria
      // create PFO
      PandoraContentApi::ParticleFlowObject::Parameters pfoParameters;
      pfoParameters.m_clusterList.push_back(aClus);

      for (const auto* aTrack : associatedTrackList) {
        // TODO find best track - add all associated tracks for now
        pfoParameters.m_trackList.push_back(aTrack);
      }

      auto sortByTrackClusterDistance = [&aClus](const pandora::Track* a, const pandora::Track* b) {
        return BremRecoveryUtility::sortByDistance(a,b,aClus);
      };

      std::vector<const pandora::Track*> tracksVector;
      tracksVector.insert(tracksVector.end(),
                          pfoParameters.m_trackList.begin(),
                          pfoParameters.m_trackList.end());

      std::sort(tracksVector.begin(),tracksVector.end(),sortByTrackClusterDistance);

      // basic (and dumb) property setup
      // TODO Dual-readout correction & E-p combination
      pfoParameters.m_energy = aClus->GetElectromagneticEnergy();
      pfoParameters.m_momentum = tracksVector.front()->GetMomentumAtDca();
      pfoParameters.m_mass = 0.; // electron mass is practically zero
      pfoParameters.m_charge = tracksVector.front()->GetCharge();
      pfoParameters.m_particleId = (pfoParameters.m_charge.Get() > 0) ? pandora::E_PLUS : pandora::E_MINUS;

      // TODO add vertex

      // Create the pfo
      const pandora::ParticleFlowObject* aPFO = nullptr;
      PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraContentApi::ParticleFlowObject::Create(*this, pfoParameters, aPFO));
    } // has associated track
  } // cluster loop

  return pandora::STATUS_CODE_SUCCESS;
}

pandora::StatusCode PfoCreationAlgorithmIdea::ReadSettings(const pandora::TiXmlHandle xmlHandle) {
  PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, pandora::XmlHelper::ReadValue(xmlHandle,
      "OutputPfoListName", m_outputPfoListName));

  return pandora::STATUS_CODE_SUCCESS;
}

} // namespace
