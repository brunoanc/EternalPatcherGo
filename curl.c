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

char update_server[128];

// Get update server from config file
int get_update_server(void)
{
    FILE *config = fopen("EternalPatcher.config", "r");

    if (!config) {
        fprintf(stderr, "ERROR: Failed to open config file!\n");
        return -1;
    }

    if(!fgets(update_server, 128, config)) {
        fprintf(stderr, "ERROR: Failed to read from config file!\n");
        return -1;
    }

    fclose(config);

    rm_whitespace(update_server);

    char *equals = strchr(update_server, '=');

    if (!equals) {
        fprintf(stderr, "ERROR: Failed to parse config file!\n");
        return -1;
    }

    strcpy(update_server, equals + 2);

    char *semicolon = strchr(update_server, ';');

    if (!semicolon) {
        fprintf(stderr, "ERROR: Failed to parse config file!\n");
        return -1;
    }

    *(semicolon - 1) = '\0';

    return 0;
}

// Used to write data to memory from curl
size_t write_clbk(void *data, size_t blksz, size_t nblk, void *ctx)
{
    static size_t sz = 0;
    size_t currsz = blksz * nblk;

    size_t prevsz = sz;
    sz += currsz;
    void *tmp = realloc(*(char**)ctx, sz);

    if (tmp == NULL) {
        free(*(char**)ctx);
        *(char**)ctx = NULL;
        return 0;
    }

    *(char**)ctx = tmp;

    memcpy(*(char**)ctx + prevsz, data, currsz);

    return currsz;
}

// Get latest patch definitions MD5 from the update server
char *get_latest_patch_defs_md5(const char *webpage)
{
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
            fprintf(stderr, "ERROR: Failed to get latest patch definitions MD5 checksum!\n");
            return "";
        }

        curl_easy_cleanup(curl);
        return pagedata;
    }

    fprintf(stderr, "ERROR: Failed to get latest patch definitions MD5 checksum!\n");
    return "";
}

// Download the latest patch definitions from the update server
int download_patch_defs(void)
{
    CURL *curl;
    FILE *fp;
    CURLcode res;

    char download_url[256];
    snprintf(download_url, 256, "http://%128s/EternalPatcher_v%d.def", update_server, patcher_version);

    curl = curl_easy_init();

    if (curl) {
        fp = fopen("EternalPatcher.def", "wb");
        curl_easy_setopt(curl, CURLOPT_URL, download_url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "ERROR: Failed to download latest patch definitions!\n");
            return -1;
        }

        curl_easy_cleanup(curl);
        fclose(fp);

        return 0;
    }

    fprintf(stderr, "ERROR: Failed to download latest patch definitions!\n");
    return -1;
}
