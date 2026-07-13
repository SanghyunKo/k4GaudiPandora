#include "Pandora/AlgorithmHeaders.h"
#include "Api/PandoraContentApi.h"
#include "Objects/Helix.h"

#include "BremRecoveryAlgorithm.h"

#include <stdexcept>

namespace BremRecoveryUtility {
  // deltaPhi function to restrict the domain to -pi to pi
  // (I stole it from cmssw)
  double deltaPhi(const double p1, const double p2) {
    double oo2pi = 1./(2.*M_PI);

    if (std::abs(p1-p2) <= M_PI)
      return p1 - p2;

    double n = std::round((p1-p2)*oo2pi);

    return p1 - p2 - n*2.*M_PI;
  }

  // sorting helper for the cluster with multiple matched tracks
  // FIXME this can be implemented in the lc_content::SortingHelper
  bool sortByDistance (const pandora::Track* a,
                       const pandora::Track* b,
                       const pandora::Cluster* const pCluster) {
    const auto& trkStateA = a->GetTrackStateAtCalorimeter();
    const auto& trkStateB = b->GetTrackStateAtCalorimeter();
    const auto& clusterPos = pCluster->GetCentroid( pCluster->GetInnerPseudoLayer() );
    const double distA2 = (clusterPos - trkStateA.GetPosition()).GetMagnitudeSquared();
    const double distB2 = (clusterPos - trkStateB.GetPosition()).GetMagnitudeSquared();

    return distA2 < distB2;
  }

  bool GetPointInZ(const double zPlane,        // z plane (endcap) coordinate
                   const ThreeVector& refPt,   // reference point
                   const ThreeVector& momVec,  // momentum
                   ThreeVector& intersectPt) { // output intersection point
    // protect from zero division
    if (std::abs(momVec.z) < std::numeric_limits<float>::epsilon()) {
      throw std::invalid_argument("BremRecoveryUtility: invalid momentum.z (zero division)");

      return false;
    }

    // check the assumption
    if (std::abs(refPt.z) > std::abs(zPlane)) {
      throw std::invalid_argument("BremRecoveryUtility: invalid refPoint.z (ref. point outside the endcap)");

      return false;
    }

    intersectPt.z = momVec.z > 0. ? std::abs(zPlane) : -std::abs(zPlane);

    const double genericTime = (intersectPt.z - refPt.z)/momVec.z;
    intersectPt.x = refPt.x + momVec.x*genericTime;
    intersectPt.y = refPt.y + momVec.y*genericTime;

    return true;
  }

  bool GetPointOnCircle(const double radius,        // (barrel) radius
                        const ThreeVector& refPt,   // reference point
                        const ThreeVector& momVec,  // momentum
                        ThreeVector& intersectPt) { // output intersection point
    double refPtXY = refPt.magXY();
    double momVecXY = momVec.magXY();
    // protect from zero division
    if (std::abs(momVecXY) < std::numeric_limits<float>::epsilon()) {
      throw std::invalid_argument("BremRecoveryUtility: invalid pt (zero division)");

      return false;
    }

    // check the assumption
    if (std::abs(refPtXY) > radius) {
      throw std::invalid_argument("BremRecoveryUtility: invalid refPoint (ref. point outside the barrel)");

      return false;
    }
    // suppose a triangle OAP in the XY plane
    //      A        A is the reference point
    //      /\       O is the detector origin
    //     /  \      P is the intersection point
    //    ------     then the length OP = radius
    //   O      P

    // now do some middle school Euclidean geometry
    // cos(O+P) = refPt*momVec / |refPt|*|momVec|
    double normedInnerProd = (refPt.x*momVec.x + refPt.y*momVec.y) / (refPtXY*momVecXY);
    // sine rule : radius/sin(P+O) = |refPt|/sin(P)
    double sinP = refPtXY*std::sqrt(1. - normedInnerProd*normedInnerProd)/radius;
    double angOplusP = std::acos(normedInnerProd);
    double angP = std::asin(sinP); // always smaller than pi/2 by definition
    double angO = angOplusP - angP;
    // sine rule again : lenAP/sin(O) = |refPt|/sin(P)
    double lenAP = refPtXY*std::sin(angO)/sinP;
    double genericTime = lenAP/momVecXY; // this is what we wanted

    // now done with the geometry, come back to 3D
    intersectPt.x = refPt.x + momVec.x*genericTime;
    intersectPt.y = refPt.y + momVec.y*genericTime;
    intersectPt.z = refPt.z + momVec.z*genericTime;

    return true;
  }
}

