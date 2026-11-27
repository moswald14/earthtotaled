# Contributing to EARTHtotaled

Players can create and share loot packs using the in-game Mod Workshop. The author kit includes a working example, a strict JSON schema and instructions for ChatGPT or another AI. [Mod Workshop guide](work/earthtotaled-unreal/Content/ModWorkshop/README.md)

To contribute to the main game, fork this repository, create a branch from the current `main`, and put your change in that branch. The Unreal project is in `work/earthtotaled-unreal`. Its README describes the engine version and build process. [Unreal project](work/earthtotaled-unreal/README.md)

For a new town interaction, start with `ETReclamation.cpp` for the world object, `ETReclamationActions.cpp` for what the player can do, and `ETReclamationHUD.cpp` for the on-screen choices. These files are in `work/earthtotaled-unreal/Source/EARTHtotaled`. For a custom object model, `Tools/make-reclamation.py` and `Tools/import-reclamation.py` show how the existing walker and cistern go from original geometry to a cooked Unreal asset. `Tools/verify-reclamation.ps1` exercises the packaged build on a disposable survivor.

1. For a loot pack, install it and test with a spare survivor slot. Test a drop, collection, redemption, and a save/reload. For code or art, build the Windows game and exercise the feature from the normal menus.
2. Commit your source files and the original assets or generation instructions needed to rebuild them. Include credit and redistribution terms for third-party material. Keep personal saves, account details, generated engine files and packaged executables out of the change.
3. Push your branch to your fork, open a pull request against **moswald14/earthtotaled → main**, and describe what the player can do, how you tested it and any known gaps. Screenshots help reviewers assess art. The maintainer reviews changes before merging. [Open a pull request](https://github.com/moswald14/earthtotaled/compare)
