
# Common Problems, known issues

These are engine related issues, google spreadsheets is used for our own bugs.

## Instanced UObject member variables are not working with ActorComponents

This is the reason we have both `KinematicActor` and `KinematicActorComponent` at the moment.
Using `KinematicActorComponent` would be way better (but it does not work right now).

There is an [issue about this](https://issues.unrealengine.com/issue/UE-34445).
Update: marked as duplicated, most likely this is the one it duplicates:
https://issues.unrealengine.com/issue/UE-38871
(Still unresolved).

At the moment we can wait and :pray:.
If Epic does not fix it, we have to live with `KinematicActor`, or we have to fix it ourselves

## GetInputKeyAtDistanceAlongSpline() function returns with time(?) instead of inputkey

It is used in `FSoSplinePoint::SetDistanceFromInputKey`

#### Workaround:
Return value multiplied with `(SplineComponent->GetNumberOfSplinePoints() - 1.0f)`

#### Problems:
Does not work with closed splines (but we don't use them anyway, so it does not matter).

Does not work if the duration (total time) is not `1.0f` (it can not be asked and I am not sure what modifies it, was `1.0f` so far).

According to Epic support, [it is not a bug](https://answers.unrealengine.com/questions/495036/usplinecomponentgetinputkeyatdistancealongspline-i.html).
I (elath) made a pull request but it is ignored.


## Visual studio is slow as :shit:

If you use [Resharper](https://www.jetbrains.com/resharper/) you will have a bad time, even after [optimizing it](https://www.jetbrains.com/help/resharper/2016.2/Speeding_Up_ReSharper.html).
[Visual Assist](http://www.wholetomato.com/) seems to be much better, performance wise.

Fortunately much smarter people figured a magical way to increase performance [in the forums](https://forums.unrealengine.com/showthread.php?93546-For-those-who-suffer-from-Visual-Studio-IntelliSense-slowness).

## Renaming assets/files generates redirectors and breaks stuff

After you moved the assets in the right directory, right click on the original directory (where you moved them from) and
click on `Fix Up Redirectors in Folder` (this should remove  the redirector files), however it means that all levels referencing the moved assets will be opened and modified (so for this reason, if you are overwriting some assets that other people are working on, you should always ask before moving anything).

## 'CheckVA': none of the 11 overloads could convert all the argument types	SOrb LogMacros.h'

Very appropriate message, you probably did not get the value of the string (did not use `*`) when logging.

You did:
```
UE_LOG(Log, Info, TEXT("%s"), Component->GetName());
```

Instead of:
```
UE_LOG(Log, Info, TEXT("%s"), *Component->GetName());
```

## Project crashes when trying to open it from the 'Epic Games Launcher'/'Any other source'

This can have multiple causes/fixes:

- If you edited C++ code and you did not build it, this is most likely the cause, build then open the project from the launcher.

- If it still crashes try to remove all cache from the project.
This means removing the following directories `Binaries`, `Saved`, `Intermediate` and the following files `Warriorb.sln`, `Warriorb.VC.db` (windows only)

If you use the git command line, you could run the following git command to remove all ignored files:
```
# remove -n to NOT run the command in dry run mode
git clean -f -d -X -n
```

- If all else fails, try getting a clean version of the repository and try that and do some :pray:.
