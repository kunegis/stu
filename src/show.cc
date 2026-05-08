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
	TRACE_FUNCTION();
	TRACE("quotes= %s", frmt("%d", (int)quotes));
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
	TRACE_FUNCTION();
	Channel channel= (Channel)(style & S_CHANNEL);
	TRACE("channel= %s", frmt("%d", channel));
	bool nocolor= Color::nocolor[channel] || style & S_NO_COLOR;
	TRACE("nocolor= %s", frmt("%d", nocolor));
	string ret;
	if (! nocolor)
		ret += Color::highlight_on[channel];

	for (size_t i= 0; i < parts.size();) {
		if (parts[i].properties == PROP_SPACE ||
			parts[i].properties == PROP_OPERATOR) {
			parts[i++].show(ret);
			continue;
		}
		bool has_marker= false;
		size_t j;
		for (j= i; j < parts.size() && parts[j].properties != PROP_SPACE && parts[j].properties != PROP_OPERATOR; ++j)
			if (parts[j].properties != PROP_TEXT) {
				TRACE("parts[%s]= %s has_marker= true", frmt("%zu", j), show(parts[j].text));
				has_marker= true;
			}
		TRACE("Contiguous non-spaces range= %s, has_marker= %s",
			frmt("[%zu, %zu[", i, j), frmt("%d", has_marker));
		while (i < j) {
			if (parts[i].properties == PROP_MARKER) {
				parts[i++].show(ret);
				continue;
			}
			bool empty= true;
			size_t k;
			for (k= i; k < j && parts[k].properties != PROP_MARKER; ++k) {
				if (! parts[k].text.empty())
					empty= false;
			}
			TRACE("Contiguous non-space/operator range= %s",
				frmt("[%zu, %zu[", i, k));
			while (i < k) {
				if (! parts[i].is_quotable()) {
					parts[i++].show(ret);
					continue;
				}
				size_t l;
				Quote_Safeness quotable= QS_MAX;
				if (style == S_ALWAYS_QUOTE)
					quotable= QS_ALWAYS_QUOTE;
				for (l= i; l < k && parts[l].is_quotable(); ++l) {
					if (quotable > QS_MIN)
						quotable= std::min(quotable,
							parts[l].need_quotes());
				}
				TRACE("Contiguous quotable range= %s",
					frmt("[%zu, %zu[", i, l));
				bool quotes= empty
					|| quotable == QS_ALWAYS_QUOTE
					|| style & S_QUOTE_SOURCE
					&& quotable <= QS_QUOTE_IN_GLOB_PATTERN
					|| !(style & S_QUOTE_SOURCE)
					&& nocolor && !has_marker
					&& !(style & S_QUOTE_MINIMUM)
					|| !(style & S_QUOTE_SOURCE)
					&& nocolor && has_marker
					&& quotable <= QS_QUOTE_IN_STU_CODE
					&& !(style & S_QUOTE_MINIMUM);
				if (quotes)  ret += '"';
				for (; i < l; ++i)
					parts[i].show(ret, quotes);
				if (quotes)  ret += '"';
			}
		}
	}

	if (! nocolor)
		ret += Color::highlight_off[channel];
	return ret;
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
void render(Prefix_View <T> prefix_view, Parts &parts, Rendering rendering)
{
	parts.append_marker(prefix_view.prefix);
	render(prefix_view.object, parts, rendering);
}

void render(Operator_View operator_view, Parts &parts, Rendering)
{
	parts.append_operator(operator_view.op);
}
