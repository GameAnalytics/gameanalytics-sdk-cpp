#include "GAUwp.h"
#include "GAUtilities.h"

#if IS_UWP

std::string gameanalytics::GAPlatformUWP::getOSVersion()
{
    auto deviceFamilyVersion = Windows::System::Profile::AnalyticsInfo::VersionInfo->DeviceFamilyVersion;
    std::istringstream stringStream(utilities::GAUtilities::ws2s(deviceFamilyVersion->Data()));
    unsigned long long version;

    stringStream >> version;
    unsigned long long major = (version & 0xFFFF000000000000L) >> 48;
    unsigned long long minor = (version & 0x0000FFFF00000000L) >> 32;
    unsigned long long build = (version & 0x00000000FFFF0000L) >> 16;
    std::ostringstream stream;
    stream << getBuildPlatform() << " " << major << "." << minor << "." << build;
    
    return stream.str();
}

std::string gameanalytics::GAPlatformUWP::getDeviceManufacturer()
{
    auto info = ref new Windows::Security::ExchangeActiveSyncProvisioning::EasClientDeviceInformation();
    return utilities::GAUtilities::ws2s(info->SystemManufacturer->Data());
}

std::string gameanalytics::GAPlatformUWP::getBuildPlatform()
{
    auto deviceFamily = Windows::System::Profile::AnalyticsInfo::VersionInfo->DeviceFamily;

    if (deviceFamily == "Windows.Mobile")
    {
        return "uwp_mobile";
    }
    else if (deviceFamily == "Windows.Desktop")
    {
        return "uwp_desktop";
    }
    else if (deviceFamily == "Windows.Universal")
    {
        return "uwp_iot";
    }
    else if (deviceFamily == "Windows.Xbox")
    {
        return "uwp_console";
    }
    else if (deviceFamily == "Windows.Team")
    {
       return "uwp_surfacehub";
    }
    else
    {
        return utilities::GAUtilities::ws2s(deviceFamily->Data());
    }
}

std::string gameanalytics::GAPlatformUWP::getPersistentPath()
{
    return utilities::GAUtilities::ws2s(Windows::Storage::ApplicationData::Current->LocalFolder->Path->Data()) + "\\GameAnalytics";
}

std::string gameanalytics::GAPlatformUWP::getDeviceModel()
{
    auto info = ref new Windows::Security::ExchangeActiveSyncProvisioning::EasClientDeviceInformation();
    return utilities::GAUtilities::ws2s(info->SystemProductName->Data());
}

std::string gameanalytics::GAPlatformUWP::getAdvertisingId()
{
    return "";
}

std::string gameanalytics::GAPlatformUWP::getDeviceId()
{
    std::string result = "";

    if (Windows::Foundation::Metadata::ApiInformation::IsTypePresent("Windows.System.Profile.HardwareIdentification"))
    {
        auto token = Windows::System::Profile::HardwareIdentification::GetPackageSpecificToken(nullptr);
        auto hardwareId = token->Id;
        auto hardwareIdString = Windows::Security::Cryptography::CryptographicBuffer::EncodeToHexString(hardwareId);
        result = utilities::GAUtilities::ws2s(hardwareIdString->Data());
    }

    return result;
}

#endif