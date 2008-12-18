/* Egoboo - menu.h
 * Implements the main menu tree, using the code in Ui.*.  This could probably
 * just go in proto.h.
 */

/*
   This file is part of Egoboo.

   Egoboo is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Egoboo is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
*/

#ifndef egoboo_Menu_h
#define egoboo_Menu_h

void menu_frameStep();

static int selectedModule = 0;

static int waitingforinput = -1;

#endif // include guard
