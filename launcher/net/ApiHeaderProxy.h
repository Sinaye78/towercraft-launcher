// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
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
 */

#pragma once

#include "Application.h"
#include "BuildConfig.h"
#include "net/HeaderProxy.h"

namespace Net {

/** No mod-platform API needs bespoke headers anymore (TowerCraft only talks to Mojang/Fabric meta
 * servers, GitHub, and its own manifest host) - kept as an attachable no-op so ApiDownload/ApiUpload
 * don't need to special-case "no header proxy". */
class ApiHeaderProxy : public HeaderProxy {
   public:
    ApiHeaderProxy() = default;
    ~ApiHeaderProxy() override = default;

   public:
    QList<HeaderPair> headers(const QNetworkRequest& request) const override
    {
        Q_UNUSED(request);
        return {};
    };
};

}  // namespace Net
