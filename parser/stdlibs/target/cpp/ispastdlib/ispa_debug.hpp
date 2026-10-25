#pragma once
// ISPA debug transport v2, C++17. Structured events are JSON carried inside
// length-framed ISDB records. Debugger stepping uses a separate command pipe.
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#ifdef _WIN32
# include <io.h>
#else
# include <unistd.h>
#endif

namespace ispa::debug {
enum class Kind : std::uint8_t { trace=1, value=2, enter_rule=3, exit_rule=4,
    diagnostic=5, breakpoint=6, lexer_enter=7, lexer_leave=8,
    lexer_capture=9, source_line=10, production_enter=11, production_leave=12,
    snapshot=13, session_end=14 };
inline constexpr std::size_t max_payload = 1024 * 1024;
inline std::mutex& mutex() { static std::mutex m; return m; }
inline int fd_env(const char* name) {
    const char* p=std::getenv(name); if (!p || !*p) return -1;
    char* end=nullptr; long n=std::strtol(p,&end,10);
    return (*end || n<0 || n>std::numeric_limits<int>::max()) ? -1 : int(n);
}
inline bool enabled() { return fd_env("ISPA_DEBUG_FD") >= 0; }
inline bool write_all(int fd, const char* p, std::size_t n) {
    while (n) {
#ifdef _WIN32
        int r=::_write(fd,p,static_cast<unsigned>(std::min<std::size_t>(n,0x7fffffff)));
#else
        auto r=::write(fd,p,n);
#endif
        if (r>0) {p+=r;n-=std::size_t(r);continue;}
#ifndef _WIN32
        if (r<0 && errno==EINTR) continue;
#endif
        return false;
    } return true;
}
inline bool read_exact(int fd, char* p, std::size_t n) {
    while(n) {
#ifdef _WIN32
        int r=::_read(fd,p,static_cast<unsigned>(std::min<std::size_t>(n,0x7fffffff)));
#else
        auto r=::read(fd,p,n);
#endif
        if(r>0){p+=r;n-=std::size_t(r);continue;}
#ifndef _WIN32
        if(r<0 && errno==EINTR)continue;
#endif
        return false;
    }return true;
}
inline bool emit(Kind kind,std::string_view payload) {
    const int fd=fd_env("ISPA_DEBUG_FD");
    if(fd<0 || payload.size()>max_payload)return false;
    const auto n=std::uint32_t(payload.size());
    const char h[12]={'I','S','D','B',2,char(kind),0,0,char(n),char(n>>8),char(n>>16),char(n>>24)};
    std::lock_guard<std::mutex> guard(mutex());
    return write_all(fd,h,12) && write_all(fd,payload.data(),payload.size());
}
inline std::string quote(std::string_view s) {
    std::string out="\""; out.reserve(s.size()+2);
    constexpr char hex[]="0123456789abcdef";
    for(unsigned char c:s) {
        switch(c){case '"':out+="\\\"";break;case '\\':out+="\\\\";break;
            case '\n':out+="\\n";break;case '\r':out+="\\r";break;
            case '\t':out+="\\t";break;
            default:if(c<32){out+="\\u00";out+=hex[c>>4];out+=hex[c&15];}
                    else out+=char(c);
        }
    }return out+'"';
}
// Structural serializer. All values retain a type name, including nested values.
// Add ADL function: template<class Visitor> void ispa_debug_fields(const MyType&, Visitor&&)
// to expose custom class fields without unsafe raw-memory dumps.
template<class T> struct type_name { static std::string get(){return "opaque";} };
#define ISPA_TYPE(T,N) template<> struct type_name<T>{static std::string get(){return N;}};
ISPA_TYPE(bool,"bool") ISPA_TYPE(int,"int") ISPA_TYPE(unsigned,"unsigned")
ISPA_TYPE(long,"long") ISPA_TYPE(unsigned long,"unsigned long")
ISPA_TYPE(long long,"long long") ISPA_TYPE(unsigned long long,"unsigned long long")
ISPA_TYPE(short,"short") ISPA_TYPE(unsigned short,"unsigned short")
ISPA_TYPE(float,"float") ISPA_TYPE(double,"double") ISPA_TYPE(char,"char")
ISPA_TYPE(std::string,"string")
#undef ISPA_TYPE

template<class T,class=void>struct has_fields:std::false_type{};
template<class T>struct has_fields<T,std::void_t<decltype(ispa_debug_fields(std::declval<const T&>(),std::declval<void(*)(const char*,int)>()))>>:std::true_type{};
// Use a templated probe callable to detect field visitation (including non-int values).
struct FieldProbe {template<class V>void operator()(std::string_view,const V&)const{}};
template<class T,class=void>struct visitable:std::false_type{};
template<class T>struct visitable<T,std::void_t<decltype(ispa_debug_fields(std::declval<const T&>(),FieldProbe{}))>>:std::true_type{};
template<class T,class=void>struct named_visitable:std::false_type{};
template<class T>struct named_visitable<T,std::void_t<decltype(ispa_debug_type(std::declval<const T&>()))>>:std::true_type{};
template<class T>std::string display_type(){
    if constexpr(named_visitable<T>::value && std::is_default_constructible_v<T>)
        return std::string(ispa_debug_type(T{}));
    else return typeid(T).name();
}
template<class T,class=void>struct cursor_like:std::false_type{};
template<class T>struct cursor_like<T,std::void_t<decltype(std::declval<const T&>().distance())>>:std::true_type{};

template<class T> std::string encode(const T& v,int depth=0);
inline std::string object(std::string_view type,std::string data){return "{\"type\":"+quote(type)+",\"value\":"+data+'}';}
inline std::string encode(const std::string& v,int=0){return object("string",quote(v));}
inline std::string encode(std::string_view v,int=0){return object("string",quote(v));}
inline std::string encode(const char* v,int=0){return v?object("string",quote(v)):object("null","null");}
inline std::string encode(char* v,int=0){return encode(static_cast<const char*>(v));}
inline std::string encode(bool v,int=0){return object("bool",v?"true":"false");}
inline std::string encode(char v,int=0){return object("char",quote(std::string(1,v)));}
template<class T> std::enable_if_t<std::is_arithmetic_v<T> && !std::is_same_v<T,bool> && !std::is_same_v<T,char>,std::string>
number(T v,int=0){std::ostringstream s;s<<std::setprecision(17)<<v;return object(type_name<T>::get(),s.str());}
template<class T>std::string encode(const std::vector<T>& v,int depth){
    if(depth>=16)return object("list","\"<depth limit>\"");
    std::string s="[";std::size_t n=0;
    for(auto& x:v){if(n++>=256)break;if(n>1)s+=',';s+=encode(x,depth+1);}return object("list",s+']');
}
template<class T>std::string encode(const std::optional<T>& v,int depth){return object("optional",v?encode(*v,depth+1):"null");}
template<class T>std::string encode(const std::shared_ptr<T>& v,int depth){return object("shared_ptr",v?encode(*v,depth+1):"null");}
template<class...T>std::string encode(const std::variant<T...>& v,int depth){return object("variant", "{\"index\":"+std::to_string(v.index())+",\"item\":"+std::visit([&](const auto& x){return encode(x,depth+1);},v)+'}');}
template<class K,class V,class C,class A>std::string encode(const std::map<K,V,C,A>& v,int depth){
    if(depth>=16)return object("map","\"<depth limit>\"");
    std::string s="[";std::size_t n=0;
    for(const auto& [k,x]:v){if(n++>=256)break;if(n>1)s+=',';s+="["+encode(k,depth+1)+","+encode(x,depth+1)+"]";}
    return object("map",s+']');
}
template<class T>std::string encode(const T& v,int depth){
    using U=std::decay_t<T>;
    if constexpr(std::is_arithmetic_v<U> && !std::is_same_v<U,bool> && !std::is_same_v<U,char>)return number(v,depth);
    else if constexpr(std::is_pointer_v<U>)return object("pointer",v?"\"<address hidden>\"":"null");
    else if constexpr(cursor_like<U>::value)
        return object("ISPA.Cursor","{\"token_index\":"+std::to_string(v.distance())+'}');
    else if constexpr(visitable<U>::value){
        if(depth>=16)return object(type_name<U>::get(),"\"<depth limit>\"");
        std::string s="{";bool first=true;
        ispa_debug_fields(v,[&](std::string_view name,const auto& field){if(!first)s+=',';first=false;s+=quote(name)+":"+encode(field,depth+1);});
        if constexpr(named_visitable<U>::value)return object(ispa_debug_type(v),s+'}');
        else return object(type_name<U>::get(),s+'}');
    }else return object(type_name<U>::get(),"\"<unavailable: register ispa_debug_fields>\"");
}
struct Location{std::string_view file;int line=0,column=0;};
inline Location& active_location(){static thread_local Location loc;return loc;}
inline std::string location(Location l){return "\"file\":"+quote(l.file)+",\"line\":"+std::to_string(l.line)+",\"column\":"+std::to_string(l.column);}
inline bool event(Kind k,Location l,std::string_view name,std::string extra=""){
    return emit(k,"{"+location(l)+",\"name\":"+quote(name)+(extra.empty()?"":","+extra)+'}');
}
inline bool trace(std::string_view s){return event(Kind::trace,{},s);}
inline bool diagnostic(std::string_view s){return event(Kind::diagnostic,{},s);}
inline bool enter_rule(std::string_view name,Location l={}){return event(Kind::enter_rule,l,name);}
inline bool exit_rule(std::string_view name,Location l={}){return event(Kind::exit_rule,l,name);}
inline bool enter_production(std::string_view name,Location l={}){return event(Kind::production_enter,l,name);}
inline bool leave_production(std::string_view name,Location l={}){return event(Kind::production_leave,l,name);}
inline bool lexer_enter(std::string_view token,Location l={}){return event(Kind::lexer_enter,l,token);}
inline bool lexer_leave(std::string_view token,Location l={}){return event(Kind::lexer_leave,l,token);}
inline bool lexer_result(std::string_view token,bool success,std::string_view actual,
                         std::string_view reason={},Location l={}){
    return event(Kind::lexer_leave,l,token,"\"success\":"+std::string(success?"true":"false")+
        ",\"actual\":"+quote(actual)+",\"reason\":"+quote(reason));
}
template<class T>bool lexer_capture(std::string_view name,const T& x,Location l={}){return event(Kind::lexer_capture,l,name,"\"data\":"+encode(x));}
template<class T>bool value(std::string_view name,const T& x,Location l={}){return event(Kind::value,l,name,"\"data\":"+encode(x));}

class Snapshot {
    std::string values_="{";bool first_=true;
public:
    template<class T>Snapshot& add(std::string_view name,const T& v){
        if(!first_) values_+=',';
        first_=false; values_+=quote(name)+":"+encode(v); return *this;
    }
    std::string json(){return values_+'}';}
};

// Stepping protocol: debugger writes 1 byte commands to ISPA_DEBUG_CONTROL_FD.
// 's' step one source_line; 'c' continue; 'n' step over; 'f' step out;
// 'b' set breakpoint (file:line in subsequent newline-terminated bytes).
// Only source-line events are stepping checkpoints. The current snapshot is
// included atomically in each checkpoint event for deterministic inspection.
struct StepState {
    bool stepping=false;
    int target_depth=-1;
    int frame_depth=0;
    std::vector<std::pair<std::string,int>> breaks;
};
inline StepState& state(){
    static thread_local StepState s;
    static thread_local bool initialized=false;
    if(!initialized){initialized=true;const char* p=std::getenv("ISPA_DEBUG_INITIAL_PAUSE");s.stepping=p&&*p;}
    return s;
}
inline bool& failure_seen(){static thread_local bool seen=false;return seen;}
inline bool command(StepState& s){
    int fd=fd_env("ISPA_DEBUG_CONTROL_FD");if(fd<0)return false;
    for(;;){char c;if(!read_exact(fd,&c,1))return false;
        if(c=='s'){s.stepping=true;s.target_depth=-1;return true;}
        if(c=='c'){s.stepping=false;s.target_depth=-1;return true;}
        if(c=='n'){s.stepping=true;s.target_depth=s.frame_depth;return true;}
        if(c=='f'){s.stepping=true;s.target_depth=s.frame_depth-1;return true;}
        if(c=='b'){
            std::string x;char ch;
            while(x.size()<8192 && read_exact(fd,&ch,1) && ch!='\n')x+=ch;
            auto split=x.rfind(':');if(split!=std::string::npos){
                try{s.breaks.emplace_back(x.substr(0,split),std::stoi(x.substr(split+1)));}catch(...){}
            }
        }
    }
}
inline bool line(Location loc,Snapshot snap=Snapshot{},std::string_view phase="before"){
    failure_seen()=false;
    auto& s=state();bool hit=s.stepping && (s.target_depth<0 || s.frame_depth<=s.target_depth);
    for(const auto& bp:s.breaks)if(bp.first==loc.file && bp.second==loc.line)hit=true;
    bool can_pause=fd_env("ISPA_DEBUG_CONTROL_FD")>=0;
    const std::string payload=location(loc)+",\"depth\":"+std::to_string(s.frame_depth)+
       ",\"paused\":"+(hit&&can_pause?"true":"false")+",\"phase\":"+quote(phase)+
       ",\"variables\":"+snap.json();
    const bool ok=emit(Kind::source_line,"{"+payload+'}');
    if(hit&&can_pause)command(s);
    return ok;
}
inline void initial_pause(){auto& s=state();s.stepping=true;}
inline void rule_depth_enter(){++state().frame_depth;}
inline void rule_depth_leave(){--state().frame_depth;}
struct RuleScope{
    std::string name;Location loc,previous;
    RuleScope(std::string_view n,Location l={}):name(n),loc(l),previous(active_location()){
        active_location()=l;enter_rule(name,loc);rule_depth_enter();
    }
    ~RuleScope(){rule_depth_leave();exit_rule(name,loc);active_location()=previous;}
    RuleScope(const RuleScope&)=delete;RuleScope& operator=(const RuleScope&)=delete;
};
struct ProductionScope{
    std::string name;Location loc,previous;
    ProductionScope(std::string_view n,Location l={}):name(n),loc(l),previous(active_location()){
        active_location()=l;enter_production(name,loc);rule_depth_enter();
    }
    ~ProductionScope(){rule_depth_leave();leave_production(name,loc);active_location()=previous;}
    ProductionScope(const ProductionScope&)=delete;ProductionScope& operator=(const ProductionScope&)=delete;
};
struct MatchScope {
    std::string expected, actual, reason="no match";
    Location loc;
    bool matched=false, active=false;
    MatchScope(std::string_view want,std::string got,Location l=active_location())
        :expected(want),actual(std::move(got)),loc(l),active(enabled()){
        if(active)lexer_enter(expected,loc);
    }
    void success(){matched=true;reason="matched";}
    void outcome(std::string_view why){reason=why;}
    ~MatchScope(){if(active)lexer_result(expected,matched,actual,reason,loc);}
    MatchScope(const MatchScope&)=delete;MatchScope& operator=(const MatchScope&)=delete;
};
inline void breakpoint(std::string_view label={}){
    if(failure_seen())return;
    failure_seen()=true;
    const bool can_pause=fd_env("ISPA_DEBUG_CONTROL_FD")>=0;
    event(Kind::breakpoint,{},label,std::string("\"paused\":")+(can_pause?"true":"false"));
    auto& s=state();s.stepping=true;s.target_depth=-1;
    if(can_pause)command(s);
}
} // namespace ispa::debug

#define ISPA_DEBUG_LINE(file,ln,ctx) ::ispa::debug::line({file,ln,0},::ispa::debug::Snapshot{}.add("ctx",ctx))
#define ISPA_DEBUG_LOCAL(snapshot,name) (snapshot).add(#name,name)
