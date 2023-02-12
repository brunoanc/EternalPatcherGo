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

int main(int argc, char **argv)
{
    printf("EternalPatcherLinux v1.4.0 by PowerBall253 :)\n\n");

    bool update = false;
    bool patch = false;
    char *filepath = NULL;

    // Print usage
    if (argc == 1) {
        printf("Usage:\n");
        printf("%s [--update] [--patch /path/to/DOOMEternalx64vk.exe]\n\n", argv[0]);
        printf("--update\tUpdates the patch definitions file using the update server specified in the config file.\n");
        printf("--patch\t\tPatches the executable in the path given.\n");
        return 1;
    }

    // Check program arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--patch") == 0 && !patch) {
            if (i > argc - 2) {
                eprintf("ERROR: No executable was specified for patching.\n");
                return 1;
            }

            patch = true;
            filepath = argv[i + 1];

            FILE *f = fopen(filepath, "rb+");

            if (!f) {
                perror("ERROR: Failed to open executable path");
                return 1;
            }
        }
        else if (strcmp(argv[i], "--update") == 0) {
            update = true;
        }
    }

    if (!patch && !update) {
        eprintf("ERROR: Unrecognized options! Run %s to see all possible arguments.\n", argv[0]);
        return 1;
    }

    // Update the patch definitions file
    char update_server[128];

    if (update) {
        printf("Checking for updates...\n");

        if (!get_update_server(update_server))
            return 1;

        char patch_defs_md5[MD5_DIGEST_LENGTH * 2 + 1];
        char latest_patch_defs_md5[MD5_DIGEST_LENGTH * 2 + 1];

        if (!get_latest_patch_defs_md5(update_server, latest_patch_defs_md5))
            return 1;

        if (!get_md5_hash("EternalPatcher.def", patch_defs_md5) || strcmp(patch_defs_md5, latest_patch_defs_md5) != 0) {
            printf("Downloading latest patch definitions...\n");

            if (!download_patch_defs(update_server))
                return 1;

            if (!get_md5_hash("EternalPatcher.def", patch_defs_md5) || strcmp(patch_defs_md5, latest_patch_defs_md5) != 0) {
                eprintf("ERROR: Failed to download the latest patch definitions.\n");
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

    // Load patches from the patch defs file
    printf("\nLoading patch definitions file...\n");

    char exe_md5[MD5_DIGEST_LENGTH * 2 + 1];
    get_md5_hash(filepath, exe_md5);

    struct GameBuild gamebuild = load_patch_defs(exe_md5);

    if (!gamebuild.offset_patches && !gamebuild.pattern_patches) {
        eprintf("ERROR: Failed to find patches for the provided executable.\n");
        return 1;
    }

    printf("%s detected.\n", gamebuild.id);

    // Apply patches to the executable
    printf("\nApplying patches...\n");

    int total_patches = cvector_size(gamebuild.offset_patches) + cvector_size(gamebuild.pattern_patches);
    int successes = 0;

    struct PatchingResult *patching_result = apply_patches(filepath, gamebuild.offset_patches, gamebuild.pattern_patches);
    cvector_free(gamebuild.offset_patches);
    cvector_free(gamebuild.pattern_patches);

    for (int i = 0; i < total_patches; i++) {
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

    printf("\n%d out of %d applied.\n", successes, total_patches);
    return (successes == total_patches) ? 0 : 1;
}
