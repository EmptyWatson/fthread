# [中文版Readme](https://github.com/EmptyWatson/fthread/blob/main/README_zh.md)
# fthread
for linux, a tool used to pause or freeze specified threads

# Background
- When analyzing multi-threaded programs, you need to exclude the interference of several of these threads, and you want to pause them to observe the performance of the system
- The gdb provides corresponding functionality but is inconvenient to use when a batch pause thread is needed
- Looking around the internet with no tools providing bulk pause thread functionality

  So, dirty my hand, just write one.

# Dependencies
- [cxxopts](https://github.com/jarro2783/cxxopts)
- [easyloggingpp](https://github.com/amrayn/easyloggingpp)
- [replxx](https://github.com/AmokHuginnsson/replxx)
- [StringUtils](https://github.com/hungtatai/StringUtils)


# Build
## Setup environment
```
git clone https://github.com/EmptyWatson/fthread.git
cd fthread
chmod +x ./setup_dev.sh
./setup_dev.sh
```

## Compile
```
mkdir build
cd build
cmake ..
make -j
```

## Strip
```
eu-strip -f ./fthread.debug ./fthread
```

# Use
## Capabilities
- list 
  List all threads / frozen threads for the specified process (get the current thread list of the process each time the list command is executed)
- find 
  Find all threads where the thread name meets criteria and list (regular expressions only)
- freeze 
  Pause the thread name, the thread index number, any thread in the thread ID that meets the specified condition, or freeze the result of the last find (support regular expressions, with "," split number list, and with "-" split number range)
- unfreeze 
  Unfreeze a thread that meets the specified condition, or restore the result of the last find (method as freeze)

## Examples

### Start fthread
``` 
[root@localhost build]# ./fthread -p $(pidof java)
2022-01-29 11:23:31,303 INFO --------->fthread is starting......
2022-01-29 11:23:31,306 INFO Welcome to fthread
Press 'tab' to view autocompletions
Type '.help' for help
Type '.quit' or '.exit' to exit

fthread>
```

### Get help info
```
fthread> h
2022-01-29 11:24:39,057 INFO
h,.help
        displays the help output
q,.quit/.exit
        exit the repl
.clear
        clears the screen
.history
        displays the history output
.save
        save op log
l,list all/freezed
         refresh and list threads
find [thread name regexpr]
        find thread by name,use regexpr
f,freeze [thread name regexpr]/[thread id]/[thread idx]
        freeze thread,use regexpr or tid/idx(1,2,.. or 1-5) or last finded threads
u,unfreeze [thread name regexpr]/[thread id]/[thread idx]
        unfreeze thread,use regexpr or tid/idx(1,2,.. or 1-5) or last finded threads

fthread>

```

### List all threads
```
fthread> l
IDX      Thread Id        Status       Name
[1     ] 1988             - running    pool-7-thread-6
[2     ] 4289             - running    java
[3     ] 4319             - running    java
[4     ] 4322             - running    java
[5     ] 4323             - running    java
[6     ] 4324             - running    java
[7     ] 4325             - running    java
[8     ] 4326             - running    java
[9     ] 4327             - running    java
[10    ] 4328             - running    java
[11    ] 4329             - running    java
[12    ] 4330             - running    java
[13    ] 4331             - running    java
[14    ] 4332             - running    java
[15    ] 4333             - running    java
[16    ] 4334             - running    java
[17    ] 4335             - running    java

...

```

### Freeze threads
```
fthread> freeze 1-3
freeze threads ....
freeze threads finish, succ cnt 3

fthread> freeze 4,5,6
freeze threads ....
freeze threads finish, succ cnt 3

fthread> f 4325-4326
freeze threads ....
freeze threads finish, succ cnt 2
fthread>

fthread> freeze .*java.*
freeze threads ....
freeze threads finish, succ cnt 21
fthread>

```

### List the threads that have already been frozen 
```
fthread> list freezed
IDX      Thread Id        Status       Name
[2     ] 4289             * freezed    java
[3     ] 4319             * freezed    java
[4     ] 4322             * freezed    java
[5     ] 4323             * freezed    java
[6     ] 4324             * freezed    java
[7     ] 4325             * freezed    java
[8     ] 4326             * freezed    java
[9     ] 4327             * freezed    java
[10    ] 4328             * freezed    java
[11    ] 4329             * freezed    java
[12    ] 4330             * freezed    java
[13    ] 4331             * freezed    java
[14    ] 4332             * freezed    java
[15    ] 4333             * freezed    java
[16    ] 4334             * freezed    java
[17    ] 4335             * freezed    java
[18    ] 4336             * freezed    java
[19    ] 4337             * freezed    java
[20    ] 4338             * freezed    java
[21    ] 4339             * freezed    java
[45    ] 4422             * freezed    java

```

### Unfreeze threads
```
fthread> unfreeze 2-3
unfreeze threads ....
unfreeze threads finish, succ cnt 2

fthread> unfreeze java
unfreeze threads ....
2022-01-29 11:34:02,710 ERROR detrace_pid fail: pid=4289
2022-01-29 11:34:02,710 ERROR detrace_pid fail: pid=4319
unfreeze threads finish, succ cnt 19
        failed cnt 2 :
4289,4319
```

### Find threads
```
fthread> find .*-thread.*
finded threads:
IDX      Thread Id        Status       Name
[1     ] 1988             - running    pool-7-thread-6
[46    ] 4433             - running    pool-1-thread-1
[61    ] 4475             - running    sysinfo-thread1
[102   ] 13641            - running    pool-7-thread-2
[103   ] 14201            - running    pool-7-thread-8
[104   ] 26427            - running    pool-7-thread-1

...

```

### Freeze the found results
```
fthread> freeze
no args, freeze finded threadsfreeze threads ....
freeze threads finish, succ cnt 38
fthread> find .*-thread.*
finded threads:
IDX      Thread Id        Status       Name
[1     ] 1988             * freezed    pool-7-thread-6
[46    ] 4433             * freezed    pool-1-thread-1
[61    ] 4475             * freezed    sysinfo-thread1
[102   ] 13641            * freezed    pool-7-thread-2
[103   ] 14201            * freezed    pool-7-thread-8
[104   ] 26427            * freezed    pool-7-thread-1
[105   ] 34015            * freezed    pool-7-thread-2
[107   ] 58240            * freezed    pool-7-thread-3
[108   ] 79725            * freezed    pool-7-thread-1
[109   ] 101419           * freezed    pool-7-thread-3
[110   ] 105710           * freezed    pool-7-thread-1
[111   ] 110990           * freezed    pool-7-thread-2
[112   ] 123730           * freezed    pool-7-thread-7
[114   ] 133070           * freezed    pool-7-thread-3
[115   ] 152928           * freezed    pool-7-thread-3
[117   ] 168584           * freezed    pool-7-thread-2
[118   ] 170886           * freezed    pool-7-thread-2
...
```

### Quit
```
fthread> q
2022-01-29 11:36:08,834 INFO
Exiting fthread
```

# TODO
- The state of the thread is maintained inside the tool without going to read the true state