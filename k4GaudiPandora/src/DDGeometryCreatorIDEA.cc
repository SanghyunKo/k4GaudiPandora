/**
 *  @file   k4GaudiPandora/src/DDGeometryCreatorIDEA.cc
 *
 *  @brief  Implementation of the geometry creator class.
 *
 *  $Log: $
 */

#include "DDGeometryCreatorIDEA.h"

#include "DD4hep/DetType.h"
#include "DDRec/DetectorData.h"

// Forward declarations. See DDPandoraPFANewAlgorithm.cc
// dd4hep::rec::LayeredCalorimeterData * getExtension(std::string detectorName);
dd4hep::rec::LayeredCalorimeterData* getExtension(unsigned int includeFlag, unsigned int excludeFlag = 0);

DDGeometryCreatorIDEA::DDGeometryCreatorIDEA(const Settings& settings, pandora::Pandora& pPandora,
                                                   Gaudi::Algorithm* algorithm)
    : DDGeometryCreator(settings, pPandora, algorithm) {}

//------------------------------------------------------------------------------------------------------------------------------------------

pandora::StatusCode DDGeometryCreatorIDEA::CreateGeometry() const {
  try {
    SubDetectorTypeMap subDetectorTypeMap;
    this->SetMandatorySubDetectorParameters(subDetectorTypeMap);

    m_algorithm.debug() << "Creating geometry for IDEA detector" << endmsg;

    for (SubDetectorTypeMap::const_iterator iter = subDetectorTypeMap.begin(), iterEnd = subDetectorTypeMap.end();
         iter != iterEnd; ++iter)
      PANDORA_RETURN_RESULT_IF(pandora::STATUS_CODE_SUCCESS, !=,
                               PandoraApi::Geometry::SubDetector::Create(m_pPandora, iter->second));
  } catch (std::exception& exception) {
    m_algorithm.error() << "Failure in DDGeometryCreatorIDEA, exception: " << exception.what() << endmsg;
    throw exception;
  }

  return pandora::STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

void DDGeometryCreatorIDEA::SetMandatorySubDetectorParameters(SubDetectorTypeMap& subDetectorTypeMap) const {
  PandoraApi::Geometry::SubDetector::Parameters eCalBarrelParameters, eCalEndCapParameters;
  // hCalBarrelParameters, hCalEndCapParameters, muonBarrelParameters, muonEndCapParameters; // TODO they're not used anywhere at the moment, so ignoring them

  this->SetDRCo1Parameters(
      *const_cast<dd4hep::rec::LayeredCalorimeterData*>(
          getExtension((dd4hep::DetType::CALORIMETER | dd4hep::DetType::ELECTROMAGNETIC | dd4hep::DetType::HADRONIC),
                       (dd4hep::DetType::AUXILIARY | dd4hep::DetType::FORWARD))),
      "DualReadoutBarrel", "DualReadoutEndcap", eCalBarrelParameters, eCalBarrelParameters);

  subDetectorTypeMap[pandora::ECAL_BARREL] = eCalBarrelParameters;
  subDetectorTypeMap[pandora::ECAL_ENDCAP] = eCalEndCapParameters;

  // PandoraApi::Geometry::SubDetector::Parameters coilParameters; // TODO retrive coil parameters
}

void DDGeometryCreatorIDEA::SetDRCo1Parameters(const dd4hep::rec::LayeredCalorimeterData& inputParameters,
                                               const std::string& nameBarrel,
                                               const std::string& nameEndcap,
                                               PandoraApi::Geometry::SubDetector::Parameters& paramBarrel,
                                               PandoraApi::Geometry::SubDetector::Parameters& paramEndcap) const {
  paramBarrel.m_subDetectorName = nameBarrel;
  paramBarrel.m_subDetectorType = pandora::ECAL_BARREL;
  paramBarrel.m_innerRCoordinate = inputParameters.extent[0] / dd4hep::mm;
  paramBarrel.m_innerZCoordinate = 0.;
  paramBarrel.m_innerPhiCoordinate = 0.; // not initialized in the LayeredCalorimeterData
  paramBarrel.m_innerSymmetryOrder = 0; // not initialized
  paramBarrel.m_outerRCoordinate = (inputParameters.extent[0] + inputParameters.extent[3] - inputParameters.extent[2]) / dd4hep::mm; // barrel innerR + tower height, tower height = endcap outer Z - endcap inner Z
  paramBarrel.m_outerZCoordinate = inputParameters.extent[2] / dd4hep::mm; // use endcap inner Z
  paramBarrel.m_outerPhiCoordinate = 0.; // not initialized
  paramBarrel.m_outerSymmetryOrder = 0; // not initialized
  paramBarrel.m_isMirroredInZ = true;
  paramBarrel.m_nLayers = inputParameters.layers.size();

  paramEndcap.m_subDetectorName = nameEndcap;
  paramEndcap.m_subDetectorType = pandora::ECAL_ENDCAP;
  paramEndcap.m_innerRCoordinate = inputParameters.extent[4] / dd4hep::mm;
  paramEndcap.m_innerZCoordinate = inputParameters.extent[2] / dd4hep::mm;
  paramEndcap.m_innerPhiCoordinate = 0.; // not initialized in the LayeredCalorimeterData
  paramEndcap.m_innerSymmetryOrder = 0; // not initialized
  paramEndcap.m_outerRCoordinate = inputParameters.extent[5] / dd4hep::mm;
  paramEndcap.m_outerZCoordinate = inputParameters.extent[3] / dd4hep::mm;
  paramEndcap.m_outerPhiCoordinate = 0.; // not initialized
  paramEndcap.m_outerSymmetryOrder = 0; // not initialized
  paramEndcap.m_isMirroredInZ = true;
  paramEndcap.m_nLayers = inputParameters.layers.size();

  return;
}
