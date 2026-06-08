#include <txl/multicast_socket.h>
#include <txl/option_parser.h>
#include <txl/event_poller.h>
#include <txl/event_timer.h>
#include <iostream>

int main(int argc, char * argv[])
{
    txl::option_parser opts{};
    std::string addr_str, send_payload;
    int port, send_delay = 1000;
    opts.add_flag('a', addr_str);
    opts.add_flag('p', port);
    opts.add_flag('s', send_payload);
    opts.add_flag('d', send_delay);
    opts.parse_or_exit(argc, const_cast<char const **>(argv));

    txl::event_poller ep{};
    ep.open().or_throw();
    txl::event_timer timer{};

    if (not send_payload.empty())
    {
        std::cout << "Sending \"" << send_payload << "\" every " << send_delay << "ms to " << addr_str << ":" << port << std::endl;
        timer.open(txl::event_timer::steady_clock).or_throw();
        timer.set_time(std::chrono::steady_clock::now() + std::chrono::milliseconds{send_delay}, std::chrono::milliseconds{send_delay}).or_throw();
        ep.add(timer.fd(), txl::event_type::in).or_throw();
    }

    txl::multicast_socket sock{};
    sock.open().or_throw();
    sock.set_option(txl::socket_option::reuse_port, 1).or_throw();
    sock.set_option(txl::socket_option::reuse_address, 1).or_throw();
    sock.join_multicast(txl::socket_address{addr_str}).or_throw();
    sock.bind(txl::socket_address{static_cast<uint16_t>(port)}).or_throw();
    ep.add(sock.fd(), txl::event_type::in).or_throw();
    txl::socket_address addr{};
    char buf[2000];
    txl::event_array<10> evts{}; 
    while (true)
    {
        auto num_events = ep.poll(evts).or_throw();
        for (auto event_index = 0; event_index < num_events; ++event_index)
        {
            auto active_fd = evts[event_index].fd();
            if (active_fd == timer.fd())
            {
                timer.read().or_throw();
                if (not send_payload.empty())
                {
                    sock.send_to(send_payload, txl::socket_address{addr_str, static_cast<uint16_t>(port)}).or_throw();
                }
                continue;
            }

            if (active_fd == sock.fd())
            {
                auto r = sock.recv_from(buf, addr).or_throw();
                if (r.size())
                {
                    std::cout << "RECV (" << r.size() << " bytes): ";
                    for (auto c : r)
                    {
                        if (static_cast<int>(c) < 32 or static_cast<int>(c) > 127)
                        {
                            std::cout << '.';
                        }
                        else
                        {
                            std::cout << static_cast<char>(c);
                        }
                    }
                    std::cout << "\n";
                }
                continue;
            }
        }
    }
}
