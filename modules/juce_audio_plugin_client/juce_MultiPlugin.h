/*
  ==============================================================================

   Several plug-ins from one bundle.

   A plug-in that sets MULTI_PLUGIN_TABLE in juce_add_plugin builds with
   JUCE_MULTI_PLUGIN=1, and JUCE_MULTI_PLUGIN_TABLE_HEADER names a header that
   defines its table:

       inline constexpr juce::PluginDescriptor juceMultiPluginTable[]
       {
           { "My Effect", "An effect", juce::fourCharCode ("Fxx1"), juce::fourCharCode ("aufx"),
             false, false, false, "Fx", "com.me.effect", 0x00000001 },
           { "My Synth",  "A synth",   juce::fourCharCode ("Syn1"), juce::fourCharCode ("aumu"),
             true,  true,  false, "Instrument|Synth", "com.me.synth", 0x00000800 },
       };

   and the plug-in defines createMultiPluginFilter(), which returns the
   processor for a row. The VST3, AU and AAX wrappers then register one plug-in
   per row: VST3 a component and controller class each, AU one AudioComponents
   entry each sharing the factory function, AAX one effect each.

   The first row is the plug-in that juce_add_plugin describes: its name,
   description, plug-in code, synth flag and VST3 category must match those
   arguments, which the checks below enforce. NEEDS_MIDI_INPUT and
   NEEDS_MIDI_OUTPUT compile the MIDI paths in, so they must be on when any row
   needs them; each row then decides at run time.

   The table is metadata only, with no factory functions, so the VST3 manifest
   helper and the AU plist helper can compile it without linking the plug-in.

   Not part of upstream JUCE: carried on the Playhead fork. Without
   MULTI_PLUGIN_TABLE nothing here is compiled, and a build is the same as
   upstream.

  ==============================================================================
*/

#pragma once

#ifndef JUCE_MULTI_PLUGIN
 #define JUCE_MULTI_PLUGIN 0
#endif

#if JUCE_MULTI_PLUGIN
 /** Expands to its arguments only in a multi-plug-in build. The wrappers use it so that a
     single-plug-in build compiles exactly the tokens upstream JUCE compiles. */
 #define JUCE_MULTI_PLUGIN_ONLY(...) __VA_ARGS__
 #define JUCE_MULTI_PLUGIN_SELECT(multi, single) multi
#else
 #define JUCE_MULTI_PLUGIN_ONLY(...)
 #define JUCE_MULTI_PLUGIN_SELECT(multi, single) single
#endif

#if JUCE_MULTI_PLUGIN

// Included only here, so that a single-plug-in build pulls in nothing upstream does not.
#include <cstddef>
#include <cstdint>
#include <iterator>

#ifndef JUCE_MULTI_PLUGIN_TABLE_HEADER
 #error "JUCE_MULTI_PLUGIN needs JUCE_MULTI_PLUGIN_TABLE_HEADER. Set MULTI_PLUGIN_TABLE in juce_add_plugin."
#endif

#if defined (JUCE_VST3_COMPATIBLE_CLASSES) || JUCE_VST3_CAN_REPLACE_VST2 || JucePlugin_Enable_ARA
 #error "A multi-plug-in build supports neither VST3 compatible classes, VST2 replacement nor ARA."
#endif

namespace juce
{
class AudioProcessor;

/** A four-character code as the big-endian integer hosts use: fourCharCode ("aufx"). */
constexpr uint32_t fourCharCode (const char (&code)[5]) noexcept
{
    return ((uint32_t) (unsigned char) code[0] << 24)
         | ((uint32_t) (unsigned char) code[1] << 16)
         | ((uint32_t) (unsigned char) code[2] << 8)
         |  (uint32_t) (unsigned char) code[3];
}

/** The identity of one plug-in among several that one bundle registers. */
struct PluginDescriptor
{
    const char* name;           // the name hosts list
    const char* description;    // the AU AudioComponents description
    uint32_t pluginCode;        // VST3 class IDs, AU subtype and AAX product ID
    uint32_t auType;            // the AU main type, e.g. fourCharCode ("aufx")
    bool isSynth;
    bool wantsMidiInput;
    bool producesMidiOutput;
    const char* vst3Category;   // e.g. "Fx" or "Instrument|Synth"
    const char* aaxIdentifier;
    uint32_t aaxCategory;       // an AAX_EPlugInCategory value
};
} // namespace juce

