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

extern GArray *gamebuilds;

int main(int argc, char **argv)
{
    printf("EternalPatcherLinux v1.0 by PowerBall253 :)\n\n");

    bool update = false;
    bool patch = false;
    char *filepath = NULL;

    if (argc == 1) {
        printf("Usage:\n");
        printf("%s [--update] [--patch /path/to/DOOMEternalx64vk.exe]\n\n", argv[0]);
        printf("--update\tUpdates the patch definitions file using the update server specified in the config file.\n");
        printf("--patch\t\tPatches the executable in the path given.\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--patch") && !patch) {
            if (i > argc - 2) {
                fprintf(stderr, "ERROR: No executable was specified for patching!\n");
                return 1;
            }

            patch = true;
            filepath = argv[i + 1];
        }
        else if (!strcmp(argv[i], "--update")) {
            update = true;
        }
    }

    if (!patch && !update) {
        fprintf(stderr, "ERROR: Unrecognized options! Run %s to see all possible arguments.\n", argv[0]);
        return 1;
    }

    if (update) {
        printf("Checking for updates...\n");

        if (update_available()) {
            printf("Downloading latest patch definitions...\n");

            if (download_patch_defs() == -1)
                return 1;

            FILE *patch_defs = fopen("EternalPatcher.def", "r");

            if (!patch_defs) {
                fprintf(stderr, "ERROR: Failed to download the latest patch definitions!\n");
                return 1;
            }

            printf("Done.\n");
        }
        else {
            printf("No updates available.\n");
        }

        if (!patch)
            return 0;
    }

    printf("\nLoading patch definitions file...\n");

    if(load_patch_defs() == -1) {
        fprintf(stderr, "ERROR: Failed to load patches!\n");
        return 1;
    }

    if (!any_patches_loaded()) {
        fprintf(stderr, "ERROR: Failed to load patches!\n");
        return 1;
    }

    printf("Done.\n");

    printf("\nChecking game builds...\n");

    struct GameBuild *gamebuild = get_gamebuild(filepath);

    if (!gamebuild) {
        fprintf(stderr, "ERROR: Unable to load patches: Unsupported game build.\n");
        return 1;
    }

    printf("%s detected.\n", gamebuild->id);

    int successes = 0;

    printf("\nApplying patches...\n");

    struct PatchingResult *patching_result = apply_patches(filepath, gamebuild->offset_patches, gamebuild->pattern_patches);

    for (int i = 0; i < gamebuild->offset_patches->len + gamebuild->pattern_patches->len; i++) {
        if (patching_result[i].success) {
            successes++;
            printf("%s: Success\n", patching_result[i].description);
        }
        else {
            printf("%s: Failure\n", patching_result[i].description);
        }

        free(patching_result[i].description);
    }

    free(patching_result);

    printf("\n%d out of %d applied.\n", successes, gamebuild->offset_patches->len + gamebuild->pattern_patches->len);

    int return_value = (successes == gamebuild->offset_patches->len + gamebuild->pattern_patches->len) ? 0 : 1;

    for (int i = 0; i < gamebuilds->len; i++) {
        struct GameBuild gamebuild_i = g_array_index(gamebuilds, struct GameBuild, i);

        free(gamebuild_i.id);
        free(gamebuild_i.exe_filename);
        free(gamebuild_i.md5_checksum);
        free(gamebuild_i.patch_group_ids);

        g_array_free(gamebuild_i.offset_patches, true);
        g_array_free(gamebuild_i.pattern_patches, true);
    }

    g_array_free(gamebuilds, true);

    return return_value;
}