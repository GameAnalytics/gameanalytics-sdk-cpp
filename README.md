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

**4.0.0**

* updated to C++17
* public functions will now expect `std::string` instead of `const char*`
* configureAvailableCustomDimensions, configureAvailableResourceCurrencies and configureAvailableItemTypes will now require a `std::vector<std::string>` as argument
* removed duplicated public functions, replaced with default arguments
* added argument for IRemoteConfigsListener callback
* rewritten build scripts, now requires python3
* changed user id logic to always use randomized id (except if custom id is set)
* replaced usage of unsafe cstrings and functions with safer alternatives
* refactored the internal codebase
* rewritten threading logic
* replaced RapidJson
* removed Tizien support
* added SDK init event
* added health event
* added external user id
* safer handling of device properties retrieval
* added exception handling
* improved stability
* various bug fixes
* added sample app

How to build
------------

Run `build.py` with the required argument for your platform:

```sh
python build.py --platform={your platform}
```

The following arguments are supported:

* `linux_x64`
* `linux_x86`
* `osx`
* `win32`
* `win64`

The generated project can be found inside the `build` folder.

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

Remember to include the GameAnalytics header file wherever you are using the SDK:

``` c++
 #include "GameAnalytics/GameAnalytics.h"
```

### Custom log handler
If you want to use your own custom log handler here is how it is done:
``` c++
void logHandler(const char *message, gameanalytics::EGALoggerMessageType type)
{
    // add your logging in here
}

gameanalytics::GameAnalytics::configureCustomLogHandler(logHandler);
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
 gameanalytics::GameAnalytics::initialize("<your game key>", "<your secret key");
```

### Send events

Example:

``` c++
 gameanalytics::GameAnalytics::addDesignEvent("testEvent");
 gameanalytics::GameAnalytics::addBusinessEvent("USD", 100, "boost", "super_boost", "shop");
 gameanalytics::GameAnalytics::addResourceEvent(gameanalytics::Source, "gems", 10, "lives", "extra_life");
 gameanalytics::GameAnalytics::addProgressionEvent(gameanalytics::Start, "progression01", "progression02");
```
