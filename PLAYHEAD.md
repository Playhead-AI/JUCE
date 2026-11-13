# Playhead's JUCE fork

This fork carries one change on top of upstream JUCE: a plug-in can register
several plug-ins from one bundle. Playhead uses it to ship its effect and its
instrument in one `Playhead.vst3`, `Playhead.component` and `Playhead.aaxplugin`.

## Branches

- `playhead/8.0.12` — upstream tag `8.0.12` plus the change below. The
  `playhead-plugin` repo pins `external/JUCE` to this branch.

Nothing else differs from upstream. Moving to a newer JUCE means rebasing the
change onto the new tag as a new `playhead/<version>` branch.

## The change

Set `MULTI_PLUGIN_TABLE <header>` in `juce_add_plugin`. The header defines a table
of `juce::PluginDescriptor` rows, and the plug-in defines
`createMultiPluginFilter()`. Then:

- **VST3**: the factory exports a component class and a controller class per
  row, and `moduleinfo.json` lists them all. Each component reports its own bus
  types and MIDI buses, and only a row that takes MIDI gets the MIDI CC
  parameters.
- **AU**: the bundle's `Info.plist` lists one `AudioComponents` entry per row,
  sharing one factory function. An instance picks its row from its component
  subtype, and a row without MIDI gets the plain factory, so it does not claim
  MIDI.
- **AAX**: `GetEffectDescriptions` adds one effect per row, each with its own
  identifier, category, product ID and create function.

`modules/juce_audio_plugin_client/juce_MultiPlugin.h` documents the table and
the rules it checks at compile time. Without `MULTI_PLUGIN_TABLE`, the wrappers
preprocess to the same tokens as upstream, so a single-plug-in build is
unchanged.

Files: `juce_MultiPlugin.h` and `AU/juce_AUMultiPluginPlistHelper.mm` (new); the
VST3, AU and AAX wrappers, `VST3/juce_VST3ModuleInfo.h`,
`detail/juce_CreatePluginFilter.h`, `juce_audio_plugin_client.h` and
`extras/Build/CMake/JUCEUtils.cmake` (edited).
