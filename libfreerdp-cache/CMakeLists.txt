# FreeRDP: A Remote Desktop Protocol Client
# libfreerdp-cache cmake build script
#
# Copyright 2011 O.S. Systems Software Ltda.
# Copyright 2011 Otavio Salvador <otavio@ossystems.com.br>
# Copyright 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(FREERDP_CACHE_SRCS
	brush.c
	brush.h
	bitmap_v2.c
	bitmap_v2.h
	offscreen.c
	offscreen.h
	color_table.c
	color_table.h
	cache.c
	cache.h)

add_library(freerdp-cache SHARED ${FREERDP_CACHE_SRCS})

set_target_properties(freerdp-cache PROPERTIES VERSION ${FREERDP_VERSION_FULL} SOVERSION ${FREERDP_VERSION})

install(TARGETS freerdp-cache DESTINATION lib)

