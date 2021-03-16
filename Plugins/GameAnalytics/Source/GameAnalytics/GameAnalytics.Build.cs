using UnrealBuildTool;
using System.IO;
using System;
using Tools.DotNETCommon;

namespace UnrealBuildTool.Rules
{
    public class GameAnalytics : ModuleRules
    {
#if WITH_FORWARDED_MODULE_RULES_CTOR
        public GameAnalytics(ReadOnlyTargetRules Target) : base(Target)
#else
        public GameAnalytics(TargetInfo Target)
#endif
        {
            PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

            var GameAnalyticsPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../ThirdParty/" ));
            var libPath = Path.Combine(GameAnalyticsPath, "lib");

            // Test plugin analytics in editor mode
            PublicDefinitions.Add("TEST_NON_EDITOR_PLUGIN_ANALYTICS_MODE=0");

            switch (Target.Platform)
            {
                case UnrealTargetPlatform.Win64:
                    PublicAdditionalLibraries.Add(Path.Combine(libPath, "win64", "GameAnalytics.lib"));
                    PrivateDependencyModuleNames.AddRange(new string[] {  "OpenSSL", "libcurl" });
                    break;

                case UnrealTargetPlatform.Win32:
                    PublicAdditionalLibraries.Add(Path.Combine(libPath, "win32", "GameAnalytics.lib"));
                    PrivateDependencyModuleNames.AddRange(new string[] {  "OpenSSL", "libcurl" });
                    break;

                case UnrealTargetPlatform.Android:
                    PrivateDependencyModuleNames.Add("Launch");
                    PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/Android"));
                    break;

                case UnrealTargetPlatform.Mac:
                    PublicAdditionalLibraries.Add(Path.Combine(libPath, "osx", "libGameAnalytics.a"));
                    PublicAdditionalLibraries.Add("curl");
                    PublicFrameworks.AddRange(
                        new string[] {
                            "CoreFoundation",
                            "Foundation",
                            "CoreServices"
                        }
                    );
                    PrivateDependencyModuleNames.AddRange(new string[] { "OpenSSL" });
                    break;

                case UnrealTargetPlatform.Linux:
                    LoadLibrary(Path.Combine(libPath, "linux"), "libGameAnalytics.a");
                    //PublicAdditionalLibraries.Add(Path.Combine(libPath, "linux", "libGameAnalytics.a"));
                    //RuntimeDependencies.Add(Path.Combine(libPath, "linux", "libGameAnalytics.a"));
                    PrivateDependencyModuleNames.AddRange(new string[] { "OpenSSL", "libcurl" });
                    break;

                case UnrealTargetPlatform.IOS:
                    PublicAdditionalLibraries.Add(Path.Combine(libPath, "ios", "libGameAnalytics.a"));
                    PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/IOS"));
                    PublicFrameworks.AddRange(
                        new string[] {
                            "AdSupport",
                            "SystemConfiguration"
                        }
                    );

                    PublicAdditionalLibraries.AddRange(
                        new string[] {
                            "sqlite3",
                            "z",
                            "c++"
                    });
                    break;

                case UnrealTargetPlatform.HTML5:
                    if (Target.Architecture != "-win32")
                    {
                        PublicAdditionalLibraries.Add(Path.Combine(libPath, "html5", "GameAnalytics.jspre"));
                        PublicAdditionalLibraries.Add(Path.Combine(libPath, "html5", "GameAnalyticsUnreal.js"));
                    }
                    break;

                case UnrealTargetPlatform.XboxOne:
                case UnrealTargetPlatform.PS4:
                default:
                    throw new NotImplementedException("This target platform is not configured for GameAnalytics SDK: " + Target.Platform.ToString());
            }

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    // ... add other public dependencies that you statically link with here ...
                }
            );

            PrivateIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory, "Private")));
            PrivateIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory, "Public")));
            PublicIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory, "Public")));


            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "Analytics",
                    "Engine"
                }
            );

            if (Target.Platform == UnrealTargetPlatform.HTML5)
            {
                PrivateDependencyModuleNames.AddRange(
                    new string[]
                    {
                        "Json"
                    }
                );

                PublicIncludePathModuleNames.AddRange(
                    new string[]
                    {
                        "Json"
                    }
                );
            }

            PublicIncludePathModuleNames.AddRange(
                new string[]
                {
                    "Analytics",
                    "Engine"
                }
            );

            if (Target.Platform == UnrealTargetPlatform.Android)
            {
                AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "GameAnalytics_APL.xml"));
            }
        }

        // From https://github.com/shadowmint/ue4-static-plugin/blob/master/Source/TestPlugin/TestPlugin.Build.cs
        /**
         * Helper to setup an arbitrary library in the given library folder
         * @param build_path Absolute build path, eg. /home/.../3rdparty/mylib/build
         * @param library_name Short library name, eg. libmylib.a, mylib.lib, etc.
         */
        private void LoadLibrary(string build_path, string library_name)
        {
            // Get the build path
            if (!Directory.Exists(build_path))
            {
                Fail("Invalid build path: " + build_path + " (Did you build the 3rdparty module already?)");
            }

            string full_library_path = Path.Combine(build_path, library_name);
            if (!File.Exists(full_library_path))
            {
                Fail("Unable to locate any build library: " + full_library_path);
            }

            // Found a library; add it to the dependencies list
            PublicAdditionalLibraries.Add(full_library_path);
            Trace("Added static library: {0}", full_library_path);
        }

        /**
         * Print out a build message
         * Why error? Well, the UE masks all other errors. *shrug*
         */
        private void Trace(string msg)
        {
            Log.TraceInformation("GameAnalytics: " + msg);
        }

        /** Trace helper */
        private void Trace(string format, params object[] args)
        {
            Trace(string.Format(format, args));
        }

        /** Raise an error */
        private void Fail(string message)
        {
            Trace(message);
            throw new Exception(message);
        }
    }
}
