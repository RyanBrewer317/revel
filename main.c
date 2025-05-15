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
  Object,
  Call
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

struct Call {
  struct Expr *obj;
  char *method;
  int method_len;
  struct Expr *args;
  int args_len;
};

struct Object {
  int len;
  struct Field *fields;
};

struct Field {
  int name_len;
  char *name;
  int params_len;
  struct String *params;
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
    struct Call call;
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

enum ResultKind {
  Success,
  FieldOrMethodExpected,
  CommaExpected,
  Todo,
  RParenExpected,
  ColonExpected,
  Unexpected,
};

struct Result {
  enum ResultKind kind;
  enum TokenKind val;
};

int ok(struct Result res) {
  return res.kind == Success;
}

struct Result parse(struct Parser *parser, struct Expr *stack);
void print_expr(struct Expr *e);

struct Result try_access(struct Parser *parser, struct Expr *stack) {
  if (peek(parser) != Dot) return (struct Result){ .kind = Success, .val = 0 };
  step(parser);
  char *name;
  int name_len;
  if (peek(parser) == NameToken) {
    name = parser->tokens[parser->pos].text;
    name_len = parser->tokens[parser->pos].len;
  } else return (struct Result){ .kind = FieldOrMethodExpected, .val = peek(parser)};
  step(parser);
  struct Expr *e = malloc(sizeof(struct Expr));
  memcpy(e, &(stack[-1]), sizeof(struct Expr));
  if (peek(parser) == LParen) {
    int i = 0;
    while (1) {
      step(parser);
      struct Result res = parse(parser, &(stack[i]));
      if (!ok(res)) {
        return res;
      }
      i += 1;
      if (peek(parser) == RParen) break;
      if (peek(parser) != Comma) {
        return (struct Result){ .kind = CommaExpected, .val = peek(parser) };
      }
    }
    step(parser); // step over the RParen
    struct Expr *call = &(stack[-1]);
    call->kind = Call;
    call->val.call.args = malloc(i * sizeof(struct Expr));
    memcpy(call->val.call.args, stack, i * sizeof(struct Expr));
    call->val.call.args_len = i;
    call->val.call.method = name;
    call->val.call.method_len = name_len;
    call->val.call.obj = e;
    return (struct Result){ .kind = Success, .val = 0 };
  }
  struct Expr *access = &(stack[-1]);
  access->kind = Access;
  access->val.access.field = name;
  access->val.access.len = name_len;
  access->val.access.expr = e;
  return (struct Result){ .kind = Success, .val = 0 };
}

struct Result parse(struct Parser *parser, struct Expr *stack) {
  switch (peek(parser)) {
    case ErrorToken:
      printf("todo ErrorToken\n");
      return (struct Result){ .kind = Todo };
    case Eof:
      printf("todo Eof\n");
      return (struct Result){ .kind = Todo };
    case LParen: {
      struct Result res = parse(parser, stack);
      if (!ok(res)) return res;
      step(parser);
      if (peek(parser) != RParen) return (struct Result){ .kind = RParenExpected, .val = peek(parser) };
      step(parser);
      return try_access(parser, &(stack[1]));
    }
    case LCurly: {
      int capacity = 10;
      int i = 0;
      struct Field *fields = malloc(capacity * sizeof(struct Field));
      while (1) {
        step(parser);
        char *name;
        int name_len;
        if (peek(parser) == NameToken) {
          name = parser->tokens[parser->pos].text;
          name_len = parser->tokens[parser->pos].len;
        } else return (struct Result){ .kind = FieldOrMethodExpected, .val = peek(parser) };
        step(parser);
        struct String *param_names;
        int num_params = 0;
        if (peek(parser) == LParen) {
          int params_capacity = 10;
          param_names = malloc(params_capacity * sizeof(struct String));
          while (1) {
            step(parser);
            char *param;
            int param_len;
            if (peek(parser) == NameToken) {
              param = parser->tokens[parser->pos].text;
              param_len = parser->tokens[parser->pos].len;
            } else if (peek(parser) == RParen) break; else {
              return (struct Result){ .kind = RParenExpected, .val = peek(parser) };
            }
            if (num_params == params_capacity) {
              params_capacity *= 2;
              struct String *new_param_names = malloc(params_capacity * sizeof(struct String));
              memcpy(new_param_names, param_names, num_params * sizeof(struct String));
              free(param_names);
              param_names = new_param_names;
            }
            param_names[num_params].ptr = param;
            param_names[num_params].len = param_len;
            num_params += 1;
            step(parser);
            if (peek(parser) == RParen) break;
            if (peek(parser) != Comma) {
              return (struct Result){ .kind = CommaExpected, .val = peek(parser) };
            }
          }
          step(parser); // over the RParen
        }
        if (peek(parser) != Colon) return (struct Result){ .kind = ColonExpected, .val = peek(parser) };
        step(parser);
        struct Result res = parse(parser, &(stack[i]));
        if (!ok(res)) {
          return res;
        }
        if (i == capacity) {
          capacity *= 2;
          struct Field *new_field_names = malloc(capacity * sizeof(struct Field));
          memcpy(new_field_names, fields, i * sizeof(struct String));
          free(fields);
          fields = new_field_names;
        }
        fields[i].name = name;
        fields[i].name_len = name_len;
        fields[i].def = malloc(sizeof(struct Expr));
        fields[i].params = param_names;
        fields[i].params_len = num_params;
        memcpy(fields[i].def, &(stack[i]), sizeof(struct Expr));
        i += 1;
        if (peek(parser) == RCurly) break;
        if (peek(parser) != Comma) return (struct Result){ .kind = CommaExpected, .val = peek(parser) };
      }
      step(parser); // over the RCurly
      stack[0].kind = Object;
      stack[0].val.object.fields = fields;
      stack[0].val.object.len = i;
      return try_access(parser, &(stack[1]));
    }
    case NameToken: {
      char *name = parser->tokens[parser->pos].text;
      stack[0].kind = Ident;
      stack[0].val.ident.name = name;
      stack[0].val.ident.len = parser->tokens[parser->pos].len;
      step(parser);
      return try_access(parser, &(stack[1]));
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
      stack[0].kind = Int;
      stack[0].val.int_lit.val = (int)l;
      step(parser);
      return try_access(parser, &(stack[1]));
    }
    case Dot: 
    case RParen:
    case RCurly:
    case Colon:
    case Comma: return (struct Result){ .kind = Unexpected, .val = peek(parser) };
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
        print_string(obj.fields[i].name, obj.fields[i].name_len);
        int params_len = obj.fields[i].params_len;
        if (params_len > 0) {
          printf("(");
          for (int j = 0; j < params_len; j++) {
            struct String *param = &(obj.fields[i].params[j]);
            print_string(param->ptr, param->len);
            if (j < params_len - 1) printf(", ");
          }
          printf(")");
        }
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
    case Call: {
      struct Call call = e->val.call;
      print_expr(call.obj);
      printf(".");
      print_string(call.method, call.method_len);
      printf("(");
      for (int i = 0; i < call.args_len; i++) {
        print_expr(&(call.args[i]));
        if (i < call.args_len - 1) printf(", ");
      }
      printf(")");
      break;
    }
  }
}

int main() {
  char *prog = "{foo(n): n.add(2, 3), baz: 7}.baz";
  struct Token *tokens = malloc(4096 * sizeof(struct Token));
  int len = lex(strlen(prog), prog, tokens);
  struct Parser parser = {
    .len = len,
    .pos = 0,
    .tokens = tokens
  };
  struct Expr *stack = malloc(4096 * sizeof(struct Expr));
  struct Result res = parse(&parser, stack);
  if (!ok(res)) {
    printf("parse error: %d %d\n", res.kind, res.val);
    return 0;
  }
  print_expr(stack);
  printf("\n");
  return 0;
}