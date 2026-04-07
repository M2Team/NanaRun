/*
 * PROJECT:    SynthRdp
 * FILE:       VirtualSmb.cpp
 * PURPOSE:    Implementation for Hyper-V Virtual SMB Guest Utility
 *
 * LICENSE:    The MIT License
 *
 * MAINTAINER: MouriNaruto (Kenji.Mouri@outlook.com)
 */

#include <Windows.h>
#include <winioctl.h>

#include <clocale>
#include <cstdio>
#include <cwchar>

// Reference: https://github.com/microsoft/hcsshim
//            /blob/ed5784127999cfd4c08c254626cc964cb20d7948
//            /internal/gcs-sidecar/vsmb.go

typedef struct _SMB2_INSTANCE_CONFIGURATION_17134
{
    UINT32 DormantDirectoryTimeout;
    UINT32 DormantFileTimeout;
    UINT32 DormantFileLimit;
    UINT32 FileInfoCacheLifetime;
    UINT32 FileNotFoundCacheLifetime;
    UINT32 DirectoryCacheLifetime;
    UINT32 FileInfoCacheEntriesMax;
    UINT32 FileNotFoundCacheEntriesMax;
    UINT32 DirectoryCacheEntriesMax;
    UINT32 DirectoryCacheSizeMax;
    UINT8 RequireSecuritySignature;
    UINT8 RequireEncryption;
    UINT8 Padding[2];
} SMB2_INSTANCE_CONFIGURATION_17134, *PSMB2_INSTANCE_CONFIGURATION_17134;

typedef struct _SMB2_INSTANCE_CONFIGURATION_20348
{
    UINT32 DormantDirectoryTimeout;
    UINT32 DormantFileTimeout;
    UINT32 DormantFileLimit;
    UINT32 FileInfoCacheLifetime;
    UINT32 FileNotFoundCacheLifetime;
    UINT32 DirectoryCacheLifetime;
    UINT32 FileInfoCacheEntriesMax;
    UINT32 FileNotFoundCacheEntriesMax;
    UINT32 DirectoryCacheEntriesMax;
    UINT32 DirectoryCacheSizeMax;
    UINT32 ReadAheadGranularity;
    UINT8 RequireSecuritySignature;
    UINT8 RequireEncryption;
    UINT8 Padding[2];
} SMB2_INSTANCE_CONFIGURATION_20348, *PSMB2_INSTANCE_CONFIGURATION_20348;

typedef struct _SMB2_INSTANCE_CONFIGURATION_25398
{
    UINT32 DormantDirectoryTimeout;
    UINT32 DormantFileTimeout;
    UINT32 DormantFileLimit;
    UINT32 FileInfoCacheLifetime;
    UINT32 FileNotFoundCacheLifetime;
    UINT32 DirectoryCacheLifetime;
    UINT32 FileInfoCacheEntriesMax;
    UINT32 FileNotFoundCacheEntriesMax;
    UINT32 DirectoryCacheEntriesMax;
    UINT32 DirectoryCacheSizeMax;
    UINT32 ReadAheadGranularity;
    UINT32 VolumeFeatureSupportCacheLifetime;
    UINT32 VolumeFeatureSupportCacheEntriesMax;
    UINT8 RequireSecuritySignature;
    UINT8 RequireEncryption;
    UINT8 Padding[2];
} SMB2_INSTANCE_CONFIGURATION_25398, *PSMB2_INSTANCE_CONFIGURATION_25398;

