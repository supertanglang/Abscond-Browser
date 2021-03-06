/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Omnijar.h"

#include "nsDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIFile.h"
#include "nsZipArchive.h"
#include "nsNetUtil.h"

namespace mozilla {

nsIFile *Omnijar::sPath[2] = { nullptr, nullptr };
nsZipArchive *Omnijar::sReader[2] = { nullptr, nullptr };
bool Omnijar::sInitialized = false;
static bool sIsUnified = false;
static bool sIsNested[2] = { false, false };

static const char *sProp[2] =
    { NS_GRE_DIR, NS_XPCOM_CURRENT_PROCESS_DIR };

#define SPROP(Type) ((Type == mozilla::Omnijar::GRE) ? sProp[GRE] : sProp[APP])

void
Omnijar::CleanUpOne(Type aType)
{
    if (sReader[aType]) {
        sReader[aType]->CloseArchive();
        NS_IF_RELEASE(sReader[aType]);
    }
    sReader[aType] = nullptr;
    NS_IF_RELEASE(sPath[aType]);
}

void
Omnijar::InitOne(nsIFile *aPath, Type aType)
{
    nsCOMPtr<nsIFile> file;
    if (aPath) {
        file = aPath;
    } else {
        nsCOMPtr<nsIFile> dir;
        nsDirectoryService::gService->Get(SPROP(aType), NS_GET_IID(nsIFile), getter_AddRefs(dir));
        NS_NAMED_LITERAL_CSTRING (kOmnijarName, NS_STRINGIFY(OMNIJAR_NAME));
        if (NS_FAILED(dir->Clone(getter_AddRefs(file))) ||
            NS_FAILED(file->AppendNative(kOmnijarName)))
            return;
    }
    bool isFile;
    if (NS_FAILED(file->IsFile(&isFile)) || !isFile) {
        // If we're not using an omni.jar for GRE, and we don't have an
        // omni.jar for APP, check if both directories are the same.
        if ((aType == APP) && (!sPath[GRE])) {
            nsCOMPtr<nsIFile> greDir, appDir;
            bool equals;
            nsDirectoryService::gService->Get(SPROP(GRE), NS_GET_IID(nsIFile), getter_AddRefs(greDir));
            nsDirectoryService::gService->Get(SPROP(APP), NS_GET_IID(nsIFile), getter_AddRefs(appDir));
            if (NS_SUCCEEDED(greDir->Equals(appDir, &equals)) && equals)
                sIsUnified = true;
        }
        return;
    }

    bool equals;
    if ((aType == APP) && (sPath[GRE]) &&
        NS_SUCCEEDED(sPath[GRE]->Equals(file, &equals)) && equals) {
        // If we're using omni.jar on both GRE and APP and their path
        // is the same, we're in the unified case.
        sIsUnified = true;
        return;
    }

    nsRefPtr<nsZipArchive> zipReader = new nsZipArchive();
    if (NS_FAILED(zipReader->OpenArchive(file))) {
        return;
    }

    nsRefPtr<nsZipHandle> handle;
    if (NS_SUCCEEDED(nsZipHandle::Init(zipReader, NS_STRINGIFY(OMNIJAR_NAME), getter_AddRefs(handle)))) {
        zipReader = new nsZipArchive();
        if (NS_FAILED(zipReader->OpenArchive(handle)))
            return;
        sIsNested[aType] = true;
    }

    CleanUpOne(aType);
    sReader[aType] = zipReader;
    NS_IF_ADDREF(sReader[aType]);
    sPath[aType] = file;
    NS_IF_ADDREF(sPath[aType]);
}

void
Omnijar::Init(nsIFile *aGrePath, nsIFile *aAppPath)
{
    InitOne(aGrePath, GRE);
    InitOne(aAppPath, APP);
    sInitialized = true;
}

void
Omnijar::CleanUp()
{
    CleanUpOne(GRE);
    CleanUpOne(APP);
    sInitialized = false;
}

already_AddRefed<nsZipArchive>
Omnijar::GetReader(nsIFile *aPath)
{
    NS_ABORT_IF_FALSE(IsInitialized(), "Omnijar not initialized");

    bool equals;
    nsresult rv;

    if (sPath[GRE] && !sIsNested[GRE]) {
        rv = sPath[GRE]->Equals(aPath, &equals);
        if (NS_SUCCEEDED(rv) && equals)
            return GetReader(GRE);
    }
    if (sPath[APP] && !sIsNested[APP]) {
        rv = sPath[APP]->Equals(aPath, &equals);
        if (NS_SUCCEEDED(rv) && equals)
            return GetReader(APP);
    }
    return nullptr;
}

nsresult
Omnijar::GetURIString(Type aType, nsACString &result)
{
    NS_ABORT_IF_FALSE(IsInitialized(), "Omnijar not initialized");

    result.Truncate();

    // Return an empty string for APP in the unified case.
    if ((aType == APP) && sIsUnified) {
        return NS_OK;
    }

    nsAutoCString omniJarSpec;
    if (sPath[aType]) {
        nsresult rv = NS_GetURLSpecFromActualFile(sPath[aType], omniJarSpec);
        NS_ENSURE_SUCCESS(rv, rv);

        result = "jar:";
        if (sIsNested[aType])
            result += "jar:";
        result += omniJarSpec;
        result += "!";
        if (sIsNested[aType])
            result += "/" NS_STRINGIFY(OMNIJAR_NAME) "!";
    } else {
        nsCOMPtr<nsIFile> dir;
        nsDirectoryService::gService->Get(SPROP(aType), NS_GET_IID(nsIFile), getter_AddRefs(dir));
        nsresult rv = NS_GetURLSpecFromActualFile(dir, result);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    result += "/";
    return NS_OK;
}

bool
Omnijar::RebaseFilename(const nsCString& filename, const nsCString& oldBase, const nsCString& newBase, nsACString &result) {
    PRInt32 pos = filename.Find(oldBase);
    PRInt32 pathLen = filename.Length() - pos - oldBase.Length();
    if (pos > -1 && pathLen > -1 && pathLen <= filename.Length()) {
        nsAutoCString path;
        filename.Right(path, pathLen);
        result = newBase + path;
        return true;
    }
    result = filename;
    return false;
}

void
Omnijar::ConvertToResourceFilename(const nsCString& filename, nsACString &result) {
    if (StringBeginsWith(filename, NS_LITERAL_CSTRING("file://"))
        || StringBeginsWith(filename, NS_LITERAL_CSTRING("jar:"))) {
        if (RebaseFilename(filename, NS_LITERAL_CSTRING("/browser/omni.ja!/"),
                           NS_LITERAL_CSTRING("resource://app/"), result)) {
            return;
        }
        if (RebaseFilename(filename, NS_LITERAL_CSTRING("/omni.ja!/"),
                           NS_LITERAL_CSTRING("resource://gre/"), result)) {
            return;
        }
    }
    result = filename;
}

} /* namespace mozilla */
