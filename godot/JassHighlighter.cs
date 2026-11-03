using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using Godot;

namespace HiveWeGodot;

public static class JassHighlighter
{
    private static readonly Color NumberColor = new(181 / 255f, 206 / 255f, 168 / 255f);
    private static readonly Color KeywordColor = new(56 / 255f, 156 / 255f, 214 / 255f);
    private static readonly Color StringColor = new(214 / 255f, 157 / 255f, 133 / 255f);
    private static readonly Color CommentColor = new(87 / 255f, 166 / 255f, 74 / 255f);
    private static readonly Color NativeColor = new(189 / 255f, 99 / 255f, 197 / 255f);
    private static readonly Color FunctionColor = new(200 / 255f, 100 / 255f, 100 / 255f);
    private static readonly Color ConstantColor = new(184 / 255f, 215 / 255f, 163 / 255f);
    private static readonly Color TypeColor = new(78 / 255f, 201 / 255f, 176 / 255f);

    private static readonly string[] Keywords =
    {
        "call", "constant", "debug", "else", "elseif", "endfunction", "endif", "endglobals", "endloop", "endstruct",
        "exitwhen", "extends", "function", "globals", "if", "interface", "local", "loop", "module", "native",
        "nothing", "return", "returns", "set", "struct", "takes", "then", "type", "array", "and", "or", "not",
    };

    private static readonly string[] BaseTypes =
    {
        "integer", "real", "boolean", "string", "code", "handle", "nothing",
    };

    private static readonly Regex NativePattern = new(@"\bnative\s+(\w+)\b", RegexOptions.Compiled);
    private static readonly Regex FunctionPattern = new(@"\bfunction\s+(\w+)\b", RegexOptions.Compiled);
    private static readonly Regex TypePattern = new(@"\b(?:type|struct)\s+(\w+)\b", RegexOptions.Compiled);
    private static readonly Regex ConstantPattern = new(@"\bconstant\s+(\w+)\s+(\w+)\b", RegexOptions.Compiled);

    public static CodeHighlighter Build(string script)
    {
        var highlighter = new CodeHighlighter
        {
            NumberColor = NumberColor,
            SymbolColor = new Color(0.75f, 0.75f, 0.75f),
            FunctionColor = FunctionColor,

        };

        // 先铺关键词和基础类型，具体声明覆盖在上面，和上游的判定顺序一致
        foreach (string keyword in Keywords)
        {
            highlighter.AddKeywordColor(keyword, KeywordColor);
        }
        foreach (string type in BaseTypes)
        {
            highlighter.AddKeywordColor(type, TypeColor);
        }

        var types = new HashSet<string>(BaseTypes);
        foreach (Match match in TypePattern.Matches(script))
        {
            string name = match.Groups[1].Value;
            if (types.Add(name))
            {
                highlighter.AddKeywordColor(name, TypeColor);
            }
        }

        foreach (Match match in ConstantPattern.Matches(script))
        {
            string type = match.Groups[1].Value;
            string name = match.Groups[2].Value;
            if (name == "function" || name == "native")
            {
                continue;
            }
            if (types.Contains(type) || BaseTypesAsSet().Contains(type))
            {
                highlighter.AddKeywordColor(name, ConstantColor);
            }
        }

        foreach (Match match in FunctionPattern.Matches(script))
        {
            highlighter.AddKeywordColor(match.Groups[1].Value, FunctionColor);
        }

        foreach (Match match in NativePattern.Matches(script))
        {
            highlighter.AddKeywordColor(match.Groups[1].Value, NativeColor);
        }

        highlighter.AddColorRegion("//", string.Empty, CommentColor);
        highlighter.AddColorRegion("/*", "*/", CommentColor);
        highlighter.AddColorRegion("\"", "\"", StringColor);
        return highlighter;
    }

    private static HashSet<string> BaseTypesAsSet() => new(BaseTypes);
}
