#include <cli.h>

#include <vector>
#include <unordered_map>
#include <string>
#include <regex>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <utility>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

#include <proc.h>

#include "string-utils.h"
#include "easylogging++.h"
#include "utils.h"

using Replxx = replxx::Replxx;
using namespace replxx::color;

class Tick {
	typedef std::vector<char32_t> keys_t;
	std::thread _thread;
	int _tick;
	int _promptState;
	bool _alive;
	keys_t _keys;
	bool _tickMessages;
	bool _promptFan;
	replxx_ptr _replxx;
public:
	Tick( replxx_ptr replxx_, std::string const& keys_, bool tickMessages_, bool promptFan_ )
		: _thread()
		, _tick( 0 )
		, _promptState( 0 )
		, _alive( false )
		, _keys( keys_.begin(), keys_.end() )
		, _tickMessages( tickMessages_ )
		, _promptFan( promptFan_ )
		, _replxx( replxx_ ) {
	}
	void start() {
		_alive = true;
		_thread = std::thread( &Tick::run, this );
	}
	void stop() {
		_alive = false;
		_thread.join();
	}
	void run() {
		std::string s;
		static char const PROMPT_STATES[] = "-\\|/";
		while ( _alive ) {
			if ( _tickMessages ) {
				_replxx->print( "%d\n", _tick );
			}
			if ( _tick < static_cast<int>( _keys.size() ) ) {
				_replxx->emulate_key_press( _keys[_tick] );
			}
			if ( ! _tickMessages && ! _promptFan && ( _tick >= static_cast<int>( _keys.size() ) ) ) {
				break;
			}
			if ( _promptFan ) {
				for ( int i( 0 ); i < 4; ++ i ) {
					char prompt[] = "\x1b[1;32mreplxx\x1b[0m[ ]> ";
					prompt[18] = PROMPT_STATES[_promptState % 4];
					++ _promptState;
					_replxx->set_prompt( prompt );
					std::this_thread::sleep_for( std::chrono::milliseconds( 250 ) );
				}
			} else {
				std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
			}
			++ _tick;
		}
	}
};


cmd_line_parser::cmd_line_parser(pid_t pid, std::string tids, std::string nmregexpr)
: m_pid(pid)
, m_tc(pid)
{
	//准备好待冻结列表
    refresh_threads();
    parse_tids(tids);
	parse_names(nmregexpr);
}

void cmd_line_parser::pre_start(){
	if(0 >= m_pid) return;

	// display initial welcome message
	LOG(INFO)
		<< "Welcome to fthread\n"
		<< "Press 'tab' to view autocompletions\n"
		<< "Type '.help' for help\n"
		<< "Type '.quit' or '.exit' to exit\n";

	//先冻结命令行传入的线程
	if(!m_threads_map.empty() && !m_need_op_thr_set.empty()){
		freeze_out_info(m_need_op_thr_set, replxx_ptr());

		LOG(INFO) << "list threads:\n"
				<< format_threads(m_threads_map)
				<< "\n"
				;
	}

	m_need_op_thr_set.clear();
	m_err_op_thr_set.clear();
}

