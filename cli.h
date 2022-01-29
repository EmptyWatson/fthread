#pragma once

#include <proc_def.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "replxx.hxx"
#include <tracer.h>


typedef std::shared_ptr<replxx::Replxx> replxx_ptr;

typedef std::vector<std::pair<std::string, replxx::Replxx::Color>> syntax_highlight_t;
typedef std::unordered_map<std::string, replxx::Replxx::Color> keyword_highlight_t;

typedef replxx::Replxx::Color cl;



struct replxx_config{
    static inline std::vector<std::string> get_def_commands(){
        std::vector<std::string> commands_def {
		".help", ".history", ".quit", ".exit", ".clear"
        "h", "q",
        "list", "freeze", "unfreeze", "find",
        "all", "freezed", 
        "l", "f", "u"
	    };
        return commands_def;
    }

	bool tickMessages = false;
	bool promptFan = false;
	bool promptInCallback = false;
	bool indentMultiline = false;
	bool bracketedPaste = false;
	bool ignoreCase = false;
	std::string keys;
	std::string prompt;
	int hintDelay = 0;
    std::string history_file_path = "./fthread_history.txt";

    //用于自动补全的字符
    std::vector<std::string> commands = get_def_commands();

    keyword_highlight_t word_color {
		// single chars
		{"`", cl::BRIGHTCYAN},
		{"'", cl::BRIGHTBLUE},
		{"\"", cl::BRIGHTBLUE},
		{"-", cl::BRIGHTBLUE},
		{"+", cl::BRIGHTBLUE},
		{"=", cl::BRIGHTBLUE},
		{"/", cl::BRIGHTBLUE},
		{"*", cl::BRIGHTBLUE},
		{"^", cl::BRIGHTBLUE},
		{".", cl::BRIGHTMAGENTA},
		{"(", cl::BRIGHTMAGENTA},
		{")", cl::BRIGHTMAGENTA},
		{"[", cl::BRIGHTMAGENTA},
		{"]", cl::BRIGHTMAGENTA},
		{"{", cl::BRIGHTMAGENTA},
		{"}", cl::BRIGHTMAGENTA},

		// commands keywords
		{"list", cl::BRIGHTMAGENTA},
		{"freezed", cl::RED},
		{"unfreeze", cl::GREEN},
		{"find", cl::MAGENTA},
		{"all", cl::BROWN},

		// commands
		{"help", cl::BRIGHTMAGENTA},
        {"h", cl::BRIGHTMAGENTA},
		{"history", cl::BRIGHTMAGENTA},
		{"quit", cl::BRIGHTMAGENTA},
        {"q", cl::BRIGHTMAGENTA},
		{"exit", cl::BRIGHTMAGENTA},
		{"clear", cl::BRIGHTMAGENTA},
	};

    syntax_highlight_t regex_color = {
            // numbers
            {"[\\-|+]{0,1}[0-9]+", cl::YELLOW}, // integers
            {"[\\-|+]{0,1}[0-9]*\\.[0-9]+", cl::YELLOW}, // decimals
            {"[\\-|+]{0,1}[0-9]+e[\\-|+]{0,1}[0-9]+", cl::YELLOW}, // scientific notation

            // strings
            {"\".*?\"", cl::BRIGHTGREEN}, // double quotes
            {"\'.*?\'", cl::BRIGHTGREEN}, // single quotes
        };
    
};

/*
主要提供：
* - list all 枚举进程当前的线程(重新扫描)
* - list freezed 枚举已经冻结的线程
* find name 搜索线程名(正则)
* freeze tid 冻结 线程id
* freeze name 冻结 线程名(正则)
* unfreeze tid 解冻 线程id
* unfreeze name  解冻 线程名(正则)
* - quit 退出
*/

class cmd_line_parser{
public:
    cmd_line_parser(pid_t pid, std::string tids, std::string nmregexpr);

    void pre_start();
    
    void start();
    
    void stop();
protected:
    void   refresh_threads();
    size_t parse_tids(std::string tids);
    size_t parse_names(std::string tids);

    replxx_ptr init_replxx(replxx_config& conf);
    
    std::string format_threads(const thread_info_map_t& tmap);
    std::string format_threads(const pid_set_t& tset);
    std::string format_threads_no_info(const pid_set_t& tset);

    //返回冻结成功的线程数
    int freeze(const pid_set_t& tset);
    void freeze_out_info(const pid_set_t& tset, replxx_ptr rx);

    //返回解冻成功的线程数
    int unfreeze(const pid_set_t& tset);
    void unfreeze_out_info(const pid_set_t& tset, replxx_ptr rx);

    pid_t                 m_pid;
    tracer                m_tc;
    replxx_config         m_conf;
    thread_info_map_t     m_threads_map;//当前获取到的进程的所有线程信息
    pid_set_t             m_need_op_thr_set;//待冻结/解冻线程列表
    pid_set_t             m_err_op_thr_set;//冻结/解冻错误的线程
};