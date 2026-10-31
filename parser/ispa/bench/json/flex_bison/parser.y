%{
#include <stdio.h>
#include <stdlib.h>
int yylex(void);
void yyerror(const char* m) { fprintf(stderr, "%s\n", m); exit(1); }
typedef struct Node { struct Node* next; struct Node* kids; } Node;
#ifdef WITH_AST
static Node* mk(Node* kids) { Node* n = malloc(sizeof *n); n->next = 0; n->kids = kids; return n; }
static Node* cat(Node* a, Node* b) { Node* t = a; if (!a) return b; while (t->next) t = t->next; t->next = b; return a; }
#define MK(k) mk(k)
#else
#define MK(k) ((Node*)0)
static Node* cat(Node* a, Node* b) { (void)b; return a; }
#endif
%}
%union { struct Node* n; }
%token STRING NUMBER BOOLEAN
%type <n> json items pairs
%%
main: json { };
json: '{' pairs '}' { $<n>$ = MK($<n>2); }
    | '[' items ']' { $<n>$ = MK($<n>2); }
    | STRING | NUMBER | BOOLEAN { $<n>$ = MK(0); } ;
pairs: /* empty */ { $<n>$ = 0; }
     | STRING ':' json { $<n>$ = MK($<n>3); }
     | pairs ',' STRING ':' json { $<n>$ = cat($<n>1, MK($<n>5)); } ;
items: /* empty */ { $<n>$ = 0; }
     | json { $<n>$ = $<n>1; }
     | items ',' json { $<n>$ = cat($<n>1, $<n>3); } ;
%%
