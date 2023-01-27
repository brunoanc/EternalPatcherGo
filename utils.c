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
#include <openssl/evp.h>

#define MD5_DIGEST_LENGTH 16

// Split a given string using the given char as a delimiter into the given array
void split_string(char *str, const char delimiter, char ***array, int *array_len)
{
    *array_len = 0;

    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == delimiter)
            (*array_len)++;
    }

    (*array_len)++;

    *array = malloc(*array_len * sizeof(char*));

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

// Convert hex string to a byte array
unsigned char *hex_to_bytes(const char *str)
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

// Remove whitespace from string
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

// Get file's MD5 hash
char *get_md5_hash(const char *filename)
{
    FILE *f = fopen(filename, "rb");

    if (!f)
        return "";

    int read;
    int md5_length;
    unsigned char data[4096];
    unsigned char hash[MD5_DIGEST_LENGTH];

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_md5(), NULL);

    while ((read = (int)fread(data, 1, 4096, f)) != 0)
        EVP_DigestUpdate(ctx, data, read);

    EVP_DigestFinal_ex(ctx, hash, &md5_length);
    EVP_MD_CTX_free(ctx);

    char *hash_str = malloc(MD5_DIGEST_LENGTH * 2 + 1);

    if (!hash_str) {
        fprintf(stderr, "ERROR: Failed to allocate memory for MD5 hash!\n");
        exit(1);
    }

    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
        sprintf(hash_str + 2 * i, "%02x", hash[i]);
    
    return hash_str;
}
