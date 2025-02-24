# Changelog

## 4.1.0

### Added

- **Boot Time in Milliseconds**: Boot time now reported in milliseconds for precise performance metrics.

### Fixed

- **Design Event Values**: Improved the consistency of design event value handling, ensuring more accurate data delivery.
- **Local Cache Path**: Refined the local cache path configuration to automatically use the default writable path, streamlining setup by making `configureWritablePath` optional.
- **Verbose Logging**: more logs are not visible when verbose logging is enabled.

---------

## Older deprecated versions

**4.0.0**

* updated to C++17
* refactored the code base
* rewritten GA thread logic
* rewritten build scripts
* changed user id logic to always use randomized id (except if custom id is set)
* replaced usage of unsafe cstrings and functions with safer alternatives
* replaced RapidJson with ModernJson
* removed Tizien support
* added SDK init event
* added health event
* added external user id
* safer handling of device properties retrieval
* added exception handling
* improved stability
* various bug fixes

**3.2.6**
* changed event uuid field name

**3.2.5**
* added event uuid to events sent

**3.2.4**
* fixed progression tries bug
* fixed max path length for configure writable path method

**3.2.3**
* added error events to be sent for invalid custom event fields used
* added optional mergeFields argument to event methods to merge with global custom fields instead of overwrite them

**3.2.2**
* fixed missing custom event fields for when trying to fix missing session end events

**3.2.1**
* added option to use custom log handler

**3.2.0**
* added global custom event fields function to allow to add custom fields to events sent automatically by the SDK

**3.1.3**
* added functionality to force a new user in a/b testing without having to uninstall app first, simply use custom user id function to set a new user id which hasn't been used yet

**3.1.2**
* fix to custom event fields

**3.1.1**
* small fixes to shared libraries

**3.1.0**
* added custom event fields feature

**3.0.8**
* updated client ts validator

**3.0.7**
* removed memory info from automatic crash reporting

**3.0.6**
* added disable device info method

**3.0.5**
* added godot to version validator

**3.0.4**
* removed gender, birth year and facebook methods
* updated curl and openssl libraries

**3.0.3**
* A/B testing fixes

**3.0.2**
* remote configs fixes

**3.0.1**
* various bug fixes

**3.0.0**
* command center is now called remote configs
* A/B testing support added

**2.1.9**
* fixes to command center

**2.1.8**
* error reporting improved

**2.1.7**
* bug fix to too large log files

**2.1.6**
* added check if log files and database can't be created

**2.1.5**
* refactored code for singleton classes

**2.1.4**
* fixed hanging background thread after closing application
* progression event bug fix

**2.1.3**
* removed std::string from the SDK

**2.1.2**
* thread fixes
* dll freeze bug fix

**2.1.1**
* thread fixes

**2.1.0**
* added automatic error reporting
* added enable/disable event submission

**2.0.4**
* fixed business event validation

**2.0.3**
* fixed shutdown issues

**2.0.2**
* fixed thread hanging on shutdown

**2.0.1**
* added command center functions to extern interface

**2.0.0**
* added command center functionality

**1.4.6**
* fixes for occasional crashes when shutting down
* fixes crashes on UWP

**1.4.5**
* fixes for Linux interface

**1.4.4**
* bug fix to events thread stopping and not starting again

**1.4.3**
* added custom dimensions to design and error events
* added GAState zero initialisation of fields
* fixed linux build script

**1.4.2**
* fixed session length bug
* fixed to not allow adding events when not in a session

**1.4.1**
* renamed onStop function to onSuspend
* added a onQuit function

**1.4.0**
* updated to new logging library (windows, osx, linux)
* added precompiled GameAnalytics libraries

**1.3.8**
* small correction to use int instead of double for session num

**1.3.7**
* https fix in curl library for mac

**1.3.6**
* bug fix for end session when using manual session handling

**1.3.5**
* session length precision improvement

**1.3.4**
* custom user id bug fix

**1.3.3**
* bug fix to dupplicate logs send to console (windows, mac, linux)

**1.3.2**
* added OS version, device model and device manufacturer to event (windows, mac)

**1.3.1**
* added support for Linux
* logging initialisation updated for windows and mac (windows, mac)

**1.3.0**
* added support for Tizen

**1.2.4**
* removed unused files

**1.2.3**
* fixed build script for UWP
* changed persistent path for UWP

**1.2.2**
* fixed issue with onstop method

**1.2.1**
* possible to set custom dimensions and demographics before initialise

**1.2.0**
* added UWP support

**1.1.1**
* fix to empty user id

**1.1.0**
* switched to use curl as network library

**1.0.1**
* fix for empty user id in events

**1.0.0**
* Initial version with Windows and Mac OS X support