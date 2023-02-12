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
#include <string.h>
#include "eternalpatcher.h"

#define OFFSET_PATCH 0
#define PATTERN_PATCH 1

// Load patch defs from file
struct GameBuild load_patch_defs(const char *exe_md5)
{
    struct GameBuild gamebuild;
    gamebuild.id = NULL;
    gamebuild.exe_filename = NULL;
    gamebuild.md5_checksum = NULL;
    gamebuild.patch_group_ids = NULL;
    gamebuild.patch_group_ids_len = 0;
    gamebuild.offset_patches = NULL;
    gamebuild.pattern_patches = NULL;

    FILE *patch_defs = fopen("EternalPatcher.def", "rb");

    if (!patch_defs) {
        perror("ERROR: Failed to open patches file");
        return gamebuild;
    }

    char current_line[256];

    while (fgets(current_line, 256, patch_defs) != NULL) {
        rm_whitespace(current_line);

        if (strchr(current_line, '#') == current_line)
            continue;

        char **data_def;
        int data_def_len = 0;

        split_string(current_line, '=', &data_def, &data_def_len);

        if (data_def_len <= 1) {
            free(data_def);
            continue;
        }

        if (strcmp(data_def[0], "patch") != 0) {
            char **gamebuild_data;
            int gamebuild_data_len = 0;

            split_string(data_def[1], ':', &gamebuild_data, &gamebuild_data_len);

            if (gamebuild_data_len != 3) {
                free(gamebuild_data);
                free(data_def);
                continue;
            }

            for (int i = 0; i < gamebuild_data_len; i++)
                rm_whitespace(gamebuild_data[i]);

            char **patch_group_ids;
            int patch_group_ids_len = 0;

            split_string(gamebuild_data[2], ',', &patch_group_ids, &patch_group_ids_len);

            if (strcmp(exe_md5, gamebuild_data[1]) != 0) {
                free(gamebuild_data);
                free(patch_group_ids);
                free(data_def);
                continue;
            }

            gamebuild.id = strdup(data_def[0]);
            gamebuild.exe_filename = strdup(gamebuild_data[0]);
            gamebuild.md5_checksum = strdup(gamebuild_data[1]);
            gamebuild.patch_group_ids = malloc(patch_group_ids_len * sizeof(char*));
            gamebuild.patch_group_ids_len = patch_group_ids_len;

            if (!gamebuild.id || !gamebuild.exe_filename || !gamebuild.md5_checksum || !gamebuild.patch_group_ids) {
                perror("ERROR: Failed to allocate memory");
                exit(1);
            }

            for (int i = 0; i < patch_group_ids_len; i++)
                gamebuild.patch_group_ids[i] = strdup(patch_group_ids[i]);

            free(gamebuild_data);
            free(patch_group_ids);
        }
        else {
            if (!gamebuild.id) {
                free(data_def);
                continue;
            }

            char **patch_data;
            int patch_data_len = 0;

            split_string(data_def[1], ':', &patch_data, &patch_data_len);

            if (patch_data_len != 5) {
                free(patch_data);
                free(data_def);
                continue;
            }

            for (int i = 0; i < patch_data_len; i++)
                rm_whitespace(patch_data[i]);

            if (strlen(patch_data[4]) % 2 != 0) {
                free(patch_data);
                free(data_def);
                continue;
            }

            int patch_type;

            if (strcmp(patch_data[1], "offset") == 0) {
                patch_type = OFFSET_PATCH;
            }
            else if (strcmp(patch_data[1], "pattern") == 0 && strlen(patch_data[3]) % 2 == 0) {
                patch_type = PATTERN_PATCH;
            }
            else {
                free(patch_data);
                free(data_def);
                continue;
            }

            char **patch_group_ids;
            int patch_group_ids_len = 0;

            split_string(patch_data[2], ',', &patch_group_ids, &patch_group_ids_len);

            if (patch_group_ids_len == 0) {
                free(patch_data);
                free(patch_group_ids);
                free(data_def);
                continue;
            }

            for (int i = 0; i < patch_group_ids_len; i++)
                rm_whitespace(patch_group_ids[i]);

            unsigned char *hex_patch = hex_to_bytes(patch_data[4]);

            if (patch_type == OFFSET_PATCH) {
                struct OffsetPatch offset_patch;

                offset_patch.description = strdup(patch_data[0]);
                offset_patch.offset = strtol(patch_data[3], NULL, 16);
                offset_patch.patch_byte_array = malloc(strlen(patch_data[4]) / 2);
                offset_patch.patch_byte_array_len = (int)strlen(patch_data[4]) / 2;

                if (!offset_patch.description || !offset_patch.patch_byte_array) {
                    perror("ERROR: Failed to allocate memory");
                    exit(1);
                }

                memcpy(offset_patch.patch_byte_array, hex_patch, offset_patch.patch_byte_array_len);
                free(hex_patch);

                if (offset_patch.description[0] == '\0') {
                    free(patch_data);
                    free(patch_group_ids);
                    free(data_def);
                    continue;
                }

                for (int i = 0; i < patch_group_ids_len; i++) {
                    bool found = false;

                    for (int k = 0; k < gamebuild.patch_group_ids_len; k++) {
                        if (strcmp(gamebuild.patch_group_ids[k], patch_group_ids[i]) == 0)
                            found = true;
                    }

                    if (!found)
                        continue;

                    struct OffsetPatch *current_patch;
                    bool already_exists = false;

                    cvector_for_each_in(current_patch, gamebuild.offset_patches) {
                        if (strcmp(current_patch->description, offset_patch.description) == 0) {
                            already_exists = true;
                            break;
                        }
                    }

                    if (already_exists)
                        break;

                    cvector_push_back(gamebuild.offset_patches, offset_patch);
                }
            }
            else {
                unsigned char *hex_pattern = hex_to_bytes(patch_data[3]);
                struct PatternPatch pattern_patch;

                pattern_patch.description = strdup(patch_data[0]);
                pattern_patch.pattern = malloc(strlen(patch_data[3]) / 2);
                pattern_patch.pattern_len = (int)strlen(patch_data[3]) / 2;
                pattern_patch.patch_byte_array = malloc(strlen(patch_data[4]) / 2);
                pattern_patch.patch_byte_array_len = (int)strlen(patch_data[4]) / 2;

                if (!pattern_patch.description || !pattern_patch.pattern || !pattern_patch.patch_byte_array) {
                    perror("ERROR: Failed to allocate memory");
                    exit(1);
                }

                memcpy(pattern_patch.pattern, hex_pattern, pattern_patch.pattern_len);
                memcpy(pattern_patch.patch_byte_array, hex_patch, pattern_patch.patch_byte_array_len);

                free(hex_pattern);
                free(hex_patch);

                if (pattern_patch.description[0] == '\0') {
                    free(patch_data);
                    free(patch_group_ids);
                    free(data_def);
                    continue;
                }

                for (int i = 0; i < patch_group_ids_len; i++) {
                    bool found = false;

                    for (int k = 0; k < gamebuild.patch_group_ids_len; k++) {
                        if (strcmp(gamebuild.patch_group_ids[k], patch_group_ids[i]) == 0)
                            found = true;
                    }

                    if (!found)
                        continue;

                    struct PatternPatch *current_patch;
                    bool already_exists = false;

                    cvector_for_each_in(current_patch, gamebuild.pattern_patches) {
                        if (strcmp(current_patch->description, pattern_patch.description) == 0) {
                            already_exists = true;
                            break;
                        }
                    }

                    if (already_exists)
                        break;

                    cvector_push_back(gamebuild.pattern_patches, pattern_patch);
                }
            }

            free(patch_data);
            free(patch_group_ids);
        }

        free(data_def);
    }

