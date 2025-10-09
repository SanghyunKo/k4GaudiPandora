#ifndef BremRecoveryAlgorithm_h
#define BremRecoveryAlgorithm_h 1

#include "Pandora/Algorithm.h"
#include "Helpers/XmlHelper.h"

#include "Objects/Track.h"
#include "Objects/Cluster.h"

// Split tangent extrapolation utilities
// so that devs can wrap them in either pandora::Algorithm or GaudiAlg
// upon a request
namespace BremRecoveryUtility {
  // Define its own ThreeVector class
  // considering possible implementation in the standalone Gaudi/Pandora algorithm
  struct ThreeVector {
    double x, y, z;
    
    ThreeVector() : x(0), y(0), z(0) {}
    ThreeVector(double x0, double y0, double z0) : x(x0), y(y0), z(z0) {}

    double mag() const { return std::sqrt(x*x + y*y + z*z); }
    double magXY() const { return std::sqrt(x*x + y*y); }
    double phi() const { return std::atan2(y,x); } // -pi to pi
    double theta() const { return std::acos(z / mag()); } // 0 to pi
    double eta() const { return -std::log(std::tan(theta()/2.)); }
  };

  // deltaPhi function to restrict the domain to -pi to pi
  double deltaPhi(const double p1, const double p2);

  // Extrapolation of a straight line to a cylinder-shaped detector
  // clearly motivated by pandora::Helix
  // but pandora::Helix cannot be approximated to a straight line
  // since if we set B = 0 then it throws an exception (to protect from zero division)
  // (the radius of the helix diverges to infinity)
  bool GetPointInZ(const double zPlane,
                   const ThreeVector& refPt,
                   const ThreeVector& momVec,
                   ThreeVector& intersectPt);
  bool GetPointOnCircle(const double radius, 
                        const ThreeVector& refPt, 
                        const ThreeVector& momVec, 
                        ThreeVector& intersectPt);

  // sorting helper for the cluster with multiple matched tracks
  // FIXME this can be implemented in the lc_content::SortingHelper
  bool sortByDistance(const pandora::Track* a,
                      const pandora::Track* b,
                      const pandora::Cluster* const pCluster);
}

class BremRecoveryAlgorithm : public pandora::Algorithm {
public:
  class Factory : public pandora::AlgorithmFactory {
  public:
    pandora::Algorithm* CreateAlgorithm() const;
  };

  BremRecoveryAlgorithm();

private:
  pandora::StatusCode Run();

  pandora::StatusCode ReadSettings(const pandora::TiXmlHandle xmlHandle);

  double m_dEtaMargin;
  double m_dPhiMargin;
};

inline pandora::Algorithm* BremRecoveryAlgorithm::Factory::CreateAlgorithm() const {
  return new BremRecoveryAlgorithm();
}

#endif
