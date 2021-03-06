/////////////////////////////////////////////////////////////////////////////
/// Dispose Handle                                                        ///
/// Author: Zhai YiMing @ derzh.com <@tinymins>(c)                        ///
/// Reference: (HOW TO Enumerate Handles):                                ///
/// http://forum.sysinternals.com/howto-enumerate-handles_topic18892.html ///
/// Usage: DisposeHandle.exe <PID> <HANDLE_NAME>                          ///
/// Sample: DisposeHandle.exe 7814 \Sessions\1\BaseNamedObjects\0DF11826  ///
/////////////////////////////////////////////////////////////////////////////
#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <stdio.h>

#define NT_SUCCESS(x) ((x) >= 0)
#define STATUS_INFO_LENGTH_MISMATCH 0xc0000004

#define SystemHandleInformation 16
#define ObjectBasicInformation 0
#define ObjectNameInformation 1
#define ObjectTypeInformation 2

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _SYSTEM_HANDLE
{
    ULONG ProcessId;
    BYTE ObjectTypeNumber;
    BYTE Flags;
    USHORT Handle;
    PVOID Object;
    ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE, *PSYSTEM_HANDLE;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
    ULONG HandleCount;
    SYSTEM_HANDLE Handles[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef enum _POOL_TYPE
{
    NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed,
    DontUseThisType,
    NonPagedPoolCacheAligned,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS
} POOL_TYPE, *PPOOL_TYPE;

typedef struct _OBJECT_TYPE_INFORMATION
{
    UNICODE_STRING Name;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG TotalPagedPoolUsage;
    ULONG TotalNonPagedPoolUsage;
    ULONG TotalNamePoolUsage;
    ULONG TotalHandleTableUsage;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    ULONG HighWaterPagedPoolUsage;
    ULONG HighWaterNonPagedPoolUsage;
    ULONG HighWaterNamePoolUsage;
    ULONG HighWaterHandleTableUsage;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccess;
    BOOLEAN SecurityRequired;
    BOOLEAN MaintainHandleCount;
    USHORT MaintainTypeList;
    POOL_TYPE PoolType;
    ULONG PagedPoolUsage;
    ULONG NonPagedPoolUsage;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

PVOID GetLibraryProcAddress(PSTR LibraryName, PSTR ProcName)
{
    return GetProcAddress(GetModuleHandleA(LibraryName), ProcName);
}

typedef NTSTATUS (NTAPI *_NtQuerySystemInformation)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
    );
typedef NTSTATUS (NTAPI *_NtDuplicateObject)(
    HANDLE SourceProcessHandle,
    HANDLE SourceHandle,
    HANDLE TargetProcessHandle,
    PHANDLE TargetHandle,
    ACCESS_MASK DesiredAccess,
    ULONG Attributes,
    ULONG Options
    );
typedef NTSTATUS (NTAPI *_NtQueryObject)(
    HANDLE ObjectHandle,
    ULONG ObjectInformationClass,
    PVOID ObjectInformation,
    ULONG ObjectInformationLength,
    PULONG ReturnLength
    );
typedef LONG (NTAPI *_RtlInitUnicodeString)(
    UNICODE_STRING* DestinationString,
    PCWSTR          SourceString
    );
typedef LONG (NTAPI *_RtlCompareUnicodeString)(
    UNICODE_STRING* String1,
    UNICODE_STRING* String2,
    BOOLEAN         CaseInSensitive
    );

BOOLEAN RtlFindUnicodeString(
    IN PUNICODE_STRING str,
    IN PUNICODE_STRING substr,
    IN BOOLEAN caseInSensitive,
    OUT PUSHORT pos
    )
{
    _RtlCompareUnicodeString RtlCompareUnicodeString =
        GetLibraryProcAddress("ntdll.dll", "RtlCompareUnicodeString");
    USHORT index;

    if(RtlCompareUnicodeString(str, substr, caseInSensitive)){
        if (pos) {
            *pos = 0;
        }
        return TRUE;
    }

    for(index = 0; index + (substr->Length/sizeof(WCHAR)) <= (str->Length/sizeof(WCHAR)); index++) {
        if (_wcsnicmp( &str->Buffer[index],
            substr->Buffer,
            (substr->Length / sizeof(WCHAR)) ) == 0) {
                if (pos) {
                    *pos = index*sizeof(WCHAR);
                }
                return TRUE;
        }
    }
    return FALSE;
}

int wmain(int argc, WCHAR *argv[])
{
    _NtQuerySystemInformation NtQuerySystemInformation =
        GetLibraryProcAddress("ntdll.dll", "NtQuerySystemInformation");
    _NtDuplicateObject NtDuplicateObject =
        GetLibraryProcAddress("ntdll.dll", "NtDuplicateObject");
    _NtQueryObject NtQueryObject =
        GetLibraryProcAddress("ntdll.dll", "NtQueryObject");
    _RtlInitUnicodeString RtlInitUnicodeString =
        GetLibraryProcAddress("ntdll.dll", "RtlInitUnicodeString");
    _RtlCompareUnicodeString RtlCompareUnicodeString =
        GetLibraryProcAddress("ntdll.dll", "RtlCompareUnicodeString");
    NTSTATUS status;
    PSYSTEM_HANDLE_INFORMATION handleInfo;
    ULONG handleInfoSize = 0x10000;
    ULONG pid;
    UNICODE_STRING oname;
    HANDLE processHandle;
    ULONG i;

    if (argc < 2)
    {
        printf("Usage: DisposeObject.exe <PID> <OBJECT_NAME>\n\n");
        printf("Source: https://github.com/tinymins/DisposeObject.c\n");
        printf("        Please visit github to find more usages and updates.\n");
        return 1;
    }

    pid = _wtoi(argv[1]);
    RtlInitUnicodeString(&oname, argv[2]);

    if (!(processHandle = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pid)))
    {
        printf("Could not open PID %d! (Don't try to open a system process.)\n", pid);
        return 1;
    }
    else
    {
        printf("Querying objects with PID %d, NAME contains %.*S...\n", pid, oname.Length / 2, oname.Buffer);
    }

    handleInfo = (PSYSTEM_HANDLE_INFORMATION)malloc(handleInfoSize);

    /* NtQuerySystemInformation won't give us the correct buffer size,
       so we guess by doubling the buffer size. */
    while ((status = NtQuerySystemInformation(
        SystemHandleInformation,
        handleInfo,
        handleInfoSize,
        NULL
        )) == STATUS_INFO_LENGTH_MISMATCH)
        handleInfo = (PSYSTEM_HANDLE_INFORMATION)realloc(handleInfo, handleInfoSize *= 2);

    /* NtQuerySystemInformation stopped giving us STATUS_INFO_LENGTH_MISMATCH. */
    if (!NT_SUCCESS(status))
    {
        printf("NtQuerySystemInformation failed!\n");
        return 1;
    }

    for (i = 0; i < handleInfo->HandleCount; i++)
    {
        SYSTEM_HANDLE handle = handleInfo->Handles[i];
        HANDLE dupHandle = NULL;
        HANDLE dupHandle1 = NULL;
        LONG nCompareResult;
        POBJECT_TYPE_INFORMATION objectTypeInfo;
        PVOID objectNameInfo;
        UNICODE_STRING objectName;
        ULONG returnLength;

        /* Check if this handle belongs to the PID the user specified. */
        if (handle.ProcessId != pid)
            continue;

        /* Duplicate the handle so we can query it. */
        if (!NT_SUCCESS(NtDuplicateObject(
            processHandle,
            handle.Handle,
            GetCurrentProcess(),
            &dupHandle,
            0,
            0,
            0
            )))
        {
            // printf("[%#x] Error!\n", handle.Handle);
            continue;
        }

        /* Query the object type. */
        objectTypeInfo = (POBJECT_TYPE_INFORMATION)malloc(0x1000);
        if (!NT_SUCCESS(NtQueryObject(
            dupHandle,
            ObjectTypeInformation,
            objectTypeInfo,
            0x1000,
            NULL
            )))
        {
            printf("[%#x] Error!\n", handle.Handle);
            CloseHandle(dupHandle);
            continue;
        }

        /* Query the object name (unless it has an access of
           0x0012019f, on which NtQueryObject could hang. */
        if (handle.GrantedAccess == 0x0012019f)
        {
            /* We have the type, so display that. */
            // printf(
            //     "[%#x] %.*S: (did not get name)\n",
            //     handle.Handle,
            //     objectTypeInfo->Name.Length / 2,
            //     objectTypeInfo->Name.Buffer
            //     );
            free(objectTypeInfo);
            CloseHandle(dupHandle);
            continue;
        }

        objectNameInfo = malloc(0x1000);
        if (!NT_SUCCESS(NtQueryObject(
            dupHandle,
            ObjectNameInformation,
            objectNameInfo,
            0x1000,
            &returnLength
            )))
        {
            /* Reallocate the buffer and try again. */
            objectNameInfo = realloc(objectNameInfo, returnLength);
            if (!NT_SUCCESS(NtQueryObject(
                dupHandle,
                ObjectNameInformation,
                objectNameInfo,
                returnLength,
                NULL
                )))
            {
                /* We have the type name, so just display that. */
                printf(
                    "[%#x] %.*S: (could not get name)\n",
                    handle.Handle,
                    objectTypeInfo->Name.Length / 2,
                    objectTypeInfo->Name.Buffer
                    );
                free(objectTypeInfo);
                free(objectNameInfo);
                CloseHandle(dupHandle);
                continue;
            }
        }

        /* Cast our buffer into an UNICODE_STRING. */
        objectName = *(PUNICODE_STRING)objectNameInfo;

        /* Print the information! */
        if (objectName.Length)
        {
            /* The object has a name. */
            // printf(
            //     "[%#x] %.*S: %.*S\n",
            //     handle.Handle,
            //     objectTypeInfo->Name.Length / 2,
            //     objectTypeInfo->Name.Buffer,
            //     objectName.Length / 2,
            //     objectName.Buffer
            //     );

            nCompareResult = RtlFindUnicodeString(&objectName, &oname, TRUE, NULL);
            if (nCompareResult == 0 && NT_SUCCESS(NtDuplicateObject(
                processHandle,
                handle.Handle,
                GetCurrentProcess(),
                &dupHandle1,
                0,
                0,
                0x1
                )))
            {
                printf(
                    "[%#x] %.*S: %.*S Closed!\n",
                    handle.Handle,
                    objectTypeInfo->Name.Length / 2,
                    objectTypeInfo->Name.Buffer,
                    objectName.Length / 2,
                    objectName.Buffer
                    );
                CloseHandle(dupHandle1);
            }
        }
        else
        {
            /* Print something else. */
            // printf(
            //     "[%#x] %.*S: (unnamed)\n",
            //     handle.Handle,
            //     objectTypeInfo->Name.Length / 2,
            //     objectTypeInfo->Name.Buffer
            //     );
        }

        free(objectTypeInfo);
        free(objectNameInfo);
        CloseHandle(dupHandle);
    }

    free(handleInfo);
    CloseHandle(processHandle);

    return 0;
}
/************************************************************************/
/* If you want to apply with a process name, here's the bat code.       */
/************************************************************************/
/************************************************************************************
echo off
cls
color 0A
cd %~dp0
%~d0
REM Require Admin
set UAC=0
bcdedit>nul
if errorlevel 1 set UAC=1
if %UAC%==1 (
color CE
echo Please run this script as administrator.
pause
color 0A
exit
)
REM Admin Required
echo --------------------
echo Querying objects...
echo --------------------
for /f "tokens=2 " %%a in ('tasklist /fi "imagename eq JX3Client.exe" /nh') do (
DisposeObject.exe %%a \BaseNamedObjects\A5DFEC3F
echo --------------------
DisposeObject.exe %%a \BaseNamedObjects\0DF11825
echo --------------------
DisposeObject.exe %%a \BaseNamedObjects\5D2D1767
echo --------------------
)
for /f "tokens=2 " %%a in ('tasklist /fi "imagename eq JX3ClientX64.exe" /nh') do (
DisposeObject.exe %%a \BaseNamedObjects\A5DFEC3F
echo --------------------
DisposeObject.exe %%a \BaseNamedObjects\0DF11825
echo --------------------
DisposeObject.exe %%a \BaseNamedObjects\5D2D1767
echo --------------------
)
pause
************************************************************************************/
