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
#include <stdlib.h>

#include "eternalpatcher.h"

bool offset_apply(char *binary_filepath, struct OffsetPatch *patch)
{
    if (patch->patch_byte_array_len == 0)
        return false;

    FILE *exe = fopen(binary_filepath, "rb+");

    if (!exe)
        return false;

    fseek(exe, 0, SEEK_END);
    long filesize = ftell(exe);
    fseek(exe, 0, SEEK_SET);

    if (patch->offset < 0 || patch->offset + patch->patch_byte_array_len > filesize)
        return false;

    fseek(exe, patch->offset, SEEK_SET);

    fwrite(patch->patch_byte_array, 1, patch->patch_byte_array_len, exe);

    return true;
}

bool pattern_apply(char *binary_filepath, struct PatternPatch *patch)
{
    if (patch->patch_byte_array_len == 0 || patch->pattern_len == 0 || patch->patch_byte_array_len != patch->pattern_len)
        return false;
    
    int buffer_size = 1024;
    int matches = 0;
    long current_file_pos = 0;
    long pattern_start_pos = -1;

    FILE *exe = fopen(binary_filepath, "rb+");

    if (!exe)
        return false;

    fseek(exe, 0, SEEK_END);
    long filesize = ftell(exe);
    fseek(exe, 0, SEEK_SET);

    unsigned char *buffer = malloc(buffer_size);

    if (!buffer) {
        fprintf(stderr, "ERROR: Failed to allocate memory for executable bytes!\n");
        exit(1);
    }

    while (ftell(exe) < filesize) {
        if (fread(buffer, 1, buffer_size, exe) == 0) {
            fprintf(stderr, "ERROR: Failed to read bytes from executable!");
            return false;
        }

        for (int i = 0; i < buffer_size; i++) {
            if (buffer[i] == patch->pattern[matches]) {
                matches++;


                if (matches == patch->pattern_len) {
                    pattern_start_pos = ftell(exe) - (buffer_size - i) - (patch->pattern_len - 1);
                    break;
                }
            }
            else {
                matches = 0;
            }
        }

        if (pattern_start_pos != -1)
            break;
    }

    free(buffer);

    if (pattern_start_pos == -1)
        return false;
    
    fseek(exe, pattern_start_pos, SEEK_SET);
    fwrite(patch->patch_byte_array, 1, patch->patch_byte_array_len, exe);

    return true;
}