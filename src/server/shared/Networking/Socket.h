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

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "MessageBuffer.h"
#include <uv.h>

#define READ_BLOCK_SIZE 4096
#include "SocketMgr.h"


template<class T>
class Socket
{
public:
    explicit Socket(uv_tcp_t* socket, SocketMgr<T>* socketMgr) : _socket(socket), _readBuffer(), _socketMgr(socketMgr)
    {
        _readBuffer.Resize(READ_BLOCK_SIZE);

        struct sockaddr_in name;
        int namelen = sizeof(name);
        uv_tcp_getpeername(_socket, reinterpret_cast<struct sockaddr*>(&name), &namelen);

        _remotePort = name.sin_port;

        char addr[16];
        uv_inet_ntop(AF_INET, &name.sin_addr, addr, sizeof(addr));
        _remoteAddress = std::string(addr);

        _socket->data = static_cast<void*>(this);
    }

    virtual ~Socket()
    {
        CloseSocket();

        delete _socket;
    }

    virtual void Start() = 0;

    const std::string& GetRemoteIpAddress() const { return _remoteAddress; }

    uint16 GetRemotePort() const { return _remotePort; }

    void SendPacket(MessageBuffer* buffer)
    {
        if (!IsOpen())
            return;

        uv_buf_t bufs[] = { uv_buf_t() };

        bufs[0].base = reinterpret_cast<char*>(buffer->GetReadPointer());
        bufs[0].len = static_cast<size_t>(buffer->GetBufferSize());

        auto request = new uv_write_t();

        request->data = static_cast<void*>(buffer);

        int writeStatus = _socketMgr->QueueWriteRequest(request, reinterpret_cast<uv_stream_t*>(_socket), bufs, 1, [](uv_write_t* req, int status)
        {
            if (status > 0)
            {
                TC_LOG_ERROR("network", "Socket::SendPacket callback failed. (%s)", uv_strerror(status));
                auto socket = static_cast<Socket<T>*>(req->handle->data);
                socket->CloseSocket();
                return;
            }

            delete static_cast<MessageBuffer*>(req->data);
            delete req;
        });

        if (writeStatus > 0)
        {
            TC_LOG_ERROR("network", "Socket::SendPacket write failed. (%s)", uv_strerror(writeStatus));
            CloseSocket();
        }
    }

    void AsyncRead()
    {
        int status = uv_read_start(reinterpret_cast<uv_stream_t*>(_socket),
            [](uv_handle_t* uvSocket, size_t /*suggestedSize*/, uv_buf_t* buffer){

            auto socket = static_cast<Socket<T>*>(uvSocket->data);

            socket->GetReadBuffer().Normalize();
            socket->GetReadBuffer().EnsureFreeSpace();

            buffer->base = reinterpret_cast<char*>(socket->GetReadBuffer().GetWritePointer());
            buffer->len = socket->GetReadBuffer().GetRemainingSpace();
        },
            [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf){

            auto socket = static_cast<Socket<T>*>(stream->data);

            socket->GetReadBuffer().WriteCompleted(nread);

            socket->ReadHandler();
        });

        if (status > 0)
        {
            TC_LOG_ERROR("network", "Socket::AsyncRead failed. (%s)", uv_strerror(status));
            CloseSocket();
        }
    }

    bool IsOpen() const { return !uv_is_closing(reinterpret_cast<uv_handle_t*>(_socket)) || !uv_is_writable(reinterpret_cast<uv_stream_t*>(_socket)); }

    void CloseSocket()
    {
        if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(_socket)))
        {
            uv_close(reinterpret_cast<uv_handle_t*>(_socket), nullptr);

            OnClose();
        }
    }

    void DelayedCloseSocket()
    {
        uv_shutdown(new uv_shutdown_t, reinterpret_cast<uv_stream_t*>(_socket), [](uv_shutdown_t* request, int status)
        {
            if (status > 0)
            {
                TC_LOG_ERROR("network", "Socket::DelayedCloseSocket shutdown callback error (%s)", uv_strerror(status));
            }
            
            auto socket = static_cast<Socket<T>*>(request->handle->data);

            socket->CloseSocket();

            delete request;
        });
    }

    MessageBuffer& GetReadBuffer() { return _readBuffer; }

protected:
    virtual void OnClose() { }

    virtual void ReadHandler() = 0;

private:

    std::string _remoteAddress;

    uv_tcp_t *_socket;

    uint16 _remotePort;

    MessageBuffer _readBuffer;

    SocketMgr<T>* _socketMgr;
};

#endif // __SOCKET_H__
