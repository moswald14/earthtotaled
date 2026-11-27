# EARTHtotaled Mod Workshop

Open **Mod workshop** on the title screen or in the pause menu. Try **Preview starter pack** and **Install this preview**. The Siltworks pack adds a stamped seal to newly searched caches and can drop a surveyor optic from a Supertech enemy. Loot some, then choose **Pause → Mod workshop → Redeem collected finds** to receive the listed supplies. Use **Previous pack** and **Next pack** to select another pack.

## Make a mod with your own ChatGPT

- **In a browser** choose **Open ChatGPT in browser**. Sign in to your own ChatGPT account and paste the copied instructions. Describe what players should find, where it should drop and what it is worth. Ask for plain JSON. Copy the full object, return to the game and choose **Preview clipboard**. Check every find and choose **Install this preview**.
- **With Codex** install Node.js and the official Codex CLI, then choose **Connect my ChatGPT**. The author tool uses the CLI's own sign-in screen and your account's usage. Describe the pack when prompted. The tool copies the proposed JSON for the same in-game preview. Only your idea and the loot format are needed. [Codex installation](https://developers.openai.com/codex/cli/) · [ChatGPT sign-in](https://developers.openai.com/codex/auth/)
- **With another AI** open **Author kit**, give it `AI-BRIEF.txt` and `mod.schema.json`, and describe the pack. The same clipboard preview works. For example, ask for an old toll ticket found on zombies that redeems for eight candybar wrappers.

## Write or share a pack

Copy `starter.earthmod.json`, give it a new filename ending in `.earthmod.json`, and change the pack ID. **Open Mods folder** shows the exact directory on your PC. Drop a pack there and choose **Reload mods**. To update an installed pack, replace its file and reload. Keep the pack ID and item IDs to preserve collected items. Send friends the JSON file. Each player chooses which packs to enable. Installed mods and survivor saves live outside the game installation. To move a modded survivor to another PC, export the survivor and copy its `.earthmod.json` packs too.

| Field | Accepted value |
| --- | --- |
| schema | The number 1 |
| id | A lowercase letter followed by up to 31 lowercase letters, digits or hyphens |
| name | A display name of up to 48 printable characters |
| finds | 1–8 entries with unique item IDs |
| source | `cache`, `zombie`, `bug`, `supertech` or `wildlife` |
| chance | Whole-number drop percentage, 1–100 |
| use | `wrappers`, `wood`, `stone`, `scrap`, `water` or `medkit` |
| yield | Units of the reward per redeemed find, 1–25 |

Each find needs `id`, `name`, `source`, `use`, `chance` and `yield`. Supertech machines carry dry salvage. Use caches, zombies or wildlife for water rewards. A pack can be up to 64 KB. The workshop reads up to 64 files. It reports malformed packs and duplicate IDs on screen.

Enemy finds spill into the world. Approach one, crouch with **Ctrl**, and hold to sweep nearby loose objects. Cache finds enter your pack when you search a fresh container. Disabling a pack stops future drops. Items already collected keep their IDs. Reinstall a removed pack with the same IDs to redeem them again.

## Test or contribute a bigger change

Use **Survivor saves** to copy your survivor to a spare slot. During play press **Ctrl + Shift + F10** for the hidden test menu. Grant supplies, visit an infested town or its cistern, scatter test loot, or advance to dawn or night. Invincibility lasts for the current session. Granted items and town changes are ordinary save data. **F1** opens the text console.

For art, quests, weapons, new mechanics or improvements to the main game, fork the public repository, create a branch and send a pull request targeting `main`. Include the steps another player can use to test the change. [Contribution guide](https://github.com/moswald14/earthtotaled/blob/main/CONTRIBUTING.md)
