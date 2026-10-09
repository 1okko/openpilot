from opendbc.car import get_safety_config, structs
from opendbc.car.interfaces import CarInterfaceBase
from opendbc.car.new_vw.carcontroller import CarController
from opendbc.car.new_vw.carstate import CarState
from opendbc.car.new_vw.radar_interface import RadarInterface
from opendbc.car.volkswagen.values import NetworkLocation, TransmissionType, VolkswagenSafetyFlags


class CarInterface(CarInterfaceBase):
  CarState = CarState
  CarController = CarController
  RadarInterface = RadarInterface

  @staticmethod
  def _get_params(ret: structs.CarParams, candidate, fingerprint, car_fw, alpha_long, is_release, docs) -> structs.CarParams:
    ret.brand = "new_vw"

    # MQB EVO Gen2 shares the MEB safety hooks in the panda firmware, so use the existing
    # volkswagenMeb safety model plus the gen2 CRC/DLC variant. No safety change and no panda
    # firmware rebuild is required for this brand.
    ret.safetyConfigs = [get_safety_config(structs.CarParams.SafetyModel.volkswagenMeb)]
    ret.safetyConfigs[0].safetyParam |= VolkswagenSafetyFlags.MEB_ALT_CRC.value

    ret.transmissionType = TransmissionType.direct
    ret.steerControlType = structs.CarParams.SteerControlType.curvature
    ret.steerAtStandstill = True
    ret.steerActuatorDelay = 0.3

    ret.lateralTuning.init('pid')
    ret.lateralTuning.pid.kpBP = [10., 40.]
    ret.lateralTuning.pid.kpV = [0., 1.45]
    ret.lateralTuning.pid.kiBP = [10., 40.]
    ret.lateralTuning.pid.kiV = [0., 0.12]
    ret.lateralTuning.pid.kf = 1.

    ret.networkLocation = NetworkLocation.gateway
    # 0x24F (radar objects) is not published by this car
    ret.radarUnavailable = True
    ret.enableBsm = 0x24C in fingerprint[0]  # MEB_Side_Assist_01

    ret.openpilotLongitudinalControl = True
    ret.longitudinalActuatorDelay = 0.3
    ret.radarDelay = 0.15
    ret.longitudinalTuning.kiBP = [0., 30.]
    ret.longitudinalTuning.kiV = [0.4, 0.]

    ret.pcmCruise = False
    ret.stopAccel = -0.55
    ret.autoResumeSng = False

    return ret