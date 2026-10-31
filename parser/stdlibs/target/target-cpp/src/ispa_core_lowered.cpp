#include "ispa_core_lowered.h"

#include <ispastdlib/python_collections.hpp>
#include <ispastdlib/failure.hpp>
#include <ispastdlib/object.hpp>
#include <ispastdlib/core.hpp>

auto python::ispa_std::chars::code_of(const std::string& ch) -> long long {
	auto code = ISPA_STD::python_char_code(ch);
	if (((code) > (255))) {
		return 255;
	}
	return code;
}
auto python::ispa_std::chars::char_of(long long code) -> std::string {
	if (((((code) < (0))) || (((code) > (255))))) {
		return "";
	}
	return ISPA_STD::python_char_from_code(code);
}
auto python::ispa_std::chars::code_at(const std::string& s, long long index) -> long long {
	"code_of(at(s, index)) without building the one-character string.";
	if (((((index) < (0))) || (((index) >= (static_cast<long long>(s.size())))))) {
		return 0;
	}
	auto code = ISPA_STD::python_char_code(std::string(1, (s)[index]));
	if (((code) > (255))) {
		return 255;
	}
	return code;
}
auto python::ispa_std::chars::at(const std::string& s, long long index) -> std::string {
	"s[index], or \"\" outside the string (the C++ NUL terminator).";
	if (((((index) < (0))) || (((index) >= (static_cast<long long>(s.size())))))) {
		return "";
	}
	return std::string(1, (s)[index]);
}
auto python::ispa_std::chars::is_space(const std::string& ch) -> bool {
	if (((static_cast<long long>(ch.size())) == (0))) {
		return false;
	}
	return ((((((((((((ch) == (" "))) || (((ch) == ("\t"))))) || (((ch) == ("\n"))))) || (((ch) == ("\r"))))) || (((ch) == ("\v"))))) || (((ch) == ("\f"))));
}
auto python::ispa_std::chars::is_digit(const std::string& ch) -> bool {
	if (((static_cast<long long>(ch.size())) == (0))) {
		return false;
	}
	return ((((ch) >= ("0"))) && (((ch) <= ("9"))));
}
long long python::ispa_std::errors::ErrorKind::OutOfRange = 1;
long long python::ispa_std::errors::ErrorKind::InvalidArgument = 2;
long long python::ispa_std::errors::ErrorKind::LengthError = 3;
long long python::ispa_std::errors::ErrorKind::BadOptionalAccess = 4;
long long python::ispa_std::errors::ErrorKind::KeyNotFound = 5;
long long python::ispa_std::errors::ErrorKind::LogicError = 6;
long long python::ispa_std::errors::ErrorKind::RuntimeFailure = 7;
long long python::ispa_std::errors::ErrorKind::InternalError = 8;
long long python::ispa_std::errors::ErrorKind::ApiError = 9;
long long python::ispa_std::errors::ErrorKind::NoMatch = 10;
long long python::ispa_std::errors::ErrorKind::ExternalParse = 11;
auto python::ispa_std::string::substr(const std::string& s, long long pos, long long count) -> std::string {
	long long n = static_cast<long long>(s.size());
	if (((((pos) < (0))) || (((pos) > (n))))) {
		::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::OutOfRange, "string.substr: pos out of range");
	}
	long long end = n;
	if (((((count) >= (0))) && (((((pos) + (count))) < (n))))) {
		end = ((pos) + (count));
	}
	std::string out = "";
	long long i = pos;
	while (((i) < (end))) {
		out = ((out) + (std::string(1, (s)[i])));
		i = ((i) + (1));
	}
	return out;
}
auto python::ispa_std::string::compare(const std::string& a, const std::string& b) -> long long {
	long long i = 0;
	while (((((i) < (static_cast<long long>(a.size())))) && (((i) < (static_cast<long long>(b.size())))))) {
		if (((std::string(1, (a)[i])) < (std::string(1, (b)[i])))) {
			return ( - (1));
		}
		if (((std::string(1, (b)[i])) < (std::string(1, (a)[i])))) {
			return 1;
		}
		i = ((i) + (1));
	}
	if (((static_cast<long long>(a.size())) < (static_cast<long long>(b.size())))) {
		return ( - (1));
	}
	if (((static_cast<long long>(b.size())) < (static_cast<long long>(a.size())))) {
		return 1;
	}
	return 0;
}
auto python::ispa_std::string::matches_at(const std::string& s, const std::string& needle, long long pos) -> bool {
	if (((((pos) < (0))) || (((((pos) + (static_cast<long long>(needle.size())))) > (static_cast<long long>(s.size())))))) {
		return false;
	}
	long long i = 0;
	while (((i) < (static_cast<long long>(needle.size())))) {
		if (((std::string(1, (s)[((pos) + (i))])) != (std::string(1, (needle)[i])))) {
			return false;
		}
		i = ((i) + (1));
	}
	return true;
}
auto python::ispa_std::string::starts_with(const std::string& s, const std::string& prefix) -> bool {
	return python::ispa_std::string::matches_at(s, prefix, 0);
}
auto python::ispa_std::string::ends_with(const std::string& s, const std::string& suffix) -> bool {
	return python::ispa_std::string::matches_at(s, suffix, ((static_cast<long long>(s.size())) - (static_cast<long long>(suffix.size()))));
}
auto python::ispa_std::string::find(const std::string& s, const std::string& needle, long long start) -> long long {
	if (((((start) < (0))) || (((start) > (static_cast<long long>(s.size())))))) {
		return python::ispa_std::string::NPOS;
	}
	long long pos = start;
	while (((((pos) + (static_cast<long long>(needle.size())))) <= (static_cast<long long>(s.size())))) {
		if (python::ispa_std::string::matches_at(s, needle, pos)) {
			return pos;
		}
		pos = ((pos) + (1));
	}
	return python::ispa_std::string::NPOS;
}
auto python::ispa_std::string::rfind(const std::string& s, const std::string& needle) -> long long {
	long long pos = ((static_cast<long long>(s.size())) - (static_cast<long long>(needle.size())));
	while (((pos) >= (0))) {
		if (python::ispa_std::string::matches_at(s, needle, pos)) {
			return pos;
		}
		pos = ((pos) - (1));
	}
	return python::ispa_std::string::NPOS;
}
auto python::ispa_std::string::contains(const std::string& s, const std::string& needle) -> bool {
	return ((python::ispa_std::string::find(s, needle, 0)) != (python::ispa_std::string::NPOS));
}
auto python::ispa_std::format::int_to_string(long long n) -> std::string {
	if (((n) == (0))) {
		return "0";
	}
	bool negative = ((n) < (0));
	if (negative) {
		n = ((0) - (n));
	}
	std::string digits = "";
	while (((n) > (0))) {
		long long d = ((n) - (((((n) / (10))) * (10))));
		digits = ((std::string(1, ("0123456789")[d])) + (digits));
		n = ((n) / (10));
	}
	if (negative) {
		return (("-") + (digits));
	}
	return digits;
}
auto python::ispa_std::format::bool_to_string(bool value) -> std::string {
	if (value) {
		return "true";
	}
	return "false";
}
auto python::ispa_std::format::fmt(const std::string& format_text, const std::vector<std::string>& args) -> std::string {
	std::string res = format_text;
	long long cursor = 0;
	long long i = 0;
	while (((i) < (static_cast<long long>(args.size())))) {
		long long pos = python::ispa_std::string::find(res, "{}", cursor);
		if (((pos) != (( - (1))))) {
			std::string value = (args)[i];
			res = ((((python::ispa_std::string::substr(res, 0, pos)) + (value))) + (python::ispa_std::string::substr(res, ((pos) + (2)), ( - (1)))));
			cursor = ((pos) + (static_cast<long long>(value.size())));
		}
		i = ((i) + (1));
	}
	return res;
}
auto python::ispa_std::format::replace_all(const std::string& s, const std::string& needle, const std::string& replacement) -> std::string {
	if (((static_cast<long long>(needle.size())) == (0))) {
		return s;
	}
	std::string out = "";
	long long i = 0;
	while (((i) < (static_cast<long long>(s.size())))) {
		long long pos = python::ispa_std::string::find(s, needle, i);
		if (((pos) == (( - (1))))) {
			out = ((out) + (python::ispa_std::string::substr(s, i, ( - (1)))));
			i = static_cast<long long>(s.size());
		} else {
			out = ((((out) + (python::ispa_std::string::substr(s, i, ((pos) - (i)))))) + (replacement));
			i = ((pos) + (static_cast<long long>(needle.size())));
		}
	}
	return out;
}
auto python::ispa_std::format::repeat(const std::string& s, long long count) -> std::string {
	std::string out = "";
	long long i = 0;
	while (((i) < (count))) {
		out = ((out) + (s));
		i = ((i) + (1));
	}
	return out;
}
auto python::ispa_std::format::join(const std::vector<std::string>& items, const std::string& separator) -> std::string {
	std::string out = "";
	long long i = 0;
	while (((i) < (static_cast<long long>(items.size())))) {
		if (((i) > (0))) {
			out = ((out) + (separator));
		}
		out = ((out) + ((items)[i]));
		i = ((i) + (1));
	}
	return out;
}
auto python::ispa_std::format::concat_all(const std::vector<std::string>& items) -> std::string {
	return python::ispa_std::format::join(items, "");
}
long long python::ispa_std::debug::Kind::trace = 1;
long long python::ispa_std::debug::Kind::value = 2;
long long python::ispa_std::debug::Kind::enter_rule = 3;
long long python::ispa_std::debug::Kind::exit_rule = 4;
long long python::ispa_std::debug::Kind::diagnostic = 5;
long long python::ispa_std::debug::Kind::breakpoint = 6;
long long python::ispa_std::debug::Kind::lexer_enter = 7;
long long python::ispa_std::debug::Kind::lexer_leave = 8;
long long python::ispa_std::debug::Kind::lexer_capture = 9;
long long python::ispa_std::debug::Kind::source_line = 10;
long long python::ispa_std::debug::Kind::production_enter = 11;
long long python::ispa_std::debug::Kind::production_leave = 12;
long long python::ispa_std::debug::Kind::snapshot = 13;
long long python::ispa_std::debug::Kind::session_end = 14;
auto python::ispa_std::debug::Location::__init__(const std::string& file, long long line, long long column) -> void {
	((*this)).file = file;
	((*this)).line = line;
	((*this)).column = column;
}
auto python::ispa_std::debug::Location::none() -> std::shared_ptr<python::ispa_std::debug::Location> {
	return ISPA_STD::construct<python::ispa_std::debug::Location>("", 0, 0);
}
auto python::ispa_std::debug::Location::copy() -> std::shared_ptr<python::ispa_std::debug::Location> {
	return ISPA_STD::construct<python::ispa_std::debug::Location>(((*this)).file, ((*this)).line, ((*this)).column);
}
auto python::ispa_std::debug::DebugSink::write(const std::string& text) -> bool {
	return false;
}
auto python::ispa_std::debug::DebugControl::read_byte() -> long long {
	return ( - (1));
}
auto python::ispa_std::debug::ScopeBase::close() -> void {
	return ;
}
auto python::ispa_std::debug::DebugState::__init__() -> void {
	((*this)).sink = ISPA_STD::construct<python::ispa_std::debug::DebugSink>();
	((*this)).has_sink = false;
	((*this)).control = ISPA_STD::construct<python::ispa_std::debug::DebugControl>();
	((*this)).has_control = false;
	((*this)).stepping = false;
	((*this)).target_depth = ( - (1));
	((*this)).frame_depth = 0;
	((*this)).break_files = {};
	((*this)).break_lines = {};
	((*this)).failure_seen = false;
	((*this)).location = python::ispa_std::debug::Location::none();
	((*this)).scopes = {};
	((*this)).serial = 0;
}
auto python::ispa_std::debug::configure(std::shared_ptr<python::ispa_std::debug::DebugSink> sink, std::shared_ptr<python::ispa_std::debug::DebugControl> control) -> void {
	((*python::ispa_std::debug::_S)).sink = sink;
	((*python::ispa_std::debug::_S)).has_sink = true;
	((*python::ispa_std::debug::_S)).control = control;
	((*python::ispa_std::debug::_S)).has_control = true;
}
auto python::ispa_std::debug::configure_sink_only(std::shared_ptr<python::ispa_std::debug::DebugSink> sink) -> void {
	((*python::ispa_std::debug::_S)).sink = sink;
	((*python::ispa_std::debug::_S)).has_sink = true;
	((*python::ispa_std::debug::_S)).has_control = false;
}
auto python::ispa_std::debug::reset() -> void {
	"Back to the unconfigured state (also clears stepping and scopes).";
	((*python::ispa_std::debug::_S)).has_sink = false;
	((*python::ispa_std::debug::_S)).has_control = false;
	((*python::ispa_std::debug::_S)).stepping = false;
	((*python::ispa_std::debug::_S)).target_depth = ( - (1));
	((*python::ispa_std::debug::_S)).frame_depth = 0;
	(((*python::ispa_std::debug::_S)).break_files).clear();
	(((*python::ispa_std::debug::_S)).break_lines).clear();
	((*python::ispa_std::debug::_S)).failure_seen = false;
	((*python::ispa_std::debug::_S)).location = python::ispa_std::debug::Location::none();
	(((*python::ispa_std::debug::_S)).scopes).clear();
}
auto python::ispa_std::debug::enabled() -> bool {
	return ((*python::ispa_std::debug::_S)).has_sink;
}
auto python::ispa_std::debug::initial_pause() -> void {
	((*python::ispa_std::debug::_S)).stepping = true;
}
auto python::ispa_std::debug::active_location() -> std::shared_ptr<python::ispa_std::debug::Location> {
	return ((*python::ispa_std::debug::_S)).location;
}
auto python::ispa_std::debug::frame(long long kind, const std::string& payload) -> std::string {
	long long n = static_cast<long long>(payload.size());
	std::string header = (((((((("ISDB") + (python::ispa_std::chars::char_of(2)))) + (python::ispa_std::chars::char_of(kind)))) + (python::ispa_std::chars::char_of(0)))) + (python::ispa_std::chars::char_of(0)));
	header = ((header) + (python::ispa_std::chars::char_of(((n) - (((((n) / (256))) * (256)))))));
	long long n1 = ((n) / (256));
	header = ((header) + (python::ispa_std::chars::char_of(((n1) - (((((n1) / (256))) * (256)))))));
	long long n2 = ((n1) / (256));
	header = ((header) + (python::ispa_std::chars::char_of(((n2) - (((((n2) / (256))) * (256)))))));
	long long n3 = ((n2) / (256));
	header = ((header) + (python::ispa_std::chars::char_of(((n3) - (((((n3) / (256))) * (256)))))));
	return ((header) + (payload));
}
auto python::ispa_std::debug::emit(long long kind, const std::string& payload) -> bool {
	if ((((!(((*python::ispa_std::debug::_S)).has_sink))) || (((static_cast<long long>(payload.size())) > (python::ispa_std::debug::MAX_PAYLOAD))))) {
		return false;
	}
	return ((*((*python::ispa_std::debug::_S)).sink)).write(python::ispa_std::debug::frame(kind, payload));
}
auto python::ispa_std::debug::quote(const std::string& s) -> std::string {
	std::string out = "\"";
	std::string hexd = "0123456789abcdef";
	long long i = 0;
	while (((i) < (static_cast<long long>(s.size())))) {
		std::string c = std::string(1, (s)[i]);
		long long code = python::ispa_std::chars::code_of(c);
		if (((c) == ("\""))) {
			out = ((out) + ("\\\""));
		} else {
			if (((c) == ("\\"))) {
				out = ((out) + ("\\\\"));
			} else {
				if (((c) == ("\n"))) {
					out = ((out) + ("\\n"));
				} else {
					if (((c) == ("\r"))) {
						out = ((out) + ("\\r"));
					} else {
						if (((c) == ("\t"))) {
							out = ((out) + ("\\t"));
						} else {
							if (((code) < (32))) {
								out = ((((((out) + ("\\u00"))) + (std::string(1, (hexd)[((code) / (16))])))) + (std::string(1, (hexd)[((code) - (((((code) / (16))) * (16))))])));
							} else {
								out = ((out) + (c));
							}
						}
					}
				}
			}
		}
		i = ((i) + (1));
	}
	return ((out) + ("\""));
}
auto python::ispa_std::debug::obj(const std::string& type_name, const std::string& data) -> std::string {
	return (((((((("{\"type\":") + (python::ispa_std::debug::quote(type_name)))) + (",\"value\":"))) + (data))) + ("}"));
}
auto python::ispa_std::debug::location_json(std::shared_ptr<python::ispa_std::debug::Location> loc) -> std::string {
	return (((((((((("\"file\":") + (python::ispa_std::debug::quote(((*loc)).file)))) + (",\"line\":"))) + (python::ispa_std::format::int_to_string(((*loc)).line)))) + (",\"column\":"))) + (python::ispa_std::format::int_to_string(((*loc)).column)));
}
auto python::ispa_std::debug::event(long long kind, std::shared_ptr<python::ispa_std::debug::Location> loc, const std::string& name, const std::string& extra) -> bool {
	std::string body = (((((("{") + (python::ispa_std::debug::location_json(loc)))) + (",\"name\":"))) + (python::ispa_std::debug::quote(name)));
	if (((static_cast<long long>(extra.size())) > (0))) {
		body = ((((body) + (","))) + (extra));
	}
	return python::ispa_std::debug::emit(kind, ((body) + ("}")));
}
auto python::ispa_std::debug::trace(const std::string& text) -> bool {
	return python::ispa_std::debug::event(python::ispa_std::debug::Kind::trace, python::ispa_std::debug::Location::none(), text, "");
}
auto python::ispa_std::debug::diagnostic(const std::string& text) -> bool {
	return python::ispa_std::debug::event(python::ispa_std::debug::Kind::diagnostic, python::ispa_std::debug::Location::none(), text, "");
}
auto python::ispa_std::debug::enter_rule(const std::string& name, std::shared_ptr<python::ispa_std::debug::Location> loc) -> bool {
	return python::ispa_std::debug::event(python::ispa_std::debug::Kind::enter_rule, loc, name, "");
}
auto python::ispa_std::debug::exit_rule(const std::string& name, std::shared_ptr<python::ispa_std::debug::Location> loc) -> bool {
	return python::ispa_std::debug::event(python::ispa_std::debug::Kind::exit_rule, loc, name, "");
}
auto python::ispa_std::debug::enter_production(const std::string& name, std::shared_ptr<python::ispa_std::debug::Location> loc) -> bool {
	return python::ispa_std::debug::event(python::ispa_std::debug::Kind::production_enter, loc, name, "");
}
auto python::ispa_std::debug::leave_production(const std::string& name, std::shared_ptr<python::ispa_std::debug::Location> loc) -> bool {
	return python::ispa_std::debug::event(python::ispa_std::debug::Kind::production_leave, loc, name, "");
}
auto python::ispa_std::debug::lexer_enter(const std::string& token, std::shared_ptr<python::ispa_std::debug::Location> loc) -> bool {
	return python::ispa_std::debug::event(python::ispa_std::debug::Kind::lexer_enter, loc, token, "");
}
auto python::ispa_std::debug::lexer_result(const std::string& token, bool success, const std::string& actual, const std::string& reason, std::shared_ptr<python::ispa_std::debug::Location> loc) -> bool {
	std::string flag = "false";
	if (success) {
		flag = "true";
	}
	return python::ispa_std::debug::event(python::ispa_std::debug::Kind::lexer_leave, loc, token, (((((((((("\"success\":") + (flag))) + (",\"actual\":"))) + (python::ispa_std::debug::quote(actual)))) + (",\"reason\":"))) + (python::ispa_std::debug::quote(reason))));
}
auto python::ispa_std::debug::value(const std::string& name, const std::string& encoded, std::shared_ptr<python::ispa_std::debug::Location> loc) -> bool {
	return python::ispa_std::debug::event(python::ispa_std::debug::Kind::value, loc, name, (("\"data\":") + (encoded)));
}
auto python::ispa_std::debug::lexer_capture(const std::string& name, const std::string& encoded, std::shared_ptr<python::ispa_std::debug::Location> loc) -> bool {
	return python::ispa_std::debug::event(python::ispa_std::debug::Kind::lexer_capture, loc, name, (("\"data\":") + (encoded)));
}
auto python::ispa_std::debug::Snapshot::__init__() -> void {
	((*this))._values = {};
}
auto python::ispa_std::debug::Snapshot::add(const std::string& name, const std::string& encoded) -> std::shared_ptr<python::ispa_std::debug::Snapshot> {
	(((*this))._values).push_back(((((python::ispa_std::debug::quote(name)) + (":"))) + (encoded)));
	return ::ISPA_STD::self_ptr(this);
}
auto python::ispa_std::debug::Snapshot::json() -> std::string {
	return (((("{") + (python::ispa_std::format::join(((*this))._values, ",")))) + ("}"));
}
auto python::ispa_std::debug::command() -> bool {
	"Block on the control channel for a stepping command. False when there is none.";
	if ((!(((*python::ispa_std::debug::_S)).has_control))) {
		return false;
	}
	while (true) {
		long long b = ((*((*python::ispa_std::debug::_S)).control)).read_byte();
		if (((b) < (0))) {
			return false;
		}
		std::string c = python::ispa_std::chars::char_of(b);
		if (((c) == ("s"))) {
			((*python::ispa_std::debug::_S)).stepping = true;
			((*python::ispa_std::debug::_S)).target_depth = ( - (1));
			return true;
		}
		if (((c) == ("c"))) {
			((*python::ispa_std::debug::_S)).stepping = false;
			((*python::ispa_std::debug::_S)).target_depth = ( - (1));
			return true;
		}
		if (((c) == ("n"))) {
			((*python::ispa_std::debug::_S)).stepping = true;
			((*python::ispa_std::debug::_S)).target_depth = ((*python::ispa_std::debug::_S)).frame_depth;
			return true;
		}
		if (((c) == ("f"))) {
			((*python::ispa_std::debug::_S)).stepping = true;
			((*python::ispa_std::debug::_S)).target_depth = ((((*python::ispa_std::debug::_S)).frame_depth) - (1));
			return true;
		}
		if (((c) == ("b"))) {
			std::string text = "";
			bool reading = true;
			while (((reading) && (((static_cast<long long>(text.size())) < (8192))))) {
				long long nb = ((*((*python::ispa_std::debug::_S)).control)).read_byte();
				if (((((nb) < (0))) || (((python::ispa_std::chars::char_of(nb)) == ("\n"))))) {
					reading = false;
				} else {
					text = ((text) + (python::ispa_std::chars::char_of(nb)));
				}
			}
			long long split = ( - (1));
			long long j = ((static_cast<long long>(text.size())) - (1));
			while (((((j) >= (0))) && (((split) == (( - (1))))))) {
				if (((std::string(1, (text)[j])) == (":"))) {
					split = j;
				}
				j = ((j) - (1));
			}
			if (((split) != (( - (1))))) {
				std::string digits = python::ispa_std::string::substr(text, ((split) + (1)), ( - (1)));
				long long number = 0;
				bool valid = ((static_cast<long long>(digits.size())) > (0));
				long long k = 0;
				while (((k) < (static_cast<long long>(digits.size())))) {
					long long d = ((python::ispa_std::chars::code_of(std::string(1, (digits)[k]))) - (48));
					if (((((d) < (0))) || (((d) > (9))))) {
						valid = false;
					} else {
						number = ((((number) * (10))) + (d));
					}
					k = ((k) + (1));
				}
				if (valid) {
					(((*python::ispa_std::debug::_S)).break_files).push_back(python::ispa_std::string::substr(text, 0, split));
					(((*python::ispa_std::debug::_S)).break_lines).push_back(number);
				}
			}
		}
	}
	return false;
}
auto python::ispa_std::debug::line(std::shared_ptr<python::ispa_std::debug::Location> loc, std::shared_ptr<python::ispa_std::debug::Snapshot> snap, const std::string& phase) -> bool {
	((*python::ispa_std::debug::_S)).failure_seen = false;
	bool hit = ((((*python::ispa_std::debug::_S)).stepping) && (((((((*python::ispa_std::debug::_S)).target_depth) < (0))) || (((((*python::ispa_std::debug::_S)).frame_depth) <= (((*python::ispa_std::debug::_S)).target_depth))))));
	long long i = 0;
	while (((i) < (static_cast<long long>(((*python::ispa_std::debug::_S)).break_files.size())))) {
		if ((((((((*python::ispa_std::debug::_S)).break_files)[i]) == (((*loc)).file))) && ((((((*python::ispa_std::debug::_S)).break_lines)[i]) == (((*loc)).line))))) {
			hit = true;
		}
		i = ((i) + (1));
	}
	bool can_pause = ((*python::ispa_std::debug::_S)).has_control;
	std::string paused = "false";
	if (((hit) && (can_pause))) {
		paused = "true";
	}
	std::string payload = ((((((((((((((((python::ispa_std::debug::location_json(loc)) + (",\"depth\":"))) + (python::ispa_std::format::int_to_string(((*python::ispa_std::debug::_S)).frame_depth)))) + (",\"paused\":"))) + (paused))) + (",\"phase\":"))) + (python::ispa_std::debug::quote(phase)))) + (",\"variables\":"))) + (((*snap)).json()));
	bool ok = python::ispa_std::debug::emit(python::ispa_std::debug::Kind::source_line, (((("{") + (payload))) + ("}")));
	if (((hit) && (can_pause))) {
		python::ispa_std::debug::command();
	}
	return ok;
}
auto python::ispa_std::debug::rule_depth_enter() -> void {
	((*python::ispa_std::debug::_S)).frame_depth = ((((*python::ispa_std::debug::_S)).frame_depth) + (1));
}
auto python::ispa_std::debug::rule_depth_leave() -> void {
	((*python::ispa_std::debug::_S)).frame_depth = ((((*python::ispa_std::debug::_S)).frame_depth) - (1));
}
auto python::ispa_std::debug::breakpoint(const std::string& label) -> void {
	if (((*python::ispa_std::debug::_S)).failure_seen) {
		return ;
	}
	((*python::ispa_std::debug::_S)).failure_seen = true;
	bool can_pause = ((*python::ispa_std::debug::_S)).has_control;
	std::string flag = "false";
	if (can_pause) {
		flag = "true";
	}
	python::ispa_std::debug::event(python::ispa_std::debug::Kind::breakpoint, python::ispa_std::debug::Location::none(), label, (("\"paused\":") + (flag)));
	((*python::ispa_std::debug::_S)).stepping = true;
	((*python::ispa_std::debug::_S)).target_depth = ( - (1));
	if (can_pause) {
		python::ispa_std::debug::command();
	}
}
auto python::ispa_std::debug::scope_depth() -> long long {
	return static_cast<long long>(((*python::ispa_std::debug::_S)).scopes.size());
}
auto python::ispa_std::debug::unwind_to(long long depth) -> void {
	"Close scopes opened after `depth`, innermost first (what C++ destructors\n    do while an exception propagates).";
	while (((static_cast<long long>(((*python::ispa_std::debug::_S)).scopes.size())) > (depth))) {
		((*(((*python::ispa_std::debug::_S)).scopes)[((static_cast<long long>(((*python::ispa_std::debug::_S)).scopes.size())) - (1))])).close();
	}
}
auto python::ispa_std::debug::_next_serial() -> long long {
	((*python::ispa_std::debug::_S)).serial = ((((*python::ispa_std::debug::_S)).serial) + (1));
	return ((*python::ispa_std::debug::_S)).serial;
}
auto python::ispa_std::debug::_forget(std::shared_ptr<python::ispa_std::debug::ScopeBase> scope) -> void {
	long long i = ((static_cast<long long>(((*python::ispa_std::debug::_S)).scopes.size())) - (1));
	while (((i) >= (0))) {
		if (((((*(((*python::ispa_std::debug::_S)).scopes)[i])).serial) == (((*scope)).serial))) {
			ISPA_STD::python_pop((((*python::ispa_std::debug::_S)).scopes), i);
			return ;
		}
		i = ((i) - (1));
	}
}
auto python::ispa_std::debug::RuleScope::__init__(const std::string& name, std::shared_ptr<python::ispa_std::debug::Location> loc) -> void {
	((*this)).name = name;
	((*this)).loc = loc;
	((*this)).previous = ((*python::ispa_std::debug::_S)).location;
	((*this)).open = true;
	((*this)).serial = python::ispa_std::debug::_next_serial();
	((*python::ispa_std::debug::_S)).location = loc;
	python::ispa_std::debug::enter_rule(name, loc);
	python::ispa_std::debug::rule_depth_enter();
	(((*python::ispa_std::debug::_S)).scopes).push_back(::ISPA_STD::self_ptr(this));
}
auto python::ispa_std::debug::RuleScope::close() -> void {
	if ((!(((*this)).open))) {
		return ;
	}
	((*this)).open = false;
	python::ispa_std::debug::_forget(::ISPA_STD::self_ptr(this));
	python::ispa_std::debug::rule_depth_leave();
	python::ispa_std::debug::exit_rule(((*this)).name, ((*this)).loc);
	((*python::ispa_std::debug::_S)).location = ((*this)).previous;
}
auto python::ispa_std::debug::ProductionScope::__init__(const std::string& name, std::shared_ptr<python::ispa_std::debug::Location> loc) -> void {
	((*this)).name = name;
	((*this)).loc = loc;
	((*this)).previous = ((*python::ispa_std::debug::_S)).location;
	((*this)).open = true;
	((*this)).serial = python::ispa_std::debug::_next_serial();
	((*python::ispa_std::debug::_S)).location = loc;
	python::ispa_std::debug::enter_production(name, loc);
	python::ispa_std::debug::rule_depth_enter();
	(((*python::ispa_std::debug::_S)).scopes).push_back(::ISPA_STD::self_ptr(this));
}
auto python::ispa_std::debug::ProductionScope::close() -> void {
	if ((!(((*this)).open))) {
		return ;
	}
	((*this)).open = false;
	python::ispa_std::debug::_forget(::ISPA_STD::self_ptr(this));
	python::ispa_std::debug::rule_depth_leave();
	python::ispa_std::debug::leave_production(((*this)).name, ((*this)).loc);
	((*python::ispa_std::debug::_S)).location = ((*this)).previous;
}
auto python::ispa_std::debug::MatchScope::__init__(const std::string& want, const std::string& got) -> void {
	((*this)).expected = want;
	((*this)).actual = got;
	((*this)).reason = "no match";
	((*this)).loc = ((*python::ispa_std::debug::_S)).location;
	((*this)).matched = false;
	((*this)).active = python::ispa_std::debug::enabled();
	((*this)).open = true;
	((*this)).serial = python::ispa_std::debug::_next_serial();
	if (((*this)).active) {
		python::ispa_std::debug::lexer_enter(want, ((*this)).loc);
		(((*python::ispa_std::debug::_S)).scopes).push_back(::ISPA_STD::self_ptr(this));
	}
}
auto python::ispa_std::debug::MatchScope::success() -> void {
	((*this)).matched = true;
	((*this)).reason = "matched";
}
auto python::ispa_std::debug::MatchScope::outcome(const std::string& why) -> void {
	((*this)).reason = why;
}
auto python::ispa_std::debug::MatchScope::close() -> void {
	if ((!(((*this)).open))) {
		return ;
	}
	((*this)).open = false;
	if (((*this)).active) {
		python::ispa_std::debug::_forget(::ISPA_STD::self_ptr(this));
		python::ispa_std::debug::lexer_result(((*this)).expected, ((*this)).matched, ((*this)).actual, ((*this)).reason, ((*this)).loc);
	}
}
auto python::ispa_std::debug::idle_match_scope() -> std::shared_ptr<python::ispa_std::debug::MatchScope> {
	"Shared scope for runs without a debug sink; avoids one allocation per matched token/rule.";
	return python::ispa_std::debug::_IDLE_MATCH_SCOPE;
}
auto python::ispa_std::failures::logic_error(const std::string& message) -> void {
	::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::LogicError, message);
}
auto python::ispa_std::failures::runtime_error(const std::string& message) -> void {
	::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::RuntimeFailure, message);
}
auto python::ispa_std::failures::out_of_range(const std::string& message) -> void {
	::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::OutOfRange, message);
}
auto python::ispa_std::failures::invalid_argument(const std::string& message) -> void {
	::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::InvalidArgument, message);
}
auto python::ispa_std::failures::length_error(const std::string& message) -> void {
	::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::LengthError, message);
}
auto python::ispa_std::failures::api_error(const std::string& message, const std::vector<std::string>& args) -> void {
	::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::ApiError, python::ispa_std::format::fmt(message, args));
}
auto python::ispa_std::failures::internal_error(const std::string& message, const std::string& issue, bool ispa_side, const std::vector<std::string>& args) -> void {
	std::string origin = "Generated parser";
	if (ispa_side) {
		origin = "ISPA standard library";
	}
	::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::InternalError, (((((((((("[") + (issue))) + (" | "))) + (origin))) + ("] "))) + (python::ispa_std::format::fmt(message, args))));
}
auto python::ispa_std::failures::no_match() -> void {
	::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::NoMatch, "no match");
}
auto python::ispa_std::failures::external_parse_error(const std::string& message) -> void {
	::ISPA_STD::fail(python::ispa_std::errors::ErrorKind::ExternalParse, message);
}
auto python::ispa_std::failures::rethrow(long long kind, const std::string& message) -> void {
	::ISPA_STD::fail(kind, message);
}
auto python::ispa_std::node::Node::__init__(long long startpos, const std::string& source, bool has_source, long long length, long long line, long long column, long long name, bool empty, std::any data) -> void {
	((*this))._startpos = startpos;
	((*this))._length = length;
	((*this))._line = line;
	((*this))._column = column;
	((*this))._source = source;
	((*this))._has_source = has_source;
	((*this))._name = name;
	((*this))._empty = empty;
	((*this))._data = data;
}
auto python::ispa_std::node::Node::blank() -> std::shared_ptr<python::ispa_std::node::Node> {
	"Node{}: empty, no position.";
	return ISPA_STD::construct<python::ispa_std::node::Node>(python::ispa_std::node::NO_POS, "", false, 0, 0, 0, python::ispa_std::node::NO_NAME, true, 0);
}
auto python::ispa_std::node::Node::with_meta(long long startpos, const std::string& source, bool has_source, long long length, long long line, long long column, long long name) -> std::shared_ptr<python::ispa_std::node::Node> {
	"A positioned node that carries no data yet (still empty()).";
	return ISPA_STD::construct<python::ispa_std::node::Node>(startpos, source, has_source, length, line, column, name, true, 0);
}
auto python::ispa_std::node::Node::with_data(long long startpos, const std::string& source, bool has_source, long long length, long long line, long long column, long long name, std::any data) -> std::shared_ptr<python::ispa_std::node::Node> {
	return ISPA_STD::construct<python::ispa_std::node::Node>(startpos, source, has_source, length, line, column, name, false, data);
}
auto python::ispa_std::node::Node::create(long long startpos, const std::string& source, long long length, long long line, long long enumv, std::any value) -> std::shared_ptr<python::ispa_std::node::Node> {
	return ISPA_STD::construct<python::ispa_std::node::Node>(startpos, source, true, length, line, 0, enumv, false, value);
}
auto python::ispa_std::node::Node::copy() -> std::shared_ptr<python::ispa_std::node::Node> {
	return ISPA_STD::construct<python::ispa_std::node::Node>(((*this))._startpos, ((*this))._source, ((*this))._has_source, ((*this))._length, ((*this))._line, ((*this))._column, ((*this))._name, ((*this))._empty, ((*this))._data);
}
auto python::ispa_std::node::Node::assign(std::shared_ptr<python::ispa_std::node::Node> other) -> void {
	((*this))._startpos = ((*other))._startpos;
	((*this))._length = ((*other))._length;
	((*this))._line = ((*other))._line;
	((*this))._column = ((*other))._column;
	((*this))._source = ((*other))._source;
	((*this))._has_source = ((*other))._has_source;
	((*this))._name = ((*other))._name;
	((*this))._empty = ((*other))._empty;
	((*this))._data = ((*other))._data;
}
auto python::ispa_std::node::Node::endpos() -> long long {
	if (((((((*this))._startpos) == (python::ispa_std::node::NO_POS))) || ((!(((*this))._has_source))))) {
		python::ispa_std::failures::api_error("Can\'t retrieve end position on an empty token", {});
	}
	return ((((*this))._startpos) + (((*this))._length));
}
auto python::ispa_std::node::Node::clear() -> void {
	((*this))._startpos = python::ispa_std::node::NO_POS;
	((*this))._line = 0;
	((*this))._column = 0;
	((*this))._length = 0;
	((*this))._source = "";
	((*this))._has_source = false;
	((*this))._name = python::ispa_std::node::NO_NAME;
	((*this))._data = 0;
	((*this))._empty = true;
}
auto python::ispa_std::node::Node::empty() -> bool {
	return ((*this))._empty;
}
auto python::ispa_std::node::Node::startpos() -> long long {
	return ((*this))._startpos;
}
auto python::ispa_std::node::Node::line() -> long long {
	return ((*this))._line;
}
auto python::ispa_std::node::Node::column() -> long long {
	return ((*this))._column;
}
auto python::ispa_std::node::Node::length() -> long long {
	return ((*this))._length;
}
auto python::ispa_std::node::Node::source() -> const std::string& {
	return ((*this))._source;
}
auto python::ispa_std::node::Node::has_source() -> bool {
	return ((*this))._has_source;
}
auto python::ispa_std::node::Node::name() -> long long {
	return ((*this))._name;
}
auto python::ispa_std::node::Node::data() -> std::any {
	return ((*this))._data;
}
auto python::ispa_std::node::Node::set_data(std::any value) -> void {
	((*this))._data = value;
	((*this))._empty = false;
}
auto python::ispa_std::node::Node::text() -> std::string {
	if ((((!(((*this))._has_source))) || (((((*this))._startpos) < (0))))) {
		return "";
	}
	return python::ispa_std::string::substr(((*this))._source, ((*this))._startpos, ((*this))._length);
}
auto python::ispa_std::raw_token::RawTokenMeta::__init__(long long startpos, const std::string& source, bool has_source, long long length, long long line, long long column, long long name) -> void {
	((*this))._startpos = startpos;
	((*this))._length = length;
	((*this))._line = line;
	((*this))._column = column;
	((*this))._source = source;
	((*this))._has_source = has_source;
	((*this))._name = name;
}
auto python::ispa_std::raw_token::RawTokenMeta::blank() -> std::shared_ptr<python::ispa_std::raw_token::RawTokenMeta> {
	return ISPA_STD::construct<python::ispa_std::raw_token::RawTokenMeta>(python::ispa_std::raw_token::NO_POS, "", false, 0, 0, 0, python::ispa_std::raw_token::NO_NAME);
}
auto python::ispa_std::raw_token::RawTokenMeta::startpos() -> long long {
	return ((*this))._startpos;
}
auto python::ispa_std::raw_token::RawTokenMeta::length() -> long long {
	return ((*this))._length;
}
auto python::ispa_std::raw_token::RawTokenMeta::line() -> long long {
	return ((*this))._line;
}
auto python::ispa_std::raw_token::RawTokenMeta::column() -> long long {
	return ((*this))._column;
}
auto python::ispa_std::raw_token::RawTokenMeta::name() -> long long {
	return ((*this))._name;
}
auto python::ispa_std::raw_token::RawTokenMeta::source() -> const std::string& {
	return ((*this))._source;
}
auto python::ispa_std::raw_token::RawTokenMeta::has_source() -> bool {
	return ((*this))._has_source;
}
auto python::ispa_std::raw_token::RawTokenMeta::end_pos() -> long long {
	return ((((*this))._startpos) + (((*this))._length));
}
auto python::ispa_std::raw_token::RawTokenMeta::text() -> std::string {
	if ((((!(((*this))._has_source))) || (((((*this))._startpos) < (0))))) {
		return "";
	}
	return python::ispa_std::string::substr(((*this))._source, ((*this))._startpos, ((*this))._length);
}
auto python::ispa_std::raw_token::RawToken::__init__(std::shared_ptr<python::ispa_std::raw_token::RawTokenMeta> meta, long long type_id) -> void {
	((*this))._meta = meta;
	((*this))._type_id = type_id;
}
auto python::ispa_std::raw_token::RawToken::empty_token() -> std::shared_ptr<python::ispa_std::raw_token::RawToken> {
	"The default-constructed (end-of-stream) token.";
	return ISPA_STD::construct<python::ispa_std::raw_token::RawToken>(python::ispa_std::raw_token::RawTokenMeta::blank(), python::ispa_std::raw_token::NO_TYPE);
}
auto python::ispa_std::raw_token::RawToken::ambiguous(std::shared_ptr<python::ispa_std::raw_token::RawTokenMeta> meta) -> std::shared_ptr<python::ispa_std::raw_token::RawToken> {
	return ISPA_STD::construct<python::ispa_std::raw_token::RawToken>(meta, python::ispa_std::raw_token::NO_TYPE);
}
auto python::ispa_std::raw_token::RawToken::make(std::shared_ptr<python::ispa_std::raw_token::RawTokenMeta> meta, long long type_id) -> std::shared_ptr<python::ispa_std::raw_token::RawToken> {
	if (((type_id) < (0))) {
		python::ispa_std::failures::logic_error("RawToken: payload type is not an alternative");
	}
	return ISPA_STD::construct<python::ispa_std::raw_token::RawToken>(meta, type_id);
}
auto python::ispa_std::raw_token::RawToken::from_node(std::shared_ptr<python::ispa_std::node::Node> node, long long type_id) -> std::shared_ptr<python::ispa_std::raw_token::RawToken> {
	auto meta = ISPA_STD::construct<python::ispa_std::raw_token::RawTokenMeta>(((*node)).startpos(), ((*node)).source(), ((*node)).has_source(), ((*node)).length(), ((*node)).line(), ((*node)).column(), ((*node)).name());
	if (((*node)).empty()) {
		return ISPA_STD::construct<python::ispa_std::raw_token::RawToken>(meta, python::ispa_std::raw_token::NO_TYPE);
	}
	return ISPA_STD::construct<python::ispa_std::raw_token::RawToken>(meta, type_id);
}
auto python::ispa_std::raw_token::RawToken::create(long long startpos, const std::string& source, bool has_source, long long length, long long line, long long enumv, long long type_id) -> std::shared_ptr<python::ispa_std::raw_token::RawToken> {
	auto meta = ISPA_STD::construct<python::ispa_std::raw_token::RawTokenMeta>(startpos, source, has_source, length, line, 0, enumv);
	return ISPA_STD::construct<python::ispa_std::raw_token::RawToken>(meta, type_id);
}
auto python::ispa_std::raw_token::RawToken::copy() -> std::shared_ptr<python::ispa_std::raw_token::RawToken> {
	return ISPA_STD::construct<python::ispa_std::raw_token::RawToken>(((*this))._meta, ((*this))._type_id);
}
auto python::ispa_std::raw_token::RawToken::empty() -> bool {
	return ((((*this))._type_id) == (python::ispa_std::raw_token::NO_TYPE));
}
auto python::ispa_std::raw_token::RawToken::isAmbiguous() -> bool {
	return ((((*((*this))._meta)).name()) == (python::ispa_std::raw_token::AMBIGUOUS_ID));
}
auto python::ispa_std::raw_token::RawToken::meta() -> std::shared_ptr<python::ispa_std::raw_token::RawTokenMeta> {
	return ((*this))._meta;
}
auto python::ispa_std::raw_token::RawToken::kind() -> long long {
	return ((*((*this))._meta)).name();
}
auto python::ispa_std::raw_token::RawToken::index() -> long long {
	return ((*this))._type_id;
}
auto python::ispa_std::raw_token::RawToken::holds(long long type_id) -> bool {
	return ((((*this))._type_id) == (type_id));
}
auto python::ispa_std::raw_token::RawToken::to_node(long long type_id) -> std::shared_ptr<python::ispa_std::node::Node> {
	"A Node reporting whether this token holds `type_id` (RawToken::to_node<T>): this base\n        has no payload to carry (see the module docstring), so a facade subclass that actually\n        holds one overrides this to carry it too.";
	auto m = ((*this))._meta;
	if (((((((*this))._type_id) == (python::ispa_std::raw_token::NO_TYPE))) || (((((*this))._type_id) != (type_id))))) {
		return python::ispa_std::node::Node::with_meta(((*m)).startpos(), ((*m)).source(), ((*m)).has_source(), ((*m)).length(), ((*m)).line(), ((*m)).column(), ((*m)).name());
	}
	return python::ispa_std::node::Node::with_data(((*m)).startpos(), ((*m)).source(), ((*m)).has_source(), ((*m)).length(), ((*m)).line(), ((*m)).column(), ((*m)).name(), 0);
}
auto python::ispa_std::raw_token::RawToken::text() -> std::string {
	"The source text this token covers (\"\" when it has no source).";
	return ((*((*this))._meta)).text();
}
auto python::ispa_std::raw_token::char_at_source(std::shared_ptr<python::ispa_std::raw_token::RawTokenMeta> meta, long long offset) -> std::string {
	return python::ispa_std::chars::at(((*meta)).source(), ((((*meta)).startpos()) + (offset)));
}
long long python::ispa_std::dfa_api::Action::UNDEF = 0;
long long python::ispa_std::dfa_api::Action::SET = 1;
long long python::ispa_std::dfa_api::Action::SET_NEXT = 2;
long long python::ispa_std::dfa_api::Action::COPY = 3;
long long python::ispa_std::dfa_api::Action::APPEND = 4;
long long python::ispa_std::dfa_api::Action::APPEND_NEXT = 5;
auto python::ispa_std::dfa_api::TdfaLayout::__init__(long long registers, long long outputs) -> void {
	((*this)).registers = registers;
	((*this)).outputs = outputs;
	if (((registers) < (outputs))) {
		python::ispa_std::failures::runtime_error("output registers are registers 0 .. outputs-1");
	}
}
auto python::ispa_std::dfa_api::dynamic_match(const std::string& src, long long cursor, const std::vector<std::string>& alternatives, bool negative, const std::string& quantifier) -> long long {
	"A semantic transition tests the whole runtime string. Returns the new\n    cursor, or -1 when the match failed. Negation consumes one character, or a\n    run when the quantifier is \"*\".";
	long long n = static_cast<long long>(src.size());
	if (negative) {
		if (((quantifier) == ("*"))) {
			long long p = cursor;
			while (((((p) < (n))) && ((!(python::ispa_std::dfa_api::_excluded(src, p, alternatives)))))) {
				p = ((p) + (1));
			}
			return p;
		}
		if (((((cursor) >= (n))) || (python::ispa_std::dfa_api::_excluded(src, cursor, alternatives)))) {
			return ( - (1));
		}
		return ((cursor) + (1));
	}
	long long i = 0;
	while (((i) < (static_cast<long long>(alternatives.size())))) {
		std::string value = (alternatives)[i];
		if (((((static_cast<long long>(value.size())) > (0))) && (python::ispa_std::dfa_api::_starts_at(src, cursor, value)))) {
			return ((cursor) + (static_cast<long long>(value.size())));
		}
		i = ((i) + (1));
	}
	return ( - (1));
}
auto python::ispa_std::dfa_api::_starts_at(const std::string& src, long long p, const std::string& value) -> bool {
	if (((((p) + (static_cast<long long>(value.size())))) > (static_cast<long long>(src.size())))) {
		return false;
	}
	long long i = 0;
	while (((i) < (static_cast<long long>(value.size())))) {
		if (((std::string(1, (src)[((p) + (i))])) != (std::string(1, (value)[i])))) {
			return false;
		}
		i = ((i) + (1));
	}
	return true;
}
auto python::ispa_std::dfa_api::_excluded(const std::string& src, long long p, const std::vector<std::string>& alternatives) -> bool {
	long long i = 0;
	while (((i) < (static_cast<long long>(alternatives.size())))) {
		if (python::ispa_std::dfa_api::_starts_at(src, p, (alternatives)[i])) {
			return true;
		}
		i = ((i) + (1));
	}
	return false;
}
auto python::ispa_std::dfa_api::DFADebug::__init__(const std::string& member, const std::string& token_name, long long position_in_token, long long call, long long group, long long rule_run, long long offset, long long length, const std::string& ch) -> void {
	((*this)).member = member;
	((*this)).token_name = token_name;
	((*this)).position_in_token = position_in_token;
	((*this)).call = call;
	((*this)).group = group;
	((*this)).rule_run = rule_run;
	((*this)).offset = offset;
	((*this)).length = length;
	((*this)).ch = ch;
}
auto python::ispa_std::dfa_api::DebugIndex::__init__() -> void {
	((*this))._keys = {};
	((*this))._values = {};
}
auto python::ispa_std::dfa_api::DebugIndex::put(long long state, long long cls, long long index) -> void {
	long long key = ((((state) * (256))) + (cls));
	long long lo = 0;
	long long hi = static_cast<long long>(((*this))._keys.size());
	while (((lo) < (hi))) {
		long long mid = ((lo) + (((((hi) - (lo))) / (2))));
		if ((((((*this))._keys)[mid]) < (key))) {
			lo = ((mid) + (1));
		} else {
			hi = mid;
		}
	}
	if (((((lo) < (static_cast<long long>(((*this))._keys.size())))) && ((((((*this))._keys)[lo]) == (key))))) {
		(((*this))._values)[lo] = index;
	} else {
		ISPA_STD::python_insert((((*this))._keys), lo, key);
		ISPA_STD::python_insert((((*this))._values), lo, index);
	}
}
auto python::ispa_std::dfa_api::DebugIndex::find(long long state, long long cls) -> long long {
	long long key = ((((state) * (256))) + (cls));
	long long lo = 0;
	long long hi = static_cast<long long>(((*this))._keys.size());
	while (((lo) < (hi))) {
		long long mid = ((lo) + (((((hi) - (lo))) / (2))));
		if ((((((*this))._keys)[mid]) < (key))) {
			lo = ((mid) + (1));
		} else {
			hi = mid;
		}
	}
	if (((((lo) < (static_cast<long long>(((*this))._keys.size())))) && ((((((*this))._keys)[lo]) == (key))))) {
		return (((*this))._values)[lo];
	}
	return ( - (1));
}
auto python::ispa_std::dfa_api::HistoryNode::__init__(long long pos, long long prev) -> void {
	((*this)).pos = pos;
	((*this)).prev = prev;
}
auto python::ispa_std::dfa_api::SourceText::__init__(const std::string& text) -> void {
	((*this)).text = text;
}
auto python::ispa_std::dfa_api::CaptureState::__init__(std::shared_ptr<python::ispa_std::dfa_api::SourceText> src) -> void {
	((*this)).src = src;
	((*this)).base = 0;
	((*this)).regs = {};
	((*this)).history = {};
	((*this)).first_line = 1;
	((*this)).dirty = true;
}
auto python::ispa_std::dfa_api::Captures::__init__(std::shared_ptr<python::ispa_std::dfa_api::CaptureState> state) -> void {
	((*this))._state = state;
}
auto python::ispa_std::dfa_api::Captures::begin_off(long long k) -> long long {
	return (((*((*this))._state)).regs)[((2) * (k))];
}
auto python::ispa_std::dfa_api::Captures::end_off(long long k) -> long long {
	return (((*((*this))._state)).regs)[((((2) * (k))) + (1))];
}
auto python::ispa_std::dfa_api::Captures::is_set(long long k) -> bool {
	long long b = ((*this)).begin_off(k);
	long long e = ((*this)).end_off(k);
	return ((((((b) != (python::ispa_std::dfa_api::UNSET))) && (((e) != (python::ispa_std::dfa_api::UNSET))))) && (((e) >= (b))));
}
auto python::ispa_std::dfa_api::Captures::begin(long long k) -> long long {
	return ((((*((*this))._state)).base) + (((*this)).begin_off(k)));
}
auto python::ispa_std::dfa_api::Captures::end(long long k) -> long long {
	return ((((*((*this))._state)).base) + (((*this)).end_off(k)));
}
auto python::ispa_std::dfa_api::Captures::text(long long k) -> std::string {
	if ((!(((*this)).is_set(k)))) {
		return "";
	}
	return python::ispa_std::string::substr(((*((*((*this))._state)).src)).text, ((*this)).begin(k), ((((*this)).end_off(k)) - (((*this)).begin_off(k))));
}
auto python::ispa_std::dfa_api::Captures::view(long long k) -> std::string {
	return ((*this)).text(k);
}
auto python::ispa_std::dfa_api::Captures::character(long long k) -> std::string {
	return python::ispa_std::chars::at(((*((*((*this))._state)).src)).text, ((*this)).begin(k));
}
auto python::ispa_std::dfa_api::Captures::list_positions(long long k) -> std::vector<long long> {
	std::vector<long long> positions = std::vector<long long>{};
	long long h = ((*this)).begin_off(k);
	while (((h) != (python::ispa_std::dfa_api::UNSET))) {
		(positions).push_back(((*(((*((*this))._state)).history)[h])).pos);
		h = ((*(((*((*this))._state)).history)[h])).prev;
	}
	ISPA_STD::python_reverse((positions));
	return positions;
}
auto python::ispa_std::dfa_api::Captures::list_texts(long long k) -> std::vector<std::string> {
	"One entry per iteration of a list capture, in input order.";
	auto positions = ((*this)).list_positions(k);
	std::vector<std::string> out = std::vector<std::string>{};
	long long i = 0;
	while (((((i) + (1))) < (static_cast<long long>(positions.size())))) {
		(out).push_back(python::ispa_std::string::substr(((*((*((*this))._state)).src)).text, ((((*((*this))._state)).base) + ((positions)[i])), (((positions)[((i) + (1))]) - ((positions)[i]))));
		i = ((i) + (2));
	}
	return out;
}
auto python::ispa_std::dfa_api::Captures::list_chars(long long k) -> std::vector<std::string> {
	auto texts = ((*this)).list_texts(k);
	std::vector<std::string> out = std::vector<std::string>{};
	long long i = 0;
	while (((i) < (static_cast<long long>(texts.size())))) {
		if (((static_cast<long long>((texts)[i].size())) == (0))) {
			(out).push_back("");
		} else {
			(out).push_back(std::string(1, ((texts)[i])[0]));
		}
		i = ((i) + (1));
	}
	return out;
}
auto python::ispa_std::dfa_api::Captures::line(long long k) -> long long {
	long long count = 0;
	long long i = ((*((*this))._state)).base;
	long long stop = ((*this)).begin(k);
	while (((((i) < (stop))) && (((i) < (static_cast<long long>(((*((*((*this))._state)).src)).text.size())))))) {
		if (((std::string(1, (((*((*((*this))._state)).src)).text)[i])) == ("\n"))) {
			count = ((count) + (1));
		}
		i = ((i) + (1));
	}
	return ((((*((*this))._state)).first_line) + (count));
}
auto python::ispa_std::dfa_api::Captures::column(long long k) -> long long {
	long long b = ((*this)).begin(k);
	long long line_start = b;
	while (((((line_start) > (((*((*this))._state)).base))) && (((python::ispa_std::chars::at(((*((*((*this))._state)).src)).text, ((line_start) - (1)))) != ("\n"))))) {
		line_start = ((line_start) - (1));
	}
	return ((((b) - (line_start))) + (1));
}
auto python::ispa_std::dfa_api::semantic_result(long long state, std::shared_ptr<python::ispa_std::raw_token::RawToken> token) -> std::shared_ptr<python::ispa_std::utility::Pair<long long, std::shared_ptr<python::ispa_std::raw_token::RawToken>>> {
	"What a generated semantic function returns: (next state, token).";
	return ISPA_STD::construct<python::ispa_std::utility::Pair<long long, std::shared_ptr<python::ispa_std::raw_token::RawToken>>>(state, token);
}
auto python::ispa_std::dfa_api::class_of(const std::vector<long long>& class_table, const std::string& ch) -> long long {
	return (class_table)[python::ispa_std::chars::code_of(ch)];
}
auto python::ispa_std::refs::Cursor::__init__(long long pos, long long line) -> void {
	((*this)).pos = pos;
	((*this)).line = line;
}
auto python::ispa_std::refs::Cursor::copy() -> std::shared_ptr<python::ispa_std::refs::Cursor> {
	return ISPA_STD::construct<python::ispa_std::refs::Cursor>(((*this)).pos, ((*this)).line);
}
auto python::ispa_std::refs::IntRef::__init__(long long value) -> void {
	((*this)).value = value;
}
auto python::ispa_std::dfa_run::DfaTables::__init__(const std::vector<std::vector<long long>>& table, const std::vector<long long>& class_table, const std::vector<std::vector<long long>>& action_table, std::shared_ptr<python::ispa_std::dfa_api::TdfaLayout> layout, const std::vector<std::shared_ptr<python::ispa_std::dfa_api::DFADebug>>& debug_array, std::shared_ptr<python::ispa_std::dfa_api::DebugIndex> debug_index) -> void {
	((*this)).table = table;
	((*this)).class_table = class_table;
	((*this)).action_table = action_table;
	((*this)).layout = layout;
	((*this)).debug_array = debug_array;
	((*this)).debug_index = debug_index;
	((*this)).state = ISPA_STD::construct<python::ispa_std::dfa_api::CaptureState>(ISPA_STD::construct<python::ispa_std::dfa_api::SourceText>(""));
	((*this)).captures = ISPA_STD::construct<python::ispa_std::dfa_api::Captures>(((*this)).state);
}
auto python::ispa_std::dfa_run::TraceSink::write(const std::string& text) -> void {
	return ;
}
auto python::ispa_std::dfa_run::set_trace_sink(std::shared_ptr<python::ispa_std::dfa_run::TraceSink> sink) -> void {
	(python::ispa_std::dfa_run::_TRACE)[0] = sink;
	(python::ispa_std::dfa_run::_TRACE_ON)[0] = true;
}
auto python::ispa_std::dfa_run::clear_trace_sink() -> void {
	(python::ispa_std::dfa_run::_TRACE_ON)[0] = false;
}
auto python::ispa_std::dfa_run::_trace_step(std::shared_ptr<python::ispa_std::dfa_run::DfaTables> tables, long long state, long long cls, const std::string& current_ch, const std::string& pos_ch) -> void {
	if ((!((python::ispa_std::dfa_run::_TRACE_ON)[0]))) {
		return ;
	}
	auto sink = (python::ispa_std::dfa_run::_TRACE)[0];
	long long index = ( - (1));
	if (((static_cast<long long>(((*tables)).debug_array.size())) > (0))) {
		index = ((*((*tables)).debug_index)).find(state, cls);
	}
	if (((index) >= (0))) {
		auto d = (((*tables)).debug_array)[index];
		std::string head = ((((*d)).token_name) + (": "));
		std::string line = ((head) + (((*d)).member));
		if (((((*d)).call) != (( - (1))))) {
			line = ((((line) + ("$call"))) + (python::ispa_std::format::int_to_string(((*d)).call)));
		}
		if (((((*d)).group) != (( - (1))))) {
			line = ((((line) + ("$group"))) + (python::ispa_std::format::int_to_string(((*d)).group)));
		}
		line = ((((((((((line) + ("; "))) + (python::ispa_std::format::int_to_string(((((*d)).offset) + (1)))))) + ("/"))) + (python::ispa_std::format::int_to_string(((*d)).length)))) + (";\n"));
		((*sink)).write(line);
		std::string pad = python::ispa_std::format::repeat(" ", ((static_cast<long long>(head.size())) + (((*d)).offset)));
		std::string mem = ((*d)).member;
		bool marked = false;
		if (((((static_cast<long long>(mem.size())) >= (2))) && (((std::string(1, (mem)[0])) == ("["))))) {
			bool escaped = false;
			long long i = 1;
			while (((((i) < (static_cast<long long>(mem.size())))) && ((!(marked))))) {
				std::string c = std::string(1, (mem)[i]);
				if (escaped) {
					escaped = false;
					if (((current_ch) == (c))) {
						((*sink)).write(((((pad) + (python::ispa_std::format::repeat(" ", i)))) + ("^\n")));
						marked = true;
					}
				} else {
					if (((c) == ("\\"))) {
						escaped = true;
					} else {
						if (((current_ch) == (c))) {
							((*sink)).write(((((pad) + (python::ispa_std::format::repeat(" ", i)))) + ("^\n")));
							marked = true;
						} else {
							if (((((((((c) == ("-"))) && (((i) > (1))))) && (((((i) + (1))) < (static_cast<long long>(mem.size())))))) && (((std::string(1, (mem)[((i) + (1))])) != ("]"))))) {
								if (((((current_ch) >= (std::string(1, (mem)[((i) - (1))])))) && (((current_ch) <= (std::string(1, (mem)[((i) + (1))])))))) {
									((*sink)).write(((((pad) + (python::ispa_std::format::repeat(" ", ((i) - (1)))))) + ("^ ^\n")));
									marked = true;
								}
							}
						}
					}
				}
				i = ((i) + (1));
			}
		} else {
			((*sink)).write(((pad) + ("^\n")));
		}
	} else {
		((*sink)).write((((((((("State ") + (python::ispa_std::format::int_to_string(state)))) + (" char \'"))) + (pos_ch))) + ("\'\n")));
	}
}
auto python::ispa_std::dfa_run::run(std::shared_ptr<python::ispa_std::dfa_api::SourceText> source, std::shared_ptr<python::ispa_std::refs::Cursor> cur, long long start_pos, std::shared_ptr<python::ispa_std::dfa_run::DfaTables> tables, std::function<std::shared_ptr<python::ispa_std::utility::Pair<long long, std::shared_ptr<python::ispa_std::raw_token::RawToken>>>(long long, std::shared_ptr<python::ispa_std::dfa_api::Captures>, long long, long long, long long, std::shared_ptr<python::ispa_std::refs::Cursor>)> semantic, std::shared_ptr<python::ispa_std::raw_token::RawToken> empty_token) -> std::shared_ptr<python::ispa_std::raw_token::RawToken> {
	"Scan one token starting at cur.pos. Returns the accepted token, or\n    `empty_token` when nothing matched (cur is then left where the walk died,\n    which the caller treats as no progress).";
	long long n = static_cast<long long>(((*source)).text.size());
	long long start = ((*cur)).pos;
	long long start_line = ((*cur)).line;
	auto scope = python::ispa_std::debug::idle_match_scope();
	if (python::ispa_std::debug::enabled()) {
		std::string first = "<end of input>";
		if (((start) < (n))) {
			first = std::string(1, (((*source)).text)[start]);
		}
		scope = ISPA_STD::construct<python::ispa_std::debug::MatchScope>("lexer scan", first);
	}
	std::vector<std::vector<long long>>& table = ((*tables)).table;
	std::vector<long long>& class_table = ((*tables)).class_table;
	std::vector<std::vector<long long>>& action_table = ((*tables)).action_table;
	long long registers = ((*((*tables)).layout)).registers;
	long long nstates = static_cast<long long>(table.size());
	long long nactions = static_cast<long long>(action_table.size());
	bool trace_on = ((((static_cast<long long>(((*tables)).debug_array.size())) > (1))) || (((((static_cast<long long>(((*tables)).debug_array.size())) == (1))) && (((((*(((*tables)).debug_array)[0])).member) != ("<Undefined>"))))));
	auto scan_state = ((*tables)).state;
	((*scan_state)).src = source;
	((*scan_state)).base = start;
	((*scan_state)).first_line = start_line;
	std::vector<long long>& regs = ((*scan_state)).regs;
	long long width = registers;
	if (((width) < (1))) {
		width = 1;
	}
	if (((((*scan_state)).dirty) || (((static_cast<long long>(regs.size())) != (width))))) {
		(regs).clear();
		long long i = 0;
		while (((i) < (width))) {
			(regs).push_back(python::ispa_std::dfa_api::UNSET);
			i = ((i) + (1));
		}
		((*scan_state)).dirty = false;
	}
	std::vector<std::shared_ptr<python::ispa_std::dfa_api::HistoryNode>>& history = ((*scan_state)).history;
	(history).clear();
	long long state = 0;
	bool immediate_action = false;
	long long last_pos = ((*cur)).pos;
	long long steps_without_progress = 0;
	bool dead = false;
	while (((((state) != (python::ispa_std::dfa_api::NULL_STATE))) && ((!(dead))))) {
		if (trace_on) {
			std::string current_ch = python::ispa_std::chars::at(((*source)).text, ((*cur)).pos);
			if (immediate_action) {
				current_ch = python::ispa_std::chars::at(((*source)).text, ((((*cur)).pos) + (1)));
			}
			python::ispa_std::dfa_run::_trace_step(tables, state, (class_table)[python::ispa_std::chars::code_of(current_ch)], current_ch, current_ch);
		}
		if (((state) < (nstates))) {
			if (immediate_action) {
				immediate_action = false;
				if (((python::ispa_std::chars::code_at(((*source)).text, ((*cur)).pos)) == (10))) {
					((*cur)).line = ((((*cur)).line) + (1));
				}
				if (((((*cur)).pos) < (n))) {
					((*cur)).pos = ((((*cur)).pos) + (1));
				}
			}
			long long here_code = python::ispa_std::chars::code_at(((*source)).text, ((*cur)).pos);
			long long nxt = ((table)[state])[(class_table)[here_code]];
			if (((nxt) == (python::ispa_std::dfa_api::NULL_STATE))) {
				dead = true;
			} else {
				state = nxt;
				if (((((((nxt) >= (0))) && (((nxt) < (nstates))))) && (((((*cur)).pos) < (n))))) {
					if (((here_code) == (10))) {
						((*cur)).line = ((((*cur)).line) + (1));
					}
					((*cur)).pos = ((((*cur)).pos) + (1));
				} else {
					immediate_action = true;
				}
			}
		} else {
			if (((state) < (((nstates) + (nactions))))) {
				std::vector<long long>& row = (action_table)[((state) - (nstates))];
				long long here = ((((*cur)).pos) - (start));
				if ((((((row)[1]) >= (registers))) || ((((((row)[2]) != (python::ispa_std::dfa_api::NULL_STATE))) && ((((row)[2]) >= (registers))))))) {
					((*scope)).close();
					python::ispa_std::failures::runtime_error("DFA: register out of range");
				}
				long long op = (row)[0];
				((*scan_state)).dirty = true;
				if (((((op) == (python::ispa_std::dfa_api::Action::SET))) || (((op) == (python::ispa_std::dfa_api::Action::SET_NEXT))))) {
					long long value = here;
					if (((op) == (python::ispa_std::dfa_api::Action::SET_NEXT))) {
						value = ((here) + (1));
					}
					(regs)[(row)[1]] = value;
				} else {
					if (((op) == (python::ispa_std::dfa_api::Action::COPY))) {
						(regs)[(row)[1]] = (regs)[(row)[2]];
					} else {
						if (((((op) == (python::ispa_std::dfa_api::Action::APPEND))) || (((op) == (python::ispa_std::dfa_api::Action::APPEND_NEXT))))) {
							long long mark = here;
							if (((op) == (python::ispa_std::dfa_api::Action::APPEND_NEXT))) {
								mark = ((here) + (1));
							}
							auto prev = python::ispa_std::dfa_api::UNSET;
							if ((((row)[2]) != (python::ispa_std::dfa_api::NULL_STATE))) {
								prev = (regs)[(row)[2]];
							}
							(history).push_back(ISPA_STD::construct<python::ispa_std::dfa_api::HistoryNode>(mark, prev));
							(regs)[(row)[1]] = ((static_cast<long long>(history.size())) - (1));
						} else {
							((*scope)).close();
							python::ispa_std::failures::runtime_error("DFA: unknown action; Report this error to github");
						}
					}
				}
				state = (row)[3];
			} else {
				auto captures = ((*tables)).captures;
				long long semantic_pos = ((*cur)).pos;
				auto result = semantic(((((state) - (nstates))) - (nactions)), captures, start_pos, ((((*cur)).pos) - (start)), start_line, cur);
				if (((((*cur)).pos) != (semantic_pos))) {
					if (((((*cur)).pos) < (semantic_pos))) {
						((*scope)).close();
						python::ispa_std::failures::runtime_error("DFA: semantic matcher moved the cursor backwards");
					}
					long long k = semantic_pos;
					while (((((k) < (((*cur)).pos))) && (((k) < (n))))) {
						if (((std::string(1, (((*source)).text)[k])) == ("\n"))) {
							((*cur)).line = ((((*cur)).line) + (1));
						}
						k = ((k) + (1));
					}
					immediate_action = false;
				}
				state = ((*result)).first;
				if (((state) == (python::ispa_std::dfa_api::DYNAMIC_FAIL_STATE))) {
					((*scope)).outcome("dynamic matcher failed");
					((*scope)).close();
					return empty_token;
				}
				if (((state) == (python::ispa_std::dfa_api::LEXER_DELETE_STATE))) {
					if (((((*cur)).pos) < (n))) {
						((*cur)).pos = ((((*cur)).pos) + (1));
					}
					((*scope)).success();
					((*scope)).close();
					return empty_token;
				}
				if (((state) == (python::ispa_std::dfa_api::NULL_STATE))) {
					((*scope)).success();
					((*scope)).close();
					return ((*result)).second;
				}
				if (((((*cur)).pos) == (last_pos))) {
					steps_without_progress = ((steps_without_progress) + (1));
					if (((steps_without_progress) > (100))) {
						((*scope)).close();
						python::ispa_std::failures::internal_error("DFA: possible zero-width transition cycle at state {}, offset {}", "Lexer TDFA generation", true, std::vector<std::string>{python::ispa_std::format::int_to_string(state), python::ispa_std::format::int_to_string(((((*cur)).pos) - (start)))});
					}
				} else {
					last_pos = ((*cur)).pos;
					steps_without_progress = 0;
				}
			}
		}
	}
	((*scope)).outcome("DFA found no accepting token");
	((*scope)).close();
	return empty_token;
}
long long python::ispa_std::lexer::LexerDecision::NONE = 0;
long long python::ispa_std::lexer::LexerDecision::DELETE = 1;
long long python::ispa_std::lexer::LexerDecision::INSERT = 2;
long long python::ispa_std::lexer::LexerDecision::ABORT = 3;
auto python::ispa_std::lexer::LexerFailureContext::__init__(const std::string& src, long long cursor, const std::vector<std::string>& diagnostics, const std::string& rule, const std::string& failed_rule) -> void {
	((*this)).src = src;
	((*this)).cursor = cursor;
	((*this)).diagnostics = diagnostics;
	((*this)).rule = rule;
	((*this)).failed_rule = failed_rule;
	((*this)).decision = python::ispa_std::lexer::LexerDecision::NONE;
}
auto python::ispa_std::lexer::LexerFailureContext::at(const std::string& value, long long offset) -> bool {
	"Is the unit `offset` positions ahead exactly `value` (\"\" = end)?";
	long long p = ((*this)).cursor;
	long long left = offset;
	while (((((left) > (0))) && (((p) < (static_cast<long long>(((*this)).src.size())))))) {
		p = ((p) + (1));
		left = ((left) - (1));
	}
	return ((value) == (python::ispa_std::chars::at(((*this)).src, p)));
}
auto python::ispa_std::lexer::LexerFailureContext::error(const std::string& message) -> void {
	(((*this)).diagnostics).push_back(message);
}
auto python::ispa_std::lexer::LexerFailureContext::erase() -> void {
	((*this)).decision = python::ispa_std::lexer::LexerDecision::DELETE;
}
auto python::ispa_std::lexer::LexerFailureContext::skip() -> void {
	((*this)).decision = python::ispa_std::lexer::LexerDecision::DELETE;
}
auto python::ispa_std::lexer::LexerFailureContext::insert() -> void {
	((*this)).decision = python::ispa_std::lexer::LexerDecision::INSERT;
}
auto python::ispa_std::lexer::LexerFailureContext::abort() -> void {
	((*this)).decision = python::ispa_std::lexer::LexerDecision::ABORT;
}
auto python::ispa_std::lexer::is_token_end(std::shared_ptr<python::ispa_std::raw_token::RawToken> token) -> bool {
	"End-of-stream: an empty token that is not the ambiguous marker.";
	return ((((*token)).empty()) && ((!(((*token)).isAmbiguous()))));
}
auto python::ispa_std::lexer::Checkpoint::__init__() -> void {
	return ;
}
auto python::ispa_std::lexer::_SharedState::__init__(long long pos) -> void {
	((*this)).pos = pos;
	((*this)).eof_processed = false;
	((*this)).checkpoints = {};
}
auto python::ispa_std::lexer::Lexer_base::__init__() -> void {
	((*this))._src = ISPA_STD::construct<python::ispa_std::dfa_api::SourceText>("");
	((*this))._has_input = false;
	((*this)).tokens = {};
	((*this)).errors = {};
	((*this)).line = 1;
	((*this)).external_boundary = false;
	((*this)).bound = ( - (1));
}
auto python::ispa_std::lexer::Lexer_base::makeToken(std::shared_ptr<python::ispa_std::refs::Cursor> cur) -> std::shared_ptr<python::ispa_std::raw_token::RawToken> {
	python::ispa_std::failures::logic_error("Lexer_base.makeToken is abstract");
	return ((*this)).empty_token();
}
auto python::ispa_std::lexer::Lexer_base::init() -> void {
	return ;
}
auto python::ispa_std::lexer::Lexer_base::empty_token() -> std::shared_ptr<python::ispa_std::raw_token::RawToken> {
	"The default-constructed token (what C++ spells `Token{}`).";
	return python::ispa_std::raw_token::RawToken::empty_token();
}
auto python::ispa_std::lexer::Lexer_base::ambiguous_token(long long before, long long after) -> std::shared_ptr<python::ispa_std::raw_token::RawToken> {
	"Marker for one source unit an external rule may still claim.";
	auto meta = ISPA_STD::construct<python::ispa_std::raw_token::RawTokenMeta>(before, ((*((*this))._src)).text, true, 1, ((*this)).line_at(before), ((*this)).column_at(before), python::ispa_std::raw_token::AMBIGUOUS_ID);
	return python::ispa_std::raw_token::RawToken::ambiguous(meta);
}
auto python::ispa_std::lexer::Lexer_base::clone() -> std::shared_ptr<python::ispa_std::lexer::Lexer_base> {
	python::ispa_std::failures::logic_error("External parser handoff requires a cloneable lexer");
	return ::ISPA_STD::self_ptr(this);
}
auto python::ispa_std::lexer::Lexer_base::checkpoint() -> std::shared_ptr<python::ispa_std::lexer::Checkpoint> {
	return ISPA_STD::construct<python::ispa_std::lexer::Checkpoint>();
}
auto python::ispa_std::lexer::Lexer_base::restoreCheckpoint(std::shared_ptr<python::ispa_std::lexer::Checkpoint> state) -> void {
	return ;
}
auto python::ispa_std::lexer::Lexer_base::externalAdvance(long long begin, long long end) -> void {
	return ;
}
auto python::ispa_std::lexer::Lexer_base::hasExternalRules() -> bool {
	return false;
}
auto python::ispa_std::lexer::Lexer_base::on_token(std::shared_ptr<python::ispa_std::raw_token::RawToken> token) -> void {
	(((*this)).tokens).push_back(token);
}
auto python::ispa_std::lexer::Lexer_base::on_eof() -> void {
	(((*this)).tokens).push_back(((*this)).empty_token());
}
auto python::ispa_std::lexer::Lexer_base::push(std::shared_ptr<python::ispa_std::raw_token::RawToken> token) -> void {
	((*this)).on_token(token);
}
auto python::ispa_std::lexer::Lexer_base::push_eof() -> void {
	((*this)).on_eof();
}
auto python::ispa_std::lexer::Lexer_base::append_token(std::shared_ptr<python::ispa_std::raw_token::RawToken> token) -> void {
	(((*this)).tokens).push_back(token);
}
auto python::ispa_std::lexer::Lexer_base::has_buffered_tokens() -> bool {
	return ((static_cast<long long>(((*this)).tokens.size())) > (0));
}
auto python::ispa_std::lexer::Lexer_base::pop_buffered_token() -> std::shared_ptr<python::ispa_std::raw_token::RawToken> {
	return ISPA_STD::python_pop((((*this)).tokens), 0);
}
auto python::ispa_std::lexer::Lexer_base::setExternalBoundary(bool enabled) -> void {
	((*this)).external_boundary = enabled;
}
auto python::ispa_std::lexer::Lexer_base::set_bound(long long end) -> void {
	"Limit lexing to source[:end] and make hitting it an external boundary.";
	((*this)).bound = end;
	((*this)).external_boundary = true;
}
auto python::ispa_std::lexer::Lexer_base::hasInput() -> bool {
	return ((*this))._has_input;
}
auto python::ispa_std::lexer::Lexer_base::hasTokens() -> bool {
	return ((static_cast<long long>(((*this)).tokens.size())) > (0));
}
auto python::ispa_std::lexer::Lexer_base::source() -> std::string {
	return ((*((*this))._src)).text;
}
auto python::ispa_std::lexer::Lexer_base::setInput(const std::string& text) -> std::shared_ptr<python::ispa_std::lexer::Lexer_base> {
	(((*this)).errors).clear();
	((*this))._src = ISPA_STD::construct<python::ispa_std::dfa_api::SourceText>(text);
	((*this))._has_input = true;
	return ::ISPA_STD::self_ptr(this);
}
auto python::ispa_std::lexer::Lexer_base::getTokens() -> const std::vector<std::shared_ptr<python::ispa_std::raw_token::RawToken>>& {
	return ((*this)).tokens;
}
auto python::ispa_std::lexer::Lexer_base::clearTokens() -> void {
	(((*this)).tokens).clear();
}
auto python::ispa_std::lexer::Lexer_base::getErrors() -> const std::vector<std::string>& {
	return ((*this)).errors;
}
auto python::ispa_std::lexer::Lexer_base::copy_base_into(std::shared_ptr<python::ispa_std::lexer::Lexer_base> other) -> void {
	"For clone() implementations: copy the base-class state.";
	((*other))._src = ((*this))._src;
	((*other))._has_input = ((*this))._has_input;
	((*other)).tokens = {};
	(((*other)).tokens).insert((((*other)).tokens).end(), ((*this)).tokens.begin(), ((*this)).tokens.end());
	((*other)).errors = {};
	(((*other)).errors).insert((((*other)).errors).end(), ((*this)).errors.begin(), ((*this)).errors.end());
	((*other)).line = ((*this)).line;
	((*other)).external_boundary = ((*this)).external_boundary;
	((*other)).bound = ((*this)).bound;
}
auto python::ispa_std::lexer::Lexer_base::assign_from(std::shared_ptr<python::ispa_std::lexer::Lexer_base> other) -> void {
	"Lexer_base::operator=: tokens, errors and input (not line/flags).";
	((*this)).tokens = {};
	(((*this)).tokens).insert((((*this)).tokens).end(), ((*other)).tokens.begin(), ((*other)).tokens.end());
	((*this)).errors = {};
	(((*this)).errors).insert((((*this)).errors).end(), ((*other)).errors.begin(), ((*other)).errors.end());
	((*this))._src = ((*other))._src;
	((*this))._has_input = ((*other))._has_input;
}
auto python::ispa_std::lexer::Lexer_base::equals(std::shared_ptr<python::ispa_std::lexer::Lexer_base> other) -> bool {
	if (((static_cast<long long>(((*this)).tokens.size())) != (static_cast<long long>(((*other)).tokens.size())))) {
		return false;
	}
	long long i = 0;
	while (((i) < (static_cast<long long>(((*this)).tokens.size())))) {
		if ((!(python::ispa_std::lexer::_token_equal((((*this)).tokens)[i], (((*other)).tokens)[i])))) {
			return false;
		}
		i = ((i) + (1));
	}
	return true;
}
auto python::ispa_std::lexer::Lexer_base::getCurrentPos(long long pos) -> long long {
	return pos;
}
auto python::ispa_std::lexer::Lexer_base::skip_spaces(std::shared_ptr<python::ispa_std::refs::Cursor> cur) -> long long {
	long long prev = ((*cur)).pos;
	while (python::ispa_std::chars::is_space(python::ispa_std::chars::at(((*((*this))._src)).text, ((*cur)).pos))) {
		((*cur)).pos = ((((*cur)).pos) + (1));
	}
	return ((((*cur)).pos) - (prev));
}
auto python::ispa_std::lexer::Lexer_base::line_at(long long pos) -> long long {
	if ((((!(((*this))._has_input))) || (((pos) < (0))))) {
		python::ispa_std::failures::logic_error("Lexer line lookup has no source position");
	}
	if (((pos) > (static_cast<long long>(((*((*this))._src)).text.size())))) {
		python::ispa_std::failures::out_of_range("Lexer line lookup exceeds source input");
	}
	long long count = 1;
	long long i = 0;
	while (((i) < (pos))) {
		if (((std::string(1, (((*((*this))._src)).text)[i])) == ("\n"))) {
			count = ((count) + (1));
		}
		i = ((i) + (1));
	}
	return count;
}
auto python::ispa_std::lexer::Lexer_base::column_at(long long pos) -> long long {
	if ((((!(((*this))._has_input))) || (((pos) < (0))))) {
		python::ispa_std::failures::logic_error("Lexer column lookup has no source position");
	}
	if (((pos) > (static_cast<long long>(((*((*this))._src)).text.size())))) {
		python::ispa_std::failures::out_of_range("Lexer column lookup exceeds source input");
	}
	long long count = 1;
	long long i = 0;
	while (((i) < (pos))) {
		if (((std::string(1, (((*((*this))._src)).text)[i])) == ("\n"))) {
			count = 0;
		} else {
			count = ((count) + (1));
		}
		i = ((i) + (1));
	}
	return count;
}
auto python::ispa_std::lexer::Lexer_base::line_prefix(long long pos) -> std::string {
	if ((((!(((*this))._has_input))) || (((pos) < (0))))) {
		python::ispa_std::failures::logic_error("Lexer line prefix has no source position");
	}
	long long begin = pos;
	while (((((((begin) > (0))) && (((python::ispa_std::chars::at(((*((*this))._src)).text, ((begin) - (1)))) != ("\n"))))) && (((python::ispa_std::chars::at(((*((*this))._src)).text, ((begin) - (1)))) != ("\r"))))) {
		begin = ((begin) - (1));
	}
	return python::ispa_std::string::substr(((*((*this))._src)).text, begin, ((pos) - (begin)));
}
auto python::ispa_std::lexer::Lexer_base::next_char(long long pos) -> std::string {
	if (((pos) < (0))) {
		python::ispa_std::failures::logic_error("Lexer next character has no source position");
	}
	return python::ispa_std::chars::at(((*((*this))._src)).text, pos);
}
auto python::ispa_std::lexer::Lexer_base::panic_mode(std::shared_ptr<python::ispa_std::refs::Cursor> cur) -> void {
	if (((((*cur)).pos) < (static_cast<long long>(((*((*this))._src)).text.size())))) {
		((*cur)).pos = ((((*cur)).pos) + (1));
	}
}
auto python::ispa_std::lexer::Lexer_base::lexer_failure_context(long long cursor, const std::string& rule, const std::string& failed_rule) -> std::shared_ptr<python::ispa_std::lexer::LexerFailureContext> {
	return ISPA_STD::construct<python::ispa_std::lexer::LexerFailureContext>(((*((*this))._src)).text, cursor, ((*this)).errors, rule, failed_rule);
}
auto python::ispa_std::lexer::Lexer_base::lexer_failure_result(std::shared_ptr<python::ispa_std::lexer::LexerFailureContext> ctx, std::shared_ptr<python::ispa_std::raw_token::RawToken> inserted) -> std::shared_ptr<python::ispa_std::utility::Pair<long long, std::shared_ptr<python::ispa_std::raw_token::RawToken>>> {
	"(state, token) for a lexer fail handler; see DFA run().";
	if (((((*ctx)).decision) == (python::ispa_std::lexer::LexerDecision::DELETE))) {
		return ISPA_STD::construct<python::ispa_std::utility::Pair<long long, std::shared_ptr<python::ispa_std::raw_token::RawToken>>>(python::ispa_std::dfa_api::LEXER_DELETE_STATE, ((*this)).empty_token());
	}
	if (((((*ctx)).decision) == (python::ispa_std::lexer::LexerDecision::INSERT))) {
		return ISPA_STD::construct<python::ispa_std::utility::Pair<long long, std::shared_ptr<python::ispa_std::raw_token::RawToken>>>(python::ispa_std::dfa_api::NULL_STATE, inserted);
	}
	if (((static_cast<long long>(((*this)).errors.size())) == (0))) {
		python::ispa_std::failures::runtime_error("unmatched lexer input");
	} else {
		python::ispa_std::failures::runtime_error((((*this)).errors)[((static_cast<long long>(((*this)).errors.size())) - (1))]);
	}
	return ISPA_STD::construct<python::ispa_std::utility::Pair<long long, std::shared_ptr<python::ispa_std::raw_token::RawToken>>>(python::ispa_std::dfa_api::NULL_STATE, inserted);
}
auto python::ispa_std::lexer::Lexer_base::lookup(std::shared_ptr<python::ispa_std::dfa_run::DfaTables> tables, std::function<std::shared_ptr<python::ispa_std::utility::Pair<long long, std::shared_ptr<python::ispa_std::raw_token::RawToken>>>(long long, std::shared_ptr<python::ispa_std::dfa_api::Captures>, long long, long long, long long, std::shared_ptr<python::ispa_std::refs::Cursor>)> semantic, std::shared_ptr<python::ispa_std::refs::Cursor> cur) -> std::shared_ptr<python::ispa_std::raw_token::RawToken> {
	"Run the DFA for one token at cur (empty token at end of input).";
	if (((((*cur)).pos) >= (static_cast<long long>(((*((*this))._src)).text.size())))) {
		return ((*this)).empty_token();
	}
	auto scan = ISPA_STD::construct<python::ispa_std::refs::Cursor>(((*cur)).pos, ((*this)).line);
	auto token = python::ispa_std::dfa_run::run(((*this))._src, scan, ((*this)).getCurrentPos(((*cur)).pos), tables, semantic, ((*this)).empty_token());
	((*cur)).pos = ((*scan)).pos;
	((*cur)).line = ((*scan)).line;
	((*this)).line = ((*scan)).line;
	return token;
}
auto python::ispa_std::lexer::Lexer_base::makeTokens() -> std::vector<std::shared_ptr<python::ispa_std::raw_token::RawToken>> {
	if (((*this)).hasExternalRules()) {
		python::ispa_std::failures::logic_error("AMBIGUOUS eager token stream: use lazy tokenization with external parsers");
	}
	if ((!(((*this))._has_input))) {
		python::ispa_std::failures::api_error("can\'t lex because no input provided. ", {});
	}
	python::ispa_std::refs::Cursor cur = ISPA_STD::make_value<python::ispa_std::refs::Cursor>(0, ((*this)).line);
	long long n = static_cast<long long>(((*((*this))._src)).text.size());
	while ((((cur).pos) < (n))) {
		long long before = (cur).pos;
		auto token = ((*this)).makeToken(ISPA_STD::borrow(cur));
		if ((((cur).pos) == (before))) {
			python::ispa_std::failures::internal_error("no matches at position {}, on character {}, and no exception can handle it", "Lexer TDFA generation", true, std::vector<std::string>{python::ispa_std::format::int_to_string((cur).pos), python::ispa_std::chars::at(((*((*this))._src)).text, (cur).pos)});
		}
		if ((!(python::ispa_std::lexer::is_token_end(token)))) {
			((*this)).push(token);
		}
	}
	((*this)).push_eof();
	return ((*this)).tokens;
}
auto python::ispa_std::lexer::Lexer_base::makeTokensFromText(const std::string& text) -> std::vector<std::shared_ptr<python::ispa_std::raw_token::RawToken>> {
	((*this)).setInput(text);
	return ((*this)).makeTokens();
}
auto python::ispa_std::lexer::_token_equal(std::shared_ptr<python::ispa_std::raw_token::RawToken> a, std::shared_ptr<python::ispa_std::raw_token::RawToken> b) -> bool {
	if (((((*a)).empty()) != (((*b)).empty()))) {
		return false;
	}
	if (((*a)).empty()) {
		return ((((*a)).kind()) == (((*b)).kind()));
	}
	return ((((((((((*a)).kind()) == (((*b)).kind()))) && (((((*a)).index()) == (((*b)).index()))))) && (((((*((*a)).meta())).startpos()) == (((*((*b)).meta())).startpos()))))) && (((((*((*a)).meta())).length()) == (((*((*b)).meta())).length()))));
}
auto python::ispa_std::lexer::TokenCursor::__init__() -> void {
	((*this)).owner = ISPA_STD::construct<python::ispa_std::lexer::Lexer_base>();
	((*this)).state = ISPA_STD::construct<python::ispa_std::lexer::_SharedState>(0);
	((*this)).token_index = 0;
	((*this)).distance_base = 0;
	((*this)).owned_owner = ((*this)).owner;
	((*this)).has_owned = false;
}
auto python::ispa_std::lexer::TokenCursor::copy() -> std::shared_ptr<python::ispa_std::lexer::TokenCursor> {
	python::ispa_std::failures::logic_error("TokenCursor.copy is abstract");
	return ::ISPA_STD::self_ptr(this);
}
auto python::ispa_std::lexer::TokenCursor::assign(std::shared_ptr<python::ispa_std::lexer::TokenCursor> other) -> void {
	((*this)).owner = ((*other)).owner;
	((*this)).state = ((*other)).state;
	((*this)).token_index = ((*other)).token_index;
	((*this)).distance_base = ((*other)).distance_base;
	((*this)).owned_owner = ((*other)).owned_owner;
	((*this)).has_owned = ((*other)).has_owned;
}
auto python::ispa_std::lexer::TokenCursor::has_source() -> bool {
	return false;
}
auto python::ispa_std::lexer::TokenCursor::sourceOrigin() -> std::string {
	return "";
}
auto python::ispa_std::lexer::TokenCursor::getErrors() -> std::vector<std::string> {
	return ((*((*this)).owner)).getErrors();
}
auto python::ispa_std::lexer::TokenCursor::sourcePosition() -> long long {
	return ( - (1));
}
auto python::ispa_std::lexer::TokenCursor::consumeExternal(long long end) -> void {
	python::ispa_std::failures::logic_error("TokenCursor.consumeExternal needs a lazy cursor");
}
auto python::ispa_std::lexer::TokenCursor::isEnd() -> bool {
	python::ispa_std::failures::logic_error("TokenCursor.isEnd is abstract");
	return true;
}
auto python::ispa_std::lexer::TokenCursor::next() -> std::shared_ptr<python::ispa_std::lexer::TokenCursor> {
	python::ispa_std::failures::logic_error("TokenCursor.next is abstract");
	return ::ISPA_STD::self_ptr(this);
}
auto python::ispa_std::lexer::TokenCursor::prev() -> std::shared_ptr<python::ispa_std::lexer::TokenCursor> {
	python::ispa_std::failures::logic_error("TokenCursor.prev is abstract");
	return ::ISPA_STD::self_ptr(this);
}
auto python::ispa_std::lexer::TokenCursor::advance_by(long long count) -> std::shared_ptr<python::ispa_std::lexer::TokenCursor> {
	python::ispa_std::failures::logic_error("TokenCursor.advance_by is abstract");
	return ::ISPA_STD::self_ptr(this);
}
auto python::ispa_std::lexer::TokenCursor::plus(long long count) -> std::shared_ptr<python::ispa_std::lexer::TokenCursor> {
	auto out = ((*this)).copy();
	((*out)).advance_by(count);
	return out;
}
auto python::ispa_std::lexer::TokenCursor::current() -> std::shared_ptr<python::ispa_std::raw_token::RawToken> {
	python::ispa_std::failures::logic_error("TokenCursor.current is abstract");
	return ((*((*this)).owner)).empty_token();
}
auto python::ispa_std::lexer::TokenCursor::kind_is(long long type_id) -> bool {
	"Whether the token here holds `type_id`; cursors that can answer without materialising a RawToken override it.";
	return ((*((*this)).current())).holds(type_id);
}
auto python::ispa_std::lexer::TokenCursor::minus(std::shared_ptr<python::ispa_std::lexer::TokenCursor> other) -> long long {
	return ((((*this)).distance()) - (((*other)).distance()));
}
auto python::ispa_std::lexer::TokenCursor::distance() -> long long {
	return ((((*this)).distance_base) + (((*this)).token_index));
}
auto python::ispa_std::lexer::TokenCursor::resumePosition() -> long long {
	return ( - (1));
}
auto python::ispa_std::lexer::TokenCursor::advanceCharacters(long long count) -> bool {
	python::ispa_std::failures::runtime_error("advanceCharacters needs a lazy cursor (external parser integration)");
	return false;
}
auto python::ispa_std::lexer::LazyIterator::__init__(std::shared_ptr<python::ispa_std::lexer::Lexer_base> lexer, std::shared_ptr<python::ispa_std::lexer::_SharedState> state, long long token_index, long long distance_base) -> void {
	((*this)).owner = lexer;
	((*this)).state = state;
	((*this)).token_index = token_index;
	((*this)).distance_base = distance_base;
	((*this)).owned_owner = lexer;
	((*this)).has_owned = false;
}
auto python::ispa_std::lexer::LazyIterator::begin(std::shared_ptr<python::ispa_std::lexer::Lexer_base> lexer) -> std::shared_ptr<python::ispa_std::lexer::LazyIterator> {
	"lazy_iterator(lexer, in): start at the lexer\'s own input.";
	return python::ispa_std::lexer::LazyIterator::at_position(lexer, 0);
}
auto python::ispa_std::lexer::LazyIterator::at_position(std::shared_ptr<python::ispa_std::lexer::Lexer_base> lexer, long long start) -> std::shared_ptr<python::ispa_std::lexer::LazyIterator> {
	"lazy_iterator(lexer, origin, start).";
	auto it = ISPA_STD::construct<python::ispa_std::lexer::LazyIterator>(lexer, ISPA_STD::construct<python::ispa_std::lexer::_SharedState>(start), 0, 0);
	if (((start) > (0))) {
		((*lexer)).line = ((*lexer)).line_at(start);
	}
	return it;
}
auto python::ispa_std::lexer::LazyIterator::copy() -> std::shared_ptr<python::ispa_std::lexer::TokenCursor> {
	auto out = ISPA_STD::construct<python::ispa_std::lexer::LazyIterator>(((*this)).owner, ((*this)).state, ((*this)).token_index, ((*this)).distance_base);
	((*out)).owned_owner = ((*this)).owned_owner;
	((*out)).has_owned = ((*this)).has_owned;
	return out;
}
auto python::ispa_std::lexer::LazyIterator::has_source() -> bool {
	return true;
}
auto python::ispa_std::lexer::LazyIterator::ensure_token() -> void {
	auto owner = ((*this)).owner;
	auto state = ((*this)).state;
	bool running = true;
	while (((running) && (((static_cast<long long>(((*owner)).tokens.size())) <= (((*this)).token_index))))) {
		if (((*state)).eof_processed) {
			running = false;
		} else {
			if ((!(((*owner)).hasInput()))) {
				python::ispa_std::failures::logic_error("Lazy lexer has no source input");
			}
			if (((((*state)).pos) >= (static_cast<long long>(((*owner)).source().size())))) {
				((*state)).eof_processed = true;
				((*owner)).push_eof();
				running = false;
			} else {
				long long before = ((*state)).pos;
				auto checkpoint = ((*owner)).checkpoint();
				python::ispa_std::refs::Cursor cur = ISPA_STD::make_value<python::ispa_std::refs::Cursor>(((*state)).pos, ((*owner)).line);
				std::shared_ptr<python::ispa_std::raw_token::RawToken> raw = ((*owner)).empty_token();
				if (((((((*owner)).bound) < (0))) || (((((*state)).pos) < (((*owner)).bound))))) {
					raw = ((*owner)).makeToken(ISPA_STD::borrow(cur));
				}
				((*state)).pos = (cur).pos;
				if (((((*raw)).empty()) && (((*owner)).external_boundary))) {
					((*state)).pos = before;
					((*state)).eof_processed = true;
					((*owner)).push_eof();
					running = false;
				} else {
					if (((((((*raw)).empty()) && ((!(((*raw)).isAmbiguous()))))) && (((*owner)).hasExternalRules()))) {
						((*state)).pos = ((before) + (1));
						raw = ((*owner)).ambiguous_token(before, ((*state)).pos);
					}
					if (((((*state)).pos) == (before))) {
						python::ispa_std::failures::runtime_error("Lazy lexer did not advance source position");
					}
					if ((!(python::ispa_std::lexer::is_token_end(raw)))) {
						((*owner)).push(raw);
					}
					while (((static_cast<long long>(((*state)).checkpoints.size())) < (static_cast<long long>(((*owner)).tokens.size())))) {
						(((*state)).checkpoints).push_back(checkpoint);
					}
				}
			}
		}
	}
}
auto python::ispa_std::lexer::LazyIterator::sourceOrigin() -> std::string {
	return ((*((*this)).owner)).source();
}
auto python::ispa_std::lexer::LazyIterator::getErrors() -> std::vector<std::string> {
	return ((*((*this)).owner)).getErrors();
}
auto python::ispa_std::lexer::LazyIterator::sourcePosition() -> long long {
	if (((((*this)).token_index) < (static_cast<long long>(((*((*this)).owner)).tokens.size())))) {
		auto token = (((*((*this)).owner)).tokens)[((*this)).token_index];
		if ((((!(python::ispa_std::lexer::is_token_end(token)))) && (((*((*token)).meta())).has_source()))) {
			return ((*((*token)).meta())).startpos();
		}
	}
	return ((*((*this)).state)).pos;
}
auto python::ispa_std::lexer::LazyIterator::consumeExternal(long long end) -> void {
	long long begin = ((*this)).sourcePosition();
	std::string origin = ((*((*this)).owner)).source();
	if (((((end) < (begin))) || (((end) > (static_cast<long long>(origin.size())))))) {
		python::ispa_std::failures::logic_error("External parser returned an invalid source cursor");
	}
	auto branch = ((*((*this)).owner)).clone();
	if (((((*this)).token_index) < (static_cast<long long>(((*((*this)).state)).checkpoints.size())))) {
		((*branch)).restoreCheckpoint((((*((*this)).state)).checkpoints)[((*this)).token_index]);
	}
	(((*branch)).tokens).clear();
	((*branch))._src = ISPA_STD::construct<python::ispa_std::dfa_api::SourceText>(origin);
	((*branch))._has_input = true;
	((*branch)).line = ((*branch)).line_at(end);
	((*branch)).externalAdvance(begin, end);
	long long extra = 0;
	if (((end) != (begin))) {
		extra = 1;
	}
	((*this)).distance_base = ((((*this)).distance()) + (extra));
	((*this)).token_index = 0;
	((*this)).owned_owner = branch;
	((*this)).has_owned = true;
	((*this)).owner = branch;
	((*this)).state = ISPA_STD::construct<python::ispa_std::lexer::_SharedState>(end);
}
auto python::ispa_std::lexer::LazyIterator::isEnd() -> bool {
	((*this)).ensure_token();
	if (((((*this)).token_index) >= (static_cast<long long>(((*((*this)).owner)).tokens.size())))) {
		return true;
	}
	return python::ispa_std::lexer::is_token_end((((*((*this)).owner)).tokens)[((*this)).token_index]);
}
auto python::ispa_std::lexer::LazyIterator::next() -> std::shared_ptr<python::ispa_std::lexer::TokenCursor> {
	"operator++";
	if ((!(((*this)).isEnd()))) {
		((*this)).token_index = ((((*this)).token_index) + (1));
	}
	return ::ISPA_STD::self_ptr(this);
}
auto python::ispa_std::lexer::LazyIterator::prev() -> std::shared_ptr<python::ispa_std::lexer::TokenCursor> {
	"operator--";
	if (((((*this)).token_index) > (0))) {
		((*this)).token_index = ((((*this)).token_index) - (1));
	}
	return ::ISPA_STD::self_ptr(this);
}
auto python::ispa_std::lexer::LazyIterator::advance_by(long long count) -> std::shared_ptr<python::ispa_std::lexer::TokenCursor> {
	"operator+=";
	long long left = count;
	while (((((left) > (0))) && ((!(((*this)).isEnd()))))) {
		((*this)).next();
		left = ((left) - (1));
	}
	return ::ISPA_STD::self_ptr(this);
}
auto python::ispa_std::lexer::LazyIterator::current() -> std::shared_ptr<python::ispa_std::raw_token::RawToken> {
	"operator* / operator->";
	((*this)).ensure_token();
	if (((((*this)).token_index) >= (static_cast<long long>(((*((*this)).owner)).tokens.size())))) {
		python::ispa_std::failures::out_of_range("lazy_iterator: dereference beyond EOF");
	}
	return (((*((*this)).owner)).tokens)[((*this)).token_index];
}
auto python::ispa_std::lexer::LazyIterator::resumePosition() -> long long {
	"Source position just after the current token (-1 when unknown).";
	((*this)).ensure_token();
	if (((((*this)).token_index) >= (static_cast<long long>(((*((*this)).owner)).tokens.size())))) {
		return ((*((*this)).state)).pos;
	}
	auto token = (((*((*this)).owner)).tokens)[((*this)).token_index];
	if (python::ispa_std::lexer::is_token_end(token)) {
		return ((*((*this)).state)).pos;
	}
	return ((*((*token)).meta())).end_pos();
}
auto python::ispa_std::lexer::LazyIterator::advanceCharacters(long long count) -> bool {
	"Resume lexing `count` source units after the current token\'s start,\n        invalidating cached tokens from here on (for external parsers).";
	((*this)).ensure_token();
	if (((((*this)).token_index) >= (static_cast<long long>(((*((*this)).owner)).tokens.size())))) {
		return false;
	}
	auto current = (((*((*this)).owner)).tokens)[((*this)).token_index];
	if (((python::ispa_std::lexer::is_token_end(current)) || ((!(((*((*current)).meta())).has_source()))))) {
		return false;
	}
	long long target = ((*((*current)).meta())).startpos();
	long long n = static_cast<long long>(((*((*this)).owner)).source().size());
	long long i = 0;
	while (((i) < (count))) {
		if (((target) >= (n))) {
			return false;
		}
		target = ((target) + (1));
		i = ((i) + (1));
	}
	while (((static_cast<long long>(((*((*this)).owner)).tokens.size())) > (((*this)).token_index))) {
		ISPA_STD::python_pop((((*((*this)).owner)).tokens));
	}
	((*((*this)).state)).pos = target;
	((*((*this)).state)).eof_processed = false;
	((*this)).ensure_token();
	return true;
}
auto python::ispa_std::lexer::TokenIterator::__init__(std::shared_ptr<python::ispa_std::lexer::Lexer_base> owner, long long index) -> void {
	((*this)).owner = owner;
	((*this)).state = ISPA_STD::construct<python::ispa_std::lexer::_SharedState>(0);
	((*this)).token_index = index;
	((*this)).distance_base = 0;
	((*this)).owned_owner = owner;
	((*this)).has_owned = false;
}
auto python::ispa_std::lexer::TokenIterator::begin(std::shared_ptr<python::ispa_std::lexer::Lexer_base> owner) -> std::shared_ptr<python::ispa_std::lexer::TokenCursor> {
	return ISPA_STD::construct<python::ispa_std::lexer::TokenIterator>(owner, 0);
}
auto python::ispa_std::lexer::TokenIterator::copy() -> std::shared_ptr<python::ispa_std::lexer::TokenCursor> {
	return ISPA_STD::construct<python::ispa_std::lexer::TokenIterator>(((*this)).owner, ((*this)).token_index);
}
auto python::ispa_std::lexer::TokenIterator::next() -> std::shared_ptr<python::ispa_std::lexer::TokenCursor> {
	((*this)).token_index = ((((*this)).token_index) + (1));
	return ::ISPA_STD::self_ptr(this);
}
auto python::ispa_std::lexer::TokenIterator::prev() -> std::shared_ptr<python::ispa_std::lexer::TokenCursor> {
	if (((((*this)).token_index) != (0))) {
		((*this)).token_index = ((((*this)).token_index) - (1));
	}
	return ::ISPA_STD::self_ptr(this);
}
auto python::ispa_std::lexer::TokenIterator::advance_by(long long count) -> std::shared_ptr<python::ispa_std::lexer::TokenCursor> {
	((*this)).token_index = ((((*this)).token_index) + (count));
	return ::ISPA_STD::self_ptr(this);
}
auto python::ispa_std::lexer::TokenIterator::isEnd() -> bool {
	if (((((*this)).token_index) >= (static_cast<long long>(((*((*this)).owner)).tokens.size())))) {
		return true;
	}
	return python::ispa_std::lexer::is_token_end((((*((*this)).owner)).tokens)[((*this)).token_index]);
}
auto python::ispa_std::lexer::TokenIterator::current() -> std::shared_ptr<python::ispa_std::raw_token::RawToken> {
	if (((((*this)).token_index) >= (static_cast<long long>(((*((*this)).owner)).tokens.size())))) {
		python::ispa_std::failures::out_of_range("token iterator: dereference beyond the token list");
	}
	return (((*((*this)).owner)).tokens)[((*this)).token_index];
}
auto python::ispa_std::lexer::TokenIterator::resumePosition() -> long long {
	"Source position after the current token, -1 when it has none.\n        (The C++ body referenced members that do not exist and was never\n        instantiated; this is its documented intent.)";
	if (((((*this)).token_index) >= (static_cast<long long>(((*((*this)).owner)).tokens.size())))) {
		return ( - (1));
	}
	auto token = (((*((*this)).owner)).tokens)[((*this)).token_index];
	if (((python::ispa_std::lexer::is_token_end(token)) || ((!(((*((*token)).meta())).has_source()))))) {
		return ( - (1));
	}
	return ((*((*token)).meta())).end_pos();
}
long long python::ispa_std::recovery::Mode::PanicUntil = 0;
long long python::ispa_std::recovery::Mode::Insert = 1;
long long python::ispa_std::recovery::Mode::Delete = 2;
long long python::ispa_std::recovery::Mode::Replace = 3;
long long python::ispa_std::recovery::Mode::Retry = 4;
long long python::ispa_std::recovery::Mode::Synchronize = 5;
long long python::ispa_std::recovery::Mode::Propagate = 6;
long long python::ispa_std::recovery::Mode::Abort = 7;
auto python::ispa_std::recovery::Decision::__init__(long long mode, long long synthesized, long long unexpected, long long skipped) -> void {
	((*this)).mode = mode;
	((*this)).synthesized = synthesized;
	((*this)).unexpected = unexpected;
	((*this)).skipped = skipped;
}
auto python::ispa_std::recovery::Decision::initial() -> std::shared_ptr<python::ispa_std::recovery::Decision> {
	return ISPA_STD::construct<python::ispa_std::recovery::Decision>(python::ispa_std::recovery::Mode::Propagate, python::ispa_std::recovery::EOF_TOKEN, python::ispa_std::recovery::EOF_TOKEN, 0);
}
auto python::ispa_std::recovery::Decision::copy() -> std::shared_ptr<python::ispa_std::recovery::Decision> {
	return ISPA_STD::construct<python::ispa_std::recovery::Decision>(((*this)).mode, ((*this)).synthesized, ((*this)).unexpected, ((*this)).skipped);
}
auto python::ispa_std::recovery::Diagnostic::__init__() -> void {
	((*this)).rule = "";
	((*this)).unexpected = python::ispa_std::recovery::EOF_TOKEN;
	((*this)).expected = {};
	((*this)).decision = python::ispa_std::recovery::Decision::initial();
	((*this)).expected_text = "";
	((*this)).offset = python::ispa_std::recovery::NO_OFFSET;
	((*this)).length = 0;
	((*this)).message = "";
	((*this)).code = "";
	((*this)).notes = {};
}
auto python::ispa_std::recovery::State::__init__() -> void {
	((*this)).max_errors = 32;
	((*this)).max_total_skipped = 2048;
	((*this)).total_skipped = 0;
	((*this)).diagnostics = {};
	((*this)).anchors = {};
	((*this)).anchor_count = 0;
}
auto python::ispa_std::recovery::State::exhausted() -> bool {
	return ((((static_cast<long long>(((*this)).diagnostics.size())) >= (((*this)).max_errors))) || (((((*this)).total_skipped) >= (((*this)).max_total_skipped))));
}
auto python::ispa_std::recovery::Plan::__init__(const std::string& rule, const std::vector<long long>& expected, const std::vector<long long>& continuation, const std::vector<long long>& follow, const std::vector<long long>& explicit_sync, bool allow_insertion, bool allow_deletion, bool allow_replacement, long long max_skip) -> void {
	((*this)).rule = rule;
	((*this)).expected = expected;
	((*this)).continuation = continuation;
	((*this)).follow = follow;
	((*this)).explicit_sync = explicit_sync;
	((*this)).allow_insertion = allow_insertion;
	((*this)).allow_deletion = allow_deletion;
	((*this)).allow_replacement = allow_replacement;
	((*this)).max_skip = max_skip;
}
auto python::ispa_std::recovery::contains(const std::vector<long long>& tokens, long long id) -> bool {
	long long i = 0;
	while (((i) < (static_cast<long long>(tokens.size())))) {
		if ((((tokens)[i]) == (id))) {
			return true;
		}
		i = ((i) + (1));
	}
	return false;
}
auto python::ispa_std::recovery::decode(const std::string& encoded) -> std::vector<long long> {
	"Comma/space separated unsigned decimal token ids (LangAPI fail wrappers).";
	std::vector<long long> result = std::vector<long long>{};
	long long i = 0;
	long long n = static_cast<long long>(encoded.size());
	while (((i) < (n))) {
		while (((((i) < (n))) && (((((std::string(1, (encoded)[i])) == (","))) || (((std::string(1, (encoded)[i])) == (" "))))))) {
			i = ((i) + (1));
		}
		if (((i) < (n))) {
			long long value = 0;
			bool has_digit = false;
			while (((((((i) < (n))) && (((std::string(1, (encoded)[i])) >= ("0"))))) && (((std::string(1, (encoded)[i])) <= ("9"))))) {
				has_digit = true;
				value = ((((value) * (10))) + (python::ispa_std::recovery::_digit(std::string(1, (encoded)[i]))));
				i = ((i) + (1));
			}
			if ((((!(has_digit))) || (((((((i) < (n))) && (((std::string(1, (encoded)[i])) != (","))))) && (((std::string(1, (encoded)[i])) != (" "))))))) {
				python::ispa_std::failures::invalid_argument("ISPA recovery: malformed token set");
				return result;
			}
			(result).push_back(value);
		}
	}
	return result;
}
auto python::ispa_std::recovery::_digit(const std::string& c) -> long long {
	long long d = 0;
	while (((((d) < (10))) && (((std::string(1, ("0123456789")[d])) != (c))))) {
		d = ((d) + (1));
	}
	return d;
}
auto python::ispa_std::recovery::token_id(std::shared_ptr<python::ispa_std::raw_token::RawToken> token) -> long long {
	return ((*token)).kind();
}
auto python::ispa_std::recovery::_kind(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> long long {
	if (((*pos)).isEnd()) {
		return python::ispa_std::recovery::EOF_TOKEN;
	}
	return python::ispa_std::recovery::token_id(((*pos)).current());
}
auto python::ispa_std::recovery::recover(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::recovery::State> state, std::shared_ptr<python::ispa_std::recovery::Plan> plan, std::function<long long(std::shared_ptr<python::ispa_std::raw_token::RawToken>)> token_kind) -> std::shared_ptr<python::ispa_std::recovery::Decision> {
	"Pick and perform a repair at `pos`. `token_kind(token) -> id`; pos is\n    advanced in place when tokens are skipped.";
	auto at = python::ispa_std::recovery::EOF_TOKEN;
	if ((!(((*pos)).isEnd()))) {
		at = token_kind(((*pos)).current());
	}
	auto offset = python::ispa_std::recovery::NO_OFFSET;
	long long length = 0;
	std::string actual = "end of input";
	if (((at) != (python::ispa_std::recovery::EOF_TOKEN))) {
		auto meta = ((*((*pos)).current())).meta();
		offset = ((*meta)).startpos();
		length = ((*meta)).length();
		if (((*meta)).has_source()) {
			actual = (((("\'") + (((*meta)).text()))) + ("\'"));
		} else {
			actual = (("token ") + (python::ispa_std::format::int_to_string(at)));
		}
	}
	auto decision = ISPA_STD::construct<python::ispa_std::recovery::Decision>(python::ispa_std::recovery::Mode::Propagate, python::ispa_std::recovery::EOF_TOKEN, at, 0);
	if (((*state)).exhausted()) {
		((*decision)).mode = python::ispa_std::recovery::Mode::Abort;
	} else {
		if (python::ispa_std::recovery::contains(((*plan)).expected, at)) {
			((*decision)).mode = python::ispa_std::recovery::Mode::Retry;
		} else {
			auto nxt = python::ispa_std::recovery::EOF_TOKEN;
			if (((at) != (python::ispa_std::recovery::EOF_TOKEN))) {
				auto lookahead = ((*pos)).copy();
				((*lookahead)).next();
				if ((!(((*lookahead)).isEnd()))) {
					nxt = token_kind(((*lookahead)).current());
				}
			}
			bool expected_one = ((static_cast<long long>(((*plan)).expected.size())) == (1));
			bool protected_token = ((python::ispa_std::recovery::contains(((*plan)).follow, at)) || (python::ispa_std::recovery::_anchor(plan, state, at)));
			if (((((((((*plan)).allow_deletion) && ((!(protected_token))))) && (((at) != (python::ispa_std::recovery::EOF_TOKEN))))) && (python::ispa_std::recovery::contains(((*plan)).expected, nxt)))) {
				((*pos)).next();
				((*decision)).mode = python::ispa_std::recovery::Mode::Delete;
				((*decision)).skipped = 1;
			} else {
				if (((((((*plan)).allow_insertion) && (expected_one))) && (((python::ispa_std::recovery::contains(((*plan)).continuation, at)) || (python::ispa_std::recovery::contains(((*plan)).follow, at)))))) {
					((*decision)).mode = python::ispa_std::recovery::Mode::Insert;
					((*decision)).synthesized = (((*plan)).expected)[0];
				} else {
					if (((((((((((*plan)).allow_replacement) && (expected_one))) && ((!(protected_token))))) && (((at) != (python::ispa_std::recovery::EOF_TOKEN))))) && (python::ispa_std::recovery::contains(((*plan)).continuation, nxt)))) {
						((*pos)).next();
						((*decision)).mode = python::ispa_std::recovery::Mode::Replace;
						((*decision)).synthesized = (((*plan)).expected)[0];
						((*decision)).skipped = 1;
					} else {
						if (((python::ispa_std::recovery::contains(((*plan)).follow, at)) || (((at) == (python::ispa_std::recovery::EOF_TOKEN))))) {
							((*decision)).mode = python::ispa_std::recovery::Mode::Propagate;
						} else {
							if (((python::ispa_std::recovery::contains(((*plan)).continuation, at)) || (python::ispa_std::recovery::_anchor(plan, state, at)))) {
								((*decision)).mode = python::ispa_std::recovery::Mode::Synchronize;
							} else {
								bool scanning = true;
								while (((((((scanning) && ((!(((*pos)).isEnd()))))) && (((((*decision)).skipped) < (((*plan)).max_skip))))) && (((((((*state)).total_skipped) + (((*decision)).skipped))) < (((*state)).max_total_skipped))))) {
									long long ident = token_kind(((*pos)).current());
									if (((((((python::ispa_std::recovery::contains(((*plan)).expected, ident)) || (python::ispa_std::recovery::contains(((*plan)).continuation, ident)))) || (python::ispa_std::recovery::contains(((*plan)).follow, ident)))) || (python::ispa_std::recovery::_anchor(plan, state, ident)))) {
										scanning = false;
									} else {
										((*pos)).next();
										((*decision)).skipped = ((((*decision)).skipped) + (1));
									}
								}
								auto here = python::ispa_std::recovery::EOF_TOKEN;
								if ((!(((*pos)).isEnd()))) {
									here = token_kind(((*pos)).current());
								}
								if (python::ispa_std::recovery::contains(((*plan)).expected, here)) {
									((*decision)).mode = python::ispa_std::recovery::Mode::Retry;
								} else {
									if (((((here) == (python::ispa_std::recovery::EOF_TOKEN))) || (((((*decision)).skipped) == (0))))) {
										((*decision)).mode = python::ispa_std::recovery::Mode::Propagate;
									} else {
										if (((((python::ispa_std::recovery::contains(((*plan)).continuation, here)) || (python::ispa_std::recovery::contains(((*plan)).follow, here)))) || (python::ispa_std::recovery::_anchor(plan, state, here)))) {
											((*decision)).mode = python::ispa_std::recovery::Mode::PanicUntil;
										} else {
											((*decision)).mode = python::ispa_std::recovery::Mode::Propagate;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	((*state)).total_skipped = ((((*state)).total_skipped) + (((*decision)).skipped));
	auto entry = ISPA_STD::construct<python::ispa_std::recovery::Diagnostic>();
	((*entry)).rule = ((*plan)).rule;
	((*entry)).unexpected = at;
	((*entry)).expected = ((*plan)).expected;
	((*entry)).decision = ((*decision)).copy();
	((*entry)).offset = offset;
	((*entry)).length = length;
	((*entry)).message = (((((("unexpected ") + (actual))) + (" in "))) + (((*plan)).rule));
	(((*state)).diagnostics).push_back(entry);
	if (((((static_cast<long long>(((*state)).diagnostics.size())) >= (((*state)).max_errors))) || (((((*state)).total_skipped) >= (((*state)).max_total_skipped))))) {
		((*decision)).mode = python::ispa_std::recovery::Mode::Abort;
	}
	((*entry)).decision = ((*decision)).copy();
	return decision;
}
auto python::ispa_std::recovery::_anchor(std::shared_ptr<python::ispa_std::recovery::Plan> plan, std::shared_ptr<python::ispa_std::recovery::State> state, long long id) -> bool {
	if (python::ispa_std::recovery::contains(((*plan)).explicit_sync, id)) {
		return true;
	}
	long long i = 0;
	while (((i) < (((*state)).anchor_count))) {
		if (python::ispa_std::recovery::contains(((*(((*state)).anchors)[i])).tokens, id)) {
			return true;
		}
		i = ((i) + (1));
	}
	return false;
}
auto python::ispa_std::recovery::recover_default(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::recovery::State> state, std::shared_ptr<python::ispa_std::recovery::Plan> plan) -> std::shared_ptr<python::ispa_std::recovery::Decision> {
	return python::ispa_std::recovery::recover(pos, state, plan, python::ispa_std::recovery::token_id);
}
auto python::ispa_std::recovery::recover_encoded(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::recovery::State> state, const std::string& rule, const std::string& expected, const std::string& continuation, const std::string& follow, const std::string& explicit_sync, bool allow_insertion, bool allow_deletion, bool allow_replacement, long long max_skip) -> std::shared_ptr<python::ispa_std::recovery::Decision> {
	return python::ispa_std::recovery::recover_default(pos, state, ISPA_STD::construct<python::ispa_std::recovery::Plan>(rule, python::ispa_std::recovery::decode(expected), python::ispa_std::recovery::decode(continuation), python::ispa_std::recovery::decode(follow), python::ispa_std::recovery::decode(explicit_sync), allow_insertion, allow_deletion, allow_replacement, max_skip));
}
auto python::ispa_std::recovery::TokenSet::__init__(const std::vector<long long>& values) -> void {
	((*this))._values = values;
}
auto python::ispa_std::recovery::TokenSet::values() -> const std::vector<long long>& {
	return ((*this))._values;
}
auto python::ispa_std::recovery::TokenSet::matches(std::shared_ptr<python::ispa_std::raw_token::RawToken> token) -> bool {
	return python::ispa_std::recovery::contains(((*this))._values, python::ispa_std::recovery::token_id(token));
}
auto python::ispa_std::recovery::empty_token_set() -> std::shared_ptr<python::ispa_std::recovery::TokenSet> {
	std::vector<long long> none = std::vector<long long>{};
	return ISPA_STD::construct<python::ispa_std::recovery::TokenSet>(none);
}
auto python::ispa_std::recovery::Policy::__init__(bool allow_insertion, bool allow_deletion, bool allow_replacement, long long max_skip) -> void {
	((*this)).allow_insertion = allow_insertion;
	((*this)).allow_deletion = allow_deletion;
	((*this)).allow_replacement = allow_replacement;
	if (((max_skip) <= (0))) {
		python::ispa_std::failures::invalid_argument("Recovery::Policy MaxSkip must be positive");
	}
	((*this)).max_skip = max_skip;
}
auto python::ispa_std::recovery::Policy::initial() -> std::shared_ptr<python::ispa_std::recovery::Policy> {
	return ISPA_STD::construct<python::ispa_std::recovery::Policy>(true, true, true, 64);
}
auto python::ispa_std::recovery::FailHandler::run(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::recovery::State> state) -> std::shared_ptr<python::ispa_std::recovery::Decision> {
	python::ispa_std::failures::logic_error("FailHandler.run is abstract");
	return python::ispa_std::recovery::Decision::initial();
}
auto python::ispa_std::recovery::FailHandler::is_custom() -> bool {
	return false;
}
auto python::ispa_std::recovery::FailHandler::is_recovering() -> bool {
	return true;
}
auto python::ispa_std::recovery::Handler::__init__(const std::string& rule, const std::string& expected_text, std::shared_ptr<python::ispa_std::recovery::TokenSet> expected, std::shared_ptr<python::ispa_std::recovery::TokenSet> continuation, std::shared_ptr<python::ispa_std::recovery::TokenSet> follow, std::shared_ptr<python::ispa_std::recovery::TokenSet> synchronization, std::shared_ptr<python::ispa_std::recovery::Policy> policy) -> void {
	((*this)).rule = rule;
	((*this)).expected_text = expected_text;
	((*this)).expected = expected;
	((*this)).continuation = continuation;
	((*this)).follow = follow;
	((*this)).synchronization = synchronization;
	((*this)).policy = policy;
}
auto python::ispa_std::recovery::Handler::run(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::recovery::State> state) -> std::shared_ptr<python::ispa_std::recovery::Decision> {
	python::ispa_std::recovery::Plan plan = ISPA_STD::make_value<python::ispa_std::recovery::Plan>(((*this)).rule, ((*((*this)).expected)).values(), ((*((*this)).continuation)).values(), ((*((*this)).follow)).values(), ((*((*this)).synchronization)).values(), ((*((*this)).policy)).allow_insertion, ((*((*this)).policy)).allow_deletion, ((*((*this)).policy)).allow_replacement, ((*((*this)).policy)).max_skip);
	auto decision = python::ispa_std::recovery::recover_default(pos, state, ISPA_STD::borrow(plan));
	if (((static_cast<long long>(((*state)).diagnostics.size())) > (0))) {
		auto diagnostic = (((*state)).diagnostics)[((static_cast<long long>(((*state)).diagnostics.size())) - (1))];
		((*diagnostic)).expected_text = ((*this)).expected_text;
		((*diagnostic)).message = ((((((*diagnostic)).message) + ("; expected "))) + (((*this)).expected_text));
		if (((((*decision)).mode) == (python::ispa_std::recovery::Mode::Insert))) {
			((*diagnostic)).message = ((((*diagnostic)).message) + ("; inserted missing token"));
		} else {
			if (((((*decision)).mode) == (python::ispa_std::recovery::Mode::Replace))) {
				((*diagnostic)).message = ((((*diagnostic)).message) + ("; replaced unexpected token"));
			} else {
				if (((((*decision)).mode) == (python::ispa_std::recovery::Mode::Delete))) {
					((*diagnostic)).message = ((((*diagnostic)).message) + ("; deleted unexpected token"));
				} else {
					if (((((*decision)).mode) == (python::ispa_std::recovery::Mode::Abort))) {
						((*diagnostic)).message = ((((*diagnostic)).message) + ("; recovery limit reached"));
					}
				}
			}
		}
		if (((((*decision)).skipped) > (1))) {
			((*diagnostic)).message = ((((((((*diagnostic)).message) + ("; skipped "))) + (python::ispa_std::format::int_to_string(((*decision)).skipped)))) + (" tokens"));
		}
	}
	return decision;
}
auto python::ispa_std::recovery::ActionFailure::__init__(std::function<std::shared_ptr<python::ispa_std::recovery::Decision>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>, std::shared_ptr<python::ispa_std::recovery::State>)> action) -> void {
	((*this)).action = action;
}
auto python::ispa_std::recovery::ActionFailure::run(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::recovery::State> state) -> std::shared_ptr<python::ispa_std::recovery::Decision> {
	return ((*this)).action(pos, state);
}
auto python::ispa_std::recovery::ActionFailure::is_custom() -> bool {
	return true;
}
auto python::ispa_std::recovery::LegacyFail::__init__(std::function<void(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> action) -> void {
	((*this)).action = action;
}
auto python::ispa_std::recovery::LegacyFail::run(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::recovery::State> state) -> std::shared_ptr<python::ispa_std::recovery::Decision> {
	((*this)).action(pos);
	return python::ispa_std::recovery::Decision::initial();
}
auto python::ispa_std::recovery::LegacyFail::is_recovering() -> bool {
	return false;
}
long long python::ispa_std::recovery::RequiredStatus::Matched = 0;
long long python::ispa_std::recovery::RequiredStatus::Missing = 1;
long long python::ispa_std::recovery::RequiredStatus::Boundary = 2;
long long python::ispa_std::recovery::RequiredStatus::Aborted = 3;
auto python::ispa_std::recovery::RequiredResult::__init__(long long status, bool has_node, std::shared_ptr<python::ispa_std::node::Node> node, std::shared_ptr<python::ispa_std::recovery::Decision> recovery) -> void {
	((*this)).status = status;
	((*this)).has_node = has_node;
	((*this)).node = node;
	((*this)).recovery = recovery;
}
auto python::ispa_std::recovery::RequiredResult::boundary_only() -> std::shared_ptr<python::ispa_std::recovery::RequiredResult> {
	"The C++ default-constructed result (status Boundary, no node).";
	return ISPA_STD::construct<python::ispa_std::recovery::RequiredResult>(python::ispa_std::recovery::RequiredStatus::Boundary, false, python::ispa_std::node::Node::blank(), python::ispa_std::recovery::Decision::initial());
}
long long python::ispa_std::recovery::ContextKind::Mismatch = 0;
long long python::ispa_std::recovery::ContextKind::Misplaced = 1;
long long python::ispa_std::recovery::ContextKind::Stray = 2;
auto python::ispa_std::recovery::ContextClaim::__init__(const std::string& name, std::shared_ptr<python::ispa_std::recovery::TokenSet> tokens) -> void {
	((*this)).name = name;
	((*this)).tokens = tokens;
}
auto python::ispa_std::recovery::ClaimSet::__init__(const std::vector<std::shared_ptr<python::ispa_std::recovery::ContextClaim>>& claims) -> void {
	((*this)).claims = claims;
	((*this)).tokens = {};
	long long i = 0;
	while (((i) < (static_cast<long long>(claims.size())))) {
		(((*this)).tokens).insert((((*this)).tokens).end(), ((*((*(claims)[i])).tokens)).values().begin(), ((*((*(claims)[i])).tokens)).values().end());
		i = ((i) + (1));
	}
}
auto python::ispa_std::recovery::empty_claim_set() -> std::shared_ptr<python::ispa_std::recovery::ClaimSet> {
	if (((static_cast<long long>(python::ispa_std::recovery::_NO_CLAIMS.size())) == (0))) {
		std::vector<std::shared_ptr<python::ispa_std::recovery::ContextClaim>> none = std::vector<std::shared_ptr<python::ispa_std::recovery::ContextClaim>>{};
		(python::ispa_std::recovery::_NO_CLAIMS).push_back(ISPA_STD::construct<python::ispa_std::recovery::ClaimSet>(none));
	}
	return (python::ispa_std::recovery::_NO_CLAIMS)[0];
}
auto python::ispa_std::recovery::ContextDiagnostic::__init__() -> void {
	((*this)).kind = python::ispa_std::recovery::ContextKind::Mismatch;
	((*this)).message = "";
	((*this)).position = 0;
	((*this)).offset = python::ispa_std::recovery::NO_OFFSET;
	((*this)).length = 0;
	((*this)).skipped = 0;
	((*this)).rule = "";
	((*this)).resume_rule = "";
	((*this)).claim = "";
}
auto python::ispa_std::recovery::describe_ids(const std::vector<long long>& ids) -> std::string {
	std::vector<std::string> parts = std::vector<std::string>{};
	long long i = 0;
	while (((i) < (static_cast<long long>(ids.size())))) {
		(parts).push_back(python::ispa_std::format::int_to_string((ids)[i]));
		i = ((i) + (1));
	}
	return python::ispa_std::format::join(parts, ", ");
}
auto python::ispa_std::counter::CountSpec::__init__(const std::string& name, bool has_count, long long count, long long minimum) -> void {
	((*this)).name = name;
	((*this)).has_count = has_count;
	((*this)).count = count;
	((*this)).minimum = minimum;
}
auto python::ispa_std::counter::CountSpec::named(const std::string& name, long long minimum) -> std::shared_ptr<python::ispa_std::counter::CountSpec> {
	return ISPA_STD::construct<python::ispa_std::counter::CountSpec>(name, false, 0, minimum);
}
auto python::ispa_std::counter::CountSpec::fixed(long long value, long long minimum) -> std::shared_ptr<python::ispa_std::counter::CountSpec> {
	if (((value) < (0))) {
		python::ispa_std::failures::invalid_argument("a repetition count cannot be negative");
	}
	long long effective = minimum;
	if (((value) >= (minimum))) {
		effective = value;
	}
	return ISPA_STD::construct<python::ispa_std::counter::CountSpec>("", true, value, effective);
}
auto python::ispa_std::match_result::MatchResult::__init__(bool status, std::shared_ptr<python::ispa_std::node::Node> node, std::shared_ptr<python::ispa_std::lexer::TokenCursor> it) -> void {
	((*this)).status = status;
	((*this)).node = node;
	((*this)).it = it;
}
auto python::ispa_std::match_result::MatchResult::failed(std::shared_ptr<python::ispa_std::lexer::TokenCursor> it) -> std::shared_ptr<python::ispa_std::match_result::MatchResult> {
	return ISPA_STD::construct<python::ispa_std::match_result::MatchResult>(false, python::ispa_std::node::Node::blank(), it);
}
auto python::ispa_std::parser_base::Parser_base::__init__() -> void {
	((*this)).lexer = ISPA_STD::construct<python::ispa_std::lexer::Lexer_base>();
	((*this)).has_lexer = false;
	((*this)).text = "";
	((*this)).has_text = false;
	((*this)).tree = python::ispa_std::node::Node::blank();
}
auto python::ispa_std::parser_base::Parser_base::with_lexer(std::shared_ptr<python::ispa_std::parser_base::Parser_base> parser, std::shared_ptr<python::ispa_std::lexer::Lexer_base> lex) -> std::shared_ptr<python::ispa_std::parser_base::Parser_base> {
	((*parser)).lexer = lex;
	((*parser)).has_lexer = true;
	return parser;
}
auto python::ispa_std::parser_base::Parser_base::parse_lexer(std::shared_ptr<python::ispa_std::lexer::Lexer_base> lex) -> std::any {
	((*this)).lexer = lex;
	((*this)).has_lexer = true;
	return ((*this)).parse_tokens();
}
auto python::ispa_std::parser_base::Parser_base::parse_text(const std::string& text) -> std::any {
	((*this)).text = text;
	((*this)).has_text = true;
	return ((*this)).parseLazy();
}
auto python::ispa_std::parser_base::Parser_base::set_input_lexer(std::shared_ptr<python::ispa_std::lexer::Lexer_base> lex) -> void {
	if ((!(((*lex)).hasTokens()))) {
		((*this)).lexer = lex;
		((*this)).has_lexer = true;
	}
}
auto python::ispa_std::parser_base::Parser_base::set_input_text(const std::string& text) -> void {
	((*this)).text = text;
	((*this)).has_text = true;
}
auto python::ispa_std::parser_base::Parser_base::clear_input() -> void {
	((*this)).has_lexer = false;
	((*this)).has_text = false;
	((*this)).text = "";
}
auto python::ispa_std::parser_base::Parser_base::parse_tokens() -> std::any {
	((*this)).before_parse();
	if ((!(((*this)).has_lexer))) {
		python::ispa_std::failures::api_error("Can\'t parse because no input lexer is provided", {});
	}
	if ((!(((*((*this)).lexer)).hasInput()))) {
		((*((*this)).lexer)).makeTokens();
	}
	((*this)).parseFromTokens();
	return ((*this)).tree;
}
auto python::ispa_std::parser_base::Parser_base::before_parse() -> void {
	return ;
}
auto python::ispa_std::parser_base::Parser_base::parseLazy() -> std::any {
	if ((!(((*this)).has_text))) {
		python::ispa_std::failures::api_error("Can\'t parse because no input text is provided", {});
	}
	auto result = ((*this)).lazyParse();
	if (((*result)).status) {
		((*this)).tree = ((*((*result)).node)).data();
	}
	return ((*this)).tree;
}
auto python::ispa_std::parser_base::Parser_base::parseFromTokens() -> void {
	python::ispa_std::failures::logic_error("Parser_base.parseFromTokens is abstract");
}
auto python::ispa_std::parser_base::Parser_base::lazyParse() -> std::shared_ptr<python::ispa_std::match_result::MatchResult> {
	python::ispa_std::failures::logic_error("Parser_base.lazyParse is abstract");
	return python::ispa_std::match_result::MatchResult::failed(ISPA_STD::construct<python::ispa_std::lexer::TokenIterator>(((*this)).lexer, 0));
}
auto python::ispa_std::scan::ScanResult::__init__(long long end, bool exhausted) -> void {
	((*this)).end = end;
	((*this)).exhausted = exhausted;
}
auto python::ispa_std::scan::_boundary_at(const std::string& src, long long p, const std::string& boundary) -> bool {
	long long i = 0;
	while (((i) < (static_cast<long long>(boundary.size())))) {
		std::string c = python::ispa_std::chars::at(src, ((p) + (i)));
		if (((((static_cast<long long>(c.size())) == (0))) || (((c) != (std::string(1, (boundary)[i])))))) {
			return false;
		}
		i = ((i) + (1));
	}
	return true;
}
auto python::ispa_std::scan::scan_balanced(const std::string& src, long long begin, const std::vector<std::string>& boundaries, long long budget) -> std::shared_ptr<python::ispa_std::scan::ScanResult> {
	"Scan forward from `begin`. The budget is in source units, including\n    strings and comments. Stops at the first boundary outside brackets (a \";\"\n    boundary also stops inside brackets), or at an unmatched closer.";
	long long p = begin;
	long long n = static_cast<long long>(src.size());
	std::vector<std::string> closers = std::vector<std::string>{};
	std::string quote = "";
	bool triple = false;
	bool comment = false;
	while (((p) < (n))) {
		if (((((p) - (begin))) >= (budget))) {
			return ISPA_STD::construct<python::ispa_std::scan::ScanResult>(p, true);
		}
		std::string c = std::string(1, (src)[p]);
		bool skip_tail = false;
		if (comment) {
			if (((((c) == ("\n"))) || (((c) == ("\r"))))) {
				comment = false;
			}
		} else {
			if (((static_cast<long long>(quote.size())) > (0))) {
				if (((((c) == ("\\"))) && (((static_cast<long long>(python::ispa_std::chars::at(src, ((p) + (1))).size())) > (0))))) {
					if (((2) > (((budget) - (((p) - (begin))))))) {
						return ISPA_STD::construct<python::ispa_std::scan::ScanResult>(p, true);
					}
					p = ((p) + (2));
					skip_tail = true;
				} else {
					if (((((c) == (quote))) && ((((!(triple))) || (((((python::ispa_std::chars::at(src, ((p) + (1)))) == (quote))) && (((python::ispa_std::chars::at(src, ((p) + (2)))) == (quote))))))))) {
						long long width = 1;
						if (triple) {
							width = 3;
						}
						if (((width) > (((budget) - (((p) - (begin))))))) {
							return ISPA_STD::construct<python::ispa_std::scan::ScanResult>(p, true);
						}
						p = ((p) + (width));
						quote = "";
						skip_tail = true;
					} else {
						if ((((!(triple))) && (((((c) == ("\n"))) || (((c) == ("\r"))))))) {
							return ISPA_STD::construct<python::ispa_std::scan::ScanResult>(p, true);
						}
					}
				}
			} else {
				long long i = 0;
				while (((i) < (static_cast<long long>(boundaries.size())))) {
					std::string boundary = (boundaries)[i];
					if (((((((static_cast<long long>(closers.size())) == (0))) || (((boundary) == (";"))))) && (python::ispa_std::scan::_boundary_at(src, p, boundary)))) {
						return ISPA_STD::construct<python::ispa_std::scan::ScanResult>(p, false);
					}
					i = ((i) + (1));
				}
				if (((((((c) == (")"))) || (((c) == ("]"))))) || (((c) == ("}"))))) {
					if (((((static_cast<long long>(closers.size())) == (0))) || ((((closers)[((static_cast<long long>(closers.size())) - (1))]) != (c))))) {
						return ISPA_STD::construct<python::ispa_std::scan::ScanResult>(p, false);
					}
					ISPA_STD::python_pop((closers));
				} else {
					if (((c) == ("("))) {
						(closers).push_back(")");
					} else {
						if (((c) == ("["))) {
							(closers).push_back("]");
						} else {
							if (((c) == ("{"))) {
								(closers).push_back("}");
							} else {
								if (((c) == ("#"))) {
									comment = true;
								} else {
									if (((((c) == ("\'"))) || (((c) == ("\""))))) {
										quote = c;
										triple = ((((python::ispa_std::chars::at(src, ((p) + (1)))) == (c))) && (((python::ispa_std::chars::at(src, ((p) + (2)))) == (c))));
										long long width = 1;
										if (triple) {
											width = 3;
										}
										if (((width) > (((budget) - (((p) - (begin))))))) {
											return ISPA_STD::construct<python::ispa_std::scan::ScanResult>(p, true);
										}
										p = ((p) + (width));
										skip_tail = true;
									}
								}
							}
						}
					}
				}
			}
		}
		if ((!(skip_tail))) {
			if (((1) > (((budget) - (((p) - (begin))))))) {
				return ISPA_STD::construct<python::ispa_std::scan::ScanResult>(p, true);
			}
			p = ((p) + (1));
		}
	}
	return ISPA_STD::construct<python::ispa_std::scan::ScanResult>(p, ((static_cast<long long>(quote.size())) > (0)));
}
auto python::ispa_std::scan::text_between(const std::string& src, long long begin, long long end) -> std::string {
	return python::ispa_std::string::substr(src, begin, ((end) - (begin)));
}
auto python::ispa_std::sortedmap::StrIntMap::__init__() -> void {
	((*this)).keys = {};
	((*this)).values = {};
}
auto python::ispa_std::sortedmap::StrIntMap::find(const std::string& key) -> long long {
	long long pos = python::ispa_std::algorithm::lower_bound(((*this)).keys, 0, static_cast<long long>(((*this)).keys.size()), key);
	if (((((pos) < (static_cast<long long>(((*this)).keys.size())))) && ((((((*this)).keys)[pos]) == (key))))) {
		return pos;
	}
	return ( - (1));
}
auto python::ispa_std::sortedmap::StrIntMap::contains(const std::string& key) -> bool {
	return ((((*this)).find(key)) != (( - (1))));
}
auto python::ispa_std::sortedmap::StrIntMap::get(const std::string& key) -> long long {
	return (((*this)).values)[((*this)).find(key)];
}
auto python::ispa_std::sortedmap::StrIntMap::set(const std::string& key, long long value) -> void {
	long long pos = python::ispa_std::algorithm::lower_bound(((*this)).keys, 0, static_cast<long long>(((*this)).keys.size()), key);
	if (((((pos) < (static_cast<long long>(((*this)).keys.size())))) && ((((((*this)).keys)[pos]) == (key))))) {
		(((*this)).values)[pos] = value;
	} else {
		ISPA_STD::python_insert((((*this)).keys), pos, key);
		ISPA_STD::python_insert((((*this)).values), pos, value);
	}
}
auto python::ispa_std::sortedmap::StrIntMap::try_emplace(const std::string& key, long long value) -> bool {
	"Insert only when absent; True when inserted.";
	if (((((*this)).find(key)) != (( - (1))))) {
		return false;
	}
	((*this)).set(key, value);
	return true;
}
auto python::ispa_std::sortedmap::StrIntMap::clear() -> void {
	(((*this)).keys).clear();
	(((*this)).values).clear();
}
auto python::ispa_std::sortedmap::StrIntMap::copy() -> std::shared_ptr<python::ispa_std::sortedmap::StrIntMap> {
	auto out = ISPA_STD::construct<python::ispa_std::sortedmap::StrIntMap>();
	(((*out)).keys).insert((((*out)).keys).end(), ((*this)).keys.begin(), ((*this)).keys.end());
	(((*out)).values).insert((((*out)).values).end(), ((*this)).values.begin(), ((*this)).values.end());
	return out;
}
auto python::ispa_std::sortedmap::StrIntMap::assign(std::shared_ptr<python::ispa_std::sortedmap::StrIntMap> other) -> void {
	(((*this)).keys).clear();
	(((*this)).values).clear();
	(((*this)).keys).insert((((*this)).keys).end(), ((*other)).keys.begin(), ((*other)).keys.end());
	(((*this)).values).insert((((*this)).values).end(), ((*other)).values.begin(), ((*other)).values.end());
}
auto python::ispa_std::sortedmap::IntSet::__init__() -> void {
	((*this)).items = {};
}
auto python::ispa_std::sortedmap::IntSet::add(long long value) -> bool {
	"True when the value was not present (std::set::insert().second).";
	long long pos = python::ispa_std::algorithm::lower_bound(((*this)).items, 0, static_cast<long long>(((*this)).items.size()), value);
	if (((((pos) < (static_cast<long long>(((*this)).items.size())))) && ((((((*this)).items)[pos]) == (value))))) {
		return false;
	}
	ISPA_STD::python_insert((((*this)).items), pos, value);
	return true;
}
auto python::ispa_std::sortedmap::IntSet::contains(long long value) -> bool {
	long long pos = python::ispa_std::algorithm::lower_bound(((*this)).items, 0, static_cast<long long>(((*this)).items.size()), value);
	return ((((pos) < (static_cast<long long>(((*this)).items.size())))) && ((((((*this)).items)[pos]) == (value))));
}
auto python::ispa_std::sortedmap::IntSet::size() -> long long {
	return static_cast<long long>(((*this)).items.size());
}
auto python::ispa_std::sortedmap::IntSet::empty() -> bool {
	return ((static_cast<long long>(((*this)).items.size())) == (0));
}
auto python::ispa_std::sortedmap::IntSet::smallest() -> long long {
	return (((*this)).items)[0];
}
auto python::ispa_std::sortedmap::IntSet::copy() -> std::shared_ptr<python::ispa_std::sortedmap::IntSet> {
	auto out = ISPA_STD::construct<python::ispa_std::sortedmap::IntSet>();
	(((*out)).items).insert((((*out)).items).end(), ((*this)).items.begin(), ((*this)).items.end());
	return out;
}
auto python::ispa_std::parser_ll::lookahead_kind(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> long long {
	"Recovery::TokenId of the token at pos (EOF_TOKEN at end of input).";
	if (((*pos)).isEnd()) {
		return python::ispa_std::recovery::END_OF_INPUT;
	}
	return ((*((*pos)).current())).kind();
}
auto python::ispa_std::parser_ll::cursor_position(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> long long {
	return ((*pos)).distance();
}
auto python::ispa_std::parser_ll::parse_token_stream(std::shared_ptr<python::ispa_std::lexer::Lexer_base> lexer, const std::vector<std::shared_ptr<python::ispa_std::raw_token::RawToken>>& tokens, std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rule) -> std::shared_ptr<python::ispa_std::node::Node> {
	"Run a generated rule against a separate token stream. The returned node\n    owns its data; the temporary cursor never escapes with the result.";
	long long i = 0;
	while (((i) < (static_cast<long long>(tokens.size())))) {
		((*lexer)).append_token((tokens)[i]);
		i = ((i) + (1));
	}
	((*lexer)).push_eof();
	auto begin = python::ispa_std::lexer::TokenIterator::begin(lexer);
	auto result = rule(begin);
	if ((((!(((*result)).status))) || ((!(((*((*result)).it)).isEnd()))))) {
		return python::ispa_std::node::Node::blank();
	}
	return ((*result)).node;
}
auto python::ispa_std::parser_ll::parse_source_span(std::shared_ptr<python::ispa_std::lexer::Lexer_base> lexer, const std::vector<std::shared_ptr<python::ispa_std::raw_token::RawToken>>& tokens, std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rule) -> std::shared_ptr<python::ispa_std::node::Node> {
	"External rules need the original source bytes. Tokens collected from a\n    contiguous source region provide its bounds while retaining the source\n    positions in the returned AST.";
	if (((static_cast<long long>(tokens.size())) == (0))) {
		return python::ispa_std::node::Node::blank();
	}
	auto first = ((*(tokens)[0])).meta();
	auto last = ((*(tokens)[((static_cast<long long>(tokens.size())) - (1))])).meta();
	if ((((((!(((*first)).has_source()))) || ((!(((*last)).has_source()))))) || (((((*last)).end_pos()) < (((*first)).startpos()))))) {
		python::ispa_std::failures::invalid_argument("source stream requires tokens with source spans");
	}
	((*lexer)).setInput(((*first)).source());
	((*lexer)).set_bound(((*last)).end_pos());
	auto begin = python::ispa_std::lexer::LazyIterator::at_position(lexer, ((*first)).startpos());
	auto result = rule(begin);
	if ((((((!(((*result)).status))) || ((!(((*((*result)).it)).isEnd()))))) || (((((*((*result)).it)).sourcePosition()) > (((*last)).end_pos()))))) {
		return python::ispa_std::node::Node::blank();
	}
	return ((*result)).node;
}
auto python::ispa_std::parser_ll::ActionToken::__init__() -> void {
	((*this)).raw = python::ispa_std::raw_token::RawToken::empty_token();
	((*this)).text = "";
	((*this)).name = "";
	((*this)).kind = python::ispa_std::parser_ll::NPOS;
	((*this)).offset = 0;
	((*this)).length = 0;
	((*this)).line = 0;
	((*this)).column = 0;
	((*this)).present = false;
}
auto python::ispa_std::parser_ll::ActionToken::empty() -> bool {
	return (!(((*this)).present));
}
auto python::ispa_std::parser_ll::ActionToken::matches_name(const std::string& expected) -> bool {
	if ((!(((*this)).present))) {
		return false;
	}
	std::string normalized = python::ispa_std::format::replace_all(expected, "::", ".");
	if (((((*this)).name) == (normalized))) {
		return true;
	}
	return ((((((static_cast<long long>(((*this)).name.size())) > (static_cast<long long>(normalized.size())))) && (python::ispa_std::string::ends_with(((*this)).name, normalized)))) && (((std::string(1, (((*this)).name)[((((static_cast<long long>(((*this)).name.size())) - (static_cast<long long>(normalized.size())))) - (1))])) == ("."))));
}
auto python::ispa_std::parser_ll::TokenBoundary::__init__(const std::string& value, bool token_name) -> void {
	((*this)).value = value;
	((*this)).token_name = token_name;
}
auto python::ispa_std::parser_ll::spelling(const std::string& value) -> std::shared_ptr<python::ispa_std::parser_ll::TokenBoundary> {
	return ISPA_STD::construct<python::ispa_std::parser_ll::TokenBoundary>(value, false);
}
auto python::ispa_std::parser_ll::token_boundary(const std::string& name) -> std::shared_ptr<python::ispa_std::parser_ll::TokenBoundary> {
	return ISPA_STD::construct<python::ispa_std::parser_ll::TokenBoundary>(name, true);
}
auto python::ispa_std::parser_ll::TokenNames::__init__(const std::vector<std::string>& names) -> void {
	((*this)).names = names;
}
auto python::ispa_std::parser_ll::TokenNames::lookup(long long id) -> std::string {
	if (((((id) < (0))) || (((id) >= (static_cast<long long>(((*this)).names.size())))))) {
		return "";
	}
	return (((*this)).names)[id];
}
auto python::ispa_std::parser_ll::context_peek(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long lookahead, std::shared_ptr<python::ispa_std::parser_ll::TokenNames> token_names, long long ambiguous_id) -> std::shared_ptr<python::ispa_std::parser_ll::ActionToken> {
	if (((lookahead) > (python::ispa_std::parser_ll::MAX_ACTION_LOOKAHEAD))) {
		python::ispa_std::failures::out_of_range("ctx.peek lookahead exceeds 64 tokens");
	}
	auto candidate = ((*pos)).copy();
	long long i = 0;
	while (((i) < (lookahead))) {
		if (((*candidate)).isEnd()) {
			return ISPA_STD::construct<python::ispa_std::parser_ll::ActionToken>();
		}
		((*candidate)).next();
		i = ((i) + (1));
	}
	if (((*candidate)).isEnd()) {
		return ISPA_STD::construct<python::ispa_std::parser_ll::ActionToken>();
	}
	auto token = ((*candidate)).current();
	auto meta = ((*token)).meta();
	auto result = ISPA_STD::construct<python::ispa_std::parser_ll::ActionToken>();
	((*result)).raw = token;
	((*result)).kind = python::ispa_std::recovery::token_id(token);
	((*result)).offset = ((*meta)).startpos();
	((*result)).length = ((*meta)).length();
	((*result)).line = ((*meta)).line();
	((*result)).column = ((*meta)).column();
	((*result)).present = true;
	if (((*meta)).has_source()) {
		((*result)).text = ((*meta)).text();
	}
	if (((((*result)).kind) != (ambiguous_id))) {
		((*result)).name = ((*token_names)).lookup(((*result)).kind);
	}
	return result;
}
auto python::ispa_std::parser_ll::context_prev(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::lexer::TokenCursor> start_pos, std::shared_ptr<python::ispa_std::parser_ll::TokenNames> token_names, long long ambiguous_id) -> std::shared_ptr<python::ispa_std::parser_ll::ActionToken> {
	"Step pos back one token (never before start_pos) and return it.";
	if (((python::ispa_std::parser_ll::cursor_position(pos)) <= (python::ispa_std::parser_ll::cursor_position(start_pos)))) {
		return ISPA_STD::construct<python::ispa_std::parser_ll::ActionToken>();
	}
	auto previous = ((*pos)).copy();
	((*previous)).prev();
	if (((((python::ispa_std::parser_ll::cursor_position(previous)) >= (python::ispa_std::parser_ll::cursor_position(pos)))) || (((python::ispa_std::parser_ll::cursor_position(previous)) < (python::ispa_std::parser_ll::cursor_position(start_pos)))))) {
		return ISPA_STD::construct<python::ispa_std::parser_ll::ActionToken>();
	}
	((*pos)).assign(previous);
	return python::ispa_std::parser_ll::context_peek(pos, 0, token_names, ambiguous_id);
}
auto python::ispa_std::parser_ll::context_at(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, const std::string& text, long long lookahead, std::shared_ptr<python::ispa_std::parser_ll::TokenNames> token_names, bool match_name, long long ambiguous_id) -> bool {
	if (((lookahead) > (python::ispa_std::parser_ll::MAX_ACTION_LOOKAHEAD))) {
		python::ispa_std::failures::out_of_range("ctx.at lookahead exceeds 64 tokens");
	}
	auto candidate = ((*pos)).copy();
	long long i = 0;
	while (((i) < (lookahead))) {
		if (((*candidate)).isEnd()) {
			return false;
		}
		((*candidate)).next();
		i = ((i) + (1));
	}
	if (((*candidate)).isEnd()) {
		return false;
	}
	auto meta = ((*((*candidate)).current())).meta();
	if ((!(match_name))) {
		return ((((*meta)).has_source()) && (((((*meta)).text()) == (text))));
	}
	if (((python::ispa_std::recovery::token_id(((*candidate)).current())) == (ambiguous_id))) {
		return false;
	}
	std::string name = python::ispa_std::format::replace_all(text, "::", ".");
	std::string actual = ((*token_names)).lookup(python::ispa_std::recovery::token_id(((*candidate)).current()));
	if (((actual) == (name))) {
		return true;
	}
	return ((((((static_cast<long long>(actual.size())) > (static_cast<long long>(name.size())))) && (python::ispa_std::string::ends_with(actual, name)))) && (((std::string(1, (actual)[((((static_cast<long long>(actual.size())) - (static_cast<long long>(name.size())))) - (1))])) == ("."))));
}
auto python::ispa_std::parser_ll::ActionCheckpoint::__init__(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> void {
	((*this)).pos = pos;
	((*this)).names = {};
	((*this)).saved = {};
}
auto python::ispa_std::parser_ll::ActionCheckpoint::mark(const std::string& name) -> void {
	if (((static_cast<long long>(name.size())) == (0))) {
		python::ispa_std::failures::invalid_argument("checkpoint name cannot be empty");
	}
	if (((static_cast<long long>(((*this)).names.size())) >= (python::ispa_std::parser_ll::MAX_ACTION_CHECKPOINTS))) {
		python::ispa_std::failures::length_error("action checkpoint limit (32) exceeded");
	}
	(((*this)).names).push_back(name);
	(((*this)).saved).push_back(((*((*this)).pos)).copy());
}
auto python::ispa_std::parser_ll::ActionCheckpoint::_find_last(const std::string& name) -> long long {
	long long i = ((static_cast<long long>(((*this)).names.size())) - (1));
	while (((i) >= (0))) {
		if ((((((*this)).names)[i]) == (name))) {
			return i;
		}
		i = ((i) - (1));
	}
	return ( - (1));
}
auto python::ispa_std::parser_ll::ActionCheckpoint::restore(const std::string& name) -> void {
	long long index = ((*this))._find_last(name);
	if (((index) == (( - (1))))) {
		python::ispa_std::failures::logic_error((((("unknown action checkpoint \'") + (name))) + ("\'")));
	}
	((*((*this)).pos)).assign((((*this)).saved)[index]);
	while (((static_cast<long long>(((*this)).names.size())) > (((index) + (1))))) {
		ISPA_STD::python_pop((((*this)).names));
		ISPA_STD::python_pop((((*this)).saved));
	}
}
auto python::ispa_std::parser_ll::ActionCheckpoint::commit(const std::string& name) -> void {
	long long index = ((*this))._find_last(name);
	if (((index) == (( - (1))))) {
		python::ispa_std::failures::logic_error((((("unknown action checkpoint \'") + (name))) + ("\'")));
	}
	while (((static_cast<long long>(((*this)).names.size())) > (index))) {
		ISPA_STD::python_pop((((*this)).names));
		ISPA_STD::python_pop((((*this)).saved));
	}
}
auto python::ispa_std::parser_ll::Ctx::__init__(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::lexer::TokenCursor> start_pos, const std::string& rule, std::shared_ptr<python::ispa_std::parser_ll::TokenNames> token_names, long long ambiguous_id) -> void {
	python::ispa_std::parser_ll::ActionCheckpoint::__init__(pos);
	((*this)).start_pos = start_pos;
	((*this)).rule = rule;
	((*this)).token_names = token_names;
	((*this)).ambiguous_id = ambiguous_id;
}
auto python::ispa_std::parser_ll::Ctx::at(const std::string& text, long long lookahead) -> bool {
	return python::ispa_std::parser_ll::context_at(((*this)).pos, text, lookahead, ((*this)).token_names, false, ((*this)).ambiguous_id);
}
auto python::ispa_std::parser_ll::Ctx::at_token(const std::string& name, long long lookahead) -> bool {
	return python::ispa_std::parser_ll::context_at(((*this)).pos, name, lookahead, ((*this)).token_names, true, ((*this)).ambiguous_id);
}
auto python::ispa_std::parser_ll::Ctx::peek(long long lookahead) -> std::shared_ptr<python::ispa_std::parser_ll::ActionToken> {
	return python::ispa_std::parser_ll::context_peek(((*this)).pos, lookahead, ((*this)).token_names, ((*this)).ambiguous_id);
}
auto python::ispa_std::parser_ll::Ctx::take() -> std::shared_ptr<python::ispa_std::parser_ll::ActionToken> {
	auto token = ((*this)).peek(0);
	if ((!(((*token)).empty()))) {
		((*((*this)).pos)).next();
	}
	return token;
}
auto python::ispa_std::parser_ll::Ctx::prev() -> std::shared_ptr<python::ispa_std::parser_ll::ActionToken> {
	return python::ispa_std::parser_ll::context_prev(((*this)).pos, ((*this)).start_pos, ((*this)).token_names, ((*this)).ambiguous_id);
}
auto python::ispa_std::parser_ll::Frame::__init__(const std::string& rule) -> void {
	((*this)).rule = rule;
	((*this)).expecting = "";
	((*this)).claim_set = python::ispa_std::recovery::empty_claim_set();
}
auto python::ispa_std::parser_ll::FrameGuard::__init__(std::shared_ptr<python::ispa_std::parser_ll::LLParser> parser, const std::string& rule) -> void {
	((*this)).parser = parser;
	((*this)).open = true;
	((*parser)).enter_frame(rule);
}
auto python::ispa_std::parser_ll::FrameGuard::close() -> void {
	if ((!(((*this)).open))) {
		return ;
	}
	((*this)).open = false;
	((*((*this)).parser)).leave_frame();
}
auto python::ispa_std::parser_ll::CounterScope::__init__(std::shared_ptr<python::ispa_std::parser_ll::LLParser> parser) -> void {
	((*this)).parser = parser;
	((*this)).saved = ((*((*parser)).counters_)).copy();
	((*this)).open = true;
}
auto python::ispa_std::parser_ll::CounterScope::restore() -> void {
	if ((!(((*this)).open))) {
		return ;
	}
	((*this)).open = false;
	((*((*((*this)).parser)).counters_)).assign(((*this)).saved);
}
auto python::ispa_std::parser_ll::RuleProbe::__init__(std::shared_ptr<python::ispa_std::parser_ll::LLParser> parser, long long start) -> void {
	((*this)).parser = parser;
	((*this)).start = start;
	((*this)).committed = false;
	((*this)).open = true;
	(((*parser)).probes_).push_back(::ISPA_STD::self_ptr(this));
}
auto python::ispa_std::parser_ll::RuleProbe::close() -> void {
	if ((!(((*this)).open))) {
		return ;
	}
	((*this)).open = false;
	ISPA_STD::python_pop((((*((*this)).parser)).probes_));
}
auto python::ispa_std::parser_ll::Resume::__init__(long long skip, long long frame, std::shared_ptr<python::ispa_std::recovery::ContextClaim> claim, bool has_claim) -> void {
	((*this)).skip = skip;
	((*this)).frame = frame;
	((*this)).claim = claim;
	((*this)).has_claim = has_claim;
}
auto python::ispa_std::parser_ll::LLParser::__init__() -> void {
	python::ispa_std::parser_base::Parser_base::__init__();
	((*this)).recovery_state_ = ISPA_STD::construct<python::ispa_std::recovery::State>();
	((*this)).counters_ = ISPA_STD::construct<python::ispa_std::sortedmap::StrIntMap>();
	((*this)).frames_ = {};
	((*this)).frame_count = 0;
	((*this)).unwind_ = 0;
	((*this)).last_resume_pos_ = python::ispa_std::parser_ll::NPOS;
	((*this)).recovering_ = false;
	((*this)).context_max_skip_ = 64;
	((*this)).context_errors_ = {};
	((*this)).probes_ = {};
	((*this)).token_names_ = python::ispa_std::parser_ll::NO_TOKEN_NAMES;
	((*this)).ambiguous_id_ = python::ispa_std::parser_ll::NPOS;
	((*this)).space_token_ = python::ispa_std::parser_ll::NPOS;
	((*this)).has_space_token_ = false;
}
auto python::ispa_std::parser_ll::LLParser::set_token_names(std::shared_ptr<python::ispa_std::parser_ll::TokenNames> names, long long ambiguous_id) -> void {
	((*this)).token_names_ = names;
	((*this)).ambiguous_id_ = ambiguous_id;
}
auto python::ispa_std::parser_ll::LLParser::set_space_token(long long type_id) -> void {
	((*this)).space_token_ = type_id;
	((*this)).has_space_token_ = true;
}
auto python::ispa_std::parser_ll::LLParser::getErrors() -> std::vector<std::shared_ptr<python::ispa_std::recovery::Diagnostic>> {
	return ((*((*this)).recovery_state_)).diagnostics;
}
auto python::ispa_std::parser_ll::LLParser::getContextErrors() -> const std::vector<std::shared_ptr<python::ispa_std::recovery::ContextDiagnostic>>& {
	return ((*this)).context_errors_;
}
auto python::ispa_std::parser_ll::LLParser::reset_recovery() -> void {
	((*((*this)).counters_)).clear();
	(((*((*this)).recovery_state_)).diagnostics).clear();
	((*((*this)).recovery_state_)).total_skipped = 0;
	((*((*this)).recovery_state_)).anchor_count = 0;
	((*this)).frame_count = 0;
	((*this)).unwind_ = 0;
	((*this)).last_resume_pos_ = python::ispa_std::parser_ll::NPOS;
	((*this)).recovering_ = false;
	(((*this)).context_errors_).clear();
}
auto python::ispa_std::parser_ll::LLParser::before_parse() -> void {
	((*this)).reset_recovery();
}
auto python::ispa_std::parser_ll::LLParser::parseLazy() -> std::any {
	((*this)).reset_recovery();
	if ((!(((*this)).has_text))) {
		python::ispa_std::failures::api_error("Can\'t parse because no input text is provided", {});
	}
	auto result = ((*this)).lazyParse();
	bool at_end = ((*((*result)).it)).isEnd();
	auto messages = ((*((*result)).it)).getErrors();
	long long i = 0;
	while (((i) < (static_cast<long long>(messages.size())))) {
		auto diagnostic = ISPA_STD::construct<python::ispa_std::recovery::Diagnostic>();
		((*diagnostic)).rule = "lexer";
		((*diagnostic)).message = (messages)[i];
		(((*((*this)).recovery_state_)).diagnostics).push_back(diagnostic);
		i = ((i) + (1));
	}
	if ((((!(((*result)).status))) || ((!(at_end))))) {
		auto diagnostic = ISPA_STD::construct<python::ispa_std::recovery::Diagnostic>();
		((*diagnostic)).rule = "main";
		if (((*result)).status) {
			((*diagnostic)).message = "unexpected trailing input";
		} else {
			((*diagnostic)).message = "could not match main rule";
		}
		if ((!(at_end))) {
			auto token = ((*((*result)).it)).current();
			((*diagnostic)).offset = ((*((*token)).meta())).startpos();
			((*diagnostic)).length = ((*((*token)).meta())).length();
		} else {
			((*diagnostic)).offset = static_cast<long long>(((*this)).text.size());
		}
		(((*((*this)).recovery_state_)).diagnostics).push_back(diagnostic);
	}
	if (((*result)).status) {
		((*this)).tree = ((*((*result)).node)).data();
	}
	return ((*this)).tree;
}
auto python::ispa_std::parser_ll::LLParser::counter_scope() -> std::shared_ptr<python::ispa_std::parser_ll::CounterScope> {
	return ISPA_STD::construct<python::ispa_std::parser_ll::CounterScope>(::ISPA_STD::self_ptr(this));
}
auto python::ispa_std::parser_ll::LLParser::bind_counter_count(const std::string& name, long long count) -> void {
	if ((!(((*((*this)).counters_)).try_emplace(name, count)))) {
		if (((((*((*this)).counters_)).get(name)) != (count))) {
			python::ispa_std::failures::logic_error((((("Shared counter \'") + (name))) + ("\' has a different count")));
		}
	}
}
auto python::ispa_std::parser_ll::LLParser::bind_counter_list(const std::string& name, const std::vector<std::shared_ptr<python::ispa_std::node::Node>>& values) -> void {
	((*this)).bind_counter_count(name, static_cast<long long>(values.size()));
}
auto python::ispa_std::parser_ll::LLParser::bind_counter_node(const std::string& name, std::shared_ptr<python::ispa_std::node::Node> node) -> void {
	long long count = 0;
	if ((!(((*node)).empty()))) {
		count = 1;
	}
	((*this)).bind_counter_count(name, count);
}
auto python::ispa_std::parser_ll::LLParser::begin_probe(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> std::shared_ptr<python::ispa_std::parser_ll::RuleProbe> {
	return ISPA_STD::construct<python::ispa_std::parser_ll::RuleProbe>(::ISPA_STD::self_ptr(this), python::ispa_std::parser_ll::cursor_position(pos));
}
auto python::ispa_std::parser_ll::LLParser::reject_absent_rule() -> void {
	"Raise NoMatch when we are inside an uncommitted probe: the rule never\n        started, so the caller\'s `optional` should simply not match.";
	if (((((static_cast<long long>(((*this)).probes_.size())) > (0))) && ((!(((*(((*this)).probes_)[((static_cast<long long>(((*this)).probes_.size())) - (1))])).committed))))) {
		python::ispa_std::failures::no_match();
	}
}
auto python::ispa_std::parser_ll::LLParser::commit_rule(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> void {
	((*this)).commit_at(python::ispa_std::parser_ll::cursor_position(pos));
}
auto python::ispa_std::parser_ll::LLParser::fast_path_ok(std::shared_ptr<python::ispa_std::recovery::FailHandler> fail, bool allow_custom) -> bool {
	"Whether a native caller may take a matched terminal/rule without the debug scope, RequiredResult and\n        recovery objects (then it reports the match with accept_at). The one place that decides; native fast paths\n        must not re-derive these conditions.";
	return (((((((!(((*this)).unwinding()))) && ((!(python::ispa_std::debug::enabled()))))) && (((*fail)).is_recovering()))) && (((allow_custom) || ((!(((*fail)).is_custom()))))));
}
auto python::ispa_std::parser_ll::LLParser::accept_at(long long here) -> void {
	"A required terminal was consumed by a native fast path (cursor now at `here`).";
	((*this)).commit_at(here);
	((*this)).note_match();
}
auto python::ispa_std::parser_ll::LLParser::commit_at(long long here) -> void {
	long long k = ((static_cast<long long>(((*this)).probes_.size())) - (1));
	while (((k) >= (0))) {
		if (((here) != (((*(((*this)).probes_)[k])).start))) {
			((*(((*this)).probes_)[k])).committed = true;
		}
		k = ((k) - (1));
	}
}
auto python::ispa_std::parser_ll::LLParser::note_match() -> void {
	((*this)).recovering_ = false;
}
auto python::ispa_std::parser_ll::LLParser::debug_actual(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> std::string {
	if ((!(python::ispa_std::debug::enabled()))) {
		return "";
	}
	if (((*pos)).isEnd()) {
		return "<end of input>";
	}
	std::string actual = (("token #") + (python::ispa_std::format::int_to_string(python::ispa_std::recovery::token_id(((*pos)).current()))));
	auto meta = ((*((*pos)).current())).meta();
	long long shown = 80;
	if (((((*meta)).has_source()) && (((((*meta)).length()) > (0))))) {
		actual = ((((((actual) + (" \'"))) + (python::ispa_std::string::substr(((*meta)).source(), ((*meta)).startpos(), python::ispa_std::utility::minimum(((*meta)).length(), shown))))) + ("\'"));
	}
	return actual;
}
auto python::ispa_std::parser_ll::LLParser::debug_recovery_mode(long long mode) -> std::string {
	if (((mode) == (python::ispa_std::recovery::Mode::PanicUntil))) {
		return "panic until synchronization";
	}
	if (((mode) == (python::ispa_std::recovery::Mode::Insert))) {
		return "insert";
	}
	if (((mode) == (python::ispa_std::recovery::Mode::Delete))) {
		return "delete and retry";
	}
	if (((mode) == (python::ispa_std::recovery::Mode::Replace))) {
		return "replace";
	}
	if (((mode) == (python::ispa_std::recovery::Mode::Retry))) {
		return "retry";
	}
	if (((mode) == (python::ispa_std::recovery::Mode::Synchronize))) {
		return "synchronize";
	}
	if (((mode) == (python::ispa_std::recovery::Mode::Propagate))) {
		return "propagate to parent";
	}
	if (((mode) == (python::ispa_std::recovery::Mode::Abort))) {
		return "abort";
	}
	return "unknown";
}
auto python::ispa_std::parser_ll::LLParser::recovery_frame(const std::string& rule) -> std::shared_ptr<python::ispa_std::parser_ll::FrameGuard> {
	return ISPA_STD::construct<python::ispa_std::parser_ll::FrameGuard>(::ISPA_STD::self_ptr(this), rule);
}
auto python::ispa_std::parser_ll::LLParser::enter_frame(const std::string& rule) -> void {
	"Open the recovery frame of a rule invocation, reusing a pooled frame when there is one.";
	if (((((*this)).frame_count) < (static_cast<long long>(((*this)).frames_.size())))) {
		auto frame = (((*this)).frames_)[((*this)).frame_count];
		((*frame)).rule = rule;
		((*frame)).expecting = "";
		((*frame)).claim_set = python::ispa_std::recovery::empty_claim_set();
	} else {
		(((*this)).frames_).push_back(ISPA_STD::construct<python::ispa_std::parser_ll::Frame>(rule));
	}
	((*this)).frame_count = ((((*this)).frame_count) + (1));
	auto state = ((*this)).recovery_state_;
	if (((((*state)).anchor_count) < (static_cast<long long>(((*state)).anchors.size())))) {
		(((*state)).anchors)[((*state)).anchor_count] = python::ispa_std::recovery::empty_claim_set();
	} else {
		(((*state)).anchors).push_back(python::ispa_std::recovery::empty_claim_set());
	}
	((*state)).anchor_count = ((((*state)).anchor_count) + (1));
}
auto python::ispa_std::parser_ll::LLParser::leave_frame() -> void {
	((*this)).frame_count = ((((*this)).frame_count) - (1));
	((*((*this)).recovery_state_)).anchor_count = ((((*((*this)).recovery_state_)).anchor_count) - (1));
	if (((((*this)).unwind_) != (0))) {
		((*this)).unwind_ = ((((*this)).unwind_) - (1));
	}
}
auto python::ispa_std::parser_ll::LLParser::recovery_at(const std::string& expecting, const std::vector<std::shared_ptr<python::ispa_std::recovery::ContextClaim>>& claims) -> void {
	"Emitted before each child member: the claims of this frame here (the\n        member itself if repeated, plus the suffix up to the first required\n        member). `claims` is a list of recovery.ContextClaim.";
	((*this)).recovery_at_set(expecting, ISPA_STD::construct<python::ispa_std::recovery::ClaimSet>(claims));
}
auto python::ispa_std::parser_ll::LLParser::recovery_at_set(const std::string& expecting, std::shared_ptr<python::ispa_std::recovery::ClaimSet> claims) -> void {
	"recovery_at with the claims prepared once per site (shared, never copied per visit).";
	if (((((*this)).frame_count) == (0))) {
		return ;
	}
	auto frame = (((*this)).frames_)[((((*this)).frame_count) - (1))];
	((*frame)).expecting = expecting;
	((*frame)).claim_set = claims;
	(((*((*this)).recovery_state_)).anchors)[((((*((*this)).recovery_state_)).anchor_count) - (1))] = claims;
}
auto python::ispa_std::parser_ll::LLParser::unwinding() -> bool {
	return ((((*this)).unwind_) != (0));
}
auto python::ispa_std::parser_ll::LLParser::describe_token(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> std::string {
	if (((*pos)).isEnd()) {
		return "end of input";
	}
	auto meta = ((*((*pos)).current())).meta();
	if (((((*pos)).has_source()) && (((((*meta)).startpos()) != (python::ispa_std::parser_ll::NPOS))))) {
		std::string origin = ((*pos)).sourceOrigin();
		if (((((((*meta)).startpos()) <= (static_cast<long long>(origin.size())))) && (((((*meta)).length()) <= (((static_cast<long long>(origin.size())) - (((*meta)).startpos()))))))) {
			return (((("\'") + (python::ispa_std::string::substr(origin, ((*meta)).startpos(), ((*meta)).length())))) + ("\'"));
		}
	}
	return "<token>";
}
auto python::ispa_std::parser_ll::LLParser::breadcrumb() -> std::string {
	std::string out = "";
	long long i = 0;
	while (((i) < (((*this)).frame_count))) {
		if (((static_cast<long long>(out.size())) > (0))) {
			out = ((out) + (" > "));
		}
		out = ((out) + (((*(((*this)).frames_)[i])).rule));
		i = ((i) + (1));
	}
	return out;
}
auto python::ispa_std::parser_ll::LLParser::resolve_context(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long max_skip) -> std::shared_ptr<python::ispa_std::parser_ll::Resume> {
	"Earliest token (within max_skip) claimed by any frame; ties go to the\n        innermost frame. If an identical spot already failed, the innermost\n        frame may not claim the current token again (guarantees progress).\n        Returns a Resume with frame == -1 when there is none.";
	bool repeat_spot = ((python::ispa_std::parser_ll::cursor_position(pos)) == (((*this)).last_resume_pos_));
	auto candidate = ((*pos)).copy();
	long long k = 0;
	while (((((k) <= (max_skip))) && ((!(((*candidate)).isEnd()))))) {
		auto token = ((*candidate)).current();
		long long f = ((*this)).frame_count;
		while (((f) > (0))) {
			f = ((f) - (1));
			if ((!(((((((k) == (0))) && (repeat_spot))) && (((((f) + (1))) == (((*this)).frame_count))))))) {
				std::vector<std::shared_ptr<python::ispa_std::recovery::ContextClaim>>& claims = ((*((*(((*this)).frames_)[f])).claim_set)).claims;
				long long c = 0;
				while (((c) < (static_cast<long long>(claims.size())))) {
					if (((*((*(claims)[c])).tokens)).matches(token)) {
						return ISPA_STD::construct<python::ispa_std::parser_ll::Resume>(k, f, (claims)[c], true);
					}
					c = ((c) + (1));
				}
			}
		}
		k = ((k) + (1));
		((*candidate)).next();
	}
	if (((((*candidate)).isEnd()) && (((((*this)).frame_count) > (0))))) {
		return ISPA_STD::construct<python::ispa_std::parser_ll::Resume>(k, 0, ISPA_STD::construct<python::ispa_std::recovery::ContextClaim>("", python::ispa_std::recovery::empty_token_set()), false);
	}
	return ISPA_STD::construct<python::ispa_std::parser_ll::Resume>(0, ( - (1)), ISPA_STD::construct<python::ispa_std::recovery::ContextClaim>("", python::ispa_std::recovery::empty_token_set()), false);
}
auto python::ispa_std::parser_ll::LLParser::report_context(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long kind, std::shared_ptr<python::ispa_std::parser_ll::Resume> r) -> void {
	auto inner = (((*this)).frames_)[((((*this)).frame_count) - (1))];
	auto target = (((*this)).frames_)[((*r)).frame];
	auto d = ISPA_STD::construct<python::ispa_std::recovery::ContextDiagnostic>();
	((*d)).kind = kind;
	((*d)).position = python::ispa_std::parser_ll::cursor_position(pos);
	if (((*pos)).has_source()) {
		long long at = ((*pos)).sourcePosition();
		if (((at) >= (0))) {
			((*d)).offset = at;
			((*d)).length = 0;
			if (((at) < (static_cast<long long>(((*pos)).sourceOrigin().size())))) {
				((*d)).length = 1;
			}
		}
	} else {
		if ((!(((*pos)).isEnd()))) {
			((*d)).offset = ((*((*((*pos)).current())).meta())).startpos();
			((*d)).length = ((*((*((*pos)).current())).meta())).length();
		}
	}
	if (((((((((*d)).offset) == (python::ispa_std::recovery::NO_OFFSET))) && (((*pos)).isEnd()))) && (((*this)).has_text))) {
		((*d)).offset = static_cast<long long>(((*this)).text.size());
	}
	((*d)).skipped = ((*r)).skip;
	((*d)).rule = ((*inner)).rule;
	((*d)).resume_rule = ((*target)).rule;
	if (((*r)).has_claim) {
		((*d)).claim = ((*((*r)).claim)).name;
	}
	std::string tok = ((*this)).describe_token(pos);
	std::string where = ((*this)).breadcrumb();
	std::string expected = "";
	if (((static_cast<long long>(((*inner)).expecting.size())) > (0))) {
		expected = (("; expected ") + (((*inner)).expecting));
	}
	if (((kind) == (python::ispa_std::recovery::ContextKind::Misplaced))) {
		((*d)).message = (((((((((((((((((((("unexpected ") + (tok))) + (" while parsing \'"))) + (((*d)).rule))) + ("\'"))) + (expected))) + ("; continuing at \'"))) + (((*d)).claim))) + ("\' in enclosing rule \'"))) + (((*d)).resume_rule))) + ("\'"));
	} else {
		if (((kind) == (python::ispa_std::recovery::ContextKind::Stray))) {
			std::string tail = "; reached end of input";
			if (((*r)).has_claim) {
				tail = (((((((("; resuming at \'") + (((*d)).claim))) + ("\' of \'"))) + (((*d)).resume_rule))) + ("\'"));
			}
			((*d)).message = (((((((((((((("unexpected ") + (tok))) + (" in "))) + (where))) + ("; no enclosing rule accepts it here; skipped "))) + (python::ispa_std::format::int_to_string(((*r)).skip)))) + (" token(s)"))) + (tail));
		} else {
			((*d)).message = (((((((("unexpected ") + (tok))) + (" in "))) + (where))) + (expected));
			if (((((*r)).skip) != (0))) {
				((*d)).message = ((((((((*d)).message) + ("; skipped "))) + (python::ispa_std::format::int_to_string(((*r)).skip)))) + (" token(s)"));
			}
			if (((*r)).has_claim) {
				((*d)).message = ((((((((((((*d)).message) + ("; resuming at \'"))) + (((*d)).claim))) + ("\' of \'"))) + (((*d)).resume_rule))) + ("\'"));
			} else {
				((*d)).message = ((((*d)).message) + ("; reached end of input"));
			}
		}
	}
	(((*this)).context_errors_).push_back(d);
}
auto python::ispa_std::parser_ll::LLParser::recover_at_context(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long kind) -> bool {
	"Local repair already failed (or does not apply). Find a recovery\n        point in the enclosing frames, skip to it, and request unwinding.";
	if (((((((*this)).frame_count) == (0))) || (((*((*this)).recovery_state_)).exhausted()))) {
		return false;
	}
	long long remaining = ((((*((*this)).recovery_state_)).max_total_skipped) - (((*((*this)).recovery_state_)).total_skipped));
	auto r = ((*this)).resolve_context(pos, python::ispa_std::utility::minimum(((*this)).context_max_skip_, remaining));
	if (((((*r)).frame) == (( - (1))))) {
		return false;
	}
	long long effective = kind;
	bool innermost = ((((((*r)).frame) + (1))) == (((*this)).frame_count));
	if ((((((!(innermost))) && (((((*r)).skip) == (0))))) && (((*r)).has_claim))) {
		effective = python::ispa_std::recovery::ContextKind::Misplaced;
	}
	if ((((!(((*this)).recovering_))) || (((effective) == (python::ispa_std::recovery::ContextKind::Misplaced))))) {
		((*this)).report_context(pos, effective, r);
	}
	long long i = 0;
	while (((((i) < (((*r)).skip))) && ((!(((*pos)).isEnd()))))) {
		((*pos)).next();
		i = ((i) + (1));
	}
	((*((*this)).recovery_state_)).total_skipped = ((((*((*this)).recovery_state_)).total_skipped) + (((*r)).skip));
	((*this)).last_resume_pos_ = python::ispa_std::parser_ll::cursor_position(pos);
	((*this)).unwind_ = ((((((*this)).frame_count) - (1))) - (((*r)).frame));
	((*this)).recovering_ = true;
	return true;
}
auto python::ispa_std::parser_ll::LLParser::sync_loop(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> void {
	"A loop ended on a token that no active frame can accept: report it\n        here, where the mistake is, rather than at the next required member.";
	if (((((((*pos)).isEnd()) || (((((*this)).frame_count) == (0))))) || (((*this)).unwinding()))) {
		return ;
	}
	auto candidate = ((*pos)).copy();
	if (((*this)).has_space_token_) {
		((*this)).skip_spaces(candidate);
	}
	if (((*candidate)).isEnd()) {
		return ;
	}
	if (((*((*candidate)).current())).isAmbiguous()) {
		return ;
	}
	auto token = ((*candidate)).current();
	bool any_claims = false;
	long long i = 0;
	while (((i) < (((*this)).frame_count))) {
		std::vector<std::shared_ptr<python::ispa_std::recovery::ContextClaim>>& claims = ((*((*(((*this)).frames_)[i])).claim_set)).claims;
		long long c = 0;
		while (((c) < (static_cast<long long>(claims.size())))) {
			any_claims = true;
			if (((*((*(claims)[c])).tokens)).matches(token)) {
				return ;
			}
			c = ((c) + (1));
		}
		i = ((i) + (1));
	}
	if ((!(any_claims))) {
		return ;
	}
	((*pos)).assign(candidate);
	((*this)).recover_at_context(pos, python::ispa_std::recovery::ContextKind::Stray);
}
auto python::ispa_std::parser_ll::LLParser::recover_choice(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, const std::string& rule, const std::string& expected, const std::vector<std::shared_ptr<python::ispa_std::recovery::TokenSet>>& first_sets, const std::vector<std::shared_ptr<python::ispa_std::recovery::TokenSet>>& inner_sets) -> long long {
	"Called only after normal LL(k) selection failed. Prefer an\n        unambiguous FIRST token, then an unambiguous terminal inside an\n        alternative (shared interior terminals do not identify a branch).\n        `first_sets[i]` / `inner_sets[i]` are the two sets of alternative i\n        (the C++ even/odd template pack). Returns the alternative, or -1.";
	((*this)).reject_absent_rule();
	if (((static_cast<long long>(first_sets.size())) != (static_cast<long long>(inner_sets.size())))) {
		python::ispa_std::failures::logic_error("recover_choice: one FIRST set and one interior set per alternative");
	}
	auto diagnostic = ISPA_STD::construct<python::ispa_std::recovery::Diagnostic>();
	((*diagnostic)).rule = rule;
	((*diagnostic)).expected_text = expected;
	((*diagnostic)).message = (((((((((("unexpected ") + (((*this)).describe_token(pos)))) + (" in "))) + (rule))) + ("; expected one of: "))) + (expected));
	if ((!(((*pos)).isEnd()))) {
		((*diagnostic)).unexpected = python::ispa_std::recovery::token_id(((*pos)).current());
		((*diagnostic)).offset = ((*((*((*pos)).current())).meta())).startpos();
		((*diagnostic)).length = ((*((*((*pos)).current())).meta())).length();
	}
	long long i = 0;
	while (((i) < (static_cast<long long>(first_sets.size())))) {
		(((*diagnostic)).expected).insert((((*diagnostic)).expected).end(), ((*(first_sets)[i])).values().begin(), ((*(first_sets)[i])).values().end());
		i = ((i) + (1));
	}
	long long selected = ( - (1));
	long long skipped = 0;
	if ((!(((*((*this)).recovery_state_)).exhausted()))) {
		bool scanning = true;
		while (((scanning) && ((!(((*pos)).isEnd()))))) {
			long long ident = python::ispa_std::recovery::token_id(((*pos)).current());
			long long phase = 0;
			while (((((phase) < (2))) && (((selected) == (( - (1))))))) {
				auto sets = first_sets;
				if (((phase) == (1))) {
					sets = inner_sets;
				}
				long long candidate = ( - (1));
				bool ambiguous = false;
				long long j = 0;
				while (((j) < (static_cast<long long>(sets.size())))) {
					if (python::ispa_std::recovery::contains(((*(sets)[j])).values(), ident)) {
						if (((candidate) != (( - (1))))) {
							ambiguous = true;
						}
						candidate = j;
					}
					j = ((j) + (1));
				}
				if (ambiguous) {
					phase = 2;
				} else {
					selected = candidate;
					phase = ((phase) + (1));
				}
			}
			if (((selected) != (( - (1))))) {
				scanning = false;
			} else {
				bool boundary = false;
				long long a = 0;
				while (((a) < (((*((*this)).recovery_state_)).anchor_count))) {
					if (python::ispa_std::recovery::contains(((*(((*((*this)).recovery_state_)).anchors)[a])).tokens, ident)) {
						boundary = true;
					}
					a = ((a) + (1));
				}
				if (((((boundary) || (((skipped) >= (((*this)).context_max_skip_))))) || (((((((*((*this)).recovery_state_)).total_skipped) + (skipped))) >= (((*((*this)).recovery_state_)).max_total_skipped))))) {
					scanning = false;
				} else {
					((*pos)).next();
					skipped = ((skipped) + (1));
				}
			}
		}
	}
	((*((*this)).recovery_state_)).total_skipped = ((((*((*this)).recovery_state_)).total_skipped) + (skipped));
	((*((*diagnostic)).decision)).skipped = skipped;
	if (((selected) == (( - (1))))) {
		((*((*diagnostic)).decision)).mode = python::ispa_std::recovery::Mode::Propagate;
		((*diagnostic)).message = ((((*diagnostic)).message) + ("; no unambiguous alternative at recovery boundary"));
	} else {
		((*((*diagnostic)).decision)).mode = python::ispa_std::recovery::Mode::Synchronize;
		((*diagnostic)).message = ((((((*diagnostic)).message) + ("; continuing in alternative "))) + (python::ispa_std::format::int_to_string(((selected) + (1)))));
	}
	if (((skipped) != (0))) {
		((*diagnostic)).message = ((((((((*diagnostic)).message) + ("; skipped "))) + (python::ispa_std::format::int_to_string(skipped)))) + (" tokens"));
	}
	(((*((*this)).recovery_state_)).diagnostics).push_back(diagnostic);
	if (((*((*this)).recovery_state_)).exhausted()) {
		auto last = (((*((*this)).recovery_state_)).diagnostics)[((static_cast<long long>(((*((*this)).recovery_state_)).diagnostics.size())) - (1))];
		((*((*last)).decision)).mode = python::ispa_std::recovery::Mode::Abort;
		((*last)).message = ((((*last)).message) + ("; recovery limit reached"));
		return ( - (1));
	}
	return selected;
}
auto python::ispa_std::parser_ll::LLParser::raise_error(std::shared_ptr<python::ispa_std::raw_token::RawToken> token, const std::string& message) -> void {
	((*this)).reject_absent_rule();
	auto diagnostic = ISPA_STD::construct<python::ispa_std::recovery::Diagnostic>();
	if (((((*this)).frame_count) == (0))) {
		((*diagnostic)).rule = "<rule>";
	} else {
		((*diagnostic)).rule = ((*(((*this)).frames_)[((((*this)).frame_count) - (1))])).rule;
	}
	((*diagnostic)).message = message;
	((*diagnostic)).offset = ((*((*token)).meta())).startpos();
	((*diagnostic)).length = ((*((*token)).meta())).length();
	(((*((*this)).recovery_state_)).diagnostics).push_back(diagnostic);
}
auto python::ispa_std::parser_ll::LLParser::required_recovering(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long type_id, std::shared_ptr<python::ispa_std::recovery::FailHandler> fail) -> std::shared_ptr<python::ispa_std::recovery::RequiredResult> {
	if (((*this)).unwinding()) {
		return python::ispa_std::recovery::RequiredResult::boundary_only();
	}
	long long retries = 0;
	while (true) {
		if (((*pos)).kind_is(type_id)) {
			auto node = ((*((*pos)).current())).to_node(type_id);
			((*pos)).next();
			((*this)).commit_rule(pos);
			((*this)).note_match();
			return ISPA_STD::construct<python::ispa_std::recovery::RequiredResult>(python::ispa_std::recovery::RequiredStatus::Matched, true, node, python::ispa_std::recovery::Decision::initial());
		}
		((*this)).reject_absent_rule();
		auto decision = ((*fail)).run(pos, ((*this)).recovery_state_);
		bool retry = false;
		if (((((*decision)).mode) == (python::ispa_std::recovery::Mode::Delete))) {
			retries = ((retries) + (1));
			retry = ((retries) <= (2));
		} else {
			if (((((*decision)).mode) == (python::ispa_std::recovery::Mode::Retry))) {
				retries = ((retries) + (1));
				retry = ((retries) <= (1));
			} else {
				if (((((((*decision)).mode) == (python::ispa_std::recovery::Mode::Insert))) || (((((*decision)).mode) == (python::ispa_std::recovery::Mode::Replace))))) {
					return ISPA_STD::construct<python::ispa_std::recovery::RequiredResult>(python::ispa_std::recovery::RequiredStatus::Missing, false, python::ispa_std::node::Node::blank(), decision);
				} else {
					if (((((*decision)).mode) == (python::ispa_std::recovery::Mode::Abort))) {
						return ISPA_STD::construct<python::ispa_std::recovery::RequiredResult>(python::ispa_std::recovery::RequiredStatus::Aborted, false, python::ispa_std::node::Node::blank(), decision);
					}
				}
			}
		}
		if ((!(retry))) {
			if (((((*decision)).mode) == (python::ispa_std::recovery::Mode::Propagate))) {
				((*this)).recover_at_context(pos, python::ispa_std::recovery::ContextKind::Mismatch);
			}
			return ISPA_STD::construct<python::ispa_std::recovery::RequiredResult>(python::ispa_std::recovery::RequiredStatus::Boundary, false, python::ispa_std::node::Node::blank(), decision);
		}
	}
}
auto python::ispa_std::parser_ll::LLParser::_attempt_rule(std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rec, std::shared_ptr<python::ispa_std::lexer::TokenCursor> attempt) -> std::shared_ptr<python::ispa_std::match_result::MatchResult> {
	"Run `rec(attempt)` inside a probe, absorbing NoMatch and\n        ExternalParse so an explicitly bound parent handler can run (C++\n        `catch (const NoMatch&)` / `catch (const ExternalParseError&)`).";
	std::vector<std::shared_ptr<python::ispa_std::match_result::MatchResult>> holder = std::vector<std::shared_ptr<python::ispa_std::match_result::MatchResult>>{};
	auto probe = ((*this)).begin_probe(attempt);
	long long scope = python::ispa_std::debug::scope_depth();
	auto failed = ::ISPA_STD::attempt([&]() {
return (holder).push_back(rec(attempt));
});
	((*probe)).close();
	if (((failed) == (0))) {
		return (holder)[0];
	}
	python::ispa_std::debug::unwind_to(scope);
	if (((((failed) == (python::ispa_std::errors::ErrorKind::NoMatch))) || (((failed) == (python::ispa_std::errors::ErrorKind::ExternalParse))))) {
		return python::ispa_std::match_result::MatchResult::failed(attempt);
	}
	python::ispa_std::failures::rethrow(failed, ::ISPA_STD::failure_message());
	return python::ispa_std::match_result::MatchResult::failed(attempt);
}
auto python::ispa_std::parser_ll::LLParser::required_rule_recovering(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rec, std::shared_ptr<python::ispa_std::recovery::FailHandler> fail) -> std::shared_ptr<python::ispa_std::recovery::RequiredResult> {
	return ((*this))._rule_recovery(pos, rec, fail, python::ispa_std::match_result::MatchResult::failed(pos), false);
}
auto python::ispa_std::parser_ll::LLParser::_rule_recovery(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rec, std::shared_ptr<python::ispa_std::recovery::FailHandler> fail, std::shared_ptr<python::ispa_std::match_result::MatchResult> first, bool have_first) -> std::shared_ptr<python::ispa_std::recovery::RequiredResult> {
	if ((((!(have_first))) && (((*this)).unwinding()))) {
		return python::ispa_std::recovery::RequiredResult::boundary_only();
	}
	long long retries = 0;
	while (true) {
		std::shared_ptr<python::ispa_std::match_result::MatchResult> match = first;
		if (have_first) {
			have_first = false;
		} else {
			auto attempt = ((*pos)).copy();
			if (((*fail)).is_custom()) {
				match = ((*this))._attempt_rule(rec, attempt);
			} else {
				match = rec(attempt);
			}
		}
		if (((*match)).status) {
			((*pos)).assign(((*match)).it);
			((*this)).commit_rule(pos);
			((*this)).note_match();
			return ISPA_STD::construct<python::ispa_std::recovery::RequiredResult>(python::ispa_std::recovery::RequiredStatus::Matched, true, ((*match)).node, python::ispa_std::recovery::Decision::initial());
		}
		if (((*this)).unwinding()) {
			((*pos)).assign(((*match)).it);
			return ISPA_STD::construct<python::ispa_std::recovery::RequiredResult>(python::ispa_std::recovery::RequiredStatus::Boundary, false, python::ispa_std::node::Node::blank(), python::ispa_std::recovery::Decision::initial());
		}
		((*this)).reject_absent_rule();
		auto decision = ((*fail)).run(pos, ((*this)).recovery_state_);
		bool again = false;
		if (((((((*decision)).mode) == (python::ispa_std::recovery::Mode::Retry))) || (((((*decision)).mode) == (python::ispa_std::recovery::Mode::Delete))))) {
			retries = ((retries) + (1));
			again = ((retries) <= (2));
		}
		if ((!(again))) {
			if (((((*decision)).mode) == (python::ispa_std::recovery::Mode::Propagate))) {
				((*this)).recover_at_context(pos, python::ispa_std::recovery::ContextKind::Mismatch);
			}
			auto status = python::ispa_std::recovery::RequiredStatus::Boundary;
			if (((((*decision)).mode) == (python::ispa_std::recovery::Mode::Abort))) {
				status = python::ispa_std::recovery::RequiredStatus::Aborted;
			} else {
				if (((((((*decision)).mode) == (python::ispa_std::recovery::Mode::Insert))) || (((((*decision)).mode) == (python::ispa_std::recovery::Mode::Replace))))) {
					status = python::ispa_std::recovery::RequiredStatus::Missing;
				}
			}
			return ISPA_STD::construct<python::ispa_std::recovery::RequiredResult>(status, false, python::ispa_std::node::Node::blank(), decision);
		}
	}
}
auto python::ispa_std::parser_ll::LLParser::required_rule_resume(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rec, std::shared_ptr<python::ispa_std::recovery::FailHandler> fail, std::shared_ptr<python::ispa_std::match_result::MatchResult> first) -> std::shared_ptr<python::ispa_std::node::Node> {
	"Continue a required rule whose first attempt (made by a native fast path) failed.";
	auto result = ((*this))._rule_recovery(pos, rec, fail, first, true);
	return ((*this))._required_outcome(result, pos, python::ispa_std::debug::idle_match_scope(), "required rule");
}
auto python::ispa_std::parser_ll::LLParser::_required_outcome(std::shared_ptr<python::ispa_std::recovery::RequiredResult> result, std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::debug::MatchScope> scope, const std::string& what) -> std::shared_ptr<python::ispa_std::node::Node> {
	"Shared tail of required_token / required_rule: a repaired or unwound\n        match yields an empty Node; anything else is unrecoverable.";
	if (((*result)).has_node) {
		((*scope)).success();
		((*scope)).close();
		return ((*result)).node;
	}
	if (((((*result)).status) != (python::ispa_std::recovery::RequiredStatus::Matched))) {
		((*scope)).outcome((((((("recovery: ") + (((*this)).debug_recovery_mode(((*((*result)).recovery)).mode)))) + (", skipped "))) + (python::ispa_std::format::int_to_string(((*((*result)).recovery)).skipped))));
	}
	((*scope)).close();
	if (((((*result)).status) != (python::ispa_std::recovery::RequiredStatus::Aborted))) {
		if (((((((((*result)).status) == (python::ispa_std::recovery::RequiredStatus::Missing))) || (((*this)).unwinding()))) || ((((!(((*pos)).isEnd()))) && (((((*((*result)).recovery)).mode) != (python::ispa_std::recovery::Mode::Propagate))))))) {
			return python::ispa_std::node::Node::blank();
		}
	}
	python::ispa_std::debug::breakpoint(((what) + (" failed")));
	if (((static_cast<long long>(((*((*this)).recovery_state_)).diagnostics.size())) == (0))) {
		python::ispa_std::failures::logic_error((((("ISPA required(): ") + (what))) + (" could not be recovered")));
	} else {
		python::ispa_std::failures::logic_error(((*(((*((*this)).recovery_state_)).diagnostics)[((static_cast<long long>(((*((*this)).recovery_state_)).diagnostics.size())) - (1))])).message);
	}
	return python::ispa_std::node::Node::blank();
}
auto python::ispa_std::parser_ll::LLParser::required_token(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long type_id, std::shared_ptr<python::ispa_std::recovery::FailHandler> fail) -> std::shared_ptr<python::ispa_std::node::Node> {
	auto scope = python::ispa_std::debug::idle_match_scope();
	if (python::ispa_std::debug::enabled()) {
		scope = ISPA_STD::construct<python::ispa_std::debug::MatchScope>(((*((*this)).token_names_)).lookup(type_id), ((*this)).debug_actual(pos));
	}
	if (((*fail)).is_recovering()) {
		if ((((!(((*this)).unwinding()))) && (((*pos)).kind_is(type_id)))) {
			auto node = ((*((*pos)).current())).to_node(type_id);
			((*pos)).next();
			((*this)).commit_rule(pos);
			((*this)).note_match();
			((*scope)).success();
			((*scope)).close();
			return node;
		}
		auto result = ((*this)).required_recovering(pos, type_id, fail);
		return ((*this))._required_outcome(result, pos, scope, "required token");
	}
	if (((*pos)).kind_is(type_id)) {
		auto node = ((*((*pos)).current())).to_node(type_id);
		((*pos)).next();
		((*this)).commit_rule(pos);
		((*scope)).success();
		((*scope)).close();
		return node;
	}
	((*this)).reject_absent_rule();
	((*fail)).run(pos, ((*this)).recovery_state_);
	((*scope)).close();
	python::ispa_std::debug::breakpoint("required token failed");
	if (((static_cast<long long>(((*((*this)).recovery_state_)).diagnostics.size())) == (0))) {
		python::ispa_std::failures::logic_error("ISPA required(): required token could not be recovered");
	} else {
		python::ispa_std::failures::logic_error(((*(((*((*this)).recovery_state_)).diagnostics)[((static_cast<long long>(((*((*this)).recovery_state_)).diagnostics.size())) - (1))])).message);
	}
	return python::ispa_std::node::Node::blank();
}
auto python::ispa_std::parser_ll::LLParser::required_rule(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, const std::string& rule_name, std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rec, std::shared_ptr<python::ispa_std::recovery::FailHandler> fail) -> std::shared_ptr<python::ispa_std::node::Node> {
	std::shared_ptr<python::ispa_std::recovery::RequiredResult> result;
	auto scope = python::ispa_std::debug::idle_match_scope();
	if (python::ispa_std::debug::enabled()) {
		scope = ISPA_STD::construct<python::ispa_std::debug::MatchScope>(rule_name, ((*this)).debug_actual(pos));
	}
	if (((*fail)).is_recovering()) {
		if ((((!(((*this)).unwinding()))) && ((!(((*fail)).is_custom()))))) {
			auto first = rec(((*pos)).copy());
			if (((*first)).status) {
				((*pos)).assign(((*first)).it);
				((*this)).commit_rule(pos);
				((*this)).note_match();
				((*scope)).success();
				((*scope)).close();
				return ((*first)).node;
			}
			result = ((*this))._rule_recovery(pos, rec, fail, first, true);
			return ((*this))._required_outcome(result, pos, scope, "required rule");
		}
		result = ((*this)).required_rule_recovering(pos, rec, fail);
		return ((*this))._required_outcome(result, pos, scope, "required rule");
	}
	auto match = rec(pos);
	if (((*match)).status) {
		((*pos)).assign(((*match)).it);
		((*this)).commit_rule(pos);
		((*scope)).success();
		((*scope)).close();
		return ((*match)).node;
	}
	((*this)).reject_absent_rule();
	((*fail)).run(pos, ((*this)).recovery_state_);
	((*scope)).close();
	python::ispa_std::debug::breakpoint("required rule failed");
	if (((static_cast<long long>(((*((*this)).recovery_state_)).diagnostics.size())) == (0))) {
		python::ispa_std::failures::logic_error("ISPA required(): required rule could not be recovered");
	} else {
		python::ispa_std::failures::logic_error(((*(((*((*this)).recovery_state_)).diagnostics)[((static_cast<long long>(((*((*this)).recovery_state_)).diagnostics.size())) - (1))])).message);
	}
	return python::ispa_std::node::Node::blank();
}
auto python::ispa_std::parser_ll::LLParser::optional_token(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long type_id) -> std::shared_ptr<python::ispa_std::node::Node> {
	auto scope = python::ispa_std::debug::idle_match_scope();
	if (python::ispa_std::debug::enabled()) {
		scope = ISPA_STD::construct<python::ispa_std::debug::MatchScope>(((*((*this)).token_names_)).lookup(type_id), ((*this)).debug_actual(pos));
	}
	if (((*pos)).kind_is(type_id)) {
		auto node = ((*((*pos)).current())).to_node(type_id);
		((*pos)).next();
		((*this)).commit_rule(pos);
		((*scope)).success();
		((*scope)).close();
		return node;
	}
	((*scope)).close();
	return python::ispa_std::node::Node::blank();
}
auto python::ispa_std::parser_ll::LLParser::optional_rule(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rec) -> std::shared_ptr<python::ispa_std::node::Node> {
	"NOTE: a rule that abandons itself during unwinding must still return\n        status == True (a partial match) or the committed-probe check throws.";
	auto attempt = ((*pos)).copy();
	std::vector<std::shared_ptr<python::ispa_std::match_result::MatchResult>> holder = std::vector<std::shared_ptr<python::ispa_std::match_result::MatchResult>>{};
	auto probe = ((*this)).begin_probe(pos);
	long long scope = python::ispa_std::debug::scope_depth();
	auto failed = ::ISPA_STD::attempt([&]() {
return (holder).push_back(rec(attempt));
});
	if (((failed) == (0))) {
		auto match = (holder)[0];
		if (((*match)).status) {
			((*pos)).assign(((*match)).it);
			((*this)).commit_rule(pos);
			((*probe)).close();
			return ((*match)).node;
		}
		bool committed = ((*probe)).committed;
		((*probe)).close();
		if (committed) {
			python::ispa_std::failures::logic_error("ISPA optional(): rule failed after consuming input");
		}
		return python::ispa_std::node::Node::blank();
	}
	((*probe)).close();
	python::ispa_std::debug::unwind_to(scope);
	if (((failed) != (python::ispa_std::errors::ErrorKind::NoMatch))) {
		python::ispa_std::failures::rethrow(failed, ::ISPA_STD::failure_message());
	}
	return python::ispa_std::node::Node::blank();
}
auto python::ispa_std::parser_ll::LLParser::skip_spaces(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> void {
	if ((!(((*this)).has_space_token_))) {
		return ;
	}
	if (((*pos)).kind_is(((*this)).space_token_)) {
		((*pos)).next();
	}
}
auto python::ispa_std::parser_ll::LLParser::skip_external_spaces(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> void {
	if (((*pos)).has_source()) {
		std::string origin = ((*pos)).sourceOrigin();
		long long begin = ((*pos)).sourcePosition();
		long long end = begin;
		while (python::ispa_std::chars::is_space(python::ispa_std::chars::at(origin, end))) {
			end = ((end) + (1));
		}
		if (((end) != (begin))) {
			((*pos)).consumeExternal(end);
		}
	} else {
		((*this)).skip_spaces(pos);
	}
}
auto python::ispa_std::parser_ll::LLParser::counted_impl(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::shared_ptr<python::ispa_std::counter::CountSpec> spec, std::function<std::shared_ptr<python::ispa_std::node::Node>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> match) -> std::vector<std::shared_ptr<python::ispa_std::node::Node>> {
	bool has_limit = ((*spec)).has_count;
	long long limit = ((*spec)).count;
	if (((static_cast<long long>(((*spec)).name.size())) > (0))) {
		if (((*((*this)).counters_)).contains(((*spec)).name)) {
			limit = ((*((*this)).counters_)).get(((*spec)).name);
			has_limit = true;
		}
	}
	std::vector<std::shared_ptr<python::ispa_std::node::Node>> nodes = std::vector<std::shared_ptr<python::ispa_std::node::Node>>{};
	auto cursor = ((*pos)).copy();
	bool matching = true;
	while (((matching) && ((((!(has_limit))) || (((static_cast<long long>(nodes.size())) < (limit))))))) {
		auto attempt = ((*cursor)).copy();
		if (((((*this)).has_space_token_) && (((static_cast<long long>(nodes.size())) > (0))))) {
			((*this)).skip_spaces(attempt);
		}
		long long before = python::ispa_std::parser_ll::cursor_position(attempt);
		auto node = match(attempt);
		if (((*node)).empty()) {
			matching = false;
		} else {
			if ((((!(has_limit))) && (((python::ispa_std::parser_ll::cursor_position(attempt)) == (before))))) {
				python::ispa_std::failures::logic_error("Unbound counted repetition matched without consuming input");
			}
			(nodes).push_back(node);
			((*cursor)).assign(attempt);
		}
	}
	if (((((has_limit) && (((static_cast<long long>(nodes.size())) != (limit))))) || (((static_cast<long long>(nodes.size())) < (((*spec)).minimum))))) {
		((*this)).reject_absent_rule();
		auto diagnostic = ISPA_STD::construct<python::ispa_std::recovery::Diagnostic>();
		if (((((*this)).frame_count) == (0))) {
			((*diagnostic)).rule = "<rule>";
		} else {
			((*diagnostic)).rule = ((*(((*this)).frames_)[((((*this)).frame_count) - (1))])).rule;
		}
		long long wanted = ((*spec)).minimum;
		if (has_limit) {
			wanted = python::ispa_std::utility::maximum(limit, ((*spec)).minimum);
		}
		std::string label = "";
		if (((static_cast<long long>(((*spec)).name.size())) > (0))) {
			label = ((((" \'") + (((*spec)).name))) + ("\'"));
		}
		((*diagnostic)).message = (((((((((("Counted repetition") + (label))) + (" expected "))) + (python::ispa_std::format::int_to_string(wanted)))) + (" matches, got "))) + (python::ispa_std::format::int_to_string(static_cast<long long>(nodes.size()))));
		if ((!(((*cursor)).isEnd()))) {
			((*diagnostic)).unexpected = python::ispa_std::recovery::token_id(((*cursor)).current());
			((*diagnostic)).offset = ((*((*((*cursor)).current())).meta())).startpos();
		}
		std::string message = ((*diagnostic)).message;
		(((*((*this)).recovery_state_)).diagnostics).push_back(diagnostic);
		python::ispa_std::failures::logic_error(message);
	}
	if (((static_cast<long long>(((*spec)).name.size())) > (0))) {
		((*((*this)).counters_)).try_emplace(((*spec)).name, static_cast<long long>(nodes.size()));
	}
	((*pos)).assign(cursor);
	((*this)).commit_rule(pos);
	return nodes;
}
auto python::ispa_std::parser_ll::LLParser::counted_token(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long type_id, std::shared_ptr<python::ispa_std::counter::CountSpec> spec) -> std::vector<std::shared_ptr<python::ispa_std::node::Node>> {
	return ((*this)).counted_impl(pos, spec, [&](auto attempt) {
return ((*this)).optional_token(attempt, type_id);
});
}
auto python::ispa_std::parser_ll::LLParser::counted_rule(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rec, std::shared_ptr<python::ispa_std::counter::CountSpec> spec) -> std::vector<std::shared_ptr<python::ispa_std::node::Node>> {
	return ((*this)).counted_impl(pos, spec, [&](auto attempt) {
return ((*this)).optional_rule(attempt, rec);
});
}
auto python::ispa_std::parser_ll::LLParser::repeat_optional(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::function<std::shared_ptr<python::ispa_std::node::Node>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> match, bool separated) -> std::shared_ptr<python::ispa_std::node::Node> {
	"Separators belong to a successful next iteration, not to the end of\n        the list: leave pos (including whitespace) intact on absence.";
	auto attempt = ((*pos)).copy();
	if (((((*this)).has_space_token_) && (separated))) {
		((*this)).skip_spaces(attempt);
	}
	long long start = python::ispa_std::parser_ll::cursor_position(attempt);
	auto node = match(attempt);
	if (((((*node)).empty()) || (((python::ispa_std::parser_ll::cursor_position(attempt)) == (start))))) {
		return python::ispa_std::node::Node::blank();
	}
	((*pos)).assign(attempt);
	return node;
}
auto python::ispa_std::parser_ll::LLParser::range_token(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long type_id, std::shared_ptr<python::ispa_std::recovery::FailHandler> fail) -> std::vector<std::shared_ptr<python::ispa_std::node::Node>> {
	std::vector<std::shared_ptr<python::ispa_std::node::Node>> result = std::vector<std::shared_ptr<python::ispa_std::node::Node>>{};
	if (((*fail)).is_recovering()) {
		auto first = ((*this)).required_recovering(pos, type_id, fail);
		if ((!(((*first)).has_node))) {
			return result;
		}
		(result).push_back(((*first)).node);
	} else {
		(result).push_back(((*this)).required_token(pos, type_id, fail));
	}
	if (((*this)).unwinding()) {
		return result;
	}
	bool repeating = true;
	while (((((repeating) && ((!(((*pos)).isEnd()))))) && ((!(((*this)).unwinding()))))) {
		auto node = ((*this)).repeat_optional(pos, [&](auto attempt) {
return ((*this)).optional_token(attempt, type_id);
}, true);
		if (((*node)).empty()) {
			repeating = false;
		} else {
			(result).push_back(node);
		}
	}
	((*this)).sync_loop(pos);
	return result;
}
auto python::ispa_std::parser_ll::LLParser::range_rule(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, const std::string& rule_name, std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rec, std::shared_ptr<python::ispa_std::recovery::FailHandler> fail) -> std::vector<std::shared_ptr<python::ispa_std::node::Node>> {
	long long start = python::ispa_std::parser_ll::cursor_position(pos);
	std::vector<std::shared_ptr<python::ispa_std::node::Node>> result = std::vector<std::shared_ptr<python::ispa_std::node::Node>>{};
	if (((*fail)).is_recovering()) {
		auto first = ((*this)).required_rule_recovering(pos, rec, fail);
		if ((!(((*first)).has_node))) {
			return result;
		}
		(result).push_back(((*first)).node);
	} else {
		(result).push_back(((*this)).required_rule(pos, rule_name, rec, fail));
	}
	if (((((python::ispa_std::parser_ll::cursor_position(pos)) == (start))) || (((*this)).unwinding()))) {
		return result;
	}
	bool repeating = true;
	while (((((repeating) && ((!(((*pos)).isEnd()))))) && ((!(((*this)).unwinding()))))) {
		auto node = ((*this)).repeat_optional(pos, [&](auto attempt) {
return ((*this)).optional_rule(attempt, rec);
}, true);
		if (((*node)).empty()) {
			repeating = false;
		} else {
			(result).push_back(node);
		}
	}
	((*this)).sync_loop(pos);
	return result;
}
auto python::ispa_std::parser_ll::LLParser::range_optional_token(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, long long type_id) -> std::vector<std::shared_ptr<python::ispa_std::node::Node>> {
	std::vector<std::shared_ptr<python::ispa_std::node::Node>> result = std::vector<std::shared_ptr<python::ispa_std::node::Node>>{};
	bool repeating = true;
	while (((((repeating) && ((!(((*pos)).isEnd()))))) && ((!(((*this)).unwinding()))))) {
		auto node = ((*this)).repeat_optional(pos, [&](auto attempt) {
return ((*this)).optional_token(attempt, type_id);
}, ((static_cast<long long>(result.size())) > (0)));
		if (((*node)).empty()) {
			repeating = false;
		} else {
			(result).push_back(node);
		}
	}
	((*this)).sync_loop(pos);
	return result;
}
auto python::ispa_std::parser_ll::LLParser::range_optional_rule(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos, std::function<std::shared_ptr<python::ispa_std::match_result::MatchResult>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>)> rec) -> std::vector<std::shared_ptr<python::ispa_std::node::Node>> {
	std::vector<std::shared_ptr<python::ispa_std::node::Node>> result = std::vector<std::shared_ptr<python::ispa_std::node::Node>>{};
	bool repeating = true;
	while (((((repeating) && ((!(((*pos)).isEnd()))))) && ((!(((*this)).unwinding()))))) {
		auto node = ((*this)).repeat_optional(pos, [&](auto attempt) {
return ((*this)).optional_rule(attempt, rec);
}, ((static_cast<long long>(result.size())) > (0)));
		if (((*node)).empty()) {
			repeating = false;
		} else {
			(result).push_back(node);
		}
	}
	((*this)).sync_loop(pos);
	return result;
}
auto python::ispa_std::parser_ll::LLParser::custom_failure_decision(std::shared_ptr<python::ispa_std::lexer::TokenCursor> before, std::shared_ptr<python::ispa_std::lexer::TokenCursor> after) -> std::shared_ptr<python::ispa_std::recovery::Decision> {
	auto decision = python::ispa_std::recovery::Decision::initial();
	if (((python::ispa_std::parser_ll::cursor_position(before)) == (python::ispa_std::parser_ll::cursor_position(after)))) {
		((*decision)).mode = python::ispa_std::recovery::Mode::Insert;
	} else {
		((*decision)).mode = python::ispa_std::recovery::Mode::Retry;
	}
	return decision;
}
auto python::ispa_std::parser_ll::LLParser::custom_failure(std::function<std::shared_ptr<python::ispa_std::recovery::Decision>(std::shared_ptr<python::ispa_std::lexer::TokenCursor>, std::shared_ptr<python::ispa_std::recovery::State>)> action) -> std::shared_ptr<python::ispa_std::recovery::ActionFailure> {
	return ISPA_STD::construct<python::ispa_std::recovery::ActionFailure>(action);
}
auto python::ispa_std::parser_ll::LLParser::action_result(std::any ast, std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> std::shared_ptr<python::ispa_std::match_result::MatchResult> {
	auto node = python::ispa_std::node::Node::blank();
	((*node)).set_data(ast);
	return ISPA_STD::construct<python::ispa_std::match_result::MatchResult>(true, node, ((*pos)).copy());
}
long long python::ispa_std::lr::LRControlPhase::Decide = 0;
long long python::ispa_std::lr::LRControlPhase::Shift = 1;
long long python::ispa_std::lr::LRControlPhase::Reduce = 2;
long long python::ispa_std::lr::LRControlPhase::Semantic = 3;
long long python::ispa_std::lr::LRControlPhase::Recover = 4;
long long python::ispa_std::lr::LRControlPhase::Accept = 5;
auto python::ispa_std::lr::LRControlState::__init__() -> void {
	((*this)).phase = python::ispa_std::lr::LRControlPhase::Decide;
	((*this)).continuation = 0;
	((*this)).production = 0;
}
auto python::ispa_std::lr::LRRuleSpan::__init__(long long begin, long long end, bool leading, bool trailing, bool empty) -> void {
	((*this)).begin = begin;
	((*this)).end = end;
	((*this)).leading = leading;
	((*this)).trailing = trailing;
	((*this)).empty = empty;
}
auto python::ispa_std::lr::LRRuleSpan::copy() -> std::shared_ptr<python::ispa_std::lr::LRRuleSpan> {
	return ISPA_STD::construct<python::ispa_std::lr::LRRuleSpan>(((*this)).begin, ((*this)).end, ((*this)).leading, ((*this)).trailing, ((*this)).empty);
}
auto python::ispa_std::lr::LRTokenValue::__init__(std::shared_ptr<python::ispa_std::node::Node> node, const std::string& text) -> void {
	((*this)).node = node;
	((*this)).text = text;
}
auto python::ispa_std::lr::lr_token(std::shared_ptr<python::ispa_std::node::Node> node) -> std::shared_ptr<python::ispa_std::lr::LRTokenValue> {
	std::string text = "";
	if (((((*node)).has_source()) && (((((*node)).length()) >= (0))))) {
		text = ((*node)).text();
	}
	return ISPA_STD::construct<python::ispa_std::lr::LRTokenValue>(node, text);
}
auto python::ispa_std::lr::lr_node(std::any value) -> std::shared_ptr<python::ispa_std::node::Node> {
	auto node = python::ispa_std::node::Node::blank();
	((*node)).set_data(value);
	return node;
}
auto python::ispa_std::lr::LRParser::__init__() -> void {
	((*this)).errors_ = {};
}
auto python::ispa_std::lr::LRParser::getErrors() -> const std::vector<std::shared_ptr<python::ispa_std::recovery::Diagnostic>>& {
	return ((*this)).errors_;
}
auto python::ispa_std::lr_conflicts::_Span::__init__(long long begin, long long end, bool leading, bool trailing, bool empty) -> void {
	((*this)).begin = begin;
	((*this)).end = end;
	((*this)).leading = leading;
	((*this)).trailing = trailing;
	((*this)).empty = empty;
}
auto python::ispa_std::lr_conflicts::_Alternative::__init__(long long rule, const std::vector<long long>& children) -> void {
	((*this)).rule = rule;
	((*this)).children = children;
}
auto python::ispa_std::lr_conflicts::_Alternative::equals(std::shared_ptr<python::ispa_std::lr_conflicts::_Alternative> other) -> bool {
	if (((((((*this)).rule) != (((*other)).rule))) || (((static_cast<long long>(((*this)).children.size())) != (static_cast<long long>(((*other)).children.size())))))) {
		return false;
	}
	long long i = 0;
	while (((i) < (static_cast<long long>(((*this)).children.size())))) {
		if ((((((*this)).children)[i]) != ((((*other)).children)[i]))) {
			return false;
		}
		i = ((i) + (1));
	}
	return true;
}
auto python::ispa_std::lr_conflicts::_Visit::__init__(long long node) -> void {
	((*this)).node = node;
	((*this)).alternative = 0;
	((*this)).child = 0;
}
auto python::ispa_std::lr_conflicts::_Forest::__init__(std::shared_ptr<python::ispa_std::lr_conflicts::_Span> span, bool has_token, std::shared_ptr<python::ispa_std::lexer::TokenCursor> token) -> void {
	((*this)).span = span;
	((*this)).has_token = has_token;
	((*this)).token = token;
	((*this)).alternatives = {};
}
auto python::ispa_std::lr_conflicts::_Stack::__init__(long long previous, long long state, long long forest) -> void {
	((*this)).previous = previous;
	((*this)).state = state;
	((*this)).forest = forest;
}
auto python::ispa_std::lr_conflicts::_b(bool flag) -> std::string {
	if (flag) {
		return "1";
	}
	return "0";
}
auto python::ispa_std::lr_conflicts::LRGeneralizedParser::__init__() -> void {
	python::ispa_std::lr::LRParser::__init__();
	((*this)).work = 0;
}
auto python::ispa_std::lr_conflicts::LRGeneralizedParser::_tick(const std::vector<std::shared_ptr<python::ispa_std::lr_conflicts::_Forest>>& forest, const std::vector<std::shared_ptr<python::ispa_std::lr_conflicts::_Stack>>& stacks) -> void {
	((*this)).work = ((((*this)).work) + (1));
	if (((((((*this)).work) > (python::ispa_std::lr_conflicts::WORK_LIMIT))) || (((((static_cast<long long>(forest.size())) + (static_cast<long long>(stacks.size())))) > (python::ispa_std::lr_conflicts::NODE_LIMIT))))) {
		python::ispa_std::failures::runtime_error("LR automatic conflict resolution resource limit exceeded");
	}
}
auto python::ispa_std::lr_conflicts::LRGeneralizedParser::_key(const std::vector<std::string>& parts) -> std::string {
	std::vector<std::string> out = std::vector<std::string>{};
	long long i = 0;
	while (((i) < (static_cast<long long>(parts.size())))) {
		(out).push_back((parts)[i]);
		i = ((i) + (1));
	}
	return python::ispa_std::format::join(out, ",");
}
auto python::ispa_std::lr_conflicts::LRGeneralizedParser::_intern(std::vector<std::shared_ptr<python::ispa_std::lr_conflicts::_Forest>>& forest, std::shared_ptr<python::ispa_std::sortedmap::StrIntMap> index, bool terminal, long long symbol, std::shared_ptr<python::ispa_std::lr_conflicts::_Span> span, bool has_token, std::shared_ptr<python::ispa_std::lexer::TokenCursor> token) -> long long {
	((*this))._tick(forest, {});
	std::string key = ((*this))._key(std::vector<std::string>{python::ispa_std::lr_conflicts::_b(terminal), python::ispa_std::format::int_to_string(symbol), python::ispa_std::format::int_to_string(((*span)).begin), python::ispa_std::format::int_to_string(((*span)).end), python::ispa_std::lr_conflicts::_b(((*span)).leading), python::ispa_std::lr_conflicts::_b(((*span)).trailing), python::ispa_std::lr_conflicts::_b(((*span)).empty)});
	if (((*index)).try_emplace(key, static_cast<long long>(forest.size()))) {
		(forest).push_back(ISPA_STD::construct<python::ispa_std::lr_conflicts::_Forest>(span, has_token, token));
	}
	return ((*index)).get(key);
}
auto python::ispa_std::lr_conflicts::LRGeneralizedParser::_push(std::vector<std::shared_ptr<python::ispa_std::lr_conflicts::_Stack>>& stacks, std::shared_ptr<python::ispa_std::sortedmap::StrIntMap> index, long long previous, long long state, long long value) -> long long {
	((*this))._tick({}, stacks);
	std::string key = ((*this))._key(std::vector<std::string>{python::ispa_std::format::int_to_string(previous), python::ispa_std::format::int_to_string(state), python::ispa_std::format::int_to_string(value)});
	if (((*index)).try_emplace(key, static_cast<long long>(stacks.size()))) {
		(stacks).push_back(ISPA_STD::construct<python::ispa_std::lr_conflicts::_Stack>(previous, state, value));
	}
	return ((*index)).get(key);
}
auto python::ispa_std::lr_conflicts::min2(long long a, long long b) -> long long {
	if (((b) < (a))) {
		return b;
	}
	return a;
}
auto python::ispa_std::external::ExternalResult::__init__(bool status, std::shared_ptr<python::ispa_std::node::Node> node, long long end) -> void {
	((*this)).status = status;
	((*this)).node = node;
	((*this)).end = end;
}
auto python::ispa_std::external::ExternalResult::failed() -> std::shared_ptr<python::ispa_std::external::ExternalResult> {
	return ISPA_STD::construct<python::ispa_std::external::ExternalResult>(false, python::ispa_std::node::Node::blank(), ( - (1)));
}
auto python::ispa_std::external::ExternalBinding::starts(const std::string& origin, long long pos) -> bool {
	return false;
}
auto python::ispa_std::external::ExternalBinding::parse(const std::string& origin, long long pos) -> std::shared_ptr<python::ispa_std::external::ExternalResult> {
	return python::ispa_std::external::ExternalResult::failed();
}
auto python::ispa_std::external::external_node(const std::string& origin, long long start, long long end, std::shared_ptr<python::ispa_std::node::Node> node) -> std::shared_ptr<python::ispa_std::node::Node> {
	"Re-anchor a foreign node to the host source: position, line, column.";
	if (((*node)).empty()) {
		return python::ispa_std::node::Node::blank();
	}
	long long line = 1;
	long long i = 0;
	while (((((i) < (start))) && (((i) < (static_cast<long long>(origin.size())))))) {
		if (((std::string(1, (origin)[i])) == ("\n"))) {
			line = ((line) + (1));
		}
		i = ((i) + (1));
	}
	long long line_start = start;
	while (((((line_start) > (0))) && (((std::string(1, (origin)[((line_start) - (1))])) != ("\n"))))) {
		line_start = ((line_start) - (1));
	}
	return python::ispa_std::node::Node::with_data(start, origin, true, ((end) - (start)), line, ((((start) - (line_start))) + (1)), ((*node)).name(), ((*node)).data());
}
auto python::ispa_std::external::_require_lazy(std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> void {
	if ((!(((*pos)).has_source()))) {
		python::ispa_std::failures::logic_error("AMBIGUOUS eager token stream: external parsers require lazy tokenization");
	}
}
auto python::ispa_std::external::external_starts(std::shared_ptr<python::ispa_std::external::ExternalBinding> binding, std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> bool {
	python::ispa_std::external::_require_lazy(pos);
	return ((*binding)).starts(((*pos)).sourceOrigin(), ((*pos)).sourcePosition());
}
auto python::ispa_std::external::external_match(std::shared_ptr<python::ispa_std::external::ExternalBinding> binding, std::shared_ptr<python::ispa_std::lexer::TokenCursor> pos) -> std::shared_ptr<python::ispa_std::match_result::MatchResult> {
	"Try the binding at pos. On success `pos` is advanced past the foreign text.";
	python::ispa_std::external::_require_lazy(pos);
	if ((!(((*binding)).starts(((*pos)).sourceOrigin(), ((*pos)).sourcePosition())))) {
		return python::ispa_std::match_result::MatchResult::failed(((*pos)).copy());
	}
	std::vector<std::shared_ptr<python::ispa_std::external::ExternalResult>> holder = std::vector<std::shared_ptr<python::ispa_std::external::ExternalResult>>{};
	std::string origin = ((*pos)).sourceOrigin();
	long long begin = ((*pos)).sourcePosition();
	auto failed = ::ISPA_STD::attempt([&]() {
return (holder).push_back(((*binding)).parse(origin, begin));
});
	if (((failed) != (0))) {
		auto message = ::ISPA_STD::failure_message();
		if (((failed) == (python::ispa_std::errors::ErrorKind::RuntimeFailure))) {
			python::ispa_std::failures::external_parse_error(message);
		} else {
			python::ispa_std::failures::rethrow(failed, message);
		}
		return python::ispa_std::match_result::MatchResult::failed(((*pos)).copy());
	}
	auto result = (holder)[0];
	if ((!(((*result)).status))) {
		return python::ispa_std::match_result::MatchResult::failed(((*pos)).copy());
	}
	((*pos)).consumeExternal(((*result)).end);
	return ISPA_STD::construct<python::ispa_std::match_result::MatchResult>(true, ((*result)).node, ((*pos)).copy());
}
auto python::ispa_std::ast_printer::ASTPrinter::__init__() -> void {
	((*this))._out = {};
	((*this))._branches = {};
}
auto python::ispa_std::ast_printer::ASTPrinter::text() -> std::string {
	return python::ispa_std::format::join(((*this))._out, "");
}
auto python::ispa_std::ast_printer::ASTPrinter::up(bool has_more_siblings) -> void {
	(((*this))._branches).push_back(has_more_siblings);
}
auto python::ispa_std::ast_printer::ASTPrinter::down() -> void {
	if (((static_cast<long long>(((*this))._branches.size())) > (0))) {
		ISPA_STD::python_pop((((*this))._branches));
	}
}
auto python::ispa_std::ast_printer::ASTPrinter::_prefix(bool last) -> void {
	long long depth = static_cast<long long>(((*this))._branches.size());
	if (((depth) == (0))) {
		return ;
	}
	long long i = 0;
	while (((((i) + (1))) < (depth))) {
		if ((((*this))._branches)[i]) {
			(((*this))._out).push_back("\xE2\x94\x82   ");
		} else {
			(((*this))._out).push_back("    ");
		}
		i = ((i) + (1));
	}
	if (last) {
		(((*this))._out).push_back("\xE2\x94\x94\xE2\x94\x80\xE2\x94\x80 ");
	} else {
		(((*this))._out).push_back("\xE2\x94\x9C\xE2\x94\x80\xE2\x94\x80 ");
	}
}
auto python::ispa_std::ast_printer::ASTPrinter::node(const std::string& name, bool last) -> void {
	((*this))._prefix(last);
	(((*this))._out).push_back(((name) + ("\n")));
}
auto python::ispa_std::ast_printer::ASTPrinter::scalar(const std::string& name, const std::string& text, bool last) -> void {
	"`name: value` for an already-formatted scalar.";
	((*this))._prefix(last);
	(((*this))._out).push_back(((((((name) + (": "))) + (text))) + ("\n")));
}
auto python::ispa_std::to_string::str_to_string(const std::string& value) -> std::string {
	return value;
}
auto python::ispa_std::to_string::bool_value_to_string(bool value) -> std::string {
	return python::ispa_std::format::bool_to_string(value);
}
auto python::ispa_std::to_string::int_value_to_string(long long value) -> std::string {
	return python::ispa_std::format::int_to_string(value);
}