// Since 26100.xxxx
typedef struct _SMB2_INSTANCE_CONFIGURATION
{
    UINT32 DormantDirectoryTimeout;
    UINT32 DormantFileTimeout;
    UINT32 DormantFileLimit;
    UINT32 FileInfoCacheLifetime;
    UINT32 FileNotFoundCacheLifetime;
    UINT32 DirectoryCacheLifetime;
    UINT32 FileInfoCacheEntriesMax;
    UINT32 FileNotFoundCacheEntriesMax;
    UINT32 DirectoryCacheEntriesMax;
    UINT32 DirectoryCacheSizeMax;
    UINT32 ReadAheadGranularity;
    UINT32 VolumeFeatureSupportCacheLifetime;
    UINT32 VolumeFeatureSupportCacheEntriesMax;
    UINT32 FileAbeStatusCacheLifetime;
    UINT8 RequireSecuritySignature;
    UINT8 RequireEncryption;
    UINT8 Padding[2];
} SMB2_INSTANCE_CONFIGURATION, *PSMB2_INSTANCE_CONFIGURATION;

typedef struct _LMR_CONNECTION_PROPERTIES_10586
{
    union
    {
        UINT8 Value;
        struct
        {
            UINT8 StatusCodeFiltering : 1; // Symbol
            UINT8 FileInfoCache : 1; // Symbol
            UINT8 FileNotFoundCache : 1; // Symbol
            UINT8 DirectoryCache : 1; // Symbol
            UINT8 Leasing : 1; // Symbol
            UINT8 SuppressRenames : 1; // Symbol
            UINT8 ForceMultiChannel : 1; // Symbol
            UINT8 ForceKeepalive : 1; // Symbol
        } Fields;
    } Flags1;
    union
    {
        UINT8 Value;
        struct
        {
            UINT8 DisableBandwidthThrottling : 1; // Symbol
            UINT8 Reserved : 7;
        } Fields;
    } Flags2;
    UINT8 Padding[2];
    UINT32 SessionTimeoutInterval;
    UINT32 CAHandleKeepaliveInterval;
    UINT32 NonCAHandleKeepaliveInterval;
    UINT32 ActiveIOKeepaliveInterval;
} LMR_CONNECTION_PROPERTIES_10586, *PLMR_CONNECTION_PROPERTIES_10586;

typedef struct _LMR_CONNECTION_PROPERTIES_25398
{
    union
    {
        UINT8 Value;
        struct
        {
            UINT8 StatusCodeFiltering : 1; // Symbol
            UINT8 FileInfoCache : 1; // Symbol
            UINT8 FileNotFoundCache : 1; // Symbol
            UINT8 DirectoryCache : 1; // Symbol
            UINT8 Leasing : 1; // Symbol
            UINT8 SuppressRenames : 1; // Symbol
            UINT8 ForceMultiChannel : 1; // Symbol
            UINT8 ForceKeepalive : 1; // Symbol
        } Fields;
    } Flags1;
    union
    {
        UINT8 Value;
        struct
        {
            UINT8 DisableBandwidthThrottling : 1; // Symbol
            UINT8 Reserved : 7;
        } Fields;
    } Flags2;
    UINT8 Padding[2];
    UINT32 SessionTimeoutInterval;
    UINT32 CAHandleKeepaliveInterval;
    UINT32 NonCAHandleKeepaliveInterval;
    UINT32 ActiveIOKeepaliveInterval;
    UINT32 DisableRdma;
    UINT32 ConnectionCountPerRdmaInterface;
} LMR_CONNECTION_PROPERTIES_25398, *PLMR_CONNECTION_PROPERTIES_25398;

// Since 26100.1
typedef struct _LMR_CONNECTION_PROPERTIES
{
    union
    {
        UINT8 Value;
        struct
        {
            UINT8 StatusCodeFiltering : 1; // Symbol
            UINT8 FileInfoCache : 1; // Symbol
            UINT8 FileNotFoundCache : 1; // Symbol
            UINT8 DirectoryCache : 1; // Symbol
            UINT8 Leasing : 1; // Symbol
            UINT8 SuppressRenames : 1; // Symbol
            UINT8 ForceMultiChannel : 1; // Symbol
            UINT8 ForceKeepalive : 1; // Symbol
        } Fields;
    } Flags1;
    union
    {
        UINT8 Value;
        struct
        {
            UINT8 DisableBandwidthThrottling : 1; // Symbol
            UINT8 Reserved : 7;
        } Fields;
    } Flags2;
    UINT8 Padding[2];
    UINT32 SessionTimeoutInterval;
    UINT32 CAHandleKeepaliveInterval;
    UINT32 NonCAHandleKeepaliveInterval;
    UINT32 ActiveIOKeepaliveInterval;
    UINT32 DisableRdma;
    UINT32 ConnectionCountPerRdmaInterface;
    UINT16 AlternateTCPPort;
    UINT16 AlternateQuicPort;
    UINT16 AlternateRdmaPort;
    UINT8 Padding2[2];
} LMR_CONNECTION_PROPERTIES, *PLMR_CONNECTION_PROPERTIES;

