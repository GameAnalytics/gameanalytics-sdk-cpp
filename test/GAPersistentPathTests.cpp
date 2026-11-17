//
// GA-SDK-CPP
// Tests for how the SDK resolves and creates its persistent (writable) path:
// the platform default under the user's home / local app data folder, and
// custom paths supplied through GameAnalytics::configureWritablePath().
//
// Non-ASCII path components are spelled as explicit UTF-8 bytes so the file
// stays ASCII and the literal is identical under every compiler, language
// standard and source charset.
//

#include <gtest/gtest.h>

#include "GameAnalytics/GameAnalytics.h"

#include <GADevice.h>
#include <GAState.h>
#include <GAStore.h>
#include <Platform/GAPlatform.h>

#include "helpers/GAStateTestAccessor.h"
#include "helpers/GATestHelpers.h"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

#if IS_WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

using namespace gameanalytics;

namespace fs = std::filesystem;

namespace
{
    constexpr const char* kGameKey      = "bd624ee6f8e6efb32a054f8d7ba11618";
    constexpr const char* kGameSecret   = "7f5c3f682cbd217841efba92e92ffb1b3b6612bc";
    constexpr const char* kDatabaseName = "ga.sqlite3";
    constexpr const char* kFolderName   = "GameAnalytics";

    // "héllo-测试-Дмитрий": Latin-1 supplement, CJK and Cyrillic in one component
    constexpr const char* kUnicodeName = "h" "\xc3\xa9" "llo-" "\xe6\xb5\x8b" "\xe8\xaf\x95" "-" "\xd0\x94" "\xd0\xbc" "\xd0\xb8" "\xd1\x82" "\xd1\x80" "\xd0\xb8" "\xd0\xb9";

#if IS_WIN32
    constexpr const wchar_t* kHomeVariable = L"LOCALAPPDATA";
#else
    constexpr const char* kHomeVariable = "HOME";
#endif

    // the SDK carries paths as UTF-8 std::string; these convert to and from
    // std::filesystem::path without going through the ANSI code page on Windows
    fs::path fromUtf8(std::string const& utf8)
    {
#if defined(__cpp_lib_char8_t)
        return fs::path(std::u8string(utf8.begin(), utf8.end()));
#else
        return fs::u8path(utf8);
#endif
    }

    std::string toUtf8(fs::path const& path)
    {
#if defined(__cpp_lib_char8_t)
        const std::u8string utf8 = path.u8string();
        return std::string(utf8.begin(), utf8.end());
#else
        return path.u8string();
#endif
    }

    // true when `utf8Path` names an existing directory that is the same
    // directory as `expected` (tolerates symlinked temp dirs and 8.3 names)
    bool sameDirectory(std::string const& utf8Path, fs::path const& expected)
    {
        std::error_code ec;
        const fs::path actual = fromUtf8(utf8Path);
        return fs::is_directory(actual, ec) && fs::equivalent(actual, expected, ec);
    }

    // overrides the environment variable the platform derives its persistent
    // path from, restoring the previous value on destruction. An empty value
    // unsets the variable.
    class ScopedHomeVariable
    {
    public:
        explicit ScopedHomeVariable(fs::path const& value)
        {
            _hadPrevious = read(_previous);
            write(value);
        }

        ~ScopedHomeVariable()
        {
            write(_hadPrevious ? _previous : fs::path());
        }

    private:
#if IS_WIN32
        static bool read(fs::path& out)
        {
            const DWORD size = GetEnvironmentVariableW(kHomeVariable, nullptr, 0);
            if (size == 0)
            {
                return false;
            }
            std::wstring value(size, L'\0');
            value.resize(GetEnvironmentVariableW(kHomeVariable, value.data(), size));
            out = value;
            return true;
        }

        static void write(fs::path const& value)
        {
            // the SDK reads the variable through the CRT, so update both views
            SetEnvironmentVariableW(kHomeVariable, value.empty() ? nullptr : value.c_str());
            _wputenv_s(kHomeVariable, value.empty() ? L"" : value.c_str());
        }
#else
        static bool read(fs::path& out)
        {
            const char* value = std::getenv(kHomeVariable);
            if (!value)
            {
                return false;
            }
            out = value;
            return true;
        }

        static void write(fs::path const& value)
        {
            if (value.empty())
            {
                unsetenv(kHomeVariable);
            }
            else
            {
                setenv(kHomeVariable, value.c_str(), 1);
            }
        }
#endif

