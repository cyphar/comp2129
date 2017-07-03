/*
 * Copyright (C) 2017 Aleksa Sarai <cyphar@cyphar.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Used to autogenerate code where the list of commands is required. While this
 * might not look very pretty at first glance, this makes command dispatching
 * extensible because the set of commands are not hardcoded anywhere. This idea
 * is based on how cgroups are defined in the Linux kernel <linux/cgroup_subsys.h>.
 */

CMD(HELP)
CMD(QUIT)
CMD(DISPLAY)
CMD(START)
CMD(PLACE)
CMD(UNDO)
CMD(STAT)
CMD(SAVE)
CMD(LOAD)
CMD(PLAYFROM)
