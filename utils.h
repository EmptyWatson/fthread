#pragma once
#include <string>
#include <memory>
#include <vector>
#include <array>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <limits>

#include <sys/time.h>
#include <time.h>
#include <assert.h>


static inline std::string gen_progress_bar(float progress){
    std::ostringstream oss;
    int barWidth = 100;
    oss << "[";
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) oss << "=";
        else if (i == pos) oss << ">";
        else oss << " ";
    }
    oss << "] " << int(progress * 100.0) << " %\r";

    return oss.str();
}



// strrpbrk would suffice but is not portable.
#ifdef __cplusplus
extern "C" {
#endif

int context_len( char const* );
int utf8str_codepoint_len( char const*, int );

#ifdef __cplusplus
}
#endif
