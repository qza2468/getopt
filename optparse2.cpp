//
// Created by root on 2021/5/23.
//

#include <cstring>
#include <cassert>
#include <iostream>
#include "optparse2.h"


OptParser::OptParser(int argc, char **argv, char *optstring,
                     option_Q *longopts)
                   : argc_t(argc), argv_t(argv),
                     optstring_t(optstring), longopts_t(longopts),
                     optind_t(1), nextchar_t(nullptr)
{}

int
OptParser::fetch(option_return &rt_opt)
{
    return getopt_int(rt_opt);
}

void
OptParser::reset(int argc, char **argv, char *optstr, option_Q *longopts_arg)
{
    argc_t = argc;
    argv_t = argv;
    optstring_t = optstr;
    longopts_t = longopts_arg;

    optind_t = 1;
    nextchar_t = nullptr;
}

void
OptParser::reset(int argc, char **argv)
{
    reset(argc, argv, optstring_t, longopts_t);
}

void
OptParser::reset(char *optstring, option_Q *longopts)
{
    reset(argc_t, argv_t, optstring, longopts);
}


void
OptParser::reset()
{
    reset(argc_t, argv_t, optstring_t, longopts_t);
}



size_t
OptParser::search_opts_lv(char c_srh)
{
    const struct option_Q *p = longopts_t;
    /// don't use `name`, it could be `nullptr` but `val must not
    while (p->val != 0 && p->val != c_srh)
    {
        p++;
    }

    return p->val != 0 ? p - longopts_t : -1;
}

size_t
OptParser::search_opts_ln(const char *name, int len)
{
    const struct option_Q *p = longopts_t;
    while (p->val != 0
    && (len != strlen(p->name) // essential condition to ensure that two string equal
    || strncmp(name, p->name, len) != 0))
        p++;

    return p->val != 0 ? p - longopts_t : -1;
}

int
OptParser::getOptInfo(char opt, option_Q &info_rst)
{

    const char *cPos = optstring_t == nullptr ? nullptr : strchr(optstring_t, opt);
    if (cPos != nullptr)
    {
        info_rst.name = nullptr;
        /// this will not lead to visit external memory cause
        /// if cPos[1] == ':' the next must be exist as '\0' or anything else
        /// cPos[1] the same.
        info_rst.has_arg = cPos[1] == ':' ? cPos[2] == ':'
                ? optional_argument : required_argument
                : no_argument;
        info_rst.val = opt;
        return 0;
    }

    size_t index_l = longopts_t == nullptr ? -1 : search_opts_lv(opt);
    if (index_l != -1)
    {
        info_rst.name = longopts_t[index_l].name;
        info_rst.val = opt;
        info_rst.has_arg = longopts_t[index_l].has_arg;
        return 0;
    }

    // the return `info_rst` remain unchange but user shouldn't depend on it
    return -1;
}

int
OptParser::getOptInfo(const char *opt_l, int len, option_Q &info_rst)
{

    size_t index_l = longopts_t == nullptr ? -1 : search_opts_ln(opt_l, len);
    if (index_l == -1)
    {
        return -1;
    }

    info_rst.name = longopts_t[index_l].name;
    info_rst.has_arg = longopts_t[index_l].has_arg;
    info_rst.val = longopts_t[index_l].val;

    return 0;
}

int
optArgLen_l(const char *opt_l)
{
    int i;
    for (i = 0; opt_l[i] != '\0' && opt_l[i] != '='; ++i) ;

    return i;
}


/*
 * The return value is the type of arg itself. not whether it is
 * in the `optstring_t` or `longopts_t`.
 */
