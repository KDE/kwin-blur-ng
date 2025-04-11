// SPDX-FileCopyrightText: Copyright (c) 2022 MBition GmbH.
// SPDX-License-Identifier: GPL-2.0-or-later

#include "blur.h"

namespace KWin
{

KWIN_EFFECT_FACTORY_SUPPORTED(BlurNGEffect,
                              "metadata.json",
                              return BlurNGEffect::supported();)

} // namespace KWin

#include "main.moc"
