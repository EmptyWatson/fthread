#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <string>
#include <set>
#include <elf.h>
#include <link.h>

#include <cli.h>

#include "easylogging++.h"
#include <cxxopts.hpp>

using addr_t = std::uintptr_t;

INITIALIZE_EASYLOGGINGPP

void init_logger(bool en_debug){
    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.set(el::Level::Global,
            el::ConfigurationType::Format, "%datetime %level %msg");
    defaultConf.set(el::Level::Global,
            el::ConfigurationType::Filename, "fthread.log");

    defaultConf.set(el::Level::Trace,
            el::ConfigurationType::Enabled, "false");

    if(!en_debug){
        defaultConf.set(el::Level::Debug,
                el::ConfigurationType::Enabled, "false");
    }

    el::Loggers::reconfigureLogger("default", defaultConf);
}

int main(int argc, char** argv) {
    try
    {
        cxxopts::Options options(argv[0], " - freeze specify threads"
                                          "\n - 暂停指定的线程，弥补gdb的不足");
        options
        .positional_help("[optional args]")
        .show_positional_help();

        options
        .set_width(200)
        .set_tab_expansion()
        .allow_unrecognised_options()
        .add_options()
        ("h,help", "Print help")
        ("d,debug", "Enable debugging log", cxxopts::value<bool>()->default_value("false"))
        ("p,pid", "process id", cxxopts::value<pid_t>())
        ("t,tids", "use ',' or '-' split threads id", cxxopts::value<std::string>())
        ("n,nmregexpr", "use regular expr match thread's name", cxxopts::value<std::string>())
        ;

        auto result = options.parse(argc, argv);
        if(result.count("help"))
        {
            std::cout << options.help() << std::endl;
            exit(0);
        }

        bool en_debug = result["debug"].as<bool>();
        pid_t  pid    = 0;

        if(result.count("pid"))
        {
            pid =  result["pid"].as<int>();
        }

        std::string tids;
        std::string nmregexpr;

        if(result.count("tids"))
        {
            tids =  result["tids"].as<std::string>();
        }

        if(result.count("nmregexpr"))
        {
            nmregexpr =  result["nmregexpr"].as<std::string>();
        }

        init_logger(en_debug);
        START_EASYLOGGINGPP(argc, argv);
        LOG(INFO) << "--------->fthread is starting......";
        if(en_debug){
            LOG(INFO) << "--------->with debug log";
        }

        if (pid <= 0) {
            LOG(ERROR) << "Program pid not specified";
            return -1;
        }

        cmd_line_parser cli_parser(pid, tids, nmregexpr);
        
        cli_parser.pre_start();

        cli_parser.start();

        cli_parser.stop();
    }
    catch (const cxxopts::OptionException& e)
    {
        std::cout << "error parsing options: " << e.what() << std::endl;
        exit(1);
    }
}
