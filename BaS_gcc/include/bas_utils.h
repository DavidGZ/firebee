/*
 * bas_utils.h
 *
 * This file is part of BaS_gcc.
 *
 * BaS_gcc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * BaS_gcc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with BaS_gcc.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Created on: 26.02.2013
 *      Author: Markus Fröschle
 */

#ifndef _BAS_UTILS_H_
#define _BAS_UTILS_H_

#define CLEAR_BIT(p,bit)        p &= ~(bit)
#define CLEAR_BIT_NO(p,nr)      CLEAR_BIT(p, (1 << (nr)))

#endif /* _BAS_UTILS_H_ */
