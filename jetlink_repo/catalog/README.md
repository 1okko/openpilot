# Extra big models

`extra_big_models.json` lists big models that sunnypilot's model library does
not carry yet, such as previews from comma's open pull requests. Commas without
a chestnut, and every Jetlink server, add it to sunnypilot's list. An edit on
`main` reaches them within the hour, with no release.

Each entry is a sunnypilot catalog bundle. The comma reads every field below,
and a bundle missing one makes it drop the whole list, so copy an existing entry:

| Field | Value |
| --- | --- |
| `ref` | The comma openpilot commit whose `big_driving_supercombo.onnx` is the model. Check it with `jetlink-server models resolve <ref>`. |
| `display_name` | The name in the picker, ending in the model's date as `(Month DD, YYYY)`. |
| `short_name` | The short name the comma shows for the selected model. |
| `index` | Its place in the list: higher comes first. |
| `minimum_selector_version` | `"19"`, the fork's version. |
| `is_big`, `is_20hz` | `true`. |
| `generation`, `environment`, `runner`, `build_time` | As sunnypilot's newest big model has them. |
| `overrides` | `folder` groups the picker; `lat` and `long` are the model's smoothing, as sunnypilot sets them. |
| `models` | `[]`. A chestnut has nothing to download, and does not see this list. |

When sunnypilot lists the same commit, its entry wins. When it lists the model
under another commit, remove ours. Removing an entry moves anyone who picked it
back to the default model.
