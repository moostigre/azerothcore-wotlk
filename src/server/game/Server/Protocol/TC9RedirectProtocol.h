/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TC9_REDIRECT_PROTOCOL_H
#define _TC9_REDIRECT_PROTOCOL_H

#include "Define.h"

namespace TC9Redirect
{
// The original request and response contain no version information:
//   request:  empty
//   response: uint8 status
// Version 2 adds option negotiation while retaining the status as the first response byte:
//   request:  uint8 version, uint8 requestedOptions
//   response: uint8 status, uint8 version, uint8 acceptedOptions
constexpr uint8 VersionedRequest = 2;
constexpr uint8 OptionSeamless = 1 << 0;
constexpr uint8 SupportedOptions = OptionSeamless;
}

#endif // _TC9_REDIRECT_PROTOCOL_H
