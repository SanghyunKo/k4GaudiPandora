#include "DDPfoCreatorIdea.h"

#include "Objects/ParticleFlowObject.h"
#include "Objects/Track.h"
#include "Objects/Cluster.h"

DDPfoCreatorIdea::DDPfoCreatorIdea(const Settings& settings, pandora::Pandora& pandora, const Gaudi::Algorithm* algorithm)
    : m_settings(settings), m_pandora(pandora), m_algorithm(*algorithm) {}

pandora::StatusCode DDPfoCreatorIdea::CreatePFOs(edm4hep::ClusterCollection& clusterColl,
                                                 edm4hep::ReconstructedParticleCollection& aPfoColl) const {
  const pandora::PfoList* pandoraPfoList = nullptr;
  PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraApi::GetCurrentPfoList(m_pandora, pandoraPfoList))

  for (const auto* aPfo : *pandoraPfoList) {
    auto reconstructedParticle = aPfoColl.create();

    // reconstructedParticle.setReferencePoint({referencePoint.GetX(), referencePoint.GetY(), referencePoint.GetZ()});
    this->AddTracksToRecoParticle(aPfo, reconstructedParticle);
    this->AddClustersToRecoParticle(aPfo, clusterColl, reconstructedParticle);
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

void DDPfoCreatorIdea::AddClustersToRecoParticle(const pandora::ParticleFlowObject* const pPandoraPfo,
                                                 edm4hep::ClusterCollection& clusterColl,
                                                 edm4hep::MutableReconstructedParticle& reconstructedParticle) const {
  const pandora::ClusterList& clusterList(pPandoraPfo->GetClusterList());

  for (const auto* pPandoraCluster : clusterList) {
    pandora::CaloHitList pandoraCaloHitList;
    pPandoraCluster->GetOrderedCaloHitList().FillCaloHitList(pandoraCaloHitList);

    // isolated hits will only contribute to the energy
    // but not the position and covariance matrix
    const auto isolatedCaloHitList = pPandoraCluster->GetIsolatedCaloHitList();

    double hitE = 0., hitX = 0., hitY = 0., hitZ = 0.;
    double hitEnErr2 = 0.; // energy error squared
    double hitXXErr = 0., hitYYErr = 0., hitZZErr = 0.; // covariances of position
    double hitXYErr = 0., hitYZErr = 0., hitZXErr = 0.;
    auto cluster = clusterColl.create();

    for (const auto* pCaloHit : pandoraCaloHitList) {
      const auto* hit = static_cast<const edm4hep::CalorimeterHit*>(pCaloHit->GetParentAddress());
      cluster.addToHits(*hit);
      hitE += hit->getEnergy();
      hitX += hit->getPosition()[0] * hit->getEnergy();
      hitY += hit->getPosition()[1] * hit->getEnergy();
      hitZ += hit->getPosition()[2] * hit->getEnergy();
      hitEnErr2 += hit->getEnergyError() * hit->getEnergyError();
      double xEnErr = hit->getPosition()[0] * hit->getEnergyError();
      double yEnErr = hit->getPosition()[1] * hit->getEnergyError();
      double zEnErr = hit->getPosition()[2] * hit->getEnergyError();
      hitXXErr += xEnErr * xEnErr;
      hitYYErr += yEnErr * yEnErr;
      hitZZErr += zEnErr * zEnErr;
      hitXYErr += xEnErr * yEnErr;
      hitYZErr += yEnErr * zEnErr;
      hitZXErr += zEnErr * xEnErr;
    } // loop calo hits in cluster

    double isoHitE = 0., isoHitEnErr2 = 0.;
    for (const auto* pIsoHit : isolatedCaloHitList) {
      const auto* hit = static_cast<const edm4hep::CalorimeterHit*>(pIsoHit->GetParentAddress());
      cluster.addToHits(*hit);
      isoHitE += hit->getEnergy();
      double eErr = hit->getEnergyError();
      isoHitEnErr2 += eErr * eErr;
    } // loop isolated hits in cluster

    cluster.setEnergy(hitE + isoHitE);
    cluster.setEnergyError(std::sqrt(hitEnErr2 + isoHitEnErr2));

    if (hitE > std::numeric_limits<float>::epsilon()) {
      double hitE2 = hitE * hitE;
      cluster.setPosition({float(hitX / hitE), float(hitY / hitE), float(hitZ / hitE)});
      cluster.setPositionError({float(hitXXErr / hitE2), float(hitXYErr / hitE2), float(hitZXErr / hitE2),
                                float(hitXYErr / hitE2), float(hitYYErr / hitE2),
                                float(hitZXErr / hitE2)});
    } else {
      m_algorithm.warning() << "DDPfoCreatorIdea::AddClustersToRecoParticle: invalid cluster energy " << hitE << endmsg;
      throw pandora::StatusCodeException(pandora::STATUS_CODE_FAILURE);
    }

    reconstructedParticle.addToClusters(cluster);
  } // loop clusters in PFO
}
