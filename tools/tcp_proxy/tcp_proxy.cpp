#include <txl/socket.h>
#include <txl/event_poller.h>
#include <iostream>
#include <map>
#include <memory>

struct proxy_client final
{
    txl::tcp_socket local;
    txl::tcp_socket remote;
    txl::byte_vector shared_buf{};

    proxy_client(txl::tcp_socket && local, txl::tcp_socket && remote)
        : local{std::move(local)}
        , remote{std::move(remote)}
    {
        this->local.set_nonblocking(true).or_throw();
        this->remote.set_nonblocking(true).or_throw();
        shared_buf.resize(2048);
    }

    auto close() -> void
    {
        remote.shutdown();
        remote.close();
        local.shutdown();
        local.close();
    }

    auto local_to_remote() -> txl::result<txl::buffer_ref>
    {
        auto bytes_read_res = local.read(shared_buf);
        if (not bytes_read_res)
        {
            return bytes_read_res;
        }
        return remote.write(*bytes_read_res);
    }
    
    auto remote_to_local() -> txl::result<txl::buffer_ref>
    {
        auto bytes_read_res = remote.read(shared_buf);
        if (not bytes_read_res)
        {
            return bytes_read_res;
        }
        return local.write(*bytes_read_res);
    }
};

int main(int argc, char * argv[])
{
    txl::tcp_socket listener{true};
    listener.bind(txl::socket_address{"0.0.0.0", 8001}).or_throw();
    listener.listen(50).or_throw();

    txl::event_poller ep{};
    ep.open().or_throw();

    ep.add(listener.fd(), txl::event_type::in, txl::event_tag::from_fd(listener.fd())).or_throw();

    std::map<int, std::shared_ptr<proxy_client>> client_fd_to_socket{};

    txl::event_array<32> evts{};
    int num_evts = 0;
    while ((num_evts = ep.poll(evts).or_value(-1)) != -1)
    {
        for (auto i = 0; i < num_evts; ++i)
        {
            if (evts[i].fd() != listener.fd())
            {
                auto p_client_it = client_fd_to_socket.find(evts[i].fd());
                if (p_client_it == client_fd_to_socket.end())
                {
                    continue;
                }

                auto p_client = p_client_it->second;
                auto is_remote = evts[i].fd() == p_client->remote.fd();
                auto is_local = evts[i].fd() == p_client->local.fd();

                if (is_remote and evts[i].has_one_of(txl::event_type::in))
                {
                    auto res = p_client->remote_to_local();
                    if (not res)
                    {
                        ep.remove(p_client->local.fd());
                        ep.remove(p_client->remote.fd());
                        client_fd_to_socket.erase(p_client->local.fd());
                        client_fd_to_socket.erase(p_client->remote.fd());
                        p_client->close();
                        continue;
                    }
                }
                else if (is_local and evts[i].has_one_of(txl::event_type::in))
                {
                    auto res = p_client->local_to_remote();
                    if (not res)
                    {
                        ep.remove(p_client->local.fd());
                        ep.remove(p_client->remote.fd());
                        client_fd_to_socket.erase(p_client->local.fd());
                        client_fd_to_socket.erase(p_client->remote.fd());
                        p_client->close();
                        continue;
                    }
                }
                continue;
            }

            if (evts[i].fd() == listener.fd())
            {
                // New client
                auto client_socket = listener.accept<txl::tcp_socket>().or_throw();
                auto client_fd = client_socket.fd();
                ep.add(client_fd, txl::event_type::in | txl::event_type::error | txl::event_type::hangup, txl::event_tag::from_fd(client_fd)).or_throw();

                txl::tcp_socket remote_socket{true};
                remote_socket.connect(txl::socket_address{argv[1], 8000}).or_throw();
                auto remote_fd = remote_socket.fd();

                ep.add(remote_fd, txl::event_type::in | txl::event_type::error | txl::event_type::hangup, txl::event_tag::from_fd(remote_fd)).or_throw();

                auto p_client = std::make_shared<proxy_client>(std::move(client_socket), std::move(remote_socket));
                client_fd_to_socket.emplace(client_fd, p_client);
                client_fd_to_socket.emplace(remote_fd, p_client);
                continue;
            }
        }
    }
}
