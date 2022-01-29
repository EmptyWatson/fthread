#include <proc.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/auxv.h>
#include <fcntl.h>

#include <unistd.h>
#include <sys/prctl.h>
#include <sys/user.h>
#include <dirent.h>
#include <errno.h>
#include <assert.h>

#include <string-utils.h>
#include <easylogging++.h>
#include <sstream>
#include <fstream>      // std::ifstream


#define	MAX_DELAY	100000	/* 100000 microseconds = 0.1 seconds */

std::string proc_util::pid2name(pid_t pid){
	if (!kill(pid, 0)) {
		int delay = 0;
        std::ostringstream oss;
        oss << "/proc/" << pid << "/exe";
		std::string proc_exe = oss.str();
		while (delay < MAX_DELAY) {
			if (!access(proc_exe.c_str(), F_OK)) {
				return proc_exe;
			}
			delay += 1000;	/* 1 milisecond */
		}
	}
	return "";
}

int proc_util::wait_for_proc(pid_t pid){
	if (waitpid(pid, NULL, __WALL) != pid) {
		LOG(ERROR) << "trace_pid: waitpid";
		return -1;
	}
	return 0;
}

int proc_util::get_sub_tids(const pid_t pid, thread_info_map_t& thr_map){
    DIR     *dir;
    pid_t   *list;
    size_t   size, used = 0;

    if (pid < (pid_t)1) {
        return -1;
    }

	std::string dir_path = "/proc/";
	dir_path += std::to_string(pid);
	dir_path += "/task/";

    dir = opendir(dir_path.c_str());
    if (!dir) {
        return -1;
    }

    while (true) {
        struct dirent *ent;
        int            value;
        char           dummy;
        ent = readdir(dir);
        if (!ent)
            break;

        /* Parse TIDs. Ignore non-numeric entries. */
        if (sscanf(ent->d_name, "%d%c", &value, &dummy) != 1)
            continue;

        /* Ignore obviously invalid entries. */
        if (value < 1)
            continue;

		thread_info_t thr_info;
		thr_info.tid = (pid_t)value;

		//获取线程名称
		std::string comm_path = dir_path;
		comm_path += ent->d_name;
		comm_path += "/comm";
		std::ifstream t(comm_path.c_str());
		std::stringstream buffer;
		buffer << t.rdbuf();
		thr_info.name = Trim(buffer.str());
		
		thr_map[thr_info.tid] = thr_info;
    }

	closedir(dir);
	return 0;
}