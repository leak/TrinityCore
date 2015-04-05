/*
 * Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2008  MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Config.h"
#include "ScriptMgr.h"
#include "WorldSocket.h"
#include "WorldSocketMgr.h"

static void OnSocketAccept(uv_tcp_t* socket)
{
    sWorldSocketMgr.OnSocketOpen(socket);
}

WorldSocketMgr::WorldSocketMgr() : BaseSocketMgr(), _socketSendBufferSize(-1), m_SockOutUBuff(65536), _tcpNoDelay(true)
{
}

bool WorldSocketMgr::StartNetwork(std::string const& bindIp, uint16 port)
{
    _tcpNoDelay = sConfigMgr->GetBoolDefault("Network.TcpNodelay", true);

    // -1 means use default
    _socketSendBufferSize = sConfigMgr->GetIntDefault("Network.OutKBuff", -1);

    // TODO: Unused at this moment
    m_SockOutUBuff = sConfigMgr->GetIntDefault("Network.OutUBuff", 65536);

    if (m_SockOutUBuff <= 0)
    {
        TC_LOG_ERROR("misc", "Network.OutUBuff is wrong in your config file");
        return false;
    }

    BaseSocketMgr::StartNetwork(bindIp, port);

    sScriptMgr->OnNetworkStart();
    return true;
}

void WorldSocketMgr::StopNetwork()
{
    BaseSocketMgr::StopNetwork();

    sScriptMgr->OnNetworkStop();
}

void WorldSocketMgr::OnSocketOpen(uv_tcp_t* socket)
{
    // set some options here
    if (_socketSendBufferSize >= 0)
    {
        int status = uv_send_buffer_size(reinterpret_cast<uv_handle_t*>(socket), &_socketSendBufferSize);

        if (status > 0)
        {
            TC_LOG_ERROR("misc", "WorldSocketMgr::OnSocketOpen uv_send_buffer_size err = %s", uv_strerror(status));
            return;
        }
    }

    // Set TCP_NODELAY.
    if (_tcpNoDelay)
    {
        int status = uv_tcp_nodelay(socket, _tcpNoDelay);

        if (status > 0)
        {
            TC_LOG_ERROR("misc", "WorldSocketMgr::OnSocketOpen uv_tcp_nodelay err = %s", uv_strerror(status));
            return;
        }
    }

    BaseSocketMgr::OnSocketOpen(socket);
}


