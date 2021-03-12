# Conventions

## Commit messages

Every commit message that has `[C]` means that after updating to that commit, a recompile is required.

Eg: `[C] hit react pushed movement cleanup | kobold damage`

Every commit message that has `[G]` means that after updating to that commit the solution files should be regenerated (and recompiled, *obvious*).
(Right click on project file -> "Generate Visual Studio project files").

Eg: `[G] new editor module with blank animation graph node`

Each engine version update has '[4.xx]' tag. Source may or may not be compatilbe with older versions anymore

Eg: `[4.16][C] Engine update`