int
OptParser::getopt_int(option_return &rt_opt)
{
    if (optind_t == 0)
        optind_t = 1;
    rt_opt.optarg = nullptr;

    if (optind_t == argc_t)
        return -1;

    // argv_t shouldn't change
    if (notOpt(argv_t[optind_t]))
    {
        rt_opt.optarg = argv_t[optind_t];
        rt_opt.name = nullptr;
        rt_opt.val = 1;
        rt_opt.name_len = -1;

        optind_t++;
        return nonopt_type;
    }
    else if (isShortOpt(argv_t[optind_t]))
    {
        // when an arg has been examined then `nextchar_t` couldn't be nullptr.
        // so we can see if one opt contains many options and this is a new arg
        // by examining `nextchar_t`
        const char *optPos = nextchar_t ? nextchar_t : &argv_t[optind_t][1];
        option_Q arg_info{};

        int res = getOptInfo(*optPos, arg_info);
        if (res == -1)
        {
            rt_opt.val = '?';
            rt_opt.name = nullptr;
            rt_opt.optopt = *optPos;
            rt_opt.optarg = nullptr;

            nextchar_t = optPos + 1;
            if (*nextchar_t == '\0')
            {
                optind_t++;
                nextchar_t = nullptr;
            }

            return short_type;
        }

        rt_opt.name = arg_info.name;
        if (rt_opt.name)
        {
            rt_opt.name_len = strlen(rt_opt.name);
        }
        rt_opt.val = arg_info.val;

        if (arg_info.has_arg)
        {
            if (optPos[1])
            {
                rt_opt.optarg = optPos + 1;
            }
            else if (argv_t[optind_t + 1] && notOpt(argv_t[optind_t + 1]))
            {
                rt_opt.optarg = argv_t[++optind_t];
            }

            optind_t++;
            nextchar_t = nullptr;

            if (rt_opt.optarg == nullptr
            && arg_info.has_arg == required_argument)
            {
                rt_opt.optopt = *optPos;
                rt_opt.val = ':';

                return short_type;
            }

            // optional and nonopt is the same type when facing no arg behind it
            return short_type;
        }
        else
        {
            // we don't want optPos change within the loop body.
            // so we shouldn't use ++;
            nextchar_t = optPos + 1;
            if (*nextchar_t == '\0')
            {
                optind_t++;
                nextchar_t = nullptr;
            }
            // rt_opt's `name`, `val`, `optarg` have been set already.
            return short_type;
        }
    }
    else if (isLongOpt(argv_t[optind_t]))
    {
        option_Q arg_info{};
        const char *optPos = argv_t[optind_t] + 2;

        int optLen = optArgLen_l(optPos);

        int res = getOptInfo(optPos, optLen, arg_info);
        if (res == -1)
        {
            rt_opt.name = optPos;
            rt_opt.optopt = 0; // cause no correspondent `val`.
            rt_opt.val = '?'; // use `name` == `nullptr` to identify long and short opt.
            rt_opt.name_len = optLen;

            if (optLen != strlen(optPos))
                rt_opt.optarg = optPos + optLen + 1;

            optind_t++;

            return long_type;
        }

        rt_opt.name = optPos;
        rt_opt.name_len = optLen;
        rt_opt.val = arg_info.val;
        rt_opt.optopt = 0;

        if (arg_info.has_arg)
        {
            if (optPos[optLen] == '=')
            {
                rt_opt.optarg = optPos + optLen + 1;
            }
            else if (optPos[optLen] == '\0' && optind_t + 1 < argc_t
            && notOpt(argv_t[optind_t + 1]))
            {
                rt_opt.optarg = argv_t[++optind_t];
            }

            ++optind_t;

            if (rt_opt.optarg == nullptr && arg_info.has_arg == required_argument)
            {
                rt_opt.optopt = rt_opt.val;
                rt_opt.val = ':';

                return long_type;
            }

            return long_type;
        }

        if (optPos[optLen] == '=')
        {
            rt_opt.optopt = rt_opt.val;
            rt_opt.val = ':';
            rt_opt.optarg = optPos + optLen + 1;

            optind_t++;

            return long_type;
        }

        return long_type;
    }

    assert(false);
}


int
main (int argc, char *argv[])
{
    option_return rst{};

    char buf[10] = "a:bc";
    option_Q list[5] = {
            {"qza", required_argument, 'q'},
            {"hy", optional_argument, 'h'},
            {"light", no_argument, 'l'},
            {nullptr, 0, 0},
    };
    OptParser parser(argc, argv, buf, list);

    int i;

    while ((i = parser.fetch(rst)) != -1)
    {
        using namespace std;
        char namebuf[1024];
        switch (i)
        {
            case nonopt_type:
                cout << "[nonopt] " << "optarg: "
                     << (rst.optarg ? rst.optarg : "['\\0']")
                     << " val: " << "\\1" << endl;
                break;

            case short_type:
                cout << "[shortopt] " << "name: ";
                if (rst.name)
                {
                    strncpy(namebuf, rst.name, rst.name_len);
                    namebuf[rst.name_len] = '\0';
                    cout << namebuf;
                }
                else
                {
                    cout << "[null]";
                }
                cout << " val: " << rst.val
                     << " optopt: " << (rst.optopt != '\0' ? rst.optopt : '0')
                     << " optarg: " << (rst.optarg == nullptr ? " [null] " : rst.optarg)
                     << endl;
                break;



            case long_type:
                cout << "[longopt]" << "name: ";
                if (rst.name)
                {
                    strncpy(namebuf, rst.name, rst.name_len);
                    namebuf[rst.name_len] = '\0';
                    cout << namebuf;
                }
                else
                {
                    cout << "[null]";
                }
                cout << " val: '" << rst.val
                << "' optopt: " << (rst.optopt != '\0' ? rst.optopt : '0')
                << " optarg: " << (rst.optarg == nullptr ? " [null] " : rst.optarg)
                << endl;
                break;

                break;
            default:
                cerr << "err";
        }
    }

    return 0;
}