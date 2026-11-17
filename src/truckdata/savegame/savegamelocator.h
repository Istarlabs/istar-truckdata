// SPDX-FileCopyrightText: 2026 Pascal Klose
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "truckdata/game.h"

#include <QDateTime>
#include <QList>
#include <QString>

namespace TruckData::SaveGame
{
    /// A profile of one of the games, and its newest save.
    struct SaveGameProfile
    {
        Game game = Game::Unknown;
        /// The name the player gave it; the folder name (the name in hex) when profile.sii cannot be read.
        QString name;
        /// The profile's folder.
        QString directory;
        /// The game.sii written last, of all the profile's saves (autosaves and those saved by hand).
        QString newestSave;
        QDateTime savedAt;
    };

    /// The profiles of both games in the user's Steam: those in the Steam Cloud (userdata/<id>/<app>/remote) and
    /// those kept locally in the Proton prefix (compatdata/<app>/.../Documents/<game>), in every Steam library.
    /// Newest save first.
    QList<SaveGameProfile> findProfiles();

    /// The same, below these Steam folders; for the tests.
    QList<SaveGameProfile> findProfiles(const QStringList& steamRoots);

    /// Where Steam may be: native, the old link to it, and the Flatpak.
    QStringList steamRoots();

    /// The Steam libraries of these Steam folders: the folders themselves and what their libraryfolders.vdf lists,
    /// each once by its canonical path.
    QStringList steamLibraries(const QStringList& steamRoots);

    /// A game's installed folder, "steamapps/common/<game>" in one of the libraries.
    struct GameInstall
    {
        Game game = Game::Unknown;
        QString directory;
    };

    /// The games installed in these Steam folders' libraries.
    QList<GameInstall> findGames(const QStringList& steamRoots);
} // namespace TruckData::SaveGame
