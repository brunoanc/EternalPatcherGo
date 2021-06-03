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

#ifndef ETERNALPATCHER_H
#define ETERNALPATCHER_H

#include <stdbool.h>
#include <glib.h>

struct PatternPatch {
    char *description;
    unsigned char *pattern;
    int pattern_len;
    unsigned char *patch_byte_array;
    int patch_byte_array_len;
};

struct OffsetPatch {
    char *description;
    long offset;
    unsigned char *patch_byte_array;
    int patch_byte_array_len;
};

struct PatchingResult {
    char *description;
    bool success;
};

struct GameBuild {
    char *id;
    char *exe_filename;
    char *md5_checksum;
    char **patch_group_ids;
    int patch_group_ids_len;
    GArray *offset_patches;
    GArray *pattern_patches;
};

bool update_available(void);
int download_patch_defs(void);
void split_string(char *str, char delimiter, char ***array, int *array_len);
int load_patch_defs(void);
unsigned char *hex_to_bytes(char *str);
bool any_patches_loaded(void);
bool offset_apply(char *binary_filepath, struct OffsetPatch *patch);
bool pattern_apply(char *binary_filepath, struct PatternPatch *patch);
struct GameBuild *get_gamebuild(char *filepath);
struct PatchingResult *apply_patches(char *binary_filepath, GArray *offset_patches, GArray *pattern_patches);
void rm_whitespace(char *str);
char *get_latest_patch_defs_md5(char *webpage);
int get_update_server(void);
char *get_md5_hash(char *filename);

#endif