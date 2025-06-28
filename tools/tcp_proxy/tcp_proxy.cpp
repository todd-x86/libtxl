#include <txl/socket.h>
#include <txl/event_poller.h>
#include <iostream>
#include <map>
#include <memory>

static int count_ = 0;

struct proxy_client final
{
    int id_;
    txl::socket local;
    txl::socket remote;
    txl::byte_vector shared_buf{};

    proxy_client(txl::socket && local, txl::socket && remote)
        : id_{count_++}
        , local{std::move(local)}
        , remote{std::move(remote)}
    {
        //this->local.set_nonblocking(true).or_throw();
        //this->remote.set_nonblocking(true).or_throw();
        shared_buf.resize(2048);
        printf("PROXY CLIENT CREATED: %d\n", id_);
    }

    ~proxy_client()
    {
        printf("PROXY CLIENT DIED: %d\n", id_);
    }

    auto close() -> void
    {
        remote.shutdown().or_throw();
        remote.close().or_throw();
        local.shutdown().or_throw();
        local.close().or_throw();
    }

    auto local_to_remote() -> bool
    {
        printf("READING FROM LOCAL...\n");
        auto bytes_read = local.read(shared_buf).or_throw();
        if (bytes_read.empty())
        {
            return false;
        }
        printf("READ %zu FROM LOCAL\n", bytes_read.size());
        remote.write(bytes_read).or_throw();
        printf("WROTE %zu TO REMOTE\n", bytes_read.size());
        return true;
    }
    
    auto remote_to_local() -> bool
    {
        printf("READING FROM REMOTE...\n");
        auto bytes_read = remote.read(shared_buf).or_throw();
        if (bytes_read.empty())
        {
            return false;
        }
        printf("READ %zu FROM REMOTE\n", bytes_read.size());
        local.write(bytes_read).or_throw();
        printf("WROTE %zu TO LOCAL\n", bytes_read.size());
        return true;
    }
};

int main(int argc, char * argv[])
{
    txl::socket listener{txl::socket::internet, txl::socket::stream};
    listener.bind(txl::socket_address{"0.0.0.0", 8001}).or_throw();
    listener.listen(50).or_throw();

    txl::event_poller ep{};
    ep.open().or_throw();

    txl::event_tag et{};
    et.fd(listener.fd());
    ep.add(listener.fd(), txl::event_type::in, et).or_throw();

    std::map<int, std::shared_ptr<proxy_client>> client_fd_to_socket{};

    txl::event_array<32> evts{};
    int num_evts = 0;
    while ((num_evts = ep.poll(evts).or_value(-1)) != -1)
    {
        printf("%d events\n", num_evts);
        for (auto i = 0; i < num_evts; ++i)
        {
            printf(" --> %d event (fd=%d)\n", static_cast<int>(evts[i].events()), evts[i].fd());
            if (evts[i].fd() != listener.fd())
            {
                std::cout << (evts[i].has_one_of(txl::event_type::in) ? "READ" : "WRITE") << " DATA FROM CLIENT (fd=" << evts[i].fd() << ", e=" << evts[i].events() << ")" << std::endl;
                auto p_client_it = client_fd_to_socket.find(evts[i].fd());
                if (p_client_it == client_fd_to_socket.end())
                {
                    printf("SKIPPED\n");
                    continue;
                }

                auto p_client = p_client_it->second;
                auto is_remote = evts[i].fd() == p_client->remote.fd();
                auto is_local = evts[i].fd() == p_client->local.fd();

                if (is_remote and evts[i].has_one_of(txl::event_type::in))
                {
                    if (not p_client->remote_to_local())
                    {
                        ep.remove(p_client->local.fd());
                        ep.remove(p_client->remote.fd());
                        client_fd_to_socket.erase(p_client->local.fd());
                        client_fd_to_socket.erase(p_client->remote.fd());
                        p_client->close();
                        printf("CLOSING CLIENT REMOTE\n");
                        continue;
                    }
                }
                else if (is_local and evts[i].has_one_of(txl::event_type::in))
                {
                    if (not p_client->local_to_remote())
                    {
                        ep.remove(p_client->local.fd());
                        ep.remove(p_client->remote.fd());
                        client_fd_to_socket.erase(p_client->local.fd());
                        client_fd_to_socket.erase(p_client->remote.fd());
                        p_client->close();
                        printf("CLOSING CLIENT LOCAL\n");
                        continue;
                    }
                }
                // Client activity
                /*if (is_remote)
                {
                    if (not p_client->remote_to_local())
                    {
                        ep.remove(p_client->local.fd());
                        ep.remove(p_client->remote.fd());
                        client_fd_to_socket.erase(p_client->local.fd());
                        client_fd_to_socket.erase(p_client->remote.fd());
                        p_client->close();
                        printf("CLOSING CLIENT REMOTE\n");
                        continue;
                    }
                }
                else //if (not is_remote)
                {
                    if (not p_client->remote_to_local())
                    {
                        ep.remove(p_client->local.fd());
                        ep.remove(p_client->remote.fd());
                        client_fd_to_socket.erase(p_client->local.fd());
                        client_fd_to_socket.erase(p_client->remote.fd());
                        p_client->close();
                        printf("CLOSING CLIENT REMOTE\n");
                        continue;
                    }
                }*/
                continue;
            }

            if (evts[i].fd() == listener.fd())
            {
                // New client
                auto client_socket = listener.accept().or_throw();
                auto client_fd = client_socket.fd();
                txl::event_tag et{};
                et.fd(client_fd);
                ep.add(client_fd, txl::event_type::in | txl::event_type::error | txl::event_type::hangup, et).or_throw();

                txl::socket remote_socket{txl::socket::internet, txl::socket::stream};
                remote_socket.connect(txl::socket_address{argv[1], 8000}).or_throw();
                auto remote_fd = remote_socket.fd();

                et.fd(remote_fd);
                ep.add(remote_fd, txl::event_type::in | txl::event_type::error | txl::event_type::hangup, et).or_throw();

                auto p_client = std::make_shared<proxy_client>(std::move(client_socket), std::move(remote_socket));
                client_fd_to_socket.emplace(client_fd, p_client);
                client_fd_to_socket.emplace(remote_fd, p_client);
                printf("ACCEPT NEW CLIENT (local=%d, remote=%d)\n", client_fd, remote_fd);
                continue;
            }
        }
    }
}
