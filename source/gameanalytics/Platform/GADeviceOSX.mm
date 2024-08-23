#include "GACommon.h"

#if IS_MAC

#import  "GADeviceOSX.h"
#import  <Foundation/Foundation.h>
#import  <CoreServices/CoreServices.h>
#include <CoreFoundation/CoreFoundation.h>
#import  <SystemConfiguration/SystemConfiguration.h>

#import <arpa/inet.h>
#import <ifaddrs.h>
#import <netdb.h>
#import <sys/socket.h>

#import <SystemConfiguration/SystemConfiguration.h>
#import <netinet/in.h>

typedef struct {
    NSInteger majorVersion;
    NSInteger minorVersion;
    NSInteger patchVersion;
} MyOperatingSystemVersion;

const char* getOSXVersion()
{
    if (floor(kCFCoreFoundationVersionNumber) > kCFCoreFoundationVersionNumber10_9)
    {
        NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
        NSString* v = [NSString stringWithFormat:@"%ld.%ld.%ld", version.majorVersion, version.minorVersion, (long)version.patchVersion];
        return [v UTF8String];
    }
    else
    {
        SInt32 majorVersion,minorVersion,bugFixVersion;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        Gestalt(gestaltSystemVersionMajor, &majorVersion);
        Gestalt(gestaltSystemVersionMinor, &minorVersion);
        Gestalt(gestaltSystemVersionBugFix, &bugFixVersion);
#pragma GCC diagnostic pop

        NSString* v = [NSString stringWithFormat:@"%d.%d.%d", majorVersion, minorVersion, bugFixVersion];
        return [v UTF8String];
    }
}

SCNetworkReachabilityRef createReachabilityRef()
{
    // Create zero address
    struct sockaddr_in zeroAddress;
    bzero(&zeroAddress, sizeof(zeroAddress));
    zeroAddress.sin_len = sizeof(zeroAddress);
    zeroAddress.sin_family = AF_INET;

    // Create reachability reference
    SCNetworkReachabilityRef reachabilityRef = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, (const struct sockaddr*)&zeroAddress);
    
    return reachabilityRef;
}

const char* getConnectionType()
{
    // todo: keep this in a class
    static SCNetworkReachabilityRef reachabilityRef = createReachabilityRef();
    
    SCNetworkReachabilityFlags flags;
    SCNetworkReachabilityGetFlags(reachabilityRef, &flags);

    if ((flags & kSCNetworkReachabilityFlagsReachable) == 0) {
        // The target host is not reachable.
        return "offline";
    }
    
    if ((flags & kSCNetworkReachabilityFlagsConnectionRequired) == 0) 
    {
        // If the target host is reachable and no connection is required then we'll assume (for now) that you're on Wi-Fi...
        return "wifi";
    }
    
    if ((((flags & kSCNetworkReachabilityFlagsConnectionOnDemand ) != 0) ||
         (flags & kSCNetworkReachabilityFlagsConnectionOnTraffic) != 0)) {
        // ... and the connection is on-demand (or on-traffic) if the calling application is using the CFSocketStream or higher APIs...
        if ((flags & kSCNetworkReachabilityFlagsInterventionRequired) == 0) {
            // ... and no [user] intervention is needed...
            return "wifi";
        }
    }

    return "";
}

int getNumCpuCores()
{
    NSProcessInfo *info = [NSProcessInfo processInfo];
    return (int)info.processorCount;
}

int64_t getTotalDeviceMemory()
{
    NSProcessInfo *info = [NSProcessInfo processInfo];
    return (uint64_t)info.physicalMemory;
}

#endif // IS_MAC
