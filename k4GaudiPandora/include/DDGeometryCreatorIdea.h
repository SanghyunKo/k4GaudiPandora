/**
 *  @file   k4GaudiPandora/include/DDGeometryCreatorIdea.h
 *
 *  @brief  Header file for the geometry creator class.
 *
 *  $Log: $
 */

#ifndef DDGeometryCreatorIdea_h
#define DDGeometryCreatorIdea_h

#include "Api/PandoraApi.h"

#include "DDGeometryCreator.h"

//------------------------------------------------------------------------------------------------------------------------------------------

/**
 *  @brief  DDGeometryCreator class
 */
class DDGeometryCreatorIdea : public DDGeometryCreator {
public:
  // inherited settings specific to IDEA geometry creator
  class Settings : public DDGeometryCreator::Settings {
  public:
    Settings()=default;
    ~Settings()=default;

    bool m_isOption2;
  };

  /**
   *  @brief  Constructor
   *
   *  @param  settings the creator settings
   *  @param  pPandora address of the relevant pandora instance
   */
  DDGeometryCreatorIdea(const Settings& settings, pandora::Pandora& pPandora,
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

  // IDEA ECAL parameters (fiber DRC for o1, crystal DRC for o2)
  void SetEcalParameters(const dd4hep::rec::LayeredCalorimeterData& inputParameters,
                         PandoraApi::Geometry::SubDetector::Parameters& paramBarrel,
                         PandoraApi::Geometry::SubDetector::Parameters& paramEndcap) const;
  void SetHcalBarrelParameters(const dd4hep::rec::LayeredCalorimeterData& inputParameters,
                         PandoraApi::Geometry::SubDetector::Parameters& paramBarrel) const;
  void SetHcalEndcapParameters(const dd4hep::rec::LayeredCalorimeterData& inputParameters,
                         PandoraApi::Geometry::SubDetector::Parameters& paramEndcap) const;

  const Settings m_settings;
};

#endif
