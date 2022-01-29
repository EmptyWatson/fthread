#pragma once

#include <sys/types.h>
#include <unistd.h>
#include <proc_def.h>

class tracer{
public:
    tracer(pid_t pid)
    : m_pid(pid)
    {
        init();
    }

    ~tracer();
public:
    int trace_pid(pid_t pid);
    
    int detrace_pid(pid_t pid);

    const pid_set_t& get_attached_tid_set(){
        return m_attached_thr_set;
    }

    void clean_no_exist_tid(const thread_info_map_t &tmap){
        pid_set_t ds;
        for(const pid_t &t : m_attached_thr_set){
            if(tmap.end() == tmap.find(t)){
                ds.insert(t);
            }
        }

        for(const pid_t & t : ds){
            m_attached_thr_set.erase(t);
        }
    }
protected:
    void init();
    void uninit();

    pid_t                 m_pid;
    pid_set_t             m_attached_thr_set;    
};