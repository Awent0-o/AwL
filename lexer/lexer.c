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
        if (c == '\r') { pos++; continue; }   // ignore carriage return
        if (isspace(c)) { pos++; continue; }

        //strings
        printf("c='%c' (%d)\n", c, c);
        if (c == '"') {
            pos++; // skip opening "
            size_t start = pos;

            while (pos < len && src[pos] != '"') {
                pos++;
            }

            char buf[256];
            size_t n = pos - start;
            
            if (n >= sizeof(buf)) n = sizeof(buf) - 1;
            memcpy(buf, src + start, n);
            buf[n] = '\0';
        
            if (pos < len) pos++; // skip closing "
            addToken(&list, TOK_STRING, buf, line);
            continue;
        }

        // numbers and floats
        if (isdigit(c)) {
            size_t start = pos;
            bool is_float = false;

            while (pos < len && (isdigit(src[pos]) || src[pos] == '.')) {
                if(src[pos] == '.'){
                    if(is_float) break;
                    is_float = true;
                }

                pos++;
            }
            char buf[64];
            size_t n = pos - start;

            if(n >= sizeof(buf)) n = sizeof(buf) - 1;
            memcpy(buf, src + start, n);
            buf[n] = '\0';

            if(is_float) addToken(&list, TOK_FLOAT, buf, line);
            else addToken(&list, TOK_NUMBER, buf, line);
            
            continue;
        }
        if (c == 'f' && pos + 1 < len && src[pos + 1] == '"') {
            pos += 2; // skip f"
            size_t start = pos;
            while (pos < len && src[pos] != '"') {
                pos++;
            }
            char buf[512] = {0};
            size_t n = pos - start;
            if (n >= sizeof(buf)) n = sizeof(buf) - 1;
            memcpy(buf, src + start, n);
            buf[n] = '\0';
            if (pos < len) pos++; // close "
            addToken(&list, TOK_FSTRING, buf, line);
            continue;
        }

        // indentifiers and keywords
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
            else if (strcmp(buf, "for") == 0) addToken(&list, TOK_KW_FOR, buf, line);
            else if (strcmp(buf, "print") == 0) addToken(&list, TOK_KW_PRINT, buf, line);
            else if (strcmp(buf, "import") == 0) addToken(&list, TOK_KW_IMPORT, buf, line);
            else if (strcmp(buf, "true") == 0) addToken(&list, TOK_TRUE, buf, line);
            else if (strcmp(buf, "false") == 0) addToken(&list, TOK_FALSE, buf, line);
            else if (strcmp(buf, "int") == 0)    addToken(&list, TOK_KW_INT, buf, line);
            else if (strcmp(buf, "float") == 0)  addToken(&list, TOK_KW_FLOAT, buf, line);
            else if (strcmp(buf, "string") == 0) addToken(&list, TOK_KW_STRING, buf, line);
            else if (strcmp(buf, "bool") == 0)   addToken(&list, TOK_KW_BOOL, buf, line);
            else if (strcmp(buf, "void") == 0)   addToken(&list, TOK_KW_VOID, buf, line);
            else addToken(&list, TOK_TEXT, buf, line);
            continue;
        }

        // one or two character operators
        if (c == '-' && pos + 1 < len && src[pos + 1] == '>') {
            addToken(&list, TOK_ARROW, "->", line); 
            pos += 2; 
            continue;
        }

        if (c == '=' && pos + 1 < len && src[pos + 1] == '=') {
            addToken(&list, TOK_EQ, "==", line);
            pos += 2; 
            continue;
        }
        if (c == '!' && pos + 1 < len && src[pos + 1] == '=') {
            addToken(&list, TOK_NE, "!=", line); 
            pos += 2; 
            continue;
        }
        if (c == '<' && pos + 1 < len && src[pos + 1] == '=') {
            addToken(&list, TOK_LE, "<=", line); 
            pos += 2; 
            continue;
        }
        if (c == '>' && pos + 1 < len && src[pos + 1] == '=') {
            addToken(&list, TOK_GE, ">=", line); 
            pos += 2; 
            continue;
        }

        // increment operator
        if (c == '+' && pos + 1 < len && src[pos + 1] == '+') {
            addToken(&list, TOK_INC, "++", line);
            pos += 2;
            continue;
        }
        
        // decrement operator
        if (c == '-' && pos + 1 < len && src[pos + 1] == '-') {
            addToken(&list, TOK_DEC, "--", line);
            pos += 2;
            continue;
        }
                
        if (c == '`') {
            while (pos < len && src[pos] != '\n') {
                pos++;
            }

            continue;
        }

        if (c == '@' && pos + 1 < len && src[pos + 1] == '{') {
            
            pos += 2;  // skip @{
            size_t start = pos;
            while (pos < len - 1 && !(src[pos] == '}')) {
                printf("%zu: '%c' (%d)\n", pos, src[pos], (unsigned char)src[pos]);
                pos++;
            }
            char buf[4096];
            size_t n = pos - start;
            memcpy(buf, src + start, n);
            buf[n] = '\0';
            pos++;  // skip }
            addToken(&list, TOK_RAW_C, buf, line);
            continue;
        }

        switch (c) {
            case '=': addToken(&list, TOK_ASSIGN, "=", line); break;
            case '<': addToken(&list, TOK_LT, "<", line); break;
            case '>': addToken(&list, TOK_GT, ">", line); break;
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
            case '`': addToken(&list, TOK_IGNOR, "`", line); break;
            case '[': addToken(&list, TOK_LBRACKET, "[", line); break;
            case ']': addToken(&list, TOK_RBRACKET, "]", line); break;
            case '.': addToken(&list, TOK_DOT, ".", line); break;

            default:
                printf("Unknown symbol '%c' in line %d\n", c, line);
                break;
        }
        pos++;
    }

    addToken(&list, TOK_EOF, "", line);
    return list;
}