from opendbc.car.interfaces import RadarInterfaceBase


class RadarInterface(RadarInterfaceBase):
  """MQB EVO Gen2 does not publish radar objects: 0x24F (MEB_Distance_01) is never seen on the
  bus, so there is nothing to parse. Publish empty radar data at the base class rate."""

  def update(self, can_strings):
    return super().update(None)