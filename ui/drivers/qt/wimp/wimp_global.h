/* RetroArch - A frontend for libretro.
 *  Copyright (C) 2011-2015 - Andres Suarez
 *
 * RetroArch is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with RetroArch.
 * If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef WIMP_GLOBAL_H
#define WIMP_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(WIMP_LIBRARY)
#  define WIMPSHARED_EXPORT Q_DECL_EXPORT
#else
#  define WIMPSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // WIMP_GLOBAL_H
