#pragma once

#include <proc_def.h>



class proc_util{
public:
    static std::string pid2name(pid_t pid);

    static int wait_for_proc(pid_t pid);

    //查找进程的子id，包含进程id自己
    static int get_sub_tids(const pid_t pid, thread_info_map_t& thr_map);
};