#include JUCE_MULTI_PLUGIN_TABLE_HEADER

namespace juce::multi_plugin
{
inline constexpr size_t size = std::size (::juceMultiPluginTable);

constexpr const PluginDescriptor& at (size_t index) noexcept { return ::juceMultiPluginTable[index]; }

constexpr bool isMidiEffect (const PluginDescriptor& d) noexcept       { return d.auType == fourCharCode ("aumi"); }
constexpr bool wantsMidiInput (const PluginDescriptor& d) noexcept     { return d.wantsMidiInput || isMidiEffect (d); }
constexpr bool producesMidiOutput (const PluginDescriptor& d) noexcept { return d.producesMidiOutput || isMidiEffect (d); }

/** The row with this plug-in code, or nullptr. */
constexpr const PluginDescriptor* find (uint32_t pluginCode) noexcept
{
    for (size_t i = 0; i < size; ++i)
        if (at (i).pluginCode == pluginCode)
            return &at (i);

    return nullptr;
}

namespace detail
{
    constexpr bool equal (const char* a, const char* b) noexcept
    {
        for (; *a != 0 && *a == *b; ++a, ++b) {}
        return *a == *b;
    }

    constexpr bool codesAreUnique() noexcept
    {
        for (size_t i = 0; i < size; ++i)
            for (size_t j = i + 1; j < size; ++j)
                if (at (i).pluginCode == at (j).pluginCode)
                    return false;

        return true;
    }

    template <typename Predicate>
    constexpr bool anyRow (Predicate predicate) noexcept
    {
        for (size_t i = 0; i < size; ++i)
            if (predicate (at (i)))
                return true;

        return false;
    }
} // namespace detail

static_assert (size >= 1, "The multi-plug-in table is empty");
static_assert (detail::codesAreUnique(), "Two rows of the multi-plug-in table share a plug-in code");

#ifdef JucePlugin_PluginCode
static_assert (at (0).pluginCode == JucePlugin_PluginCode,
               "The first row of the multi-plug-in table must have the PLUGIN_CODE given to juce_add_plugin");
static_assert (at (0).isSynth == (JucePlugin_IsSynth != 0),
               "The first row of the multi-plug-in table must match IS_SYNTH in juce_add_plugin");
static_assert (detail::equal (at (0).name, JucePlugin_Name),
               "The first row of the multi-plug-in table must have the PLUGIN_NAME given to juce_add_plugin");
static_assert (detail::equal (at (0).description, JucePlugin_Desc),
               "The first row of the multi-plug-in table must have the DESCRIPTION given to juce_add_plugin");
static_assert (detail::equal (at (0).vst3Category, JucePlugin_Vst3Category),
               "The first row of the multi-plug-in table must have the VST3_CATEGORIES given to juce_add_plugin");
static_assert (JucePlugin_WantsMidiInput || ! detail::anyRow (wantsMidiInput),
               "A row of the multi-plug-in table takes MIDI in: set NEEDS_MIDI_INPUT TRUE in juce_add_plugin");
static_assert (JucePlugin_ProducesMidiOutput || ! detail::anyRow (producesMidiOutput),
               "A row of the multi-plug-in table sends MIDI out: set NEEDS_MIDI_OUTPUT TRUE in juce_add_plugin");
#endif
} // namespace juce::multi_plugin

/** Defined by the plug-in: a new processor for one row of its multi-plug-in table. */
juce::AudioProcessor* createMultiPluginFilter (const juce::PluginDescriptor&);

#endif // JUCE_MULTI_PLUGIN
