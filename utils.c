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
#include <ctype.h>
#include <openssl/md5.h>

void split_string(char *str, char delimiter, char ***array, int *array_len)
{
    *array_len = 0;

    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == delimiter)
            (*array_len)++;
    }

    (*array_len)++;

    *array = malloc(sizeof(char*) * *array_len);

    if (!(*array)) {
        fprintf(stderr, "ERROR: Failed to allocate memory for string!\n");
        exit(1);
    }

    for (int i = *array_len - 1; i > 0; i--) {
        char *pos = strrchr(str, delimiter);

        if (pos == NULL)
            continue;

        *pos = '\0';
        (*array)[i] = pos + 1;
    }

    (*array)[0] = str;
}

unsigned char* hex_to_bytes(char *str)
{
    unsigned char *bytes = malloc(strlen(str) / 2);

    if (!bytes) {
        fprintf(stderr, "ERROR: Failed to allocate memory for bytes\n");
        exit(1);
    }

    for (int i = 0; i < strlen(str); i += 2) {
        char byte_str[3];
        memcpy(byte_str, str + i, 2);
        byte_str[2] = '\0';
        bytes[i / 2] = strtol(byte_str, NULL, 16);
    }

    return bytes;
}

void rm_whitespace(char *str)
{
    int j = 0;

    for (int i = 0; i < strlen(str); i++) {
        str[i] = str[i + j];

        if (isspace(str[i])) {
            j++;
            i--;
        }
    }
}

char* get_md5_hash(char *filename)
{
    FILE *f = fopen(filename, "rb");

    if (!f)
        return "";

    MD5_CTX mdContext;
    int bytes;
    unsigned char data[4096];
    unsigned char hash[MD5_DIGEST_LENGTH];

    MD5_Init(&mdContext);

    while ((bytes = fread(data, 1, 4096, f)) != 0)
        MD5_Update(&mdContext, data, bytes);

    MD5_Final(hash, &mdContext);

    char *hash_str = malloc(MD5_DIGEST_LENGTH * 2 + 1);

    if (!hash_str) {
        fprintf(stderr, "ERROR: Failed to allocate memory for MD5 hash!\n");
        exit(1);
    }

    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
        sprintf(hash_str + 2 * i, "%02x", hash[i]);
    
    return hash_str;
}