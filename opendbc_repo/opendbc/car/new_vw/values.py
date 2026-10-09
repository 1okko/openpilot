from dataclasses import dataclass, field

from opendbc.car import Bus, CarSpecs, DbcDict, PlatformConfig, Platforms
from opendbc.car.docs_definitions import CarHarness, CarDocs, CarParts
# Reuse the VW/MEB scaffolding instead of duplicating it. MQB EVO Gen2 uses the MEB gateway
# harness, the MEB safety hooks (see interface.py) and the same CAN bus layout.
from opendbc.car.volkswagen.values import (CanBus, CarControllerParams, FW_QUERY_CONFIG, NetworkLocation,
                                           TransmissionType, GearShifter, VolkswagenCarSpecs, VolkswagenFlags)


@dataclass
class NewVwCarDocs(CarDocs):
  package: str = "All"
  car_parts: CarParts = field(default_factory=CarParts.common([CarHarness.vw_j533]))


@dataclass
class NewVwPlatformConfig(PlatformConfig):
  dbc_dict: DbcDict = field(default_factory=lambda: {Bus.pt: 'vw_mqbevo_2024', Bus.radar: 'vw_mqbevo_2024'})


class CAR(Platforms):
  NEW_VW_AUDI_A3_MK4 = NewVwPlatformConfig(
    [NewVwCarDocs("Audi RS3 2026")],
    VolkswagenCarSpecs(mass=1650., wheelbase=2.631),
  )


DBC = CAR.create_dbc_map()