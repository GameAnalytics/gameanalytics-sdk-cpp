GA-SDK-CPP
==========

GameAnalytics C++ SDK

Documentation can be found [here](https://gameanalytics.com/docs/cpp-sdk).

Supported platforms:

* Mac OSX
* Windows 32-bit and 64-bit
* Linux

Dependencies
------------

* python 3.6  or higher
* cmake  3.20 or higher
* **Mac:**      XCode
* **Windows:**  Visual Studio 2017 or later
* **Linux:**    clang or gcc

Changelog
------------

See the full [CHANGELOG](./CHANGELOG) for detailed version history.

How to build
------------

Run `setup.py` with the required arguments for your platform:

```sh
python setup.py --platform {linux_x64,linux_x86,osx,win32,win64,uwp} [--cfg {Release,Debug}] [--compiler {gcc,clang}] [--shared] [--build] [--test] [--coverage]
```

| Argument | Values | Description |
|---|---|---|
| `--platform` | `linux_x64`, `linux_x86`, `osx`, `win32`, `win64`, `uwp` | Target platform (**required**) |
| `--cfg` | `Release`, `Debug` | Build configuration (default: `Debug`) |
| `--compiler` | `gcc`, `clang` | Compiler selection — **Linux only** (default: `clang`) |
| `--shared` | — | Build a shared library (`.dll`/`.so`/`.dylib`) instead of a static library |
| `--build` | — | Execute the build step |
| `--test` | — | Execute the test step (not available with `--shared`) |
| `--coverage` | — | Generate code coverage report (not available with `--shared`) |

#### Examples

Static library (default):
```sh
python setup.py --platform osx --cfg Release --build --test
```

Shared library (e.g. for Unity native plugin):
```sh
python setup.py --platform win64 --cfg Release --shared --build
python setup.py --platform osx --cfg Release --shared --build
python setup.py --platform linux_x64 --cfg Release --shared --build
python setup.py --platform linux_x64 --compiler gcc --cfg Release --shared --build
```

The generated project and build artifacts can be found inside the `build` folder. Packaged output (library + headers) is placed in `build/package/`.

Lib Dependencies
----------------

* **crossguid** (*as source*) - Cross platform library to generate a Guid.
* **cryptoC++** (*as source*) - collection of functions and classes for cryptography related tasks.
* **curl** (*as binary*) - library used to make HTTP requests.
* **nlohmann json** (*as source*) - lightweight C++ library for manipulating JSON values including serialization and deserialization.
* **openssl** (*as binary*) - used by **curl** to make HTTPS requests.
* **SQLite** (*as source*) - SQLite is a software library that implements a self-contained, serverless, zero-configuration, transactional SQL database engine.

*as source* means the dependency will be compiled with the project itself, *as binary* means the dependency is prebuild and will be linked to the project

Usage of the SDK
----------------

### Static library (C++ API)

Include the GameAnalytics header file wherever you are using the SDK:

``` c++
 #include "GameAnalytics/GameAnalytics.h"
```

### Shared library / Unity native plugin (C API)

When building with `--shared` / `-DGA_SHARED_LIB=ON`, the SDK exposes a C API via `GameAnalyticsExtern.h`. This is the intended integration path for Unity (P/Invoke) and other managed runtimes.

Include the header in your native wrapper:
```c
#include "GameAnalytics/GameAnalyticsExtern.h"
```

From Unity C#, import via `[DllImport]`:
```csharp
[DllImport("GameAnalytics")]
private static extern void gameAnalytics_initialize(string gameKey, string gameSecret);
```

> **Note:** Always call `gameAnalytics_freeString(ptr)` on any `const char*` returned by the C API to avoid memory leaks.

### Custom log handler
If you want to use your own custom log handler here is how it is done:

**C++ API:**
``` c++
void logHandler(std::string const& message, gameanalytics::EGALoggerMessageType type)
{
    // add your logging in here
}

gameanalytics::GameAnalytics::configureCustomLogHandler(logHandler);
```

**C API (shared lib):**
```c
void myLogHandler(const char* message, GALoggerMessageType type)
{
    // add your logging in here
}

gameAnalytics_configureCustomLogHandler(myLogHandler);
```

### Configuration

Example:

``` c++
 gameanalytics::GameAnalytics::setEnabledInfoLog(true);
 gameanalytics::GameAnalytics::setEnabledVerboseLog(true);

 gameanalytics::GameAnalytics::configureBuild("0.10");

 {
     std::vector<std::string> list;
     list.push_back("gems");
     list.push_back("gold");
     gameanalytics::GameAnalytics::configureAvailableResourceCurrencies(list);
 }
 {
     std::vector<std::string> list;
     list.push_back("boost");
     list.push_back("lives");
     gameanalytics::GameAnalytics::configureAvailableResourceItemTypes(list);
 }
 {
     std::vector<std::string> list;
     list.push_back("ninja");
     list.push_back("samurai");
     gameanalytics::GameAnalytics::configureAvailableCustomDimensions01(list);
 }
 {
     std::vector<std::string> list;
     list.push_back("whale");
     list.push_back("dolphin");
     gameanalytics::GameAnalytics::configureAvailableCustomDimensions02(list);
 }
 {
     std::vector<std::string> list;
     list.push_back("horde");
     list.push_back("alliance");
     gameanalytics::GameAnalytics::configureAvailableCustomDimensions03(list);
 }
```

### Initialization

Example:

``` c++
 gameanalytics::GameAnalytics::initialize("<your game key>", "<your secret key>");
```

### Send events

Example:

``` c++
 gameanalytics::GameAnalytics::addDesignEvent("testEvent");
 gameanalytics::GameAnalytics::addBusinessEvent("USD", 100, "boost", "super_boost", "shop");
 gameanalytics::GameAnalytics::addResourceEvent(gameanalytics::Source, "gems", 10, "lives", "extra_life");
 gameanalytics::GameAnalytics::addProgressionEvent(gameanalytics::Start, "progression01", "progression02");
```