void cmd_line_parser::start(){
	if(0 >= m_pid) return;

	replxx_ptr rx = init_replxx(m_conf);
	Tick tick( rx, m_conf.keys, m_conf.tickMessages, m_conf.promptFan );
	// set the repl prompt
	if ( m_conf.prompt.empty() ) {
		m_conf.prompt = "\x1b[1;32mfthread\x1b[0m> ";
	}

	// main repl loop
	if ( ! m_conf.keys.empty() || m_conf.tickMessages || m_conf.promptFan ) {
		tick.start();
	}

	while(true) {
		// display the prompt and retrieve input from the user
		char const* cinput{ nullptr };

		do {
			cinput = rx->input(m_conf.prompt);
		} while ( ( cinput == nullptr ) && ( errno == EAGAIN ) );

		if (cinput == nullptr) {
			break;
		}

		// change cinput into a std::string
		// easier to manipulate
		std::string input {cinput};

		if (input.empty()) {
			// user hit enter on an empty line

			continue;

		} else if (input.compare(0, 5, "q") == 0 || input.compare(0, 5, ".quit") == 0 || input.compare(0, 5, ".exit") == 0) {
			// exit the repl
			rx->history_add(input);
			break;
		} else if (input.compare(0, 1, "h") == 0 || input.compare(0, 5, ".help") == 0) {
			// display the help output
			LOG(INFO)
			    << "\n"
				<< "h,.help\n\tdisplays the help output\n"
				<< "q,.quit/.exit\n\texit the repl\n"
				<< ".clear\n\tclears the screen\n"
				<< ".history\n\tdisplays the history output\n"
				<< ".save\n\tsave op log\n"
				<< "l,list all/freezed\n\t refresh and list threads\n"
				<< "find [thread name regexpr]\n\tfind thread by name,use regexpr\n"
				<< "f,freeze [thread name regexpr]/[thread id]/[thread idx]\n\tfreeze thread,use regexpr or tid/idx(1,2,.. or 1-5) or last finded threads\n"
				<< "u,unfreeze [thread name regexpr]/[thread id]/[thread idx]\n\tunfreeze thread,use regexpr or tid/idx(1,2,.. or 1-5) or last finded threads\n"
				;

			rx->history_add(input);
			continue;

		} else if (input.compare(0, 8, ".history") == 0) {
			// display the current history
			Replxx::HistoryScan hs( rx->history_scan() );
			for ( int i( 0 ); hs.next(); ++ i ) {
				std::cout << std::setw(4) << i << ": " << hs.get().text() << "\n";
			}

			rx->history_add(input);
			continue;

		} else if (input.compare(0, 5, ".save") == 0) {
			m_conf.history_file_path = "fthread_history_alt.txt";
			std::ofstream history_file( m_conf.history_file_path.c_str() );
			rx->history_save( history_file );
			continue;
		} else if (input.compare(0, 6, ".clear") == 0) {
			// clear the screen
			rx->clear_screen();
			rx->history_add(input);
			continue;
		} else if (input.compare(0, 1, "l") == 0 || input.compare(0, 4, "list") == 0) {
			// set the repl prompt text
			auto pos = input.find(" ");
			std::string sub_cmd;
			if (pos == std::string::npos) {
				sub_cmd = "all";
			} else {
				sub_cmd = input.substr(pos + 1) + " ";
			}
			trim(sub_cmd);

			refresh_threads();
			m_tc.clean_no_exist_tid(m_threads_map);

			if(sub_cmd == "all"){
				std::string thrs = format_threads(m_threads_map);
				rx->print( "%s\n", thrs.c_str() );
			}
			else if(sub_cmd == "freezed"){
				std::string thrs = format_threads(m_tc.get_attached_tid_set());
				rx->print( "%s\n", thrs.c_str() );
			}
			else{
				std::cout << "Error: 'list' unknown argument\n";
			}

			rx->history_add(input);
			continue;
		} else if (input.compare(0, 4, "find") == 0) {
			auto pos = input.find(" ");
			std::string sub_cmd;
			if (pos == std::string::npos) {
				sub_cmd = "all";
			} else {
				sub_cmd = input.substr(pos + 1) + " ";
			}
			trim(sub_cmd);

			m_need_op_thr_set.clear();

			if(!sub_cmd.empty()){
				if(parse_names(sub_cmd)){				
					std::string os = format_threads(m_need_op_thr_set);
					rx->print("finded threads:\n%s\n", os.c_str());
				}
			}

			rx->history_add(input);
			continue;
		} else if (input.compare(0, 1, "f") == 0 || input.compare(0, 6, "freeze") == 0) {
			auto pos = input.find(" ");
			std::string sub_cmd;
			if (pos == std::string::npos) {
				sub_cmd = "";
			} else {
				sub_cmd = input.substr(pos + 1) + " ";
			}

			trim(sub_cmd);

			refresh_threads();
			m_tc.clean_no_exist_tid(m_threads_map);

			//主要有2种形式：数字，正则
			const std::regex tid_regex("^[0-9]+(,[0-9]+)*$|^[0-9]+-[0-9]+$");
			if(std::regex_match(sub_cmd, tid_regex)){
				m_need_op_thr_set.clear();
				m_err_op_thr_set.clear();
				parse_tids(sub_cmd);
				freeze_out_info(m_need_op_thr_set, rx);
			}
			else if(!sub_cmd.empty()){
				m_need_op_thr_set.clear();
				m_err_op_thr_set.clear();
				parse_names(sub_cmd);
				freeze_out_info(m_need_op_thr_set, rx);
			}
			else{
				if(m_need_op_thr_set.size()){
					rx->print("no args, freeze finded threads");
					freeze_out_info(m_need_op_thr_set, rx);
				}
				else{
					rx->print("do nothing\n");
				}
			}

			m_need_op_thr_set.clear();
			m_err_op_thr_set.clear();

			rx->history_add(input);
			continue;
		} else if (input.compare(0, 1, "u") == 0 || input.compare(0, 8, "unfreeze") == 0) {
			auto pos = input.find(" ");
			std::string sub_cmd;
			if (pos == std::string::npos) {
				sub_cmd = "";
			} else {
				sub_cmd = input.substr(pos + 1) + " ";
			}

			trim(sub_cmd);

			refresh_threads();
			m_tc.clean_no_exist_tid(m_threads_map);

			//主要有2种形式：数字，正则
			const std::regex tid_regex("^[0-9]+(,[0-9]+)*$|^[0-9]+-[0-9]+$");
			if(std::regex_match(sub_cmd, tid_regex)){
				m_need_op_thr_set.clear();
				m_err_op_thr_set.clear();
				parse_tids(sub_cmd);
				unfreeze_out_info(m_need_op_thr_set, rx);
			}
			else if(!sub_cmd.empty()){
				m_need_op_thr_set.clear();
				m_err_op_thr_set.clear();
				parse_names(sub_cmd);
				unfreeze_out_info(m_need_op_thr_set, rx);
			}
			else{
				if(m_need_op_thr_set.size()){
					rx->print("no args, unfreeze finded threads");
					unfreeze_out_info(m_need_op_thr_set, rx);
				}
				else{
					rx->print("do nothing\n");
				}
			}

			m_need_op_thr_set.clear();
			m_err_op_thr_set.clear();
			rx->history_add(input);
			continue;
		} else {
			// default action
			// echo the input
			rx->print( "unknown cmd: %s\n", input.c_str() );
			rx->history_add( input );
			continue;
		}
	}

	if ( ! m_conf.keys.empty() || m_conf.tickMessages || m_conf.promptFan ) {
		tick.stop();
	}

	// save the history
	rx->history_sync( m_conf.history_file_path );
	if ( m_conf.bracketedPaste ) {
		rx->disable_bracketed_paste();
	}
	if ( m_conf.bracketedPaste || m_conf.promptInCallback || m_conf.promptFan ) {
		LOG(INFO) << "\n" << m_conf.prompt;
	}

	LOG(INFO) << "\nExiting fthread\n";
}

