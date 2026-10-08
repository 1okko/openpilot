from opendbc.can import CANParser
from opendbc.car import Bus, structs
from opendbc.car.interfaces import RadarInterfaceBase
from opendbc.car.volkswagen.values import DBC, VolkswagenFlags, CanBus

NO_OBJECT_ID = 0
DISTANCE_STATUS_VALID = 0
RADAR_UNAVAILABLE_THRESH = 5
# radar object drel is not the end of the radar facing side but probably the longitudinal
# center of the object; the DBC drel offset of -3.6 m is measured for a point mass (person)
# type object, so statically subtract something between the first half and the end of a
# typical car length.
DREL_FRONT_EDGE_MARGIN = 1.5  # in m
LANE_TYPES = ("Same_Lane", "Left_Lane", "Right_Lane")
SIGNAL_SETS = tuple(
  (
    f"{prefix}_ObjectID",
    f"{prefix}_Long_Distance",
    f"{prefix}_Lat_Distance",
    f"{prefix}_Rel_Velo",
  )
  for lane in LANE_TYPES
  for idx in (1, 2)
  for prefix in (f"{lane}_0{idx}",)
)


# The gateway harness does not expose the raw radar points, but the camera publishes filtered
# tracks: two per lane, for the left, center and right lanes. MEB calls that message
# MEB_Distance_01, while the MQB EVO DBC carries the exact same payload as Strukturen_01.
RADAR_TRACK_MESSAGE = (
  (VolkswagenFlags.MEB, "MEB_Distance_01"),
  (VolkswagenFlags.MQB_EVO, "Strukturen_01"),
)


def get_radar_message(CP):
  if not (CP.flags & (VolkswagenFlags.MEB | VolkswagenFlags.MQB_EVO)):
    return None

  if CP.flags & VolkswagenFlags.MQB_EVO_GEN2:  # generation specific, no track message on the bus
    return None

  for flag, message in RADAR_TRACK_MESSAGE:
    if CP.flags & flag:
      return message

  return None


class RadarInterface(RadarInterfaceBase):
  def __init__(self, CP, CP_SP):
    super().__init__(CP, CP_SP)

    self.radar_off_can: bool = CP.radarUnavailable
    self.radar_unavailable_cnt: int = 0
    self.rcp: CANParser | None = None
    self.radar_message: str | None = get_radar_message(CP)

    if self.radar_message is not None:
      self.rcp = CANParser(DBC[CP.carFingerprint][Bus.radar], [(self.radar_message, 25)], CanBus(CP).cam)

  def update(self, can_strings):
    if self.radar_off_can or self.rcp is None:
      return super().update(None)

    self.rcp.update(can_strings)

    if len(self.rcp.vl_all[self.radar_message]["Distance_Status"]) == 0:
      # The track message hasn't been seen yet. Keep publishing empty radar data instead of
      # nothing, so consumers waiting on radarTracks don't stall.
      return super().update(None)

    return self._update()

  def _update(self):
    ret = structs.RadarData()

    if not self.rcp.can_valid:
      ret.errors.canError = True
      return ret

    msg = self.rcp.vl[self.radar_message]

    # Radar reports its overall validity via Distance_Status (0 = Valid, 3 = Invalid).
    # Treat consecutive invalid reports as a temporary radar unavailability, similar to Ford MRR.
    if msg["Distance_Status"] != DISTANCE_STATUS_VALID:
      self.radar_unavailable_cnt += 1
    else:
      self.radar_unavailable_cnt = 0

    if self.radar_unavailable_cnt >= RADAR_UNAVAILABLE_THRESH:
      self.pts.clear()
      ret.errors.radarUnavailableTemporary = True
      return ret

    seen_ids = set()
    for obj_id_sig, long_sig, lat_sig, vel_sig in SIGNAL_SETS:
      obj_id = int(msg[obj_id_sig])
      if obj_id == NO_OBJECT_ID:
        continue

      # We shouldn't see duplicate track ids
      if obj_id in seen_ids:
        ret.errors.canError = True
        return ret

      seen_ids.add(obj_id)

      if obj_id not in self.pts:
        pt = structs.RadarData.RadarPoint()
        pt.trackId = self.track_id
        self.track_id += 1
        self.pts[obj_id] = pt
      else:
        pt = self.pts[obj_id]

      pt.dRel = msg[long_sig] - DREL_FRONT_EDGE_MARGIN
      pt.yRel = msg[lat_sig]
      pt.vRel = msg[vel_sig]

    inactive_ids = self.pts.keys() - seen_ids
    for obj_id in inactive_ids:
      self.pts.pop(obj_id, None)

    ret.points = list(self.pts.values())
    return ret
