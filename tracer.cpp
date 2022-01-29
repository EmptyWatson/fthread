#include <sys/param.h>
#include <sys/wait.h>
#include <sys/ptrace.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include <tracer.h>
#include <proc.h>
#include <easylogging++.h>

int exiting = 0;		/* =1 if a SIGINT or SIGTERM has been received */

static void
signal_alarm(int sig) {
	signal(SIGALRM, SIG_DFL);
	//each_process(NULL, &stop_non_p_processes, NULL);
}

static void
signal_exit(int sig)
{
	if (exiting != 0)
		return;
    
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGALRM, signal_alarm);
	//alarm(1);

    //直接退出
    exit(-1);
}

static void
normal_exit(void)
{
}

tracer::~tracer(){
    uninit();
}

void tracer::init() {
	//atexit(normal_exit);
	//signal(SIGINT, signal_exit);	/* Detach processes when interrupted */
	//signal(SIGTERM, signal_exit);	/*  ... or killed */
}

void tracer::uninit(){
    //解除线程绑定
    //for(const pid_t & tid : m_attached_sub_tids){
    //    if(0 != detrace_pid(tid)){
    //        //assert(false);
    //    }
    //}
    //解除进程绑定
    //detrace_pid(m_pid);
    //m_attached_sub_tids.clear();
}

int tracer::trace_pid(pid_t pid)
{
	LOG(DEBUG) << "trace_pid: pid=" << pid;
    //进程将被暂停住
	/* This shouldn't emit error messages, as there are legitimate
	 * reasons that the PID can't be attached: like it may have
	 * already ended.  */
	if (ptrace(PTRACE_ATTACH, pid, 0, 0) < 0)
		return -1;

    int rc = proc_util::wait_for_proc(pid);
    m_attached_thr_set.insert(pid);
	return rc;
}

int tracer::detrace_pid(pid_t pid){
	if (ptrace(PTRACE_DETACH, pid, 0, 0) < 0){
        LOG(ERROR) << "detrace_pid fail: pid=" << pid;
        return -1;
    }
    m_attached_thr_set.erase(pid);
    return 0;
}
