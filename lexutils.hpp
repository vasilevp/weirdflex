#pragma once
#include <string>
#include <tuple>

#ifdef _DEBUG
#define WITH_LOG(t) (printf("token '%s', value '%s'\n", #t, yytext), t)
#else
#define WITH_LOG(t) (t)
#endif

#define SAVE_TOKEN (yylval.string = new std::string(yytext, yyleng))
#define TOKEN(t) (yylval.token = t)
extern "C" int yywrap() { return 1; }
extern "C" void yyterminate();

auto unescape(const std::string &s)
{
    std::string res;
    res.reserve(s.size());

    for (auto it = s.begin(); it != s.end();)
    {
        char c = *it++;
        if (c == '\\' && it != s.end())
        {
            switch (auto next = *it++)
            {
            case '\\':
                c = '\\';
                break;
            case '\'':
                c = '\'';
                break;
            case '\"':
                c = '\"';
                break;
            case '?':
                c = '\?';
                break;
            case 'a':
                c = '\a';
                break;
            case 'b':
                c = '\b';
                break;
            case 'f':
                c = '\f';
                break;
            case 'n':
                c = '\n';
                break;
            case 'r':
                c = '\r';
                break;
            case 't':
                c = '\t';
                break;
            case 'v':
                c = '\v';
                break;
            default:
                printf("Unknown escape sequence: '\\%c'\n", next);
                return std::make_tuple(res, false);
            }
        }
        res += c;
    }

    return std::make_tuple(res, true);
}