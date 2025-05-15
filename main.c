// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

enum TokenKind {
  ErrorToken,
  Eof,
  LParen,
  RParen,
  LCurly,
  RCurly,
  Comma,
  Colon,
  Dot,
  NameToken,
  IntToken
};

struct Token {
  enum TokenKind kind;
  int len;
  char *text;
};

#define is_ident_char(c) \
  (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || c == '_')

#define push_token(kind1, len1, text1) {\
  struct Token *slot = out + out_i++; \
  slot->kind = kind1; slot->len = len1; slot->text = text1; \
}

int lex(int in_len, char *in, struct Token *out) {
  int out_i = 0;
  for (int i = 0; i < in_len; i++) {
    char c = in[i];
    switch (in[i]) {
      case '(': push_token(LParen, 1, in + i); break;
      case ')': push_token(RParen, 1, in + i); break;
      case '{': push_token(LCurly, 1, in + i); break;
      case '}': push_token(RCurly, 1, in + i); break;
      case ',': push_token(Comma, 1, in + i); break;
      case ':': push_token(Colon, 1, in + i); break;
      case '.': push_token(Dot, 1, in + i); break;
      case ' ':
      case '\t':
      case '\n': break; // ignore whitespace for now
      default:
        if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')) {
          char *tok = in + i;
          int old_i = i;
          while (i + 1 < in_len && is_ident_char(in[i + 1])) {
            i++;
          }
          int tok_len = 1 + i - old_i;
          push_token(NameToken, tok_len, tok);
          continue;
        }
        if ('0' <= c && c <= '9') {
          char *tok = in + i;
          int old_i = i;
          while (i + 1 <= in_len && '0' <= in[i + 1] && in[i + 1] <= '9') {
            i++;
          }
          int tok_len = 1 + i - old_i;
          push_token(IntToken, tok_len, tok);
          continue;
        }
        push_token(ErrorToken, 1, in + i);
    }
  }
  push_token(Eof, 0, in + in_len);
  return out_i;
}

enum ExprKind {
  Ident,
  Int,
  Access,
  Object
};

struct Ident {
  int len;
  char *name;
};

struct IntLit {
  int val;
};

struct Access {
  struct Expr *expr;
  char *field;
  int len;
};

struct Object {
  int len;
  struct Field *fields;
};

struct Field {
  int len;
  char *name;
  struct Expr *def;
};

struct String {
  int len;
  char *ptr;
};

void print_string(char *s, int len) {
  for (int i = 0; i < len; i++) printf("%c", *(s + i));
}

struct Expr {
  enum ExprKind kind;
  union {
    struct Ident ident;
    struct IntLit int_lit;
    struct Access access;
    struct Object object;
  } val;
};

struct Parser {
  struct Token *tokens;
  int len;
  int pos;
};

enum TokenKind peek(struct Parser *parser) {
  return parser->tokens[parser->pos].kind;
}

void step(struct Parser *parser) {
  parser->pos++;
}

int try_access(struct Parser *parser, struct Expr *stack, int *stack_pos) {
  if (peek(parser) != Dot) return 1;
  step(parser);
  char *name;
  int name_len;
  if (peek(parser) == NameToken) {
    name = parser->tokens[parser->pos].text;
    name_len = parser->tokens[parser->pos].len;
  } else return 0;
  step(parser);
  struct Expr *e = malloc(sizeof(struct Expr));
  memcpy(e, &(stack[*stack_pos - 1]), sizeof(struct Expr));
  stack[*stack_pos - 1].kind = Access;
  stack[*stack_pos - 1].val.access.field = name;
  stack[*stack_pos - 1].val.access.len = name_len;
  stack[*stack_pos - 1].val.access.expr = e;
  return 1;
}

