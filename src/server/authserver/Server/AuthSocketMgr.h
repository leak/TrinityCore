/*
 * Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
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

#ifndef AuthSocketMgr_h__
#define AuthSocketMgr_h__

#include "SocketMgr.h"
#include "AuthSocket.h"

class AuthSocketMgr : public SocketMgr<AuthSocket>
{

public:
    static AuthSocketMgr& Instance()
    {
        static AuthSocketMgr instance;
        return instance;
    }

    /// Start network, listen at address:port .
    bool StartNetwork(std::string const& bindIp, uint16 port) override
    {
        SocketMgr::StartNetwork(bindIp, port);

        return true;
    }

    /// Stops listener
    void StopNetwork() override
    {
        SocketMgr::StopNetwork();
    }

    // void OnSocketOpen(uv_tcp_t* socket) override;

    // AuthSocketMgr();

};

#define sAuthSocketMgr AuthSocketMgr::Instance()

#endif // AuthSocketMgr_h__