void cmd_line_parser::stop(){

}

void cmd_line_parser::refresh_threads(){
    if(0 >= m_pid) return;

    thread_info_map_t thr_map;
    proc_util::get_sub_tids(m_pid, thr_map);
    for(const auto & t : thr_map){
        auto it = m_threads_map.find(t.first);
        if(it == m_threads_map.end()){
           m_threads_map.insert(std::make_pair<>(t.first, t.second));
        }
    }

    //清理已经退出的线程
	for(auto it = m_threads_map.begin(); it != m_threads_map.end();){
		if(thr_map.find(it->first) == thr_map.end()){
			it = m_threads_map.erase(it);
		}
		else{
			++it;
		}
	}

	//重新编号-增加自动补全能力
	int idx = 1;
	//初始自动补全
	m_conf.commands = replxx_config::get_def_commands();
	for(auto it = m_threads_map.begin(); it != m_threads_map.end(); ++it){
		it->second.idx = idx++;
		m_conf.commands.push_back(std::to_string(it->first));
		m_conf.commands.push_back(it->second.name);
		m_conf.commands.push_back(std::to_string(it->second.idx));
	}
}

size_t cmd_line_parser::parse_tids(std::string tids){
	//以idx去查找
	//遭到了返回线程id，没找到返回0
	auto find_idx = [&](int i) -> pid_t{
		for(const auto & t : m_threads_map){
			if(t.second.idx == i){
				return t.first;
			}
		}
		return 0;
	};

    std::vector<std::string> tids_str_vec;
	const std::regex multi_regex("^[0-9]+(,[0-9]+)*$");
	const std::regex range_regex("^[0-9]+-[0-9]+$");
	if(std::regex_match(tids, multi_regex)){
		tids_str_vec = Split(tids, ",", true);
		if(!tids_str_vec.empty()){
			for(const std::string & t : tids_str_vec){
				pid_t tid  = std::stoul(t);
				if(m_threads_map.find(tid) != m_threads_map.end()){
					m_need_op_thr_set.insert(tid);
				}
				else {
					pid_t tid2 = find_idx(tid);
					if(tid2 > 0){
						m_need_op_thr_set.insert(tid2);
					}
					else{
						m_err_op_thr_set.insert(tid);
					}
				}
			}
		}
	}
	else if(std::regex_match(tids, range_regex)){
		tids_str_vec = Split(tids, "-", true);
		if(tids_str_vec.size() == 2){
			pid_t begin = std::stoul(tids_str_vec[0]);
			pid_t end = std::stoul(tids_str_vec[1]);
			for(pid_t i = begin; i <= end; ++i){
				if(m_threads_map.find(i) != m_threads_map.end()){
					
					m_need_op_thr_set.insert(i);
				}
				else{
					pid_t tid2 = find_idx(i);
					if(tid2 > 0){
						m_need_op_thr_set.insert(tid2);
					}
					else{
						m_err_op_thr_set.insert(i);
					}
				}        
			}
		}
	}

    return m_need_op_thr_set.size();
}

size_t cmd_line_parser::parse_names(std::string tids){
	const std::regex tid_regex(tids);
	for(const auto & t : m_threads_map){
		if(std::regex_match(t.second.name, tid_regex)){
			if(m_threads_map.find(t.first) == m_threads_map.end()){
				m_err_op_thr_set.insert(t.first);
			}
			else{
				m_need_op_thr_set.insert(t.first);
			}   
		}
	}
    return m_need_op_thr_set.size();
}


// prototypes
Replxx::completions_t hook_completion(std::string const& context, int& contextLen, std::vector<std::string> const& user_data, bool);
Replxx::hints_t hook_hint(std::string const& context, int& contextLen, Replxx::Color& color, std::vector<std::string> const& user_data, bool);

void hook_color( std::string const& str, Replxx::colors_t& colors, syntax_highlight_t const&, keyword_highlight_t const& );
void hook_modify( std::string& line, int& cursorPosition, replxx_ptr );

