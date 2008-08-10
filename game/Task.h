/* Egoboo - Task.h
 * This code is not currently in use.
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
#ifndef egoboo_Task_h
#define egoboo_Task_h

typedef void (*TaskCallback)(float timeElapsed);

void task_register(const char *taskName, float timeInterval, TaskCallback f);
void task_remove(const char *taskName);
void task_pause(const char *taskName);
void task_play(const char *taskName);

void task_updateAllTasks();

#endif // include guard
