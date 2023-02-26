/*
* This file is part of EternalPatcherGo (https://github.com/PowerBall253/EternalPatcherGo).
* Copyright (C) 2023 PowerBall253
*
* EternalPatcherGo is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* EternalPatcherGo is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with EternalPatcherGo. If not, see <https://www.gnu.org/licenses/>.
*/

package main

import (
	"bufio"
	"crypto/md5"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/http"
	"os"
	"strconv"
	"strings"
)

// Patcher's internal major version
const PatcherVersion = 3

// Patch types
type Patch int

const (
	Pattern Patch = iota
	Offset  Patch = iota
)

// Pattern patch definition
type PatternPatch struct {
	description    string
	pattern        []byte
	patchByteArray []byte
}

// Offset patch definition
type OffsetPatch struct {
	description    string
	offset         int64
	patchByteArray []byte
}

// Game build information
type GameBuild struct {
	id             string
	exeFilename    string
	md5Checksum    string
	patchGroupIds  []string
	offsetPatches  []OffsetPatch
	patternPatches []PatternPatch
}

// Patching result
type PatchingResult struct {
	description string
	success     bool
}

// Get update server from config file
func getUpdateServer() (string, error) {
	// Read config file
	configBytes, err := os.ReadFile("EternalPatcher.config")
	if err != nil {
		return "", err
	}

	// Deserialize JSON
	type configFile struct {
		UpdateServer string
	}
	var config configFile
	err = json.Unmarshal(configBytes, &config)
	if err != nil {
		return "", err
	}

	// Get value
	return config.UpdateServer, nil
}

// Get MD5 hash of new patch definitions from server
func getLatestPatchDefsMD5(updateServer string) (string, error) {
	// Get file from server
	link := fmt.Sprintf("http://%s/EternalPatcher_v%d.md5", updateServer, PatcherVersion)
	resp, err := http.Get(link)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()

	// Make sure file was found
	if resp.StatusCode != 200 {
		return "", errors.New("http request: not found")
	}

	// Read from http server
	md5Bytes := make([]byte, 32)
	_, err = resp.Body.Read(md5Bytes)
	if err != nil {
		return "", err
	}

	// Convert MD5 bytes to string
	return string(md5Bytes), nil
}

// Download patch definitions from server
func downloadPatchDefs(updateServer string) error {
	// Create patch defs file
	file, err := os.Create("EternalPatcher.def")
	if err != nil {
		return err
	}
	defer file.Close()

	// Get file from server
	link := fmt.Sprintf("http://%s/EternalPatcher_v%d.def", updateServer, PatcherVersion)
	resp, err := http.Get(link)
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	// Make sure file was found
	if resp.StatusCode != 200 {
		return errors.New("http request: not found")
	}

	// Copy from http server to local file
	_, err = io.Copy(file, resp.Body)
	if err != nil {
		return err
	}

	return nil
}

// Get MD5 hash of file
func getMD5Hash(path string) (string, error) {
	// Open file
	file, err := os.Open(path)
	if err != nil {
		return "", err
	}
	defer file.Close()

	// Init hasher
	hasher := md5.New()
	if _, err := io.Copy(hasher, file); err != nil {
		return "", err
	}

	// Get hash
	return hex.EncodeToString(hasher.Sum(nil)), nil
}

// Apply pattern patch
func patternApply(exePath string, patch PatternPatch) bool {
	// Open executable
	exe, err := os.OpenFile(exePath, os.O_RDWR, 0)
	if err != nil {
		return false
	}
	defer exe.Close()

	// Find pattern
	matches := 0
	patternStartPosition := int64(-1)
	buffer := make([]byte, 1024)
	for {
		// Read into buffer
		count, err := exe.Read(buffer)
		if err != nil {
			if err == io.EOF {
				break
			}
			return false
		}
		buffer = buffer[:count]

		// Look for matching bytes
		for i := range buffer {
			// Check if byte matches
			if buffer[i] == patch.pattern[matches] {
				matches++

				// Check if we've matched the entire patch
				if matches == len(patch.pattern) {
					// Get current position
					pos, _ := exe.Seek(0, io.SeekCurrent)

					// Get pattern start position
					patternStartPosition = pos - int64(len(buffer) - i) - int64(len(patch.pattern) - 1)
					break
				}
			} else {
				matches = 0
			}
		}

		// Check if we found the match
		if patternStartPosition != -1 {
			break
		}
	}

	// Check if we found the match
	if patternStartPosition == -1 {
		return false
	}

	// Write patch
	exe.Seek(patternStartPosition, io.SeekStart)
	count, err := exe.Write(patch.patchByteArray)
	if count != len(patch.patchByteArray) || err != nil {
		return false
	}

	return true
}