int parse(struct Parser *parser, struct Expr *stack, int *stack_pos) {
  switch (peek(parser)) {
    case ErrorToken:
      printf("todo ErrorToken");
      return 0;
    case Eof:
      printf("todo Eof");
      return 0;
    case LParen: {
      struct Expr *new_stack = &(stack[*stack_pos]);
      int new_stack_pos = 0;
      int ok = parse(parser, new_stack, &new_stack_pos);
      if (!ok || new_stack_pos != 1) return 0;
      step(parser);
      if (peek(parser) != RParen) return 0;
      step(parser);
      *stack_pos += 1;
      return try_access(parser, stack, stack_pos);
    }
    case LCurly: {
      int capacity = 10;
      int i = 0;
      struct String *field_names = malloc(capacity * sizeof(struct String));
      while (1) {
        step(parser);
        char *name;
        int name_len;
        if (peek(parser) == NameToken) {
          name = parser->tokens[parser->pos].text;
          name_len = parser->tokens[parser->pos].len;
        } else return 0;
        step(parser);
        if (peek(parser) != Colon) return 0;
        step(parser);
        struct Expr *new_stack = &(stack[*stack_pos]);
        int new_stack_pos = 0;
        int ok = parse(parser, new_stack, &new_stack_pos);
        if (!ok || new_stack_pos != 1) return 0;
        if (i == capacity) {
          capacity *= 2;
          struct String *new_field_names = malloc(capacity * sizeof(struct String));
          memcpy(new_field_names, field_names, i * sizeof(struct String));
          free(field_names);
          field_names = new_field_names;
        }
        field_names[i].ptr = name;
        field_names[i].len = name_len;
        i += 1;
        *stack_pos += 1; // leave the definition on the stack to be gathered later
        step(parser);
        if (peek(parser) == RCurly) break;
        if (peek(parser) != Comma) return 0;
      }
      step(parser); // over the RCurly
      struct Field *fields = malloc(i * sizeof(struct Field));
      *stack_pos -= i;
      for (int j = 0; j < i; j++) {
        fields[j].name = field_names[j].ptr;
        fields[j].len = field_names[j].len;
        fields[j].def = malloc(sizeof(struct Expr));
        memcpy(fields[j].def, &(stack[*stack_pos + j]), sizeof(struct Expr));
      }
      stack[*stack_pos].kind = Object;
      stack[*stack_pos].val.object.fields = fields;
      stack[*stack_pos].val.object.len = i;
      *stack_pos += 1;
      return try_access(parser, stack, stack_pos);
    }
    case NameToken: {
      char *name = parser->tokens[parser->pos].text;
      stack[*stack_pos].kind = Ident;
      stack[*stack_pos].val.ident.name = name;
      stack[*stack_pos].val.ident.len = parser->tokens[parser->pos].len;
      *stack_pos += 1;
      return try_access(parser, stack, stack_pos);
    }
    case IntToken: {
      char *str_ptr = parser->tokens[parser->pos].text;
      int len = parser->tokens[parser->pos].len;
      char *str = malloc(len + 1);
      memcpy(str, str_ptr, len);
      str[len] = '\0';
      long l = strtol(str, NULL, 0);
      free(str);
      if (l > INT_MAX) l = INT_MAX;
      if (l < INT_MIN) l = INT_MIN;
      stack[*stack_pos].kind = Int;
      stack[*stack_pos].val.int_lit.val = (int)l;
      *stack_pos += 1;
      return try_access(parser, stack, stack_pos);
    }
    case Dot: 
    case RParen:
    case RCurly:
    case Colon:
    case Comma: return 0;
  }
}

void print_expr(struct Expr *e) {
  switch (e->kind) {
    case Ident: print_string(e->val.ident.name, e->val.ident.len); break;
    case Int: printf("%d", e->val.int_lit.val); break;
    case Object: {
      struct Object obj = e->val.object;
      printf("{");
      for (int i = 0; i < obj.len; i++) {
        print_string(obj.fields[i].name, obj.fields[i].len);
        printf(": ");
        print_expr(obj.fields[i].def);
        if (i < obj.len - 1) printf(", ");
      }
      printf("}");
      break;
    }
    case Access: {
      struct Access acc = e->val.access;
      print_expr(acc.expr);
      printf(".");
      print_string(acc.field, acc.len);
      break;
    }
  }
}

int main() {
  char *prog = "{foo: bar, baz: 7}.baz";
  struct Token *tokens = malloc(4096 * sizeof(struct Token));
  int len = lex(22, prog, tokens);
  struct Parser parser = {
    .len = len,
    .pos = 0,
    .tokens = tokens
  };
  struct Expr *stack = malloc(4096 * sizeof(struct Expr));
  int stack_pos = 0;
  int ok = parse(&parser, stack, &stack_pos);
  print_expr(stack);
  printf("\n");
  return 0;
}