    fclose(patch_defs);

    return gamebuild;
}

// Apply the loaded patches to the executable
struct PatchingResult *apply_patches(const char *binary_filepath,
    cvector_vector_type(struct OffsetPatch) offset_patches, cvector_vector_type(struct PatternPatch) pattern_patches)
{
    struct PatchingResult *patching_results = malloc((cvector_size(pattern_patches) + cvector_size(offset_patches)) * sizeof(struct PatchingResult));

    if (!patching_results) {
        perror("ERROR: Failed to allocate memory");
        exit(1);
    }

    int i = 0;
    struct PatternPatch *pattern_patch_i;

    cvector_for_each_in(pattern_patch_i, pattern_patches) {
        struct PatchingResult patching_result = {
            strdup(pattern_patch_i->description),
            pattern_apply(binary_filepath, pattern_patch_i)
        };

        patching_results[i] = patching_result;

        free(pattern_patch_i->description);
        free(pattern_patch_i->pattern);
        free(pattern_patch_i->patch_byte_array);
        i++;
    }

    struct OffsetPatch *offset_patch_i;

    cvector_for_each_in(offset_patch_i, offset_patches) {
        struct PatchingResult patching_result = {
            strdup(offset_patch_i->description),
            offset_apply(binary_filepath, offset_patch_i)
        };

        patching_results[i] = patching_result;

        free(offset_patch_i->description);
        free(offset_patch_i->patch_byte_array);
        i++;
    }

    return patching_results;
}
