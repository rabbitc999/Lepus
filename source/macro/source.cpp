#include "source.hpp"
#include <cctype>
#include <algorithm>

namespace lepus_macro
{
/**
 * @brief 屏蔽注释和字面量以定位编译期指令，不把屏蔽结果交给语言解析器。
 * @param source 保留原文的输入。
 * @param lineComments 非空时收集真实行注释的开始位置。
 * @return 长度和换行完全相同的词法扫描掩码。
 */
static Memory::String directiveMask(const Memory::String &source, std::vector<std::size_t> *lineComments = nullptr)
{
    auto result = source;
    char quote = 0;
    bool block = false, line = false;
    for (std::size_t at = 0; at < source.size(); ++at)
    {
        const char ch = source[at];
        if (line || block || quote)
        {
            if (ch != '\n' && ch != '\r') result[at] = ' ';
            if (line && ch == '\n')
                line = false;
            else if (block && source.compare(at, 2, "*/") == 0)
            {
                result[++at] = ' ';
                block = false;
            }
            else if (quote && ch == '\\' && at + 1 < source.size())
            {
                if (source[at + 1] != '\n') result[++at] = ' ';
            }
            else if (quote && ch == quote)
                quote = 0;
            continue;
        }
        if (ch == '"' || ch == '\'')
        {
            quote = ch;
            result[at] = ' ';
        }
        else if (source.compare(at, 2, "//") == 0)
        {
            if (lineComments) lineComments->push_back(at);
            line = true;
            result[at] = ' ';
            result[++at] = ' ';
        }
        else if (source.compare(at, 2, "/*") == 0)
        {
            block = true;
            result[at] = ' ';
            result[++at] = ' ';
        }
    }
    return result;
}

/**
 * @brief 在源码掩码中跳过当前行内的空白。
 * @param mask 注释及字面量已屏蔽的源码。
 * @param at 扫描起点。
 * @param limit 当前行的结束位置。
 * @return 非空白位置或行末。
 */
static std::size_t skipHeaderSpace(const Memory::String &mask, std::size_t at, std::size_t limit)
{
    while (at < limit && std::isspace(static_cast<unsigned char>(mask[at]))) ++at;
    return at;
}

/**
 * @brief 查找已屏蔽注释和字面量的配对括号。
 * @param mask 源码掩码。
 * @param at 开括号位置。
 * @param limit 扫描上界。
 * @param closing 收括号字符。
 * @return 匹配的收括号位置，无法配对时返回上界。
 */
static std::size_t findClosing(const Memory::String &mask, std::size_t at, std::size_t limit, char closing)
{
    const char opening = mask[at];
    std::size_t depth = 1;
    while (++at < limit)
    {
        if (mask[at] == opening) ++depth;
        if (mask[at] == closing && !--depth) return at;
    }
    return limit;
}

/**
 * @brief 读取宏名及各参数组之后的头部位置。
 * @param mask 源码掩码。
 * @param at 宏定义关键字之后的位置。
 * @param limit 当前行的结束位置。
 * @return 参数组之后的第一个非空白位置。
 */
static std::size_t macroHeadEnd(const Memory::String &mask, std::size_t at, std::size_t limit)
{
    at = skipHeaderSpace(mask, at, limit);
    while (at < limit && (std::isalnum(static_cast<unsigned char>(mask[at])) || mask[at] == '_')) ++at;
    at = skipHeaderSpace(mask, at, limit);
    while (at < limit && (mask[at] == '(' || mask[at] == '[' || mask[at] == '{' || mask[at] == '<'))
    {
        const char closing = mask[at] == '(' ? ')' : mask[at] == '[' ? ']' : mask[at] == '{' ? '}' : '>';
        const auto end = findClosing(mask, at, limit, closing);
        if (end == limit) return limit;
        at = skipHeaderSpace(mask, end + 1, limit);
    }
    return at;
}

/**
 * @brief 定位明确属于宏模板的左花括号，兼容同一位置的旧式换行界线。
 * @param mask 源码掩码。
 * @param word 宏指令。
 * @param after 指令之后的位置。
 * @param limit 当前行的结束位置。
 * @return 模板左括号位置，无花括号写法时返回行末。
 */
static std::size_t braceHeaderOpening(const Memory::String &mask, const Memory::String &word,
                                      std::size_t after, std::size_t limit)
{
    if (word != "$macro") return limit;
    auto at = macroHeadEnd(mask, after, limit);
    if (mask.compare(at, 2, "->") != 0) return limit;
    at = skipHeaderSpace(mask, at + 2, limit);
    if (at < limit && mask[at] == '{') return at;
    for (const auto *result : { "@!macroitem", "@!macromember" })
    {
        const auto size = std::char_traits<char>::length(result);
        if (at + size <= limit && mask.compare(at, size, result) == 0)
        {
            at += size;
            break;
        }
    }
    while (at < limit && (std::isalnum(static_cast<unsigned char>(mask[at])) || mask[at] == '_')) ++at;
    at = skipHeaderSpace(mask, at, limit);
    return at < limit && mask[at] == '{' ? at : limit;
}

/**
 * @brief 判断宏常量声明是否在本行使用独立的分号终结。
 * @param mask 屏蔽后的源码。
 * @param at 指令之后的位置。
 * @param limit 当前行的结束位置。
 * @return 发现未被括号包住的分号时返回真。
 */
static bool hasLetSemicolon(const Memory::String &mask, std::size_t at, std::size_t limit)
{
    while (at < limit)
    {
        switch (mask[at])
        {
            case ';': return true;
            case '}': return false;
            case '(': at = findClosing(mask, at, limit, ')'); break;
            case '[': at = findClosing(mask, at, limit, ']'); break;
            case '{': at = findClosing(mask, at, limit, '}'); break;
            default: break;
        }
        ++at;
    }
    return false;
}

/**
 * @brief 只在宏头将声明结果标记为内部词法项，避免新增普通保留字。
 * @param source 待归一化源码。
 */
static void markDeclarationResults(Memory::String &source)
{
    const auto mask = directiveMask(source);
    std::vector<std::pair<std::size_t, std::size_t>> positions;
    for (std::size_t line = 0; line < mask.size();)
    {
        const auto end = mask.find('\n', line);
        const auto limit = end == Memory::String::npos ? mask.size() : end;
        const auto at = skipHeaderSpace(mask, line, limit);
        if (at < limit && (mask.compare(at, 6, "$macro") == 0 || mask.compare(at, 8, "$replace") == 0))
        {
            const auto arrow = macroHeadEnd(mask, at + (mask.compare(at, 6, "$macro") == 0 ? 6 : 8), limit);
            const auto result = mask.compare(arrow, 2, "->") == 0 ? skipHeaderSpace(mask, arrow + 2, limit) : limit;
            for (const auto length : {4u, 6u})
                if (result + length <= limit && mask.compare(result, length, length == 4 ? "item" : "member") == 0
                    && (result + length == limit || !(std::isalnum(static_cast<unsigned char>(mask[result + length])) || mask[result + length] == '_')))
                    positions.emplace_back(result, length);
        }
        line = end == Memory::String::npos ? mask.size() : end + 1;
    }
    for (auto position = positions.rbegin(); position != positions.rend(); ++position)
        source.replace(position->first, position->second, position->second == 4 ? "@!macroitem" : "@!macromember");
}

/**
 * @brief 给单行宏插入词法边界，不展开正文也不为表达式补括号。
 * @param source 待处理源码。
 * @param error 接收无正文或头部括号错误。
 * @return 边界标记成功时返回真。
 */
bool markReplacementBoundaries(Memory::String &source, std::string &error)
{
    markDeclarationResults(source);
    error.clear();
    std::vector<std::size_t> comments;
    const auto mask = directiveMask(source, &comments);
    std::vector<std::pair<std::size_t, Memory::String>> insertions;
    for (std::size_t at = 0; at < mask.size(); ++at)
    {
        if (mask.compare(at, 8, "$replace") != 0 || (at + 8 < mask.size() && (std::isalnum(static_cast<unsigned char>(mask[at + 8])) || mask[at + 8] == '_'))) continue;
        auto end = mask.find('\n', at);
        if (end == Memory::String::npos) end = mask.size();
        auto header = at + 8;
        const auto spaces = [&] { while (header < end && std::isspace(static_cast<unsigned char>(source[header]))) ++header; };
        spaces();
        if (header == end || !(std::isalpha(static_cast<unsigned char>(mask[header])) || mask[header] == '_'))
        {
            error = "单行宏缺少宏名称";
            return false;
        }
        while (header < end && (std::isalnum(static_cast<unsigned char>(mask[header])) || mask[header] == '_')) ++header;
        spaces();
        bool generic = true;
        while (header < end && (mask[header] == '(' || mask[header] == '[' || mask[header] == '{' || (generic && mask[header] == '<')))
        {
            const char opening = mask[header];
            const char closing = opening == '(' ? ')' : opening == '[' ? ']' :
                                                    opening == '{'     ? '}' :
                                                                         '>';
            const auto close = mask.find(closing, header + 1);
            if (close == Memory::String::npos || close >= end)
            {
                error = "单行宏的形参组缺少结束括号";
                return false;
            }
            bool parameters = true;
            for (auto index = header + 1; index < close; ++index)
            {
                if (opening == '<' && mask.compare(index, 4, "#res") == 0)
                {
                    auto before = index;
                    while (before > header + 1 && std::isspace(static_cast<unsigned char>(mask[before - 1]))) --before;
                    if (before == header + 1 || mask[before - 1] != ':' ||
                        (index + 4 < close && (std::isalnum(static_cast<unsigned char>(mask[index + 4])) || mask[index + 4] == '_')))
                        parameters = false;
                    index += 3;
                }
                else if (opening != '<' && mask[index] == '&')
                {
                    auto before = index;
                    while (before > header + 1 && std::isspace(static_cast<unsigned char>(mask[before - 1]))) --before;
                    if (before >= header + 4 && mask.compare(before - 3, 3, "mut") == 0) before -= 3;
                    while (before > header + 1 && std::isspace(static_cast<unsigned char>(mask[before - 1]))) --before;
                    if (before != header + 1 && mask[before - 1] != ',') parameters = false;
                }
                else if (!(std::isalnum(static_cast<unsigned char>(mask[index])) || mask[index] == '_' || mask[index] == ',' || mask[index] == '.' || std::isspace(static_cast<unsigned char>(mask[index])) || mask[index] == ':'))
                    parameters = false;
            }
            if (!parameters) break;
            header = close + 1;
            generic = false;
            spaces();
        }
        if (mask.compare(header, 2, "->") == 0)
        {
            header += 2;
            spaces();
            if (mask.compare(header, 11, "@!macroitem") == 0) header += 11;
            else if (mask.compare(header, 13, "@!macromember") == 0) header += 13;
            else while (header < end && (std::isalnum(static_cast<unsigned char>(mask[header])) || mask[header] == '_')) ++header;
            spaces();
        }
        if (header >= end)
        {
            error = "单行宏必须在同一行提供正文";
            return false;
        }
        // 行尾注释必须留在结束标记之后；字面量中的斜线已被掩码屏蔽。
        auto bodyEnd = end;
        const auto comment = std::lower_bound(comments.begin(), comments.end(), header);
        if (comment != comments.end() && *comment < end) bodyEnd = *comment;
        insertions.push_back({ header, " @!macrobody " });
        insertions.push_back({ bodyEnd, " @!macroreplaceend " });
        at = end;
    }
    for (auto item = insertions.rbegin(); item != insertions.rend(); ++item) source.insert(item->first, item->second);
    return true;
}
/**
 * @brief 扫描注释与字符串之外的宏定义及分支头，为解析器保留换行边界。
 * @param source 原地添加内部头部结束标记的源码。
 */
void markHeaderBoundaries(Memory::String &source)
{
    markDeclarationResults(source);
    const auto mask = directiveMask(source);
    std::vector<std::pair<std::size_t, const char *>> positions;
    bool blockComment = false;
    bool lineComment = false;
    char quote = 0;
    bool header = false;
    std::size_t headerComment = Memory::String::npos;
    for (std::size_t at = 0; at < source.size(); ++at)
    {
        const char ch = source[at];
        if (ch == '\n')
        {
            lineComment = false;
            if (header && !quote && !blockComment)
            {
                positions.emplace_back(headerComment == Memory::String::npos ? at : headerComment, " @!macrobody ");
                header = false;
            }
            headerComment = Memory::String::npos;
            continue;
        }
        if (lineComment) continue;
        if (blockComment)
        {
            if (source.compare(at, 2, "*/") == 0)
            {
                blockComment = false;
                ++at;
            }
            continue;
        }
        if (quote)
        {
            if (ch == '\\')
                ++at;
            else if (ch == quote)
                quote = 0;
            continue;
        }
        if (ch == '\'' || ch == '"')
            quote = ch;
        else if (source.compare(at, 2, "//") == 0)
        {
            lineComment = true;
            if (header) headerComment = at;
        }
        else if (source.compare(at, 2, "/*") == 0)
            blockComment = true;
        else if (ch == '$')
        {
            constexpr std::size_t length = 4;
            const auto end = at + length;
            if (source.compare(at, length, "$let") == 0
                && (end == source.size() || !(std::isalnum(static_cast<unsigned char>(source[end])) || source[end] == '_'))
                && source.compare(end, 2, "::") != 0)
            {
                const auto newline = mask.find('\n', end);
                const auto limit = newline == Memory::String::npos ? mask.size() : newline;
                if (!hasLetSemicolon(mask, end, limit)) header = true;
            }
        }
    }
    if (header) positions.emplace_back(source.size(), " @!macrobody ");
    for (auto position = positions.rbegin(); position != positions.rend(); ++position)
        source.insert(position->first, position->second);
}

/**
 * @brief 隐藏字符串和注释之外的完整宏定义，保持宏体原文及原有换行。
 * @param source 含定义的源码。
 * @param error 接收宏结构错误。
 * @return 定义全部闭合且不嵌套时返回真。
 */
bool ProtectedDefinitions::hide(Memory::String &source, std::string &error)
{
    definitions_.clear();
    error.clear();
    const auto mask = directiveMask(source);
    constexpr std::string_view replacementEnd = "@!macroreplaceend";
    std::vector<std::size_t> starts;
    std::size_t begin = Memory::String::npos, braceClose = Memory::String::npos;
    const auto protect = [&](std::size_t end) {
        const auto marker = Memory::String("/*@!m") + Memory::toString(definitions_.size()) + "*/";
        definitions_.push_back({ marker, source.substr(begin, end - begin) });
        starts.push_back(begin);
        begin = braceClose = Memory::String::npos;
    };
    for (std::size_t at = 0; at < mask.size(); ++at)
    {
        if (at == braceClose)
        {
            protect(at + 1);
            continue;
        }
        if (begin != Memory::String::npos && braceClose == Memory::String::npos
            && mask.compare(at, replacementEnd.size(), replacementEnd.data()) == 0)
        {
            protect(at + replacementEnd.size());
            at += replacementEnd.size() - 1;
            continue;
        }
        if (mask[at] != '$') continue;
        auto end = at + 1;
        while (end < mask.size() && (std::isalnum(static_cast<unsigned char>(mask[end])) || mask[end] == '_')) ++end;
        const auto word = mask.substr(at, end - at);
        if (mask.compare(end, 2, "::") == 0) continue;
        if (word == "$macro" || word == "$replace")
        {
            if (begin != Memory::String::npos)
            {
                error = "不支持嵌套宏定义";
                return false;
            }
            begin = at;
            if (word == "$macro")
            {
                const auto newline = mask.find('\n', end);
                const auto limit = newline == Memory::String::npos ? mask.size() : newline;
                const auto arrow = macroHeadEnd(mask, end, limit);
                if (mask.compare(arrow, 2, "=>") == 0)
                {
                    const auto brace = skipHeaderSpace(mask, arrow + 2, limit);
                    if (brace < limit && mask[brace] == '{')
                    {
                        error = "花括号宏定义使用 -> 引出模板";
                        return false;
                    }
                }
                const auto opening = braceHeaderOpening(mask, word, end, limit);
                if (opening == limit)
                {
                    error = "宏定义需要 -> 和花括号模板";
                    return false;
                }
                if (opening < limit)
                {
                    braceClose = findClosing(mask, opening, mask.size(), '}');
                    if (braceClose == mask.size())
                    {
                        error = "花括号宏定义缺少右花括号";
                        return false;
                    }
                }
            }
        }
        at = end - 1;
    }
    if (begin != Memory::String::npos)
    {
        error = "单行宏缺少词法结束标记";
        return false;
    }
    for (std::size_t index = starts.size(); index-- > 0;)
    {
        const auto at = starts[index];
        const auto &definition = definitions_[index];
        for (auto pos = at; pos < at + definition.second.size(); ++pos)
            if (source[pos] != '\n' && source[pos] != '\r') source[pos] = ' ';
        source.insert(at, definition.first);
    }
    return true;
}

/**
 * @brief 根据唯一注释占位恢复模板，不重新生成宏体或改写其内容。
 * @param source 普通闭包及泛型归一化后的源码。
 * @param error 接收丢失占位的错误。
 * @return 每个定义恢复成功时返回真。
 */
bool ProtectedDefinitions::restore(Memory::String &source, std::string &error) const
{
    error.clear();
    for (const auto &definition : definitions_)
    {
        const auto at = source.find(definition.first);
        if (at == Memory::String::npos || source.size() - at < definition.second.size() + definition.first.size())
        {
            error = "普通源码归一化丢失了宏定义占位";
            return false;
        }
        source.replace(at, definition.second.size() + definition.first.size(), definition.second);
    }
    return true;
}
}  // namespace lepus_macro