        fs::path _previous;
        bool     _hadPrevious = false;
    };

    class GAPersistentPathTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            // configureWritablePath() is rejected once the SDK is initialized
            state::GAStateTestAccessor::forceUninitialized();

            // the store places the database in a per-game-key subfolder and reads
            // the key from GAState, so pin it rather than depend on test order
            state::GAState::setKeys(kGameKey, kGameSecret);

            // remembered before any test touches HOME, so TearDown can put the
            // shared test database back where the other suites expect it
            _originalWritablePath = device::GADevice::getWritablePath();

            std::error_code ec;
            fs::path base = fs::temp_directory_path(ec);
            if (ec)
            {
                base = fs::current_path();
            }
            _root = base / ("ga-sdk-path-test-" + GATestHelpers::getRandomString(12));
            ASSERT_TRUE(fs::create_directories(_root)) << toUtf8(_root);
        }

        void TearDown() override
        {
            device::GADevice::setWritablePath(_originalWritablePath);
            EXPECT_TRUE(store::GAStore::ensureDatabase(false, kGameKey));

            std::error_code ec;
            fs::remove_all(_root, ec);
        }

        static std::string platformPath()
        {
            return device::GADevice::getPlatform()->getPersistentPath();
        }

        static fs::path databaseFile(fs::path const& writablePath)
        {
            return writablePath / kGameKey / kDatabaseName;
        }

        fs::path    _root;
        std::string _originalWritablePath;
    };
}

// ---- platform default path ----

TEST_F(GAPersistentPathTest, PlatformPathIsCreatedUnderAsciiHome)
{
    const fs::path home = _root / "ascii-home";
    ASSERT_TRUE(fs::create_directories(home));
    ScopedHomeVariable scopedHome(home);

    const std::string path = platformPath();

    ASSERT_FALSE(path.empty());
    EXPECT_EQ(kFolderName, fromUtf8(path).filename().string());
    EXPECT_TRUE(sameDirectory(path, home / kFolderName)) << path;
}

TEST_F(GAPersistentPathTest, PlatformPathPreservesUnicodeHome)
{
    const fs::path home = _root / fromUtf8(kUnicodeName);
    ASSERT_TRUE(fs::create_directories(home));
    ScopedHomeVariable scopedHome(home);

    const std::string path = platformPath();

    ASSERT_FALSE(path.empty());
    // the string is handed to sqlite and the logger, so it has to be UTF-8
    EXPECT_NE(std::string::npos, path.find(kUnicodeName)) << path;
    EXPECT_TRUE(sameDirectory(path, home / kFolderName)) << path;
}

TEST_F(GAPersistentPathTest, PlatformPathCreatesMissingDirectories)
{
    const fs::path home = _root / "missing" / "nested" / "home";
    ASSERT_FALSE(fs::exists(home));
    ScopedHomeVariable scopedHome(home);

    const std::string path = platformPath();

    ASSERT_FALSE(path.empty());
    EXPECT_TRUE(sameDirectory(path, home / kFolderName)) << path;
}

TEST_F(GAPersistentPathTest, PlatformPathFallsBackToRelativeDirectoryWithoutHome)
{
#if IS_LINUX
    // cannot be asserted in-process: the current Linux implementation builds a
    // std::string from the null getenv() result, which would take the whole
    // test binary down rather than fail this one test
    GTEST_SKIP() << "Linux getPersistentPath() dereferences a null HOME";
#endif

    const fs::path fallback      = fs::current_path() / kFolderName;
    const bool     existedBefore = fs::exists(fallback);
    ScopedHomeVariable scopedHome{fs::path()};

    const std::string path = platformPath();

    EXPECT_EQ(kFolderName, path);
    EXPECT_TRUE(sameDirectory(path, fallback)) << path;

    if (!existedBefore)
    {
        std::error_code ec;
        fs::remove_all(fallback, ec);
    }
}

#if !IS_WIN32
TEST_F(GAPersistentPathTest, PlatformPathIsEmptyWhenHomeIsUnwritable)
{
    if (geteuid() == 0)
    {
        GTEST_SKIP() << "root is not subject to directory permissions";
    }

    const fs::path home = _root / "locked";
    ASSERT_TRUE(fs::create_directories(home));
    fs::permissions(home, fs::perms::owner_read | fs::perms::owner_exec);
    ScopedHomeVariable scopedHome(home);

    std::string path;
    ASSERT_NO_THROW(path = platformPath());

    EXPECT_TRUE(path.empty()) << path;
    EXPECT_FALSE(fs::exists(home / kFolderName));

    fs::permissions(home, fs::perms::owner_all);
}
#endif

