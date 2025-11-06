#include <stdio.h>
#include "scanner.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
    int column;
    int startLine;
    int startColumn;
} Scanner;

typedef enum {
    STR_DOUBLE,
    STR_SINGLE,
} StringType;

Scanner scanner;

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
    scanner.column = 0;
    scanner.startLine = scanner.line;
    scanner.startColumn = scanner.column;
}

static bool isNumber(TokenType ty) {
    return ty == TOKEN_NUMBER || ty == TOKEN_HEXINT || ty == TOKEN_INTEGER;
}

static bool isKeyword(TokenType ty) {
    return ty == TOKEN_AND || ty == TOKEN_CLASS || ty == TOKEN_ELSE || ty == TOKEN_FALSE || ty == TOKEN_FOR || ty == TOKEN_FUN || ty == TOKEN_IF || ty == TOKEN_NIL || ty == TOKEN_OR || ty == TOKEN_PRINT || ty == TOKEN_RETURN || ty == TOKEN_SUPER || ty == TOKEN_THIS || ty == TOKEN_TRUE || ty == TOKEN_VAR || ty == TOKEN_WHILE || ty == TOKEN_ERROR;
}

static bool isAtEnd(void) {
    return *scanner.current == '\0';
}

static Token makeToken(TokenType ty) {
    Token tk;
    tk.type = ty;
    tk.start = scanner.start;
    tk.length = (int)(scanner.current - scanner.start);
    tk.line = scanner.line;
    tk.column = scanner.column;
    tk.startLine = scanner.startLine;
    tk.startColumn = scanner.startColumn;
    return tk;
}

static Token errorToken(const char* message) {
    Token tk;
    tk.type = TOKEN_ERROR;
    tk.start = message;
    tk.length = (int)strlen(message);
    tk.line = scanner.line;
    tk.column = scanner.column;
    return tk;
}

static char advance(void) {
    if (isAtEnd()) {
        return '\0';
    }
    char c = scanner.current[0];
    scanner.current++;
    scanner.column++;
    if (c == '\n') {
        scanner.line++;
        scanner.column = 0;
    }
    return c;
}

static char peek(void) {
    return *scanner.current;
}

static bool match(char expected) {
    if (isAtEnd()) {
        return false;
    }
    if (peek() != expected) {
        return false;
    }
    advance();
    return true;
}

static char peekNext(void) {
    if (isAtEnd()) {
        return '\0';
    }
    return *(scanner.current + 1);
}

static char peekNextNext(void) {
    if (isAtEnd() || peekNext() == '\0') {
        return '\0';
    }
    return *(scanner.current + 2);
}

static bool isDigit(char c, bool hex) {
    if (hex && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
        return true;
    }
    return c >= '0' && c <= '9';
}

static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static TokenType checkKeyword(int start, int length, const char* rest, TokenType type) {
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType identifierType(void) {
    // Check the start of the token to see if it's a keyword
    switch (scanner.start[0]) {
    case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
    case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
    case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
    case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
    case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
    case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
    case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
    case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
    case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);

    // Keywords starting with i: Infinity inf if
    case 'I': return checkKeyword(1, 7, "nfinity", TOKEN_INF);
    case 'i':
        if (scanner.current - scanner.start > 1) {
            switch(scanner.start[1]) {
                case 'f': return TOKEN_IF;
                case 'n': return checkKeyword(2, 1, "f", TOKEN_INF);
            }
        }
        break;
    // Keywords starting with n: nil nan NaN
    case 'N': return checkKeyword(1, 2, "aN", TOKEN_NAN);
    case 'n':
        if (scanner.current - scanner.start > 1) {
            switch(scanner.start[1]) {
                case 'i': return checkKeyword(2, 1, "l", TOKEN_NIL);
                case 'a': return checkKeyword(2, 1, "n", TOKEN_NAN);
            }
        }
        break;
    // Keywords starting with f: false for fun
    case 'f':
        // A token of length > 1
        if (scanner.current - scanner.start > 1) {
            switch(scanner.start[1]) {
                case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
            }
        }
        break;

    // Keywords starting with t: this true
    case 't':
        // A token of length > 1
        if (scanner.current - scanner.start > 1) {
            switch(scanner.start[1]) {
                case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
                case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
            }
        }
        break;
    }
    return TOKEN_IDENTIFIER;
}

static Token identifier(void) {
    while (isAlpha(peek()) || isDigit(peek(), false)) {
        advance();
    }
    return makeToken(identifierType());
}

static Token hexnum(void) {
    // Starting 0x
    advance();
    while (isDigit(peek(), true)) {
        advance();
    }
    return makeToken(TOKEN_HEXINT);
}

static Token number(void) {
    bool hasDigit = false;
    while (isDigit(peek(), false)) {
        advance();
    }
    /*
      From JSON RFC 8259 (December 2017)

      number = [ minus ] int [ frac ] [ exp ]
      decimal-point = %x2E       ; .
      digit1-9 = %x31-39         ; 1-9
      e = %x65 / %x45            ; e E
      exp = e [ minus / plus ] 1*DIGIT
      frac = decimal-point 1*DIGIT
      int = zero / ( digit1-9 *DIGIT )
      minus = %x2D               ; -
      plus = %x2B                ; +
      zero = %x30                ; 0
    */
    if (peek() == 'e' || peek() == 'E' || peek() == '-' || peek() == '.') {
        // Float fractional part
        hasDigit = true;
        advance();
        while (isDigit(peek(), false)
            || ((peek() == 'e' || peek() == 'E') && (peekNext() == '-' || peekNext() == '+' || isDigit(peekNext(), false)))
            || ((peek() == '-' || peek() == '+') && isDigit(peekNext(), false))
            ) {
            // Float exponent part
            advance();
        }
    }
    if (hasDigit) {
        return makeToken(TOKEN_NUMBER);
    } else {
        return makeToken(TOKEN_INTEGER);
    }
}

