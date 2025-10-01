#include "DDPfoCreatorIdea.h"

#include "Objects/ParticleFlowObject.h"
#include "Objects/Track.h"

DDPfoCreatorIdea::DDPfoCreatorIdea(const Settings& settings, pandora::Pandora& pandora, const Gaudi::Algorithm* algorithm)
    : m_settings(settings), m_pandora(pandora), m_algorithm(*algorithm) {}

pandora::StatusCode DDPfoCreatorIdea::CreatePFOs(edm4hep::ReconstructedParticleCollection& aPfoColl) const {
  const pandora::PfoList* pandoraPfoList = nullptr;
  PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::GetCurrentPfoList(m_pandora, pandoraPfoList))

  for (const auto* aPfo : *pandoraPfoList) {
    auto reconstructedParticle = aPfoColl.create();

    // TODO handle cluster
    // reconstructedParticle.setReferencePoint({referencePoint.GetX(), referencePoint.GetY(), referencePoint.GetZ()});
    this->AddTracksToRecoParticle(aPfo, reconstructedParticle);
    this->SetRecoParticlePropertiesFromPFO(aPfo, reconstructedParticle);

    // TODO add vtx
    // auto startVertex = pStartVertexCollection.create();
    // startVertex.setAlgorithmType(0);
    // startVertex.setPosition({referencePoint.GetX(), referencePoint.GetY(), referencePoint.GetZ()});
    // startVertex.addToParticles(reconstructedParticle);
    //
    // reconstructedParticle.setDecayVertex(startVertex);
  } // loop pandora PFO list

  return pandora::STATUS_CODE_SUCCESS;
}


void DDPfoCreatorIdea::AddTracksToRecoParticle(const pandora::ParticleFlowObject* const pPandoraPfo,
                                               edm4hep::MutableReconstructedParticle& pReconstructedParticle) const {
  for (const auto* pTrack : pPandoraPfo->GetTrackList()) {
    const auto& pLcioTrack = *static_cast<const edm4hep::Track*>(pTrack->GetParentAddress());
    pReconstructedParticle.addToTracks(pLcioTrack);
  }
}

void DDPfoCreatorIdea::SetRecoParticlePropertiesFromPFO(
    const pandora::ParticleFlowObject* const pPandoraPfo,
    edm4hep::MutableReconstructedParticle& reconstructedParticle) const {
  const float momentum[3] = {pPandoraPfo->GetMomentum().GetX(), pPandoraPfo->GetMomentum().GetY(),
                             pPandoraPfo->GetMomentum().GetZ()};
  reconstructedParticle.setMomentum(momentum);
  reconstructedParticle.setEnergy(pPandoraPfo->GetEnergy());
  reconstructedParticle.setMass(pPandoraPfo->GetMass());
  reconstructedParticle.setCharge(pPandoraPfo->GetCharge());
  reconstructedParticle.setPDG(pPandoraPfo->GetParticleId());
}
