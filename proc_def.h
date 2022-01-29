
#pragma once
#include <sys/types.h>
#include <unistd.h>

#include <vector>
#include <map>
#include <set>
#include <string>

enum thread_state {
	TS_RUNNING = 0,      //正常运行
	TS_FREEZED = 1,      //已冻结
};

static inline std::string get_state_name(thread_state  state){
	switch (state)
	{
	case TS_RUNNING:
		return "- running";
	case TS_FREEZED:
		return "* freezed";
	default:
		return "unknown";
	}
	return "running";
}

struct thread_info{
    pid_t         tid = 0;
	pid_t         idx = 0;//0代表无效，从1开始计数
    std::string   name;
	thread_state  state = TS_RUNNING;
};

typedef thread_info thread_info_t;
typedef std::vector<thread_info_t>     thread_info_vec_t;
typedef std::map<pid_t, thread_info_t> thread_info_map_t;


typedef std::set<pid_t> pid_set_t;