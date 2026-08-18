/**
 *  @file   k4GaudiPandora/include/GeometryCreatorIdea.h
 *
 *  @brief  Header file for the geometry creator class.
 *
 *  $Log: $
 */

#ifndef GeometryCreatorIdea_h
#define GeometryCreatorIdea_h

#include "Api/PandoraApi.h"

#include "DDGeometryCreator.h"

//------------------------------------------------------------------------------------------------------------------------------------------

/**
 *  @brief  Geometry creator for the IDEA detector
 */
class GeometryCreatorIdea : public DDGeometryCreator {
public:
  /**
   *  @brief  Constructor
   *
   *  @param  settings the creator settings
   *  @param  pPandora address of the relevant pandora instance
   */
  GeometryCreatorIdea(const Settings& settings, pandora::Pandora& pPandora,
                        Gaudi::Algorithm* algorithm);

  /**
   *  @brief  Create geometry
   */
  pandora::StatusCode CreateGeometry() const override;

private:
  /**
   *  @brief  Set mandatory sub detector parameters
   *
   *  @param  subDetectorTypeMap the sub detector type map
   */
  void SetMandatorySubDetectorParameters(SubDetectorTypeMap& subDetectorTypeMap) const override;

  // IDEA ECAL parameters (fiber DRC for o1, crystal DRC for o2)
  void SetEcalParameters(const dd4hep::rec::LayeredCalorimeterData& inputParameters,
                         PandoraApi::Geometry::SubDetector::Parameters& paramBarrel,
                         PandoraApi::Geometry::SubDetector::Parameters& paramEndcap) const;
  void SetHcalBarrelParameters(const dd4hep::rec::LayeredCalorimeterData& inputParameters,
                         PandoraApi::Geometry::SubDetector::Parameters& paramBarrel) const;
  void SetHcalEndcapParameters(const dd4hep::rec::LayeredCalorimeterData& inputParameters,
                         PandoraApi::Geometry::SubDetector::Parameters& paramEndcap) const;

};

#endif
