# Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

file(GLOB sources_libuv src/*.c src/*.h include/*.h)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if( UNIX )
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/unix)
  file(GLOB sources_libuv_unix src/unix/*.c src/unix/*.h)
  list(APPEND sources_libuv ${sources_libuv_unix})
elseif( WIN32 )
  add_definitions(-DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=0x0600)
  
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/win)
  file(GLOB sources_libuv_win src/win/*.c src/win/*.h)
  list(APPEND sources_libuv ${sources_libuv_win})
endif()

add_library(libuv STATIC ${sources_libuv})

target_link_libraries(libuv advapi32.lib iphlpapi.lib psapi.lib shell32.lib ws2_32.lib)