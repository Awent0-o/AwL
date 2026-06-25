#include "lexer.h"

void addToken(TokenList *list, TokenType type, const char *text, int line) {
    if (list->count >= list->capacity) {
        list->capacity = list->capacity ? list->capacity * 2 : 64;
        list->tokens = realloc(list->tokens, list->capacity * sizeof(Token));
    }
    Token *t = &list->tokens[list->count++];
    t->type = type;
    strncpy(t->text, text, sizeof(t->text) - 1);
    t->line = line;
}

TokenList tokenize(const char *src, size_t len) {
    TokenList list = {0};
    size_t pos = 0;
    int line = 1;

    while (pos < len) {
        char c = src[pos];

        if (c == '\n') { line++; pos++; continue; }
        if (isspace(c)) { pos++; continue; }

        //строкі
        if (c == '"') {
            pos++; // пропустити відкриваючу "
            size_t start = pos;
            while (pos < len && src[pos] != '"') {
                pos++;
            }
            char buf[256];
            size_t n = pos - start;
            if (n >= sizeof(buf)) n = sizeof(buf) - 1;
            memcpy(buf, src + start, n);
            buf[n] = '\0';
        
            if (pos < len) pos++; // пропустити закриваючу "
            addToken(&list, TOK_STRING, buf, line);
            continue;
        }

        // числа
        if (isdigit(c)) {
            size_t start = pos;
            while (pos < len && isdigit(src[pos])) pos++;
            char buf[64];
            size_t n = pos - start;
            memcpy(buf, src + start, n);
            buf[n] = '\0';
            addToken(&list, TOK_NUMBER, buf, line);
            continue;
        }

        // ідентифікатори / ключові слова
        if (isalpha(c) || c == '_') {
            size_t start = pos;
            while (pos < len && (isalnum(src[pos]) || src[pos] == '_')) pos++;
            char buf[64];
            size_t n = pos - start;
            memcpy(buf, src + start, n);
            buf[n] = '\0';

            if (strcmp(buf, "func") == 0) addToken(&list, TOK_KW_FUNC, buf, line);
            else if (strcmp(buf, "return") == 0) addToken(&list, TOK_KW_RETURN, buf, line);
            else if (strcmp(buf, "if") == 0) addToken(&list, TOK_KW_IF, buf, line);
            else if (strcmp(buf, "else") == 0) addToken(&list, TOK_KW_ELSE, buf, line);
            else if (strcmp(buf, "while") == 0) addToken(&list, TOK_KW_WHILE, buf, line);
            else if (strcmp(buf, "print") == 0) addToken(&list, TOK_KW_PRINT, buf, line);
            else addToken(&list, TOK_TEXT, buf, line);
            continue;
        }

        // односимвольні токени
        switch (c) {
            case '+': addToken(&list, TOK_PLUS, "+", line); break;
            case '-': addToken(&list, TOK_MINUS, "-", line); break;
            case '*': addToken(&list, TOK_STAR, "*", line); break;
            case '/': addToken(&list, TOK_SLASH, "/", line); break;
            case '(': addToken(&list, TOK_LPAREN, "(", line); break;
            case ')': addToken(&list, TOK_RPAREN, ")", line); break;
            case '{': addToken(&list, TOK_LBRACE, "{", line); break;
            case '}': addToken(&list, TOK_RBRACE, "}", line); break;
            case ',': addToken(&list, TOK_COMMA, ",", line); break;
            case ';': addToken(&list, TOK_SEMI, ";", line); break;
            case '=': 
                if (pos + 1 < len && src[pos + 1] == '=') {
                    addToken(&list, TOK_EQ, "==", line);
                    pos += 2;
                } else {
                    addToken(&list, TOK_ASSIGN, "=", line);
                    pos++;
                }
                break;
            case '!':
                if (pos + 1 < len && src[pos + 1] == '=') {
                    addToken(&list, TOK_NE, "!=", line);
                    pos += 2;
                } else {
                    printf("Unknown symbol '!' in line %d\n", line);
                    pos++;
                }
                break;
            case '<':
                if (pos + 1 < len && src[pos + 1] == '=') {
                    addToken(&list, TOK_LE, "<=", line);
                    pos += 2;
                } else {
                    addToken(&list, TOK_LT, "<", line);
                    pos++;
                }
                break;
            case '>':
                if (pos + 1 < len && src[pos + 1] == '=') {
                    addToken(&list, TOK_GE, ">=", line);
                    pos += 2;
                } else {
                    addToken(&list, TOK_GT, ">", line);
                    pos++;
                }
                break;

            default:
                printf("Unknown symbol '%c' in line %d\n", c, line);
                break;
        }
        pos++;
    }

    addToken(&list, TOK_EOF, "", line);
    return list;
}