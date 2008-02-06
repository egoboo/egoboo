/* Egoboo - System.h
 * Interface to operating system dependant stuff.  Currently only in use to
 * use a higher performance timer than SDL provides on Windows machines.
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
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef egoboo_System_h
#define egoboo_System_h

int sys_frameStep();

void   sys_initialize(); // Allow any setup necessary for platform specific code
void   sys_shutdown();   // Allow any necessary cleanup for platform specific code
double sys_getTime();    // Return the current time, in seconds

#endif // include guard
