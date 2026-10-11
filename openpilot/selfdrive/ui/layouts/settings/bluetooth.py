import pyray as rl

from openpilot.common.params import Params
from openpilot.system.ui.lib.bluetooth_manager import BluetoothManager
from openpilot.system.ui.lib.multilang import tr, tr_noop
from openpilot.system.ui.widgets import Widget
from openpilot.system.ui.widgets.list_view import button_item, multiple_button_item, text_item, toggle_item
from openpilot.system.ui.widgets.scroller_tici import Scroller


BLUETOOTH_DESCRIPTION = tr_noop("Pair phones, speakers, and controllers with your device over Bluetooth.")
SCAN_DESCRIPTION = tr_noop("Look for nearby Bluetooth devices. Keep the device offroad while pairing.")


class BluetoothLayout(Widget):
  def __init__(self):
    super().__init__()
    self._params = Params()
    self._manager = BluetoothManager()
    self._addresses: tuple[str, ...] | None = None
    self._active = False
    self._prompt_id = ""
    self._scroller = Scroller([], line_separator=True, spacing=0)
    self._enable_item = toggle_item(lambda: tr("Bluetooth"), description=tr(BLUETOOTH_DESCRIPTION),
                                    initial_state=False, callback=self._on_power)
    self._scan_item = button_item(lambda: tr("Find Devices"), self._scan_label,
                                  description=tr(SCAN_DESCRIPTION), callback=self._on_scan)
    self._status_item = text_item(lambda: tr("Status"), self._status_text)
    self._prompt_item = multiple_button_item(lambda: tr("Pairing Request"), self._prompt_text,
                                             [lambda: tr("ACCEPT"), lambda: tr("DECLINE")], 0,
                                             callback=self._on_prompt_response)
    self._prompt_item.set_visible(lambda: self._prompt() is not None)
    self._rebuild(force=True)

  # ---------------------------------------------------------------- lifecycle
  def show_event(self):
    super().show_event()
    self._active = True
    self._manager.set_active(True)
    self._rebuild(force=True)
    self._scroller.show_event()

  def hide_event(self):
    super().hide_event()
    self._active = False
    self._manager.set_active(False)
    self._scroller.hide_event()

  def _render(self, rect):
    self._enable_item.action_item.set_state(self._status().enabled)
    self._rebuild()
    self._scroller.render(rect)

  # ------------------------------------------------------------------ helpers
  def _status(self):
    return self._manager.status

  @staticmethod
  def _norm(address: str) -> str:
    return (address or "").upper()

  def _device(self, address: str):
    for device in self._status().devices:
      if self._norm(device.address) == self._norm(address):
        return device
    return None

  def _prompt(self):
    prompt = self._status().prompt
    if prompt is None or prompt.get("display_only", False):
      return None
    return prompt

  # -------------------------------------------------------------------- build
  def _rebuild(self, force: bool = False):
    signature = tuple(sorted(self._norm(device.address) for device in self._status().devices))
    if not force and signature == self._addresses:
      return
    self._addresses = signature

    self._scan_item.action_item.set_enabled(lambda: self._status().enabled and self._status().powered)

    items = [self._enable_item, self._scan_item, self._prompt_item, self._status_item]
    for address in signature:
      items.append(self._device_item(address))

    self._scroller = Scroller(items, line_separator=True, spacing=0)
    if self._active:
      self._scroller.show_event()

  def _device_item(self, address: str):
    def title():
      device = self._device(address)
      name = (device.name if device is not None else "") or address
      return name if name != address else address

    def description():
      device = self._device(address)
      if device is None:
        return address
      parts = [address]
      if device.connected:
        parts.append(tr("Connected"))
      elif device.paired or device.trusted:
        parts.append(tr("Paired"))
      if device.audio:
        parts.append(tr("Audio"))
      if device.controller:
        parts.append(tr("Controller"))
      if device.rssi is not None:
        parts.append(f"{device.rssi} dBm")
      operation = self._manager.operation_for(address)
      if operation:
        parts.append(tr("working..."))
      return "  ".join(parts)

    def primary_label():
      device = self._device(address)
      if device is None or not (device.paired or device.trusted):
        return tr("PAIR")
      return tr("CONNECT") if not device.connected else tr("DISCONNECT")

    def secondary_label():
      device = self._device(address)
      return tr("FORGET") if device is not None and (device.paired or device.trusted) else tr("FORGET")

    def on_action(index: int):
      device = self._device(address)
      if device is None:
        return
      if index == 1:
        if device.paired or device.trusted:
          self._manager.forget(address)
        return
      if not (device.paired or device.trusted):
        self._manager.pair(address)
      elif not device.connected:
        self._manager.connect(address)
        if device.audio:
          self._manager.select_audio(address)
      else:
        self._manager.disconnect(address)

    return multiple_button_item(title, description, [primary_label, secondary_label], 0, callback=on_action)

  # ------------------------------------------------------------------ actions
  def _on_power(self, enabled: bool):
    self._manager.set_power(enabled)

  def _on_scan(self):
    status = self._status()
    self._manager.set_scanning(not status.discovering)

  def _on_prompt_response(self, index: int):
    prompt = self._prompt()
    if prompt is None:
      return
    self._manager.respond(str(prompt.get("id", "")), index == 0)

  # ------------------------------------------------------------------- labels
  def _scan_label(self):
    return tr("STOP") if self._status().discovering else tr("SCAN")

  def _prompt_text(self):
    prompt = self._prompt()
    if prompt is None:
      return ""
    name = str(prompt.get("name") or prompt.get("address") or tr("Bluetooth device"))
    kind = str(prompt.get("kind", ""))
    value = str(prompt.get("value", "") or "")
    if kind in ("pin", "passkey") and value:
      return f"{name}  {value}"
    return name

  def _status_text(self):
    status = self._status()
    if not status.enabled:
      return tr("Off")
    if not status.powered:
      return tr("Starting...")
    parts = [tr("On")]
    if status.discovering:
      parts.append(tr("scanning"))
    error = self._manager.consume_error()
    if error:
      parts.append(error)
    elif status.error:
      parts.append(status.error)
    elif status.selected_audio:
      parts.append(f"{tr('Audio output')}: {status.selected_audio}")
    return "  ".join(parts)
