# Achievements

**Guides:**

1. https://partner.steamgames.com/doc/features/achievements
2. https://forums.unrealengine.com/community/community-content-tools-and-tutorials/26348-tutorial-steam-achievements
3. https://forums.unrealengine.com/development-discussion/blueprint-visual-scripting/1432526-steam-achievements-implementation-for-ue4-developers-tips-and-tricks
4. https://www.orfeasel.com/handling-steam-achievements-steam-integration-part-2/


## Add achievement and use it

1. **[OPTIONAL]** This is not needed but recommended because it appears in the console commands.
Open the **Project Settings** -> **Warriorb Achivement** and add the achievement under the **Game** or **Demo** Category.
2. Add the steam achivements inside steamworks settings for  either the [**Game**](https://partner.steamgames.com/apps/achievements/790360) or for the [**Demo**](https://partner.steamgames.com/apps/achievements/1193760)
3. Call `USoAchievementManager::UnlockAchievement` to unlock an achievement

## Console commands
`SO.Achievements.*`


## Localization
```sh
# Export
./Tools/Steam/Localization_ExportAchivementsToPOFiles.py -p WarriOrb 790360_loc_all.vdf
./Tools/Steam/Localization_ExportAchivementsToPOFiles.py -p WarriOrbPrologue 1193760_loc_all.vdf

# Import
./Tools/Steam//Localization_ImportAchivementsFromPOFiles.py -d 790360_loc_all.vdf <po_files>
./Tools/Steam//Localization_ImportAchivementsFromPOFiles.py -d 1193760_loc_all.vdf <po_files>
```

Then just upload it for the achivements localization

### Chinese
```sh
# Export to excel
po2xls.exe -o WarriorbAchievements_zh-Hans+zh-Hant.xlsx WarriorbAchievements_schinese.po WarriorbAchievements_tchinese.po

# Import from excel
xls2po.exe zh-Hans WarriorbAchievements_zh-Hans+zh-Hant.xlsx WarriorbAchievements_schinese.po
xls2po.exe zh-Hant WarriorbAchievements_zh-Hans+zh-Hant.xlsx WarriorbAchievements_tchinese.po

# Run the import to po files as the scripts above
```
