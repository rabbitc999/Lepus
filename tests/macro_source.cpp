#include "macro/source.hpp"
#include <algorithm>
#include <iostream>

/**
 * @brief 验证宏头词法边界标记保留宏体、注释、字符串及源码行数。
 * @return 所有边界保持规则通过时返回零。
 */
int main()
{
    Memory::String source = "$macro A() -> { // declaration\nvalue + 1\n}\n"
                            "// $macro fake()\n/* $macro fake()\n */\n"
                            "String s = \"$macro fake()\";\n"
                            "$macro B -> {\n{ int32 local = 1; }\n}\n";
    const auto original = source;
    lepus_macro::markHeaderBoundaries(source);
    if (source != original) return 1;
    Memory::String lets = "$let value = 42\n$let other = 1;\n";
    lepus_macro::markHeaderBoundaries(lets);
    if (lets != "$let value = 42 @!macrobody \n$let other = 1;\n") return 2;
    lepus_macro::ProtectedDefinitions definitions;
    Memory::String protectedSource = original;
    std::string error;
    if (!definitions.hide(protectedSource, error) || !error.empty()) return 8;
    if (protectedSource.find("value + 1") != Memory::String::npos || protectedSource.find("$macro A") != Memory::String::npos) return 9;
    if (std::count(protectedSource.begin(), protectedSource.end(), '\n') != std::count(original.begin(), original.end(), '\n')) return 10;
    protectedSource = "// relocated\n" + protectedSource + "\nint64 generated() { return 1; }\n";
    if (!definitions.restore(protectedSource, error) || protectedSource != "// relocated\n" + original + "\nint64 generated() { return 1; }\n") return 11;
    Memory::String replacement = "$replace ADD<T:a>(a, b) a + b // comment\n$replace URL() \"https://example\"\n";
    if (!lepus_macro::markReplacementBoundaries(replacement, error)) return 12;
    if (replacement.find("$replace ADD<T:a>(a, b)  @!macrobody a + b  @!macroreplaceend // comment") == Memory::String::npos) return 13;
    if (replacement.find("\"https://example\" @!macroreplaceend") == Memory::String::npos) return 14;
    Memory::String resultSource = "$replace Example<T: #res, U:b>(a,b) cast<T>(a) + cast<T>(b)\n";
    if (!lepus_macro::markReplacementBoundaries(resultSource, error)) return 15;
    if (resultSource.find("(a,b)  @!macrobody cast<T>(a)") == Memory::String::npos) return 16;
    Memory::String categorySource = "$replace Inline(x:expr) x+x\n";
    if (!lepus_macro::markReplacementBoundaries(categorySource, error)) return 17;
    if (categorySource.find("(x:expr)  @!macrobody x+x") == Memory::String::npos) return 18;
    Memory::String itemSource = "$macro Make(name:ident) -> item {\nint64 name(){return 42;}\n}\n";
    lepus_macro::markHeaderBoundaries(itemSource);
    if (itemSource.find("-> @!macroitem {") == Memory::String::npos) return 19;
    Memory::String inlineItem = "$replace Make(name:ident) -> item int64 name(){return 42;}\n";
    if (!lepus_macro::markReplacementBoundaries(inlineItem, error)) return 20;
    if (inlineItem.find("-> @!macroitem  @!macrobody int64") == Memory::String::npos) return 21;
    Memory::String bodyArrow = "$replace Member(x) x->item\n";
    if (!lepus_macro::markReplacementBoundaries(bodyArrow, error)) return 22;
    if (bodyArrow.find("x->item") == Memory::String::npos) return 23;
    Memory::String memberSource = "$macro Make(name:ident) -> member {\nint64 name(){return 42;}\n}\n";
    lepus_macro::markHeaderBoundaries(memberSource);
    if (memberSource.find("-> @!macromember {") == Memory::String::npos) return 24;
    Memory::String inlineMember = "$replace Make(name:ident) -> member int64 name(){return 42;}\n";
    if (!lepus_macro::markReplacementBoundaries(inlineMember, error)) return 25;
    if (inlineMember.find("-> @!macromember  @!macrobody int64") == Memory::String::npos) return 26;
    const Memory::String braced = "$macro Choose[x]{fallback} -> {\n"
                                   "$if (x) { \"}\" } $else { fallback }\n"
                                   "/* } */\n"
                                   "}\n"
                                   "int64 after() { return 1; }\n";
    auto bracedSource = braced;
    if (!definitions.hide(bracedSource, error) || bracedSource.find("int64 after") == Memory::String::npos) return 27;
    if (!definitions.restore(bracedSource, error) || bracedSource != braced) return 28;
    lepus_macro::markHeaderBoundaries(bracedSource);
    if (bracedSource.find("@!macrobody") != Memory::String::npos || bracedSource != braced) return 29;
    bracedSource = "$macro Missing() -> {\n";
    if (definitions.hide(bracedSource, error) || error != "花括号宏定义缺少右花括号") return 30;
    std::cout << "macro declaration boundaries passed\n";
    return 0;
}
