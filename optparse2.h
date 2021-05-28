//
// Created by root on 2021/5/23.
//

#ifndef TMP_TEST_OPTPARSE2_H
#define TMP_TEST_OPTPARSE2_H

inline bool isOpt (const char *arg)
{
    return arg[0] == '-' && arg[1] != '\0' && (arg[1] != '-' || arg[2] != '\0');
}

inline bool notOpt (const char *arg)
{
    return not isOpt(arg);
}

inline bool isLongOpt (const char *arg)
{
    return arg[0] == '-' && arg[1] == '-' && arg[2] != '\0';
}

inline bool isShortOpt (const char *arg)
{
    return arg[0] == '-' && arg[1] != '\0' && not isLongOpt(arg);
}

const int no_argument = 0;
const int required_argument = 1;
const int optional_argument = 2;

/// this struct has two usages,
/// first, it will be used to identify how to process the longopt,
/// second, it will be used in the getopt to polymorphism the long
/// and short opt. and when with short opt `name` is nullptr.
struct option_Q
{
    // when name is `nullptr`, it is consider as the short option.
    const char *name;
    int has_arg;
    /// option_Q list stop at val = 0;
    char val;
};


const int nonopt_type = 0;
const int short_type = 1;
const int long_type = 2;

struct option_return
{
    const char *name;
    char val;
    char optopt; // only when val == ':' or '?' we see `optopt`.
    int name_len; // need `name_len` for return unspecified longopt.
    // only effective when `name` != nullptr. otherwise, unspecified.
    const char *optarg;
};


class OptParser {
public:
    OptParser(int argc, char **argv,
              char *optstring, option_Q *longopts);

    int fetch(option_return &re_opt); // whatever we get with optstring_t,
                  // we assume it with ':' at the front

    void reset(int argc, char **argv,
               char *optstring, option_Q *longopts);
    void reset(int argc, char **argv);
    void reset(char *optstring, option_Q *longopts);
    void reset();

private:
    int getopt_int(option_return &rt_opt);

    size_t search_opts_lv(char c_srh);
    size_t search_opts_ln(const char *name, int len);

    int getOptInfo(char opt, option_Q &info_rst);
    int getOptInfo(const char *opt_l, int len, option_Q &info_rst);

    int optind_t;
    const char *nextchar_t;

    char **argv_t;
    int argc_t;

    char *optstring_t;
    option_Q *longopts_t;
};
#endif //TMP_TEST_OPTPARSE2_H
