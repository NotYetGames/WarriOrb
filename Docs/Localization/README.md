# [Localization](https://docs.unrealengine.com/en-US/Gameplay/Localization/index.html)


**Config files**: [Config/Localization/](/Config/Localization/)

The files with the `<All|number>.` prefix are written by hand and are intended to be ran from the command line, the other ones (with the prefix `Warriorb_`) are generated by the [Localization Dashboard](https://docs.unrealengine.com/en-US/Gameplay/Localization/LocalizationTools/index.html) and should generally not be written by hand


**Output data**: [Content/Localization/Warriorb](/Content/Localization/Warriorb)

The file/directory structure of the output is explained [here](https://docs.unrealengine.com/en-US/Gameplay/Localization/Overview/#localizationpipeline).

## [Translation Guide](./TranslationGuide.md)

Export markdown into pdf/tml (see [guide](https://superuser.com/questions/689056/how-can-i-convert-github-flavored-markdown-to-a-pdf)):
```sh
npm install -g markdown-pdf
markdown-pdf Docs/Localization/TranslationGuide.md

# OR
# HTML, preserves the format better
pip install grip
grip Docs/Localization/TranslationGuide.md
# or
rip Docs/Localization/TranslationGuide.md --export TranslationGuide.html
```

## [Romanian](./Romanian.md)


## Translating portable object files (po)

After you run the pipeline export step, you edit the `*.po` files in something like:
1. [POEDIT](https://poedit.net/)
	- Desktop program, locally on your computer
	- Shows warnings and easily translate strings
	- More advanced features like automatic translator cost money
	- Can NOT collaborate with multiple people
2. [WEBLATE](https://weblate.org/en/)
	- Server program, host on remote server
	- Harder to setup, needs a server (but this has a docker image)
	- Can collaborate with multiple people
3. [Convert the po files to an excel (xlsx)](https://github.com/NotYetGames/po-xls) and edit them in something like google spreadsheets
	- Another step for conversion to xlsx
	- Can [automatic translate](https://jakemiller.net/translate-in-google-sheets/) using spreadsheets
	- Can collaborate with multiple people

## Portable Object files (po) to excel files (xlsx)

Sometimes people don't want to use poedit to edit

## Demo (WarriorbDemo Target)
```sh
# Does all the steps gathering, import of po files and export of new po files
./Tools/Warriorb/Localization/Demo_All_Run.py
```

## Run (Warriorb Target)
```sh
# Does all the steps gathering, import of po files and export of new po files
./Tools/Warriorb/Localization/All_Run.py
```

## [House keeping (pipeline optimization)](https://docs.unrealengine.com/en-US/Gameplay/Localization/LocalizationPipelineOptimization/index.html)


### ReportStaleGatherCache
Generates a `StaleGatherCacheReport.txt` file alongside the manifest for your localization target. This file contains a list of any Assets that contain a stale gather cache.

```sh
./Tools/Warriorb/Localization/ReportStaleGatherCache.py
```

### FixStaleGatherCache
Attempts to automatically fix any Assets that contain a stale gather cache, by re-saving them.

```sh
./Tools/Warriorb/Localization/FixStaleGatherCache.py
```


### FixMissingGatherCache
For Assets too old to have a gather cache, this attempts to automatically fix Assets that are missing a gather cache by re-saving them.

```sh
./Tools/Warriorb/Localization/FixMissingGatherCache.py
```

## Packaging
You must always edit the [following fields](https://docs.unrealengine.com/en-US/Gameplay/Localization/Overview/index.html) inside package settings if you add/remove a language:
- **Localizations to Package** - With this you can choose which cultures you want to stage localization data for. You can use the Show Localized option to filter the list to only show cultures that you have LocRes files for.
- **Internationalization Support** - With this you can choose which set of ICU internationalization data you want to stage. This option must overlap with the localization data you intend to stage.


## [Asset Localization](https://docs.unrealengine.com/en-US/Gameplay/Localization/Asset/index.html)

Asset localization allows you to completely replace one Asset with another on a per-culture basis.

**Not that useful for us.**


## ELocalizedTextCollapseMode
```cpp
/** Collapse texts with the same text identity (namespace + key) and source text (default 4.15+ behavior). */
IdenticalTextIdAndSource - "Identical Text Identity (Namespace + Key) and Source Text"

/** Collapse texts with the same namespace and source text (legacy pre-4.14 behavior). */
IdenticalNamespaceAndSource	- "Identical Namespace and Source Text")

// USELESS
/** Collapse texts with the same package ID, text identity (namespace + key), and source text (deprecated 4.14 behavior, removed in 4.17). */
IdenticalPackageIdTextIdAndSource - "Identical Package ID, Text Identity (Namespace + Key) and Source Text"
// USELESS
```
