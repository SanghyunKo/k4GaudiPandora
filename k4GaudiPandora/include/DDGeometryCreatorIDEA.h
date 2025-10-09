/**
 *  @file   k4GaudiPandora/include/DDGeometryCreatorIDEA.h
 *
 *  @brief  Header file for the geometry creator class.
 *
 *  $Log: $
 */

#ifndef DDGEOMETRYIDEA_CREATOR_H
#define DDGEOMETRYIDEA_CREATOR_H

#include "Api/PandoraApi.h"

#include "DDGeometryCreator.h"

//------------------------------------------------------------------------------------------------------------------------------------------

/**
 *  @brief  DDGeometryCreator class
 */
class DDGeometryCreatorIDEA : public DDGeometryCreator {
public:
  /**
   *  @brief  Constructor
   *
   *  @param  settings the creator settings
   *  @param  pPandora address of the relevant pandora instance
   */
  DDGeometryCreatorIDEA(const Settings& settings, pandora::Pandora& pPandora,
                        Gaudi::Algorithm* algorithm);

  /**
   *  @brief  Create geometry
   */
  pandora::StatusCode CreateGeometry() const; // override;
  // FIXME base function is not virtual

private:
  /**
   *  @brief  Set mandatory sub detector parameters
   *
   *  @param  subDetectorTypeMap the sub detector type map
   */
  void SetMandatorySubDetectorParameters(SubDetectorTypeMap& subDetectorTypeMap) const;

  // IDEA o1 DRC parameters
  void SetDRCo1Parameters(const dd4hep::rec::LayeredCalorimeterData& inputParameters,
                          const std::string& nameBarrel,
                          const std::string& nameEndcap,
                          PandoraApi::Geometry::SubDetector::Parameters& paramBarrel,
                          PandoraApi::Geometry::SubDetector::Parameters& paramEndcap) const;
};

#endif
