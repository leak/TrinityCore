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

#ifndef SocketMgr_h__
#define SocketMgr_h__

#include "uv.h"

template<class SocketType>
class SocketMgr
{
    std::thread _networkThread;

    uv_loop_t* _loop;

    std::mutex _loopGuard;

    uv_tcp_t _listener;

public:
    virtual ~SocketMgr() { }

    virtual bool StartNetwork(std::string const& bindIp, uint16 port)
    {
        sockaddr_in listenAddr;

        uv_ip4_addr(bindIp.c_str(), port, &listenAddr);

        uv_tcp_bind(&_listener, reinterpret_cast<const struct sockaddr*>(&listenAddr), 0);

        _listener.data = static_cast<void*>(this);

        int err = uv_listen(reinterpret_cast<uv_stream_t*>(&_listener), 128, &ListenCallback);

        if (err > 0)
        {
            TC_LOG_ERROR("network", "Error starting listener: %s", uv_strerror(err));
            return false;
        }

        _networkThread = std::thread([this]()
        {
            uv_run(this->_loop, UV_RUN_DEFAULT);
        });

        return true;
    }

    virtual void StopNetwork()
    {
        uv_stop(_loop);

        _networkThread.join();
    }

    virtual void OnSocketOpen(uv_tcp_t* socket)
    {

    }

    int QueueWriteRequest(uv_write_t* req,
        uv_stream_t* handle,
        const uv_buf_t bufs[],
        unsigned int nbufs,
        uv_write_cb cb)
    {
        std::unique_lock<std::mutex> guard(_loopGuard);

        return uv_write(req, handle, bufs, nbufs, cb);
    }

private:
    static void ListenCallback(uv_stream_t* listener, int status) {
        auto socketMgr = static_cast<SocketMgr<SocketType>*>(listener->data);

        if (status < 0)
        {
            TC_LOG_ERROR("network", "Error when accepting connection: %s", uv_strerror(status));
            return;
        }

        uv_tcp_t *clientSocket = new uv_tcp_t();

        uv_tcp_init(socketMgr->_loop, clientSocket);

        if (uv_accept(listener, reinterpret_cast<uv_stream_t*>(clientSocket)) == 0)
        {
            SocketType* socket = new SocketType(clientSocket, socketMgr);

            socketMgr->OnSocketOpen(clientSocket);

            socket->Start();
        }
        else
        {
            uv_close(reinterpret_cast<uv_handle_t*>(clientSocket), nullptr);
            delete clientSocket;
        }
    }

protected:
    SocketMgr()
    {
        _loop = uv_default_loop();

        uv_tcp_init(_loop, &_listener);
    }
};

#endif // SocketMgr_h__