// Apply offset patch
func offsetApply(exePath string, patch OffsetPatch) bool {
	// Open executable
	exe, err := os.Open(exePath)
	if err != nil {
		return false
	}
	defer exe.Close()

	// Get exe size
	fi, err := exe.Stat()
	if err != nil {
		return false
	}
	fileSize := fi.Size()

	// Make sure patch is within exe bounds
	if patch.offset + int64(len(patch.patchByteArray)) > fileSize {
		return false
	}

	// Write patch
	exe.Seek(patch.offset, io.SeekStart)
	count, err := exe.Write(patch.patchByteArray)
	if count != len(patch.patchByteArray) || err != nil {
		return false
	}

	return true
}

// Apply all patches to exe
func applyPatches(exePath string, patternPatches []PatternPatch, offsetPatches []OffsetPatch) []PatchingResult {
	patchingResults := make([]PatchingResult, len(offsetPatches) + len(patternPatches))

	// Apply pattern patches
	for i, patch := range patternPatches {
		patchingResults[i] = PatchingResult {
			description: patch.description,
			success:     patternApply(exePath, patch),
		}
	}

	// Apply offset patches
	for i, patch := range offsetPatches {
		patchingResults[len(patternPatches) + i] = PatchingResult {
			description: patch.description,
			success:     offsetApply(exePath, patch),
		}
	}

	return patchingResults
}

// Load patch definitions from file
func loadPatchDefs(exeMD5 string) (GameBuild, error) {
	gamebuild := GameBuild{
		id:             "",
		exeFilename:    "",
		md5Checksum:    "",
		patchGroupIds:  nil,
		offsetPatches:  nil,
		patternPatches: nil,
	}

	// Open patch definitions file
	patchDefs, err := os.Open("EternalPatcher.def")
	if err != nil {
		return gamebuild, err
	}
	defer patchDefs.Close()

	// Read line by line
	scanner := bufio.NewScanner(patchDefs)
	for scanner.Scan() {
		// Strip leading and trailing whitespace
		currentLine := strings.TrimSpace(scanner.Text())

		if currentLine == "" {
			continue
		}

		// Ignore comments
		if currentLine[0] == '#' {
			continue
		}

		// Split on equals
		dataDef := strings.Split(currentLine, "=")
		if len(dataDef) < 2 {
			continue
		}

		// Check if line is a game build or a patch
		if dataDef[0] != "patch" {
			// Gamebuild found, parse it

			// Separate game build data with colons
			gamebuildData := strings.Split(dataDef[1], ":")
			if len(gamebuildData) != 3 {
				continue
			}

			// Check if hash matches
			if exeMD5 != gamebuildData[1] {
				continue
			}

			// Set gamebuild properties
			gamebuild.id = dataDef[0]
			gamebuild.exeFilename = gamebuildData[0]
			gamebuild.md5Checksum = gamebuildData[1]
			gamebuild.patchGroupIds = strings.Split(gamebuildData[2], ",")
		} else {
			// Patch found, parse it

			// Check if game build is already set
			if gamebuild.id == "" {
				continue
			}

			// Separate patch data with colons
			patchData := strings.Split(dataDef[1], ":")
			if len(patchData) != 5 {
				continue
			}

			// Make sure hex string has even length
			if len(patchData[4]) % 2 != 0 {
				continue
			}

			// Get patch type
			var patchType Patch
			switch patchData[1] {
			case "pattern":
				// Make sure hex string has even length
				if len(patchData[3]) % 2 != 0 {
					continue
				}

				patchType = Pattern
			case "offset":
				patchType = Offset
			default:
				continue
			}

			// Check if game build contains patch group id and if it isn't loaded already
			patchGroupIds := strings.Split(patchData[2], ",")
			loadPatch := false
			for _, id := range patchGroupIds {
				// Find patch group id
				found := false
				for _, gamebuildId := range gamebuild.patchGroupIds {
					if id == gamebuildId {
						found = true
						break
					}
				}

				// Do not load if it isn't found
				if !found {
					continue
				}

				// Check if patch already exists in game build
				alreadyExists := false
				if patchType == Pattern {
					for _, patch := range gamebuild.patternPatches {
						if patch.description == patchData[0] {
							alreadyExists = true
							break
						}
					}
				} else {
					for _, patch := range gamebuild.offsetPatches {
						if patch.description == patchData[0] {
							alreadyExists = true
							break
						}
					}
				}

				// Do not load if it already exists
				if alreadyExists {
					break
				}

				loadPatch = true
			}
			if !loadPatch {
				continue
			}

			// Get patch as bytes
			patchBytes, err := hex.DecodeString(patchData[4])
			if err != nil {
				continue
			}

			// Parse patches
			if patchType == Pattern {
				// Get pattern as bytes
				patternBytes, err := hex.DecodeString(patchData[3])
				if err != nil {
					continue
				}

				// Add pattern patch to gamebuild
				gamebuild.patternPatches = append(gamebuild.patternPatches, PatternPatch {
					description:    patchData[0],
					pattern:        patternBytes,
					patchByteArray: patchBytes,
				})
			} else {
				// Get offset as int64
				offset, err := strconv.ParseInt(patchData[3], 10, 64)
				if err != nil {
					continue
				}

				// Add offset patch to gamebuild
				gamebuild.offsetPatches = append(gamebuild.offsetPatches, OffsetPatch {
					description:    patchData[0],
					offset:         offset,
					patchByteArray: patchBytes,
				})
			}
		}
	}

	return gamebuild, nil
}