BremRecoveryAlgorithm::BremRecoveryAlgorithm() :
    m_dEtaMargin(0.005),
    m_dPhiMargin(0.05)
{}

pandora::StatusCode BremRecoveryAlgorithm::Run() {
  const pandora::TrackList *pTrackList = nullptr;
  PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, pTrackList));

  // retrieve clusters
  const pandora::ClusterList *pClusterList = nullptr;
  PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraContentApi::GetCurrentList(*this, pClusterList));

  // to modify the content by merging clusters
  auto tempClusterVec = pandora::ClusterVector(pClusterList->begin(),pClusterList->end());

  // sort by the decending EM energy
  auto sortByEmEnergy = [] (const pandora::Cluster* c1, const pandora::Cluster* c2) -> bool {
    return c1->GetElectromagneticEnergy() > c2->GetElectromagneticEnergy();
  };

  std::sort(tempClusterVec.begin(), tempClusterVec.end(), sortByEmEnergy);

  // loop over the clusters and find clusters matched to a track
  for (const pandora::Cluster* const pCluster : tempClusterVec) {
    if (pCluster==nullptr) // already merged
      continue;

    if (!pCluster->IsAvailable())
      continue;

    if (pCluster->GetAssociatedTrackList().empty())
      continue;

    // TODO apply a templated electron ID
    // if (not pass ID...)
    //   continue;

    // FIXME for the moment we assume all clusters matched to a track are electron
    
    // For now it is very simple algorithm
    // 0. Take the best-matched track (based on the distance btn cluster & traj. state on calo)
    // 1. Extrapolate the tangent to the calo surface
    // 2. Select any cluster located within a strip between the electron cluster and the extrapolated tangent position
    // 3. Incorporate the selected clusters and recluster the electron cluster
    // TODO
    // can be further optimized e.g. the "mustache supercluster" + "refined cluster" a la CMS

    // 0. Take the best-matched track (based on the distance btn cluster & traj. state on calo)
    auto sortByTrackClusterDistance = [&pCluster] (const pandora::Track* a, const pandora::Track* b) -> bool {
      return BremRecoveryUtility::sortByDistance(a,b,pCluster);
    };

    pandora::TrackList matchedTrackList = pCluster->GetAssociatedTrackList(); // pandora::TrackList = std::list<pandora::Track*>
    matchedTrackList.sort(sortByTrackClusterDistance);
    const auto* closestTrack = *matchedTrackList.begin();

    // 1. Extrapolate the tangent to the calo surface
    const auto& trackState1st = closestTrack->GetTrackStateAtStart();
    const auto refPt = BremRecoveryUtility::ThreeVector(trackState1st.GetPosition().GetX(),
                                                        trackState1st.GetPosition().GetY(),
                                                        trackState1st.GetPosition().GetZ());
    const auto momVec = BremRecoveryUtility::ThreeVector(trackState1st.GetMomentum().GetX(),
                                                         trackState1st.GetMomentum().GetY(),
                                                         trackState1st.GetMomentum().GetZ());
    bool aStatus = false;
    // first try out the extrapolation on z to figure out whether it is the barrel or endcap
    // retrieve the detector geometry parameters
    const pandora::GeometryManager *const pGeometryManager(PandoraContentApi::GetGeometry(*this));
    double zPlane = pGeometryManager->GetSubDetector(pandora::SubDetectorType::ECAL_ENDCAP).GetInnerZCoordinate();
    double radius = pGeometryManager->GetSubDetector(pandora::SubDetectorType::ECAL_BARREL).GetInnerRCoordinate();
    BremRecoveryUtility::ThreeVector intersectZ;
    aStatus = BremRecoveryUtility::GetPointInZ(zPlane,refPt,momVec,intersectZ);

    if (!aStatus) {
      throw std::runtime_error("BremRecoveryAlgorithm: failed to run BremRecoveryUtility::GetPointInZ!");

      return pandora::STATUS_CODE_FAILURE;
    }

    BremRecoveryUtility::ThreeVector finalIntersectPt = intersectZ;

    if (radius <= 0.) { // this should never happen
      throw std::invalid_argument("BremRecoveryAlgorithm: ECAL barrel inner radius < 0!");

      return pandora::STATUS_CODE_NOT_ALLOWED;
    }    

    if (intersectZ.magXY() > radius) // the tangent intersects the barrel before reaching the endcap - recalculate it
      aStatus = BremRecoveryUtility::GetPointOnCircle(radius,refPt,momVec,finalIntersectPt);

    if (!aStatus) {
      throw std::runtime_error("BremRecoveryAlgorithm: failed to run BremRecoveryUtility::GetPointOnCircle!");

      return pandora::STATUS_CODE_FAILURE;
    }

    // 2. Select any cluster located within a strip between the electron cluster and the extrapolated tangent position
    pandora::ClusterList bremCands;
    const auto& clusterPos1st = pCluster->GetCentroid( pCluster->GetInnerPseudoLayer() );
    const auto vecClus1 = BremRecoveryUtility::ThreeVector(clusterPos1st.GetX(),
                                                           clusterPos1st.GetY(),
                                                           clusterPos1st.GetZ());
    const double eta1st = vecClus1.eta();
    const double phi1st = vecClus1.phi();
    const double maxEta = eta1st + m_dEtaMargin;
    const double minEta = eta1st - m_dEtaMargin;
    const double bremBandPhi = BremRecoveryUtility::deltaPhi(finalIntersectPt.phi(),phi1st);
    const double maxDPhi = bremBandPhi > 0. ? bremBandPhi + m_dPhiMargin : m_dPhiMargin;
    const double minDPhi = bremBandPhi > 0. ? -m_dPhiMargin : bremBandPhi - m_dPhiMargin;

    // loop over clusters again
    for (unsigned iclus = 0; iclus < tempClusterVec.size(); iclus++) {
      const auto* pCluster2nd = tempClusterVec.at(iclus);

      if (pCluster2nd==nullptr) // already merged
        continue;

      if (!pCluster2nd->IsAvailable())
        continue;

      if (pCluster2nd==pCluster) // skip the same cluster
        continue;

      // TODO apply a templated e/gamma ID
      // if (not pass ID...)
      //   continue;

      // check the strip
      const auto& clusterPos2nd = pCluster2nd->GetCentroid( pCluster2nd->GetInnerPseudoLayer() );
      const auto vecClus2 = BremRecoveryUtility::ThreeVector(clusterPos2nd.GetX(),
                                                             clusterPos2nd.GetY(),
                                                             clusterPos2nd.GetZ());
      const double eta2nd = vecClus2.eta();
      const double phi2nd = vecClus2.phi();
      const double finalDeltaPhi = BremRecoveryUtility::deltaPhi(phi2nd,phi1st);

      // 3. Incorporate the selected clusters and recluster the electron cluster
      if (minEta < eta2nd && eta2nd < maxEta && minDPhi < finalDeltaPhi && finalDeltaPhi < maxDPhi) {
        tempClusterVec.at(iclus) = nullptr;
        PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=, PandoraContentApi::MergeAndDeleteClusters(*this, pCluster, pCluster2nd));
      }
    } // loop clusters (2nd)
  } // loop clusters (1st)

  return pandora::STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

pandora::StatusCode BremRecoveryAlgorithm::ReadSettings(const pandora::TiXmlHandle xmlHandle) {
  PANDORA_RETURN_RESULT_IF_AND_IF(pandora::STATUS_CODE_SUCCESS, pandora::STATUS_CODE_NOT_FOUND, !=, pandora::XmlHelper::ReadValue(xmlHandle,
      "dEtaMargin", m_dEtaMargin));

  PANDORA_RETURN_RESULT_IF_AND_IF(pandora::STATUS_CODE_SUCCESS, pandora::STATUS_CODE_NOT_FOUND, !=, pandora::XmlHelper::ReadValue(xmlHandle,
      "dPhiMargin", m_dPhiMargin));

  if (m_dEtaMargin < 0.) {
    throw std::invalid_argument("BremRecoveryAlgorithm: dEtaMargin must be > 0!");

    return pandora::STATUS_CODE_INVALID_PARAMETER;
  }

  if (m_dPhiMargin < 0.) {
    throw std::invalid_argument("BremRecoveryAlgorithm: dPhiMargin must be > 0!");

    return pandora::STATUS_CODE_INVALID_PARAMETER;
  }

  return pandora::STATUS_CODE_SUCCESS;
}