bool eq( std::string const& l, std::string const& r, int s, bool ic ) {
	if ( static_cast<int>( l.length() ) < s ) {
		return false;
	}
	if ( static_cast<int>( r.length() ) < s ) {
		return false;
	}
	bool same( true );
	for ( int i( 0 ); same && ( i < s ); ++ i ) {
		same = ( ic && ( towlower( l[i] ) == towlower( r[i] ) ) ) || ( l[i] == r[i] );
	}
	return same;
}

Replxx::completions_t hook_completion(std::string const& context, int& contextLen, std::vector<std::string> const& commands, bool ignoreCase) {
	Replxx::completions_t completions;
	int utf8ContextLen( context_len( context.c_str() ) );
	int prefixLen( static_cast<int>( context.length() ) - utf8ContextLen );
	if ( ( prefixLen > 0 ) && ( context[prefixLen - 1] == '\\' ) ) {
		-- prefixLen;
		++ utf8ContextLen;
	}
	contextLen = utf8str_codepoint_len( context.c_str() + prefixLen, utf8ContextLen );

	std::string prefix { context.substr(prefixLen) };
	if ( prefix == "\\pi" ) {
		completions.push_back( "π" );
	} else {
		for (auto const& e : commands) {
			bool lowerCasePrefix( std::none_of( prefix.begin(), prefix.end(), iswupper ) );
			if ( eq( e, prefix, static_cast<int>( prefix.size() ), ignoreCase && lowerCasePrefix ) ) {
				Replxx::Color c( Replxx::Color::DEFAULT );
				if ( e.find( "brightred" ) != std::string::npos ) {
					c = Replxx::Color::BRIGHTRED;
				} else if ( e.find( "red" ) != std::string::npos ) {
					c = Replxx::Color::RED;
				}
				completions.emplace_back(e.c_str(), c);
			}
		}
	}

	return completions;
}

Replxx::hints_t hook_hint(std::string const& context, int& contextLen, Replxx::Color& color, std::vector<std::string> const& commands, bool ignoreCase) {
	Replxx::hints_t hints;

	// only show hint if prefix is at least 'n' chars long
	// or if prefix begins with a specific character

	int utf8ContextLen( context_len( context.c_str() ) );
	int prefixLen( static_cast<int>( context.length() ) - utf8ContextLen );
	contextLen = utf8str_codepoint_len( context.c_str() + prefixLen, utf8ContextLen );
	std::string prefix { context.substr(prefixLen) };

	if (prefix.size() >= 2 || (! prefix.empty() && prefix.at(0) == '.')) {
		bool lowerCasePrefix( std::none_of( prefix.begin(), prefix.end(), iswupper ) );
		for (auto const& e : commands) {
			if ( eq( e, prefix, prefix.size(), ignoreCase && lowerCasePrefix ) ) {
				hints.emplace_back(e.c_str());
			}
		}
	}

	// set hint color to green if single match found
	if (hints.size() == 1) {
		color = Replxx::Color::GREEN;
	}

	return hints;
}

inline bool is_kw( char ch ) {
	return isalnum( ch ) || ( ch == '_' );
}

void hook_color( std::string const& context, Replxx::colors_t& colors, syntax_highlight_t const& regex_color, keyword_highlight_t const& word_color ) {
	// highlight matching regex sequences
	for (auto const& e : regex_color) {
		size_t pos {0};
		std::string str = context;
		std::smatch match;

		while(std::regex_search(str, match, std::regex(e.first))) {
			std::string c{ match[0] };
			std::string prefix( match.prefix().str() );
			pos += utf8str_codepoint_len( prefix.c_str(), static_cast<int>( prefix.length() ) );
			int len( utf8str_codepoint_len( c.c_str(), static_cast<int>( c.length() ) ) );

			for (int i = 0; i < len; ++i) {
				colors.at(pos + i) = e.second;
			}

			pos += len;
			str = match.suffix();
		}
	}
	bool inWord( false );
	int wordStart( 0 );
	int wordEnd( 0 );
	int colorOffset( 0 );
	auto dohl = [&](int i) {
		inWord = false;
		std::string intermission( context.substr( wordEnd, wordStart - wordEnd ) );
		colorOffset += utf8str_codepoint_len( intermission.c_str(), intermission.length() );
		int wordLen( i - wordStart );
		std::string keyword( context.substr( wordStart, wordLen ) );
		bool bold( false );
		if ( keyword.substr( 0, 5 ) == "bold_" ) {
			keyword = keyword.substr( 5 );
			bold = true;
		}
		bool underline( false );
		if ( keyword.substr( 0, 10 ) == "underline_" ) {
			keyword = keyword.substr( 10 );
			underline = true;
		}
		keyword_highlight_t::const_iterator it( word_color.find( keyword ) );
		Replxx::Color color = Replxx::Color::DEFAULT;
		if ( it != word_color.end() ) {
			color = it->second;
		}
		if ( bold ) {
			color = replxx::color::bold( color );
		}
		if ( underline ) {
			color = replxx::color::underline( color );
		}
		for ( int k( 0 ); k < wordLen; ++ k ) {
			Replxx::Color& c( colors.at( colorOffset + k ) );
			if ( color != Replxx::Color::DEFAULT ) {
				c = color;
			}
		}
		colorOffset += wordLen;
		wordEnd = i;
	};
	for ( int i( 0 ); i < static_cast<int>( context.length() ); ++ i ) {
		if ( !inWord ) {
			if ( is_kw( context[i] ) ) {
				inWord = true;
				wordStart = i;
			}
		} else if ( inWord && !is_kw( context[i] ) ) {
			dohl(i);
		}
		if ( ( context[i] != '_' ) && ispunct( context[i] ) ) {
			wordStart = i;
			dohl( i + 1 );
		}
	}
	if ( inWord ) {
		dohl(context.length());
	}
}

