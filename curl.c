/*
* This file is part of EternalPatcherLinux (https://github.com/PowerBall253/EternalPatcherLinux).
* Copyright (C) 2021 PowerBall253
*
* EternalPatcherLinux is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* EternalPatcherLinux is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with EternalPatcherLinux. If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "eternalpatcher.h"

// Get update server from config file
bool get_update_server(char update_server[static 128])
{
    FILE *config = fopen("EternalPatcher.config", "r");

    if (!config) {
        perror("ERROR: Failed to open config file");
        return false;
    }

    if(!fgets(update_server, 128, config)) {
        perror("ERROR: Failed to read from config file");
        fclose(config);
        return false;
    }

    fclose(config);

    rm_whitespace(update_server);

    char *equals = strchr(update_server, '=');

    if (!equals) {
        eprintf("ERROR: Failed to parse config file.\n");
        return false;
    }

    strncpy(update_server, equals + 2, 127);

    char *semicolon = strchr(update_server, ';');

    if (!semicolon) {
        eprintf("ERROR: Failed to parse config file.\n");
        return false;
    }

    *(semicolon - 1) = '\0';

    return true;
}

// Used to write data to memory from curl
size_t write_clbk(void *data, size_t blksz, size_t nblk, void *ctx)
{
    static size_t sz = 0;
    size_t currsz = blksz * nblk;

    size_t prevsz = sz;
    sz += currsz;
    void *tmp = realloc(*(char**)ctx, sz);

    if (!tmp) {
        free(*(char**)ctx);
        *(char**)ctx = NULL;
        return 0;
    }

    *(char**)ctx = tmp;

    memcpy(*(char**)ctx + prevsz, data, currsz);

    return currsz;
}

// Get latest patch definitions MD5 from the update server
bool get_latest_patch_defs_md5(const char update_server[static 128], char md5[static MD5_DIGEST_LENGTH * 2 + 1])
{
    char webpage[256];
    snprintf(webpage, 255, "http://%.128s/EternalPatcher_v%d.md5", update_server, PATCHER_VERSION);

    CURL *curl;
    CURLcode res;
    char *pagedata = NULL;

    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, webpage);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_clbk);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &pagedata);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            eprintf("ERROR: Failed to get latest patch definitions MD5 checksum: %s\n", curl_easy_strerror(res));

            if (pagedata)
                free(pagedata);

            curl_easy_cleanup(curl);

            return false;
        }

        curl_easy_cleanup(curl);

        memcpy(md5, pagedata, MD5_DIGEST_LENGTH * 2);
        md5[MD5_DIGEST_LENGTH * 2] = '\0';

        free(pagedata);

        return true;
    }

    eprintf("ERROR: Failed to get latest patch definitions MD5 checksum.\n");
    return false;
}

// Download the latest patch definitions from the update server
bool download_patch_defs(const char update_server[static 128])
{
    CURL *curl;
    FILE *fp;
    CURLcode res;

    char download_url[256];
    snprintf(download_url, 255, "http://%.128s/EternalPatcher_v%d.def", update_server, PATCHER_VERSION);

    curl = curl_easy_init();

    if (curl) {
        fp = fopen("EternalPatcher.def", "wb");
        curl_easy_setopt(curl, CURLOPT_URL, download_url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            eprintf("ERROR: Failed to download the latest patch definitions: %s\n", curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            return false;
        }

        curl_easy_cleanup(curl);
        fclose(fp);

        return true;
    }

    eprintf("ERROR: Failed to download the latest patch definitions.\n");
    return false;
}
