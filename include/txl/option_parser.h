#pragma once

#include <txl/iterator_view.h>

#include <functional>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace txl
{
    class option_parser final
    {
    private:
        struct option final
        {
            enum value_type
            {
                str_value,
                bool_value,
            };

            value_type val_type;
            union
            {
                std::string * str_val;
                bool * bool_val;
            };

            option(std::string & dst)
                : val_type{str_value}
                , str_val{&dst}
            {
            }

            option(bool & dst)
                : val_type{bool_value}
                , bool_val{&dst}
            {
            }

            auto is_flag() const -> bool
            {
                return val_type == bool_value;
            }
        };

        std::map<char, option> opts_{};

        auto process_option(option & opt, char const ** & arg_iter) -> void
        {
            switch (opt.val_type)
            {
                case option::str_value:
                    *opt.str_val = std::string{*arg_iter};
                    ++arg_iter;
                    break;
                case option::bool_value:
                    *opt.bool_val = true;
                    break;
            }
        }
    public:
        using UnknownArgFunc = std::function<void(std::string_view)>;

        auto add_flag(char flag, bool & value) -> void
        {
            value = false;
            auto [_, emplaced] = opts_.emplace(flag, option{value});
            if (not emplaced)
            {
                throw std::runtime_error{"flag already added"};
            }
        }
        
        auto add_flag(char flag, std::string & value) -> void
        {
            auto [_, emplaced] = opts_.emplace(flag, option{value});
            if (not emplaced)
            {
                throw std::runtime_error{"flag already added"};
            }
        }

        auto get_usage_string(std::string_view exe) const -> std::string
        {
            std::ostringstream ss{};
            ss << "Usage: " << exe;
            for (auto const & item : opts_)
            {
                ss << " [ -" << item.first;
                switch (item.second.val_type)
                {
                    case option::str_value:
                        ss << " <str>";
                        break;
                    case option::bool_value:
                        // Deliberately empty
                        break;
                }
                ss << " ]";
            }
            return ss.str();
        }

        auto parse(int argc, char const * argv[], UnknownArgFunc && on_unknown_arg = nullptr) -> void
        {
            parse(make_iterator_view(&argv[1], &argv[argc]), std::move(on_unknown_arg));
        }

        auto parse(iterator_view<char const **> const & args, UnknownArgFunc && on_unknown_arg = nullptr) -> void
        {
            auto it = args.begin();
            while (it != args.end())
            {
                std::string_view arg{*it};
                if (arg.size() != 2 or arg[0] != '-')
                {
                    if (on_unknown_arg)
                    {
                        on_unknown_arg(arg);
                    }
                    ++it;
                    continue;
                }

                auto flag_name = arg[1];
                if (auto flag_iter = opts_.find(flag_name); flag_iter != opts_.end())
                {
                    ++it;
                    if (it == args.end() and not flag_iter->second.is_flag())
                    {
                        throw std::runtime_error{"Argument required"};
                    }
                    process_option(flag_iter->second, it);
                    continue;
                }

                if (on_unknown_arg)
                {
                    on_unknown_arg(arg);
                }
                ++it;
            }
        }
    };
}
