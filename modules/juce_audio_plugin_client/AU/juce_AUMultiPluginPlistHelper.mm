/*
  ==============================================================================

   Rewrites a multi-plug-in AU bundle's Info.plist to list one AudioComponents
   entry per row of the plug-in's table (see juce_MultiPlugin.h).

   JUCE's plist already holds the entry for the first row. Every other entry
   is a copy of it, with that row's name, description, type and subtype, so it
   shares the first entry's factory function, version and resource settings.
   Running it again leaves an up-to-date plist untouched.

   Built and run by juce_add_plugin when MULTI_PLUGIN_TABLE is set.

  ==============================================================================
*/

#import <Foundation/Foundation.h>

#include <juce_audio_plugin_client/juce_MultiPlugin.h>

#include <cstdio>

#if ! JUCE_MULTI_PLUGIN
 #error "The AU multi-plug-in plist helper only builds for a plug-in with MULTI_PLUGIN_TABLE set"
#endif

static NSString* fourCharString (uint32_t code)
{
    const char chars[] { (char) (code >> 24), (char) (code >> 16), (char) (code >> 8), (char) code, 0 };
    return [NSString stringWithUTF8String: chars];
}

static NSString* componentName (const juce::PluginDescriptor& d)
{
    // JUCE names an entry "<COMPANY_NAME>: <PLUGIN_NAME>".
    return [NSString stringWithFormat: @"%s: %s", JucePlugin_Manufacturer, d.name];
}

static int fail (const char* message)
{
    std::fprintf (stderr, "AU multi-plug-in plist helper: %s\n", message);
    return 1;
}

int main (int argc, const char* argv[])
{
    @autoreleasepool
    {
        if (argc != 2)
            return fail ("usage: <helper> <path to the AU bundle's Info.plist>");

        auto* path = [NSString stringWithUTF8String: argv[1]];
        NSMutableDictionary* plist = [[NSDictionary dictionaryWithContentsOfFile: path] mutableCopy];

        if (plist == nil)
            return fail ("cannot read the plist");

        NSArray* components = plist[@"AudioComponents"];

        if (! [components isKindOfClass: [NSArray class]] || components.count == 0)
            return fail ("the plist has no AudioComponents entry");

        NSDictionary* first = components[0];
        const auto& firstRow = juce::multi_plugin::at (0);

        if (! [first[@"subtype"] isEqual: fourCharString (firstRow.pluginCode)]
            || ! [first[@"name"] isEqual: componentName (firstRow)])
            return fail ("the first AudioComponents entry is not the first row of the table");

        NSMutableArray* entries = [NSMutableArray arrayWithObject: first];

        for (size_t i = 1; i < juce::multi_plugin::size; ++i)
        {
            const auto& row = juce::multi_plugin::at (i);
            NSMutableDictionary* entry = [first mutableCopy];
            entry[@"name"] = componentName (row);
            entry[@"description"] = [NSString stringWithUTF8String: row.description];
            entry[@"type"] = fourCharString (row.auType);
            entry[@"subtype"] = fourCharString (row.pluginCode);
            [entries addObject: entry];
        }

        if ([entries isEqualToArray: components])
            return 0;

        plist[@"AudioComponents"] = entries;

        NSError* error = nil;
        NSData* data = [NSPropertyListSerialization dataWithPropertyList: plist
                                                                  format: NSPropertyListXMLFormat_v1_0
                                                                 options: 0
                                                                   error: &error];

        if (data == nil || ! [data writeToFile: path atomically: YES])
            return fail ("cannot write the plist");

        std::printf ("AU plist: %zu AudioComponents entries\n", juce::multi_plugin::size);
    }

    return 0;
}