#define LMR_INSTANCE_FLAG_REGISTER_FILESYSTEM 0x2
#define LMR_INSTANCE_FLAG_USE_CUSTOM_TRANSPORTS 0x4
#define LMR_INSTANCE_FLAG_ALLOW_GUEST_AUTH 0x8
#define LMR_INSTANCE_FLAG_SUPPORTS_DIRECTMAPPED_IO 0x10

typedef struct _LMR_START_INSTANCE_REQUEST_10586
{
    UINT32 StructureSize;
    UINT32 IoTimeout;
    UINT32 IoRetryCount;
    UINT16 Flags; // LMR_INSTANCE_FLAG_*
    UINT16 AlternatePort; // Symbol
    UINT32 Reserved1;
    LMR_CONNECTION_PROPERTIES_10586 DefaultConnectionProperties;
    UINT8 InstanceId;
    UINT8 Reserved2;
    UINT16 DeviceNameLength;
    WCHAR DeviceName[ANYSIZE_ARRAY]; // Symbol
} LMR_START_INSTANCE_REQUEST_10586, *PLMR_START_INSTANCE_REQUEST_10586;

typedef struct _LMR_START_INSTANCE_REQUEST_17134
{
    UINT32 StructureSize;
    UINT32 IoTimeout;
    UINT32 IoRetryCount;
    UINT16 Flags; // LMR_INSTANCE_FLAG_*
    UINT16 AlternatePort; // Symbol
    UINT32 Reserved1;
    SMB2_INSTANCE_CONFIGURATION_17134 InstanceConfig;
    LMR_CONNECTION_PROPERTIES_10586 DefaultConnectionProperties;
    UINT8 InstanceId;
    UINT8 Reserved2;
    UINT16 DeviceNameLength;
    WCHAR DeviceName[ANYSIZE_ARRAY]; // Symbol
} LMR_START_INSTANCE_REQUEST_17134, *PLMR_START_INSTANCE_REQUEST_17134;

typedef struct _LMR_START_INSTANCE_REQUEST_20348
{
    UINT32 StructureSize;
    UINT32 IoTimeout;
    UINT32 IoRetryCount;
    UINT16 Flags; // LMR_INSTANCE_FLAG_*
    UINT16 AlternatePort; // Symbol
    UINT32 Reserved1;
    SMB2_INSTANCE_CONFIGURATION_20348 InstanceConfig;
    LMR_CONNECTION_PROPERTIES_10586 DefaultConnectionProperties;
    UINT8 InstanceId;
    UINT8 Reserved2;
    UINT16 DeviceNameLength;
    WCHAR DeviceName[ANYSIZE_ARRAY]; // Symbol
} LMR_START_INSTANCE_REQUEST_20348, *PLMR_START_INSTANCE_REQUEST_20348;

typedef struct _LMR_START_INSTANCE_REQUEST_25398
{
    UINT32 StructureSize;
    UINT32 IoTimeout;
    UINT32 IoRetryCount;
    UINT16 Flags; // LMR_INSTANCE_FLAG_*
    UINT16 AlternatePort; // Symbol
    UINT32 Reserved1;
    SMB2_INSTANCE_CONFIGURATION_25398 InstanceConfig;
    LMR_CONNECTION_PROPERTIES_25398 DefaultConnectionProperties;
    UINT8 InstanceId;
    UINT8 Reserved2;
    UINT16 DeviceNameLength;
    WCHAR DeviceName[ANYSIZE_ARRAY]; // Symbol
} LMR_START_INSTANCE_REQUEST_25398, *PLMR_START_INSTANCE_REQUEST_25398;

