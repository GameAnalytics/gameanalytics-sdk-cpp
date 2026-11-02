# Steam storefront sample

A minimal console "game" that runs the GameAnalytics SDK with the Steam storefront (`GASteamFront`). It initialises Steam, turns on the storefront with one line, sends three game events once, unlocks an achievement and creates a lobby so the storefront has Steam events to report, then pumps Steam callbacks for about five seconds and quits.

## Requirements

- The [Steamworks SDK](https://partner.steamgames.com/downloads/steamworks_sdk.zip) (needs a Steamworks partner login). It is not included in this repository: its licence does not allow redistributing it.
- The Steam client running and logged in.
- Your game key and secret in `Main.cpp` (`GAME_KEY`, `SECRET_KEY`).

## Build

Configure this folder directly; it pulls in the SDK from the parent folder. Pass the same options you use for the main SDK build (vcpkg, curl, ...).

```sh
cmake -S sample_steam -B build_steam -DSTEAMWORKS_SDK_DIR=/path/to/steamworks_sdk/sdk
cmake --build build_steam --target GASteamSample
```

The build copies the Steam runtime library and a `steam_appid.txt` next to the executable. The App ID defaults to `480` (Spacewar, Valve's public test app); change it with `-DSTEAM_APP_ID=<your app id>`.

## What to look for in the log

- `Storefront 'steam' configured`: the SteamID was applied as the user id.
- `store:achievement:ACH_WIN_ONE_GAME`: sent shortly after start, once Steam confirms the achievement unlock.
- `store:social:lobby_joined`: sent shortly after start, when the lobby created by the sample is entered.
- One progression event (`forest`), one `gold` resource event and one `sample:jump` design event: the game's own events.

On a Steam Deck the device is reported as "Steam Deck".