static Token string(StringType ty) {
    char end = ty == STR_DOUBLE ? '"' : '\'';
    while (peek() != end && !isAtEnd()) {
        if (peek() == '\\') {
            advance();
        }
        advance();
    }

    if (isAtEnd()) {
        return errorToken("Unterminated string.");
    }
    // Consume closing quote
    advance();
    return makeToken(TOKEN_STRING);
}

static void skipWhitespace(bool semicolonsAreWhitespace) {
    while (true) {
        char c = peek();
        switch (c) {
            case '\n':
            case ' ':
            case '\t':
            case '\r':
                advance();
                break;
            case ';':
                if (semicolonsAreWhitespace) {
                    advance();
                    break;
                }
                return;
            case '#':
                // Skip the shebang line in a script
                if (peekNext() == '!') {
                    while (!(peek() == '\n' || isAtEnd())) {
                        advance();
                    }
                } else {
                    return;
                }
                break;
            case '/':
                if (peekNext() == '*') {
                    while (!((peek() == '*' && peekNext() == '/') || isAtEnd())) {
                        advance();
                    }
                    if (!isAtEnd()) {
                        advance();
                        advance();
                    }
                } else if (peekNext() == '/') {
                    while (!(peek() == '\n' || isAtEnd())) {
                        advance();
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

Token scanToken(void) {
    #define MATCH_OR(if_next, then_tok, else_tok) (match(if_next) ? (then_tok) : (else_tok))

    skipWhitespace(false);
    scanner.startLine = scanner.line;
    scanner.startColumn = scanner.column;
    scanner.start = scanner.current;
    if (isAtEnd()) {
        return makeToken(TOKEN_EOF);
    }
    char c = advance();
    if (isAlpha(c)) {
        return identifier();
    }
    if (c == '0' && (peek() == 'X' || peek() == 'x')) {
        return hexnum();
    }
    if ((c == '.' && isDigit(peek(), false)) || isDigit(c, false)) {
        return number();
    }
    switch (c) {
        // One character tokens
        case ':': return makeToken(TOKEN_COLON);
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case '[': return makeToken(TOKEN_LEFT_SQUARE_BRACE);
        case ']': return makeToken(TOKEN_RIGHT_SQUARE_BRACE);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case ';': {
            Token t = makeToken(TOKEN_SEMICOLON);
            skipWhitespace(false);
            if (peek() == ';' && peekNext() == ';' && peekNextNext() == ';') {
                skipWhitespace(true);
            }
            return t;
        }
        case '#': return makeToken(TOKEN_SIZE);
        case '~': return makeToken(TOKEN_BITNEG);

        case '-': return makeToken(MATCH_OR('=', TOKEN_MINUS_EQUAL, TOKEN_MINUS));
        case '+': return makeToken(MATCH_OR('=', TOKEN_PLUS_EQUAL, TOKEN_PLUS));
        case '&': return makeToken(MATCH_OR('=', TOKEN_BITAND_EQUAL, TOKEN_BITAND));
        case '|': return makeToken(MATCH_OR('=', TOKEN_BITOR_EQUAL, TOKEN_BITOR));
        case '^': return makeToken(MATCH_OR('=', TOKEN_BITXOR_EQUAL, TOKEN_BITXOR));
        case '/': return makeToken(MATCH_OR('=', TOKEN_SLASH_EQUAL, TOKEN_SLASH));
        case '%': return makeToken(MATCH_OR('=', TOKEN_REMAINDER_EQUAL, TOKEN_REMAINDER));
        
        // One or Two-character tokens
        case '!': return makeToken(MATCH_OR('=', TOKEN_BANG_EQUAL, TOKEN_BANG));
        case '=': return makeToken(MATCH_OR('=', TOKEN_EQUAL_EQUAL, TOKEN_EQUAL));
        // 1, 2, or 3-char tokens
        case '*': return makeToken(MATCH_OR('*',
            MATCH_OR('=', TOKEN_STAR_STAR_EQUAL, TOKEN_STAR_STAR),
                MATCH_OR('=', TOKEN_STAR_EQUAL, TOKEN_STAR)));
        case '>': return makeToken(MATCH_OR('=', TOKEN_GREAT_EQUAL,
            (MATCH_OR('>', MATCH_OR('=', TOKEN_RIGHT_SHIFT_EQUAL, TOKEN_RIGHT_SHIFT),
                TOKEN_GREAT))));
        case '<': return makeToken(MATCH_OR('=', TOKEN_LESS_EQUAL,
            (MATCH_OR('<', MATCH_OR('=', TOKEN_LEFT_SHIFT_EQUAL, TOKEN_LEFT_SHIFT) ,
                TOKEN_LESS))));
        case '"':
            return string(STR_DOUBLE);
        case '\'':
            return string(STR_SINGLE);
    }
    return errorToken("Unexpected character");
}

void scanAndPrint(const char* source) {
    initScanner(source);
    while (true) {
        Token tk = scanToken();
        printf("%4d:%-4d ", tk.line, tk.column);
        if (tk.type == TOKEN_EOF) {
            printf("EOF\n");
            break;
        } else if (isKeyword(tk.type) || isNumber(tk.type)) {
            printf("%.*s\n", tk.length, tk.start);
        } else {
            printf("'%.*s'\n", tk.length, tk.start);
        }
    }
}