typedef struct _LMR_START_INSTANCE_REQUEST_26100
{
    UINT32 StructureSize;
    UINT32 IoTimeout;
    UINT32 IoRetryCount;
    UINT16 Flags; // LMR_INSTANCE_FLAG_*
    UINT16 AlternatePort; // Symbol
    UINT32 Reserved1;
    SMB2_INSTANCE_CONFIGURATION_25398 InstanceConfig;
    LMR_CONNECTION_PROPERTIES DefaultConnectionProperties;
    UINT8 InstanceId;
    UINT8 Reserved2;
    UINT16 DeviceNameLength;
    WCHAR DeviceName[ANYSIZE_ARRAY]; // Symbol
} LMR_START_INSTANCE_REQUEST_26100, *PLMR_START_INSTANCE_REQUEST_26100;

typedef struct _LMR_START_INSTANCE_REQUEST
{
    UINT32 StructureSize;
    UINT32 IoTimeout;
    UINT32 IoRetryCount;
    UINT16 Flags; // LMR_INSTANCE_FLAG_*
    UINT16 AlternatePort; // Symbol
    UINT32 Reserved1;
    SMB2_INSTANCE_CONFIGURATION InstanceConfig;
    LMR_CONNECTION_PROPERTIES DefaultConnectionProperties;
    UINT8 InstanceId;
    UINT8 Reserved2;
    UINT16 DeviceNameLength;
    WCHAR DeviceName[ANYSIZE_ARRAY]; // Symbol
} LMR_START_INSTANCE_REQUEST, *PLMR_START_INSTANCE_REQUEST;

#ifndef FSCTL_LMR_START_INSTANCE
#define FSCTL_LMR_START_INSTANCE CTL_CODE( \
    FILE_DEVICE_NETWORK_FILE_SYSTEM, \
    232, \
    METHOD_BUFFERED, \
    FILE_ANY_ACCESS)
#endif // !FSCTL_LMR_START_INSTANCE

typedef enum _LMR_TRANSPORT_TYPE
{
    SmbCeTransportTypeTdi = 0x0,
    SmbCeTransportTypeTcpIp = 0x1,
    SmbCeTransportTypeVmbus = 0x2,
} LMR_TRANSPORT_TYPE, *PLMR_TRANSPORT_TYPE;

typedef struct _LMR_BIND_UNBIND_TRANSPORT_REQUEST
{
    UINT16 StructureSize;
    UINT16 Flags;
    LMR_TRANSPORT_TYPE Type;
    UINT TransportIdLength;
    WCHAR TransportId[ANYSIZE_ARRAY];
} LMR_BIND_UNBIND_TRANSPORT_REQUEST, *PLMR_BIND_UNBIND_TRANSPORT_REQUEST;

#ifndef FSCTL_LMR_BIND_TO_TRANSPORT
#define FSCTL_LMR_BIND_TO_TRANSPORT CTL_CODE( \
    FILE_DEVICE_NETWORK_FILE_SYSTEM, \
    108, \
    METHOD_BUFFERED, \
    FILE_ANY_ACCESS)
#endif // !FSCTL_LMR_BIND_TO_TRANSPORT

int main()
{
    // I can't use that because of the limitation in VC-LTL.
    // std::setlocale(LC_ALL, "zh_CN.UTF-8");

    std::setlocale(LC_ALL, "chs");

    std::wprintf(
        L"VirtualSmb\n"
        L"================================================================\n"
        L"The F@cking MSVC 2019 toolset ruined our $1M project!\n");

    return 0;
}
