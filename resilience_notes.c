


// typedef enum TreeKind {
//   ErrorTree,
//   Ident,
//   ObjLit,
//   Access,
//   IntLit,
//   Paren
// };

// struct Tree {
//   enum TreeKind kind;
//   int len;
//   int capacity;
//   struct Child *children;
// };

// struct Child {
//   struct Token *token;
//   struct Tree *tree;
// };


// #define new_tree(tree_kind) {\
//   struct Tree *new = &(stack[stack_pos++]); \
//   new->children = malloc(sizeof(struct Tree)); \
//   new->len = 0; \
//   new->capacity = 1; \
//   new->kind = tree_kind; \
// }

// void append_child(struct Tree *stack, int *stack_pos) {
//   struct Tree *parent = &(stack[*stack_pos - 2]);
//   if (parent->len == parent->capacity) {
//     struct Tree *new_children = malloc(parent->capacity * 2 * sizeof(struct Tree));
//     memcpy(new_children, parent->children, parent->capacity * sizeof(struct Tree));
//     parent->children = new_children;
//     parent->capacity *= 2;
//   }
//   struct Tree *child_ptr = &(stack[--*stack_pos]);
//   memcpy(&(parent->children[parent->len++]), child_ptr, sizeof(struct Tree));
// }

// struct Tree *parse(struct Token *tokens, int tokens_len) {
//   struct Tree *stack = malloc(4096 * sizeof(struct Tree));
//   int stack_pos = 0;
//   for (int i = 0; i < tokens_len; i++) {
//     struct Token tok = tokens[i];
//     switch (tok.kind) {
//       case LParen: new_tree(ErrorTree); break;
//       case RParen: {
//         append_child(&stack, &stack_pos);
//       }
//       case LCurly: new_tree(ObjLit); break;
//       case Comma: {
//         append_child(&stack, &stack_pos);
//         new_tree(ErrorTree);
//         break;
//       }
//     }
//   }
// }