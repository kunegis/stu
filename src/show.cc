#include "show.hh"

Quote_Safeness Part::need_quotes() const
{
	assert(is_quotable());
	if (properties == PROP_MARKUP_QUOTABLE) {
		return QS_SAFE;
	} else if (properties == PROP_TEXT) {
		Quote_Safeness ret= QS_MAX;
		for (char c: text)
			ret= std::min(ret, need_quotes(c));
		return ret;
	} else {
		should_not_happen();
		return QS_MAX;
	}
}

Quote_Safeness Part::need_quotes(unsigned char c)
{
	if (c & 0x80)
		return QS_SAFE;
	if (c <= 0x20 || c == 0x7F)
		return QS_ALWAYS_QUOTE;
	if (c == '\\' || c == '"' || c == '\'' || c == '$')
		return QS_QUOTE_IN_STU_CODE;
	if (c == '*' || c == '?' || c == '#')
		return QS_QUOTE_IN_GLOB_PATTERN;
	return QS_SAFE;
}

void Part::show(string &ret) const
{
	assert(! is_quotable());
	ret += text;
}

void Part::show(string &ret, bool quotes) const
{
	TRACE_FUNCTION(SHOW, Part::show(string &, bool));
	TRACE("quotes=%s", frmt("%u", (unsigned)quotes));
	assert(is_quotable());
	if (properties == PROP_MARKUP_QUOTABLE) {
		ret += text;
		return;
	}
	if (!quotes) {
		ret += text;
		return;
	}
	for (unsigned char c: text) {
		if (c == '\\' || c == '"' || c == '$') {
			ret += '\\';
			ret += c;
		} else if (c == '\0') {
			ret += "\\0";
		} else if (c == '\a') {
			ret += "\\a";
		} else if (c == '\b') {
			ret += "\\b";
		} else if (c == '\f') {
			ret += "\\f";
		} else if (c == '\n') {
			ret += "\\n";
		} else if (c == '\r') {
			ret += "\\r";
		} else if (c == '\t') {
			ret += "\\t";
		} else if (c == '\v') {
			ret += "\\v";
		} else if (c >= 0x20 && c != 0x7F) {
			ret += c;
		} else {
			ret += frmt("\\x%02X", c);
		}
	}
}

string show(const Parts &parts, Style style)
{
	Channel channel= (Channel)(style & S_CHANNEL);
	bool nocolor= Color::nocolor[channel] || style & S_NO_COLOR;
	string ret;
	if (! nocolor)
		ret += Color::highlight_on[channel];

	// TODO make this less indented
	for (size_t i= 0; i < parts.size();) {
		if (parts[i].properties == PROP_SPACE) {
			parts[i++].show(ret);
		} else {
			bool has_marker= false;
			size_t j;
			for (j= i; j < parts.size() && parts[j].properties != PROP_SPACE; ++j)
				if (parts[j].properties != PROP_TEXT)
					has_marker= true;
			while (i < j) {
				if (parts[i].properties == PROP_OPERATOR) {
					parts[i++].show(ret);
				} else {
					bool empty= true;
					size_t k;
					for (k= i; k < j && parts[k].properties != PROP_OPERATOR; ++k) {
						if (! parts[k].text.empty())
							empty= false;
					}
					while (i < k) {
						if (! parts[i].is_quotable()) {
							parts[i++].show(ret);
						} else {
							size_t l;
							Quote_Safeness quotable= QS_MAX;
							if (style == S_ALWAYS_QUOTE)
								quotable= QS_ALWAYS_QUOTE;
							for (l= i; l < k && parts[l].is_quotable(); ++l) {
								if (quotable > QS_MIN)
									quotable= std::min(quotable, parts[l].need_quotes());
							}
							bool quotes= empty
								|| quotable == QS_ALWAYS_QUOTE
								|| style & S_QUOTE_SOURCE && quotable <= QS_QUOTE_IN_GLOB_PATTERN
								|| !(style & S_QUOTE_SOURCE) && nocolor && !has_marker && !(style & S_QUOTE_MINIMUM)
								|| !(style & S_QUOTE_SOURCE) && nocolor && has_marker && quotable <= QS_QUOTE_IN_STU_CODE && !(style & S_QUOTE_MINIMUM);
							if (quotes)  ret += '"';
							for (; i < l; ++i)
								parts[i].show(ret, quotes);
							if (quotes)  ret += '"';
						}
					}
				}
			}
		}
	}

	if (! nocolor)
		ret += Color::highlight_off[channel];
	return ret;
}

string show_operator(char c, Style style)
{
	Parts parts;
	parts.append_operator(string(1, c));
	return show(parts, style);
}

string show_operator(string s, Style style)
{
	Parts parts;
	parts.append_operator(s);
	return show(parts, style);
}

string show_text(string text, Style style)
{
	Parts parts;
	parts.append_text(text);
	return show(parts, style);
}

void render_dynamic_variable(string name, Parts &parts, Rendering)
{
	parts.append_operator("$[");
	parts.append_text(name);
	parts.append_operator("]");
}

string show_dynamic_variable(string name, Style style)
{
	Parts parts;
	render_dynamic_variable(name, parts);
	return show(parts, style);
}

void render(string s, Parts &parts, Rendering)
{
	parts.append_text(s);
}

template <typename T>
string show(const T &object, Style style, Rendering rendering)
{
	Parts parts;
	render(object, parts, rendering);
	return show(parts, style);
}

template <typename T>
void render_prefix(string prefix, const T &object, Parts &parts, Rendering rendering)
{
	TRACE_FUNCTION(SHOW, render_prefix);
	parts.append_operator(prefix);
	render(object, parts, rendering);
}

template <typename T>
string show_prefix(string prefix, const T &object, Style style)
{
	Parts parts;
	render_prefix(prefix, object, parts);
	return show(parts, style);
}
