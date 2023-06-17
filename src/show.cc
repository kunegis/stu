#include "show.hh"

Quotable Part::need_quotes() const
{
	TRACE_FUNCTION(SHOW, Part::need_quotes());
	assert(is_quotable());
	if (properties == PROP_OPERATOR_QUOTABLE) {
		return Q_NO;
	} else if (properties == PROP_TEXT) {
		TRACE("%s", "PROP_TEXT");
		TRACE("text=%s", text);
		Quotable ret= Q_MIN;
		for (char c:  text) {
			ret= max(ret, need_quotes(c));
		}
		TRACE("ret = %s", frmt("%d", ret));
		return ret;
	} else {
		assert(false);
		return Q_MIN;
	}
}

Quotable Part::need_quotes(unsigned char c)
{
	if (c & 0x80)
		return Q_NO;
	if (c <= 0x20 || c == 0x7F)
		return Q_COLOR;
	if (c == '\\' || c == '"' || c == '\'' || c == '$')
		return Q_NO_COLOR;
	return Q_NO;
}

void Part::show(string &ret) const
{
	assert(! is_quotable());
	ret += text;
}

void Part::show(string &ret, bool quotes) const
{
	assert(is_quotable());
	if (properties == PROP_OPERATOR_QUOTABLE) {
		ret += text;
		return;
	}
	if (!quotes) {
		ret += text;
		return;
	}
	for (unsigned char c:  text) {
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
	bool quotes_outer= Color::quotes[channel];
	string ret= Color::highlight_on[channel];

	bool has_marker= false;
	size_t i;
	for (i= 0;  i < parts.size();  ++i)
		if (parts[i].is_operator())
			has_marker= true;
	
	for (i= 0;  i < parts.size();) {
		if (parts[i].is_quotable()) {
			size_t j;
			Quotable quotable= Q_MIN;
			if (style == S_ALWAYS_QUOTE)
				quotable= Q_COLOR;
			bool empty= true;//parts[i].text.empty();
			for (j= i;  j < parts.size() && parts[j].is_quotable();  ++j) {
				if (quotable <= Q_MAX)
					quotable= max(quotable, parts[j].need_quotes());
				if (!parts[j].text.empty())
					empty= false;
			}
			bool quotes= empty
				|| quotable == Q_COLOR
				|| (quotes_outer && !has_marker && !(style & S_QUOTE_MINIMUM))
				|| (quotes_outer && has_marker && quotable == Q_NO_COLOR && !(style & S_QUOTE_MINIMUM));
			if (quotes)  ret += '"';
			for (;  i < j;  ++i)
				parts[i].show(ret, quotes);
			if (quotes)  ret += '"';
		} else {
			parts[i++].show(ret);
		}
	}
	
	ret += Color::highlight_off[channel];
	return ret;
}

string show_operator(char c, Style style)
{
	Parts parts;
	parts.append_operator_unquotable(c);
	return show(parts, style); 
}

string show_operator(string s, Style style)
{
	Parts parts;
	parts.append_operator_unquotable(s);
	return show(parts, style); 
}

void render_dynamic_variable(string name, Parts &parts, Rendering)
{
	parts.append_operator_unquotable("$[");
	parts.append_text(name);
	parts.append_operator_unquotable("]");
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
void render_prefix(string prefix, const T &object, Parts &parts, Rendering rendering)
{
	TRACE_FUNCTION(SHOW, render_prefix);
	parts.append_operator_unquotable(prefix);
	render(object, parts, rendering);
}

template <typename T>
string show_prefix(string prefix, const T &object, Style style)
{
	Parts parts;
	render_prefix(prefix, object, parts);
	return show(parts, style);
}

template <typename T>
string show(const T &object, Style style, Rendering rendering)
{
	Parts parts;
	render(object, parts, rendering);
	return show(parts, style);
}