// Program version: to be set with -ldflags="-x 'main.Version=vX.X.X'"
var Version = "dev"

// Main function
func main() {
	fmt.Printf("EternalPatcherGo %s by PowerBall253 :)\n\n", Version)

	// Print usage
	if len(os.Args) == 1 {
		fmt.Println("Usage:")
		fmt.Printf("%s [--update] [--patch /path/to/DOOMEternalx64vk.exe]\n\n", os.Args[0])
		fmt.Println("--update\tUpdates the patch definitions file using the update server specified in the config file.")
		fmt.Println("--patch\t\tPatches the executable in the path given.")
		os.Exit(1)
	}

	// Check program arguments
	update := false
	patch := false
	exePath := ""
	for i, arg := range os.Args {
		if arg == "--patch" {
			// Get executable path
			if i > len(os.Args) - 2 {
				fmt.Fprintln(os.Stderr, "ERROR: No executable was specified for patching.")
				os.Exit(1)
			}
			patch = true
			exePath = os.Args[i + 1]

			// Check if executable exists
			if _, err := os.Stat(exePath); err != nil {
				fmt.Fprintf(os.Stderr, "ERROR: Could not access executable path: %s\n", err.Error())
				os.Exit(1)
			}
		} else if arg == "--update" {
			update = true
		}
	}

	// Make sure an operation was specified
	if !update && !patch {
		fmt.Fprintf(os.Stderr, "ERROR: Unrecognized options! Run %s to see all possible arguments.\n", os.Args[0])
		os.Exit(1)
	}

	// Update patch definitions
	var updateServer string
	var err error
	if update {
		fmt.Println("Checking for updates...")

		// Get the update server
		updateServer, err = getUpdateServer()
		if err != nil {
			fmt.Fprintf(os.Stderr, "ERROR: Failed to get update server from config: %s\n", err.Error())
			os.Exit(1)
		}

		// Get latest patch definitions MD5
		latestMD5, err := getLatestPatchDefsMD5(updateServer)
		if err != nil {
			fmt.Fprintf(os.Stderr, "ERROR: Failed to get latest patch definitions' MD5: %s\n", latestMD5)
		}

		// Get current patch definitions MD5
		currentMD5, err := getMD5Hash("EternalPatcher.def")

		// Check if files have equal hash
		if err != nil || currentMD5 != latestMD5 {
			// Files aren't equal, download patch definitions
			fmt.Println("Downloading latest patch definitions...")
			err = downloadPatchDefs(updateServer)
			if err != nil {
				fmt.Fprintf(os.Stderr, "ERROR: Failed to download latest patch definitions: %s\n", err.Error())
				os.Exit(1)
			}
			fmt.Println("Done.")
		} else {
			// Files are equal
			fmt.Println("No updates available.")
		}

		if !patch {
			// Nothing left to do, exit
			os.Exit(0)
		}
	}

	// Load patches
	fmt.Println("\nLoading patch definitions file...")

	// Get exe MD5
	exeMD5, err := getMD5Hash(exePath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "ERROR: Failed to get the executable's MD5 hash: %s\n", err.Error())
		os.Exit(1)
	}

	// Get game build with patches
	gamebuild, err := loadPatchDefs(exeMD5)
	if err != nil {
		fmt.Fprintf(os.Stderr, "ERROR: Failed to load patch definitions: %s\n", err.Error())
		os.Exit(1)
	}
	fmt.Printf("%s detected.\n", gamebuild.id)

	// Check if any patches were loaded
	if len(gamebuild.offsetPatches) == 0 && len(gamebuild.patternPatches) == 0 {
		fmt.Fprintln(os.Stderr, "ERROR: Failed to find patches for the provided executable.")
		os.Exit(1)
	}

	// Apply patches to exe
	fmt.Println("\nApplying patches...")
	patchingResult := applyPatches(exePath, gamebuild.patternPatches, gamebuild.offsetPatches)
	successes := 0
	for _, result := range patchingResult {
		if result.success {
			successes++
			fmt.Printf("%s: Success\n", result.description)
		} else {
			fmt.Printf("%s: Failure\n", result.description)
		}
	}

	// Exit
	fmt.Printf("\n%d out of %d applied.\n", successes, len(patchingResult))
	if successes != len(patchingResult) {
		os.Exit(1)
	}
}