void hook_modify( std::string& currentInput_, int&, replxx_ptr rx ) {
	char prompt[64];
	snprintf( prompt, 64, "\x1b[1;32mreplxx\x1b[0m[%lu]> ", currentInput_.length() );
	rx->set_prompt( prompt );
}

Replxx::ACTION_RESULT message( replxx_ptr& replxx, std::string s, char32_t ) {
	replxx->invoke( Replxx::ACTION::CLEAR_SELF, 0 );
	replxx->print( "%s\n", s.c_str() );
	replxx->invoke( Replxx::ACTION::REPAINT, 0 );
	return ( Replxx::ACTION_RESULT::CONTINUE );
}

replxx_ptr cmd_line_parser::init_replxx(replxx_config& conf){
	// highlight specific words
	// a regex string, and a color
	// the order matters, the last match will take precedence

	static int const MAX_LABEL_NAME( 32 );
	char label[MAX_LABEL_NAME];
	for ( int r( 0 ); r < 6; ++ r ) {
		for ( int g( 0 ); g < 6; ++ g ) {
			for ( int b( 0 ); b < 6; ++ b ) {
				snprintf( label, MAX_LABEL_NAME, "rgb%d%d%d", r, g, b );
				conf.word_color.insert( std::make_pair( label, replxx::color::rgb666( r, g, b ) ) );
				for ( int br( 0 ); br < 6; ++ br ) {
					for ( int bg( 0 ); bg < 6; ++ bg ) {
						for ( int bb( 0 ); bb < 6; ++ bb ) {
							snprintf( label, MAX_LABEL_NAME, "fg%d%d%dbg%d%d%d", r, g, b, br, bg, bb );
							conf.word_color.insert(
								std::make_pair(
									label,
									rgb666( r, g, b ) | replxx::color::bg( rgb666( br, bg, bb ) )
								)
							);
						}
					}
				}
			}
		}
	}
	for ( int gs( 0 ); gs < 24; ++ gs ) {
		snprintf( label, MAX_LABEL_NAME, "gs%d", gs );
		conf.word_color.insert( std::make_pair( label, grayscale( gs ) ) );
		for ( int bgs( 0 ); bgs < 24; ++ bgs ) {
			snprintf( label, MAX_LABEL_NAME, "gs%dgs%d", gs, bgs );
			conf.word_color.insert( std::make_pair( label, grayscale( gs ) | bg( grayscale( bgs ) ) ) );
		}
	}
	Replxx::Color colorCodes[] = {
		Replxx::Color::BLACK, Replxx::Color::RED, Replxx::Color::GREEN, Replxx::Color::BROWN, Replxx::Color::BLUE,
		Replxx::Color::CYAN, Replxx::Color::MAGENTA, Replxx::Color::LIGHTGRAY, Replxx::Color::GRAY, Replxx::Color::BRIGHTRED,
		Replxx::Color::BRIGHTGREEN, Replxx::Color::YELLOW, Replxx::Color::BRIGHTBLUE, Replxx::Color::BRIGHTCYAN,
		Replxx::Color::BRIGHTMAGENTA, Replxx::Color::WHITE
	};
	for ( Replxx::Color bg : colorCodes ) {
		for ( Replxx::Color fg : colorCodes ) {
			snprintf( label, MAX_LABEL_NAME, "c_%d_%d", static_cast<int>( fg ), static_cast<int>( bg ) );
			conf.word_color.insert( std::make_pair( label, fg | replxx::color::bg( bg ) ) );
		}
	}

	// init the repl
	replxx_ptr rx = std::make_shared<Replxx>();

	rx->install_window_change_handler();

	// load the history file if it exists
	/* scope for ifstream object for auto-close */ {
		std::ifstream history_file( conf.history_file_path.c_str() );
		rx->history_load( history_file );
	}

	// set the max history size
	rx->set_max_history_size(10000);

	// set the max number of hint rows to show
	rx->set_max_hint_rows(3);

	// set the callbacks
	using namespace std::placeholders;
	rx->set_completion_callback( std::bind( &hook_completion, _1, _2, cref( conf.commands ), conf.ignoreCase ) );
	rx->set_highlighter_callback( std::bind( &hook_color, _1, _2, cref( conf.regex_color ), cref( conf.word_color ) ) );
	rx->set_hint_callback( std::bind( &hook_hint, _1, _2, _3, cref( conf.commands ), conf.ignoreCase ) );
	if ( conf.promptInCallback ) {
		rx->set_modify_callback( std::bind( &hook_modify, _1, _2, rx ) );
	}

	// other api calls
	rx->set_word_break_characters( " \n\t.,-%!;:=*~^'\"/?<>|[](){}" );
	rx->set_completion_count_cutoff( 128 );
	rx->set_hint_delay( conf.hintDelay );
	rx->set_double_tab_completion( false );
	rx->set_complete_on_empty( true );
	rx->set_beep_on_ambiguous_completion( false );
	rx->set_no_color( false );
	rx->set_indent_multiline( conf.indentMultiline );
	if ( conf.bracketedPaste ) {
		rx->enable_bracketed_paste();
	}
	rx->set_ignore_case( conf.ignoreCase );

	// showcase key bindings
	rx->bind_key_internal( Replxx::KEY::BACKSPACE,                      "delete_character_left_of_cursor" );
	rx->bind_key_internal( Replxx::KEY::DELETE,                         "delete_character_under_cursor" );
	rx->bind_key_internal( Replxx::KEY::LEFT,                           "move_cursor_left" );
	rx->bind_key_internal( Replxx::KEY::RIGHT,                          "move_cursor_right" );
	rx->bind_key_internal( Replxx::KEY::UP,                             "line_previous" );
	rx->bind_key_internal( Replxx::KEY::DOWN,                           "line_next" );
	rx->bind_key_internal( Replxx::KEY::meta( Replxx::KEY::UP ),        "history_previous" );
	rx->bind_key_internal( Replxx::KEY::meta( Replxx::KEY::DOWN ),      "history_next" );
	rx->bind_key_internal( Replxx::KEY::PAGE_UP,                        "history_first" );
	rx->bind_key_internal( Replxx::KEY::PAGE_DOWN,                      "history_last" );
	rx->bind_key_internal( Replxx::KEY::HOME,                           "move_cursor_to_begining_of_line" );
	rx->bind_key_internal( Replxx::KEY::END,                            "move_cursor_to_end_of_line" );
	rx->bind_key_internal( Replxx::KEY::TAB,                            "complete_line" );
	rx->bind_key_internal( Replxx::KEY::control( Replxx::KEY::LEFT ),   "move_cursor_one_word_left" );
	rx->bind_key_internal( Replxx::KEY::control( Replxx::KEY::RIGHT ),  "move_cursor_one_word_right" );
	rx->bind_key_internal( Replxx::KEY::control( Replxx::KEY::UP ),     "hint_previous" );
	rx->bind_key_internal( Replxx::KEY::control( Replxx::KEY::DOWN ),   "hint_next" );
	rx->bind_key_internal( Replxx::KEY::control( Replxx::KEY::ENTER ),  "commit_line" );
	rx->bind_key_internal( Replxx::KEY::control( 'R' ),                 "history_incremental_search" );
	rx->bind_key_internal( Replxx::KEY::control( 'W' ),                 "kill_to_begining_of_word" );
	rx->bind_key_internal( Replxx::KEY::control( 'U' ),                 "kill_to_begining_of_line" );
	rx->bind_key_internal( Replxx::KEY::control( 'K' ),                 "kill_to_end_of_line" );
	rx->bind_key_internal( Replxx::KEY::control( 'Y' ),                 "yank" );
	rx->bind_key_internal( Replxx::KEY::control( 'L' ),                 "clear_screen" );
	rx->bind_key_internal( Replxx::KEY::control( 'D' ),                 "send_eof" );
	rx->bind_key_internal( Replxx::KEY::control( 'C' ),                 "abort_line" );
	rx->bind_key_internal( Replxx::KEY::control( 'T' ),                 "transpose_characters" );
#ifndef _WIN32
	rx->bind_key_internal( Replxx::KEY::control( 'V' ),                 "verbatim_insert" );
	rx->bind_key_internal( Replxx::KEY::control( 'Z' ),                 "suspend" );
#endif
	rx->bind_key_internal( Replxx::KEY::meta( Replxx::KEY::BACKSPACE ), "kill_to_whitespace_on_left" );
	rx->bind_key_internal( Replxx::KEY::meta( 'p' ),                    "history_common_prefix_search" );
	rx->bind_key_internal( Replxx::KEY::meta( 'n' ),                    "history_common_prefix_search" );
	rx->bind_key_internal( Replxx::KEY::meta( 'd' ),                    "kill_to_end_of_word" );
	rx->bind_key_internal( Replxx::KEY::meta( 'y' ),                    "yank_cycle" );
	rx->bind_key_internal( Replxx::KEY::meta( 'u' ),                    "uppercase_word" );
	rx->bind_key_internal( Replxx::KEY::meta( 'l' ),                    "lowercase_word" );
	rx->bind_key_internal( Replxx::KEY::meta( 'c' ),                    "capitalize_word" );
	rx->bind_key_internal( 'a',                                         "insert_character" );
	rx->bind_key_internal( Replxx::KEY::INSERT,                         "toggle_overwrite_mode" );
	rx->bind_key( Replxx::KEY::F1, std::bind( &message, std::ref( rx ), "<F1>", _1 ) );
	rx->bind_key( Replxx::KEY::F2, std::bind( &message, std::ref( rx ), "<F2>", _1 ) );
	rx->bind_key( Replxx::KEY::F3, std::bind( &message, std::ref( rx ), "<F3>", _1 ) );
	rx->bind_key( Replxx::KEY::F4, std::bind( &message, std::ref( rx ), "<F4>", _1 ) );
	rx->bind_key( Replxx::KEY::F5, std::bind( &message, std::ref( rx ), "<F5>", _1 ) );
	rx->bind_key( Replxx::KEY::F6, std::bind( &message, std::ref( rx ), "<F6>", _1 ) );
	rx->bind_key( Replxx::KEY::F7, std::bind( &message, std::ref( rx ), "<F7>", _1 ) );
	rx->bind_key( Replxx::KEY::F8, std::bind( &message, std::ref( rx ), "<F8>", _1 ) );
	rx->bind_key( Replxx::KEY::F9, std::bind( &message, std::ref( rx ), "<F9>", _1 ) );
	rx->bind_key( Replxx::KEY::F10, std::bind( &message, std::ref( rx ), "<F10>", _1 ) );
	rx->bind_key( Replxx::KEY::F11, std::bind( &message, std::ref( rx ), "<F11>", _1 ) );
	rx->bind_key( Replxx::KEY::F12, std::bind( &message, std::ref( rx ), "<F12>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::F1 ), std::bind( &message, std::ref( rx ), "<S-F1>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::F2 ), std::bind( &message, std::ref( rx ), "<S-F2>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::F3 ), std::bind( &message, std::ref( rx ), "<S-F3>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::F4 ), std::bind( &message, std::ref( rx ), "<S-F4>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::F5 ), std::bind( &message, std::ref( rx ), "<S-F5>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::F6 ), std::bind( &message, std::ref( rx ), "<S-F6>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::F7 ), std::bind( &message, std::ref( rx ), "<S-F7>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::F8 ), std::bind( &message, std::ref( rx ), "<S-F8>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::F9 ), std::bind( &message, std::ref( rx ), "<S-F9>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::F10 ), std::bind( &message, std::ref( rx ), "<S-F10>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::F11 ), std::bind( &message, std::ref( rx ), "<S-F11>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::F12 ), std::bind( &message, std::ref( rx ), "<S-F12>", _1 ) );
	rx->bind_key( Replxx::KEY::control( Replxx::KEY::F1 ), std::bind( &message, std::ref( rx ), "<C-F1>", _1 ) );
	rx->bind_key( Replxx::KEY::control( Replxx::KEY::F2 ), std::bind( &message, std::ref( rx ), "<C-F2>", _1 ) );
	rx->bind_key( Replxx::KEY::control( Replxx::KEY::F3 ), std::bind( &message, std::ref( rx ), "<C-F3>", _1 ) );
	rx->bind_key( Replxx::KEY::control( Replxx::KEY::F4 ), std::bind( &message, std::ref( rx ), "<C-F4>", _1 ) );
	rx->bind_key( Replxx::KEY::control( Replxx::KEY::F5 ), std::bind( &message, std::ref( rx ), "<C-F5>", _1 ) );
	rx->bind_key( Replxx::KEY::control( Replxx::KEY::F6 ), std::bind( &message, std::ref( rx ), "<C-F6>", _1 ) );
	rx->bind_key( Replxx::KEY::control( Replxx::KEY::F7 ), std::bind( &message, std::ref( rx ), "<C-F7>", _1 ) );
	rx->bind_key( Replxx::KEY::control( Replxx::KEY::F8 ), std::bind( &message, std::ref( rx ), "<C-F8>", _1 ) );
	rx->bind_key( Replxx::KEY::control( Replxx::KEY::F9 ), std::bind( &message, std::ref( rx ), "<C-F9>", _1 ) );
	rx->bind_key( Replxx::KEY::control( Replxx::KEY::F10 ), std::bind( &message, std::ref( rx ), "<C-F10>", _1 ) );
	rx->bind_key( Replxx::KEY::control( Replxx::KEY::F11 ), std::bind( &message, std::ref( rx ), "<C-F11>", _1 ) );
	rx->bind_key( Replxx::KEY::control( Replxx::KEY::F12 ), std::bind( &message, std::ref( rx ), "<C-F12>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::TAB ), std::bind( &message, std::ref( rx ), "<S-Tab>", _1 ) );
	rx->bind_key( Replxx::KEY::control( Replxx::KEY::HOME ), std::bind( &message, std::ref( rx ), "<C-Home>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::HOME ), std::bind( &message, std::ref( rx ), "<S-Home>", _1 ) );
	rx->bind_key( Replxx::KEY::control( Replxx::KEY::END ), std::bind( &message, std::ref( rx ), "<C-End>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::END ), std::bind( &message, std::ref( rx ), "<S-End>", _1 ) );
	rx->bind_key( Replxx::KEY::control( Replxx::KEY::PAGE_UP ), std::bind( &message, std::ref( rx ), "<C-PgUp>", _1 ) );
	rx->bind_key( Replxx::KEY::control( Replxx::KEY::PAGE_DOWN ), std::bind( &message, std::ref( rx ), "<C-PgDn>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::LEFT ), std::bind( &message, std::ref( rx ), "<S-Left>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::RIGHT ), std::bind( &message, std::ref( rx ), "<S-Right>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::UP ), std::bind( &message, std::ref( rx ), "<S-Up>", _1 ) );
	rx->bind_key( Replxx::KEY::shift( Replxx::KEY::DOWN ), std::bind( &message, std::ref( rx ), "<S-Down>", _1 ) );
	rx->bind_key( Replxx::KEY::meta( '\r' ), std::bind( &message, std::ref( rx ), "<M-Enter>", _1 ) );

	return rx;
}

std::string cmd_line_parser::format_threads(const thread_info_map_t& tmap){
	std::ostringstream oss;
	//head
	oss << "IDX      Thread Id        Status       Name\n"
		;

	constexpr int BUF_LEN = 4096;
	char buf[BUF_LEN] = {0};
	for(const auto &th : tmap){
		memset(buf, 0, BUF_LEN);
		snprintf(buf, BUF_LEN
		    , "[%-6llu] %-16llu %-12s %s\n"
			,  th.second.idx
		    ,  th.first
			,  get_state_name(th.second.state).c_str()
			,  th.second.name.c_str()
			);
		oss << buf;
	}
	return oss.str();
}

std::string cmd_line_parser::format_threads(const pid_set_t& tset){
	std::ostringstream oss;
	//head
	oss << "IDX      Thread Id        Status       Name\n"
		;

	constexpr int BUF_LEN = 4096;
	char buf[BUF_LEN] = {0};
	for(const auto &th : tset){
		memset(buf, 0, BUF_LEN);

		auto it = m_threads_map.find(th);
		if(it == m_threads_map.end()){
			snprintf(buf, BUF_LEN
				, "[%-6lld] %-16llu %s\n"
				,  -1
				,  th
				,  "err: can not find it"
				);
			oss << buf;
			continue;
		}
		snprintf(buf, BUF_LEN
		    , "[%-6llu] %-16llu %-12s %s\n"
			,  it->second.idx
		    ,  it->first
			,  get_state_name(it->second.state).c_str()
			,  it->second.name.c_str()
			);
		oss << buf;
	}
	return oss.str();
}

std::string cmd_line_parser::format_threads_no_info(const pid_set_t& tset){
	std::ostringstream oss;
	size_t idx  = 0;
	for(const auto &th : tset){
		if(idx != 0){
			oss << ",";
		}
		oss << th;
		++idx;
	}
	return oss.str();
}

int cmd_line_parser::freeze(const pid_set_t& tset){
	size_t cnt  = 0;
	for(const pid_t &t : tset){
		if(0 != m_tc.trace_pid(t)){
			m_err_op_thr_set.insert(t);
		}
		else{
			++cnt;
			auto it = m_threads_map.find(t);
			if(it != m_threads_map.end()){
				it->second.state = TS_FREEZED;
			}
		}
	}
	return cnt;
}

void cmd_line_parser::freeze_out_info(const pid_set_t& tset, replxx_ptr rx){
	std::ostringstream oss;
	if(rx){
		rx->print("freeze threads ....\n");
	}
	else{
		std::cout << "freeze threads ....\n";
	}
	
	size_t fz_cnt = freeze(tset);
	if(fz_cnt){
		oss << "freeze threads finish, succ cnt " << fz_cnt << "\n";
	}

	if(m_err_op_thr_set.size()){
		oss << "\tfailed cnt " << m_err_op_thr_set.size() << " :\n"
			<< format_threads_no_info(m_err_op_thr_set)
			<< "\n"
			;
	}

	if(rx){
		rx->print("%s", oss.str().c_str());
	}
	else{
		std::cout << oss.str();
	}
}

int cmd_line_parser::unfreeze(const pid_set_t& tset){
	size_t cnt  = 0;
	for(const pid_t &t : tset){
		if(0 != m_tc.detrace_pid(t)){
			m_err_op_thr_set.insert(t);
		}
		else{
			++cnt;
			auto it = m_threads_map.find(t);
			if(it != m_threads_map.end()){
				it->second.state = TS_RUNNING;
			}
		}
	}
	return cnt;
}

void cmd_line_parser::unfreeze_out_info(const pid_set_t& tset, replxx_ptr rx){
	std::ostringstream oss;
	if(rx){
		rx->print("unfreeze threads ....\n");
	}
	else{
		std::cout << "unfreeze threads ....\n";
	}
	
	size_t fz_cnt = unfreeze(tset);
	if(fz_cnt){
		oss << "unfreeze threads finish, succ cnt " << fz_cnt << "\n";
	}

	if(m_err_op_thr_set.size()){
		oss << "\tfailed cnt " << m_err_op_thr_set.size() << " :\n"
			<< format_threads_no_info(m_err_op_thr_set)
			<< "\n"
			;
	}

	if(rx){
		rx->print("%s", oss.str().c_str());
	}
	else{
		std::cout << oss.str();
	}
}