// ---- device-level resolution ----

TEST_F(GAPersistentPathTest, DeviceResolvesPlatformPathWhenNoneConfigured)
{
    const fs::path home = _root / "device-home";
    ASSERT_TRUE(fs::create_directories(home));
    ScopedHomeVariable scopedHome(home);

    device::GADevice::setWritablePath("");
    EXPECT_FALSE(device::GADevice::getWritablePathStatus());

    const std::string path = device::GADevice::getWritablePath();

    EXPECT_TRUE(device::GADevice::getWritablePathStatus());
    EXPECT_TRUE(sameDirectory(path, home / kFolderName)) << path;
}

// ---- custom path through the public API ----

TEST_F(GAPersistentPathTest, CustomPathIsUsedForDatabase)
{
    const fs::path custom = _root / "custom";
    ASSERT_TRUE(fs::create_directories(custom));

    GameAnalytics::configureWritablePath(toUtf8(custom));
    ASSERT_EQ(toUtf8(custom), device::GADevice::getWritablePath());
    EXPECT_TRUE(device::GADevice::getWritablePathStatus());

    ASSERT_TRUE(store::GAStore::ensureDatabase(false, kGameKey));
    EXPECT_TRUE(fs::is_regular_file(databaseFile(custom)));
}

TEST_F(GAPersistentPathTest, CustomUnicodePathIsUsedForDatabase)
{
    const fs::path custom = _root / fromUtf8(kUnicodeName) / "custom";
    ASSERT_TRUE(fs::create_directories(custom));

    GameAnalytics::configureWritablePath(toUtf8(custom));
    ASSERT_EQ(toUtf8(custom), device::GADevice::getWritablePath());

    ASSERT_TRUE(store::GAStore::ensureDatabase(false, kGameKey));
    ASSERT_TRUE(fs::is_regular_file(databaseFile(custom)));

    // the database is not just created but usable at that location
    store::GAStore::setState("persistent_path_test", "ok");
    json rows;
    store::GAStore::executeQuerySync("SELECT value FROM ga_state WHERE key='persistent_path_test';", rows);
    ASSERT_TRUE(rows.is_array());
    ASSERT_EQ(1u, rows.size());
    EXPECT_EQ("ok", rows[0]["value"].get<std::string>());
}

TEST_F(GAPersistentPathTest, CustomPathIsCreatedWhenMissing)
{
    const fs::path custom = _root / "not" / "yet" / "there";
    ASSERT_FALSE(fs::exists(custom));

    GameAnalytics::configureWritablePath(toUtf8(custom));

    bool opened = false;
    ASSERT_NO_THROW(opened = store::GAStore::ensureDatabase(false, kGameKey));
    EXPECT_TRUE(opened);
    EXPECT_TRUE(fs::is_regular_file(databaseFile(custom)));
}

TEST_F(GAPersistentPathTest, CustomPathToleratesTrailingSeparator)
{
    const fs::path custom = _root / "trailing";
    ASSERT_TRUE(fs::create_directories(custom));

    GameAnalytics::configureWritablePath(toUtf8(custom) + "/");
    ASSERT_TRUE(store::GAStore::ensureDatabase(false, kGameKey));

    EXPECT_TRUE(fs::is_regular_file(databaseFile(custom)));
}

#if !IS_WIN32
TEST_F(GAPersistentPathTest, UnwritableCustomPathFailsWithoutThrowing)
{
    if (geteuid() == 0)
    {
        GTEST_SKIP() << "root is not subject to directory permissions";
    }

    const fs::path locked = _root / "locked";
    ASSERT_TRUE(fs::create_directories(locked));
    fs::permissions(locked, fs::perms::owner_read | fs::perms::owner_exec);
    const fs::path custom = locked / "custom";

    GameAnalytics::configureWritablePath(toUtf8(custom));

    bool opened = false;
    ASSERT_NO_THROW(opened = store::GAStore::ensureDatabase(false, kGameKey));
    EXPECT_FALSE(opened);
    EXPECT_FALSE(fs::exists(custom));

    fs::permissions(locked, fs::perms::owner_all);
}
#endif
