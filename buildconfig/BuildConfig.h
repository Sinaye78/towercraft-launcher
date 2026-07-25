// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#pragma once
#include <QList>
#include <QString>

/**
 * \brief The Config class holds all the build-time information passed from the build system.
 */
class Config {
   public:
    Config();
    QString LAUNCHER_NAME;
    QString LAUNCHER_APP_BINARY_NAME;
    QString LAUNCHER_DISPLAYNAME;
    QString LAUNCHER_COPYRIGHT;
    QString LAUNCHER_DOMAIN;
    QString LAUNCHER_CONFIGFILE;
    QString LAUNCHER_GIT;
    QString LAUNCHER_APPID;
    QString LAUNCHER_SVGFILENAME;
    QString LAUNCHER_ENVNAME;

    /// The major version number.
    int VERSION_MAJOR;
    /// The minor version number.
    int VERSION_MINOR;
    /// The patch version number.
    int VERSION_PATCH;

    /**
     * The version channel
     * This is used by the updater to determine what channel the current version came from.
     */
    QString VERSION_CHANNEL;

    bool UPDATER_ENABLED = false;
    bool JAVA_DOWNLOADER_ENABLED = false;

    /// A short string identifying this build's platform or distribution.
    QString BUILD_PLATFORM;

    /// A short string identifying this build's valid artifacts int he updater. For example, "lin64" or "win32".
    QString BUILD_ARTIFACT;

    /// A string containing the build timestamp
    QString BUILD_DATE;

    /// A string identifying the compiler use to build
    QString COMPILER_NAME;

    /// A string identifying the compiler version used to build
    QString COMPILER_VERSION;

    /// A string identifying the compiler target system os
    QString COMPILER_TARGET_SYSTEM;

    /// A String identifying the compiler target system version
    QString COMPILER_TARGET_SYSTEM_VERSION;

    /// A String identifying the compiler target processor
    QString COMPILER_TARGET_SYSTEM_PROCESSOR;

    /// URL for the updater's channel
    QString UPDATER_GITHUB_REPO;

    /// The public key used to sign releases for the Sparkle updater appcast
    QString MAC_SPARKLE_PUB_KEY;

    /// URL for the Sparkle updater's appcast
    QString MAC_SPARKLE_APPCAST_URL;

    /// User-Agent to use.
    QString USER_AGENT;

    /// The git commit hash of this build
    QString GIT_COMMIT;

    /// The git tag of this build
    QString GIT_TAG;

    /// The git refspec of this build
    QString GIT_REFSPEC;

    /**
     * This is used to fetch the news RSS feed.
     * It defaults in CMakeLists.txt to "https://multimc.org/rss.xml"
     */
    QString NEWS_RSS_URL;

    /**
     * URL that gets opened when the user clicks "More News"
     */
    QString NEWS_OPEN_URL;

    /**
     * URL that gets opened when the user clicks 'Launcher Help'
     */
    QString WIKI_URL;

    /**
     * URL (with arg %1 to be substituted with page-id) that gets opened when the user requests help in a dialog window
     */
    QString HELP_URL;

    /**
     * URL that gets opened when the user succesfully logins.
     */
    QString LOGIN_CALLBACK_URL;

    /**
     * Client ID you can get from Imgur when you register an application
     */
    QString IMGUR_CLIENT_ID;

    /**
     * Client ID you can get from Microsoft Identity Platform when you register an application
     */
    QString MSA_CLIENT_ID;

    /**
     * Metadata repository URL prefix
     */
    QString META_URL;

    /**
     * URL to the manifest.json (name/size/sha256/url per file) that drives TowerCraft's mod-file
     * sync. Blank until a real repo exists (see DECISIONS.md) - the sync task no-ops cleanly when
     * empty rather than failing.
     */
    QString TOWERCRAFT_MANIFEST_URL;

    /**
     * Exact Minecraft version and Fabric Loader version the single TowerCraft instance is
     * auto-provisioned with. Pinned exactly (not "recommended") to match what the TowerCraft mod
     * jar actually requires - see the mod project's fabric.mod.json/gradle.properties.
     */
    QString TOWERCRAFT_MINECRAFT_VERSION;
    QString TOWERCRAFT_FABRIC_LOADER_VERSION;

    QString GLFW_LIBRARY_NAME;
    QString OPENAL_LIBRARY_NAME;

    QString BUG_TRACKER_URL;
    QString TRANSLATIONS_URL;
    QString MATRIX_URL;
    QString DISCORD_URL;
    QString SUBREDDIT_URL;

    QString DEFAULT_RESOURCE_BASE = "https://resources.download.minecraft.net/";
    QString LIBRARY_BASE = "https://libraries.minecraft.net/";
    QString IMGUR_BASE_URL = "https://api.imgur.com/3/";
    QString LEGACY_FMLLIBS_BASE_URL;
    QString TRANSLATION_FILES_URL;

    QString versionString() const;
    /**
     * \brief Converts the Version to a string.
     * \return The version number in string format (major.minor.revision.build).
     */
    QString printableVersionString() const;

    /**
     * \brief Compiler ID String
     * \return a string of the form "Name - Version"  of just "Name" if the version is empty
     */
    QString compilerID() const;

    /**
     * \brief System ID String
     * \return a string of the form "OS Verison Processor"
     */
    QString systemID() const;
};

extern const Config BuildConfig;
