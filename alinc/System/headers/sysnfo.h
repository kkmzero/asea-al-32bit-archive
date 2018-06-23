/*
 * This file is part of Asea OS.
 * Copyright (C) 2018 Ivan Kmeťo
 *
 * Asea OS is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Asea OS is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Asea OS.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __ASEA__SYSTEM__HEADERS__SYSNFO_H
#define __ASEA__SYSTEM__HEADERS__SYSNFO_H

#include <common/types.h>

namespace asea
{
	namespace System
	{
		namespace headers
		{
			class AseaSystemInfo
			{
			public:
				AseaSystemInfo();
				~AseaSystemInfo();
				
				void AS_PrintSysInfoMsg(asea::common::uint16_t sysinfomsg_ID);
			};

		}
	}
}

#endif