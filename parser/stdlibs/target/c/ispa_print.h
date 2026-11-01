/* ispa_print.h - printing of ASTs (port of ispastdlib ast_printer/to_string): the native helpers generated
 * write_to_output functions use. Link ispa_print.c with the lowered library. */
#ifndef ISPA_PRINT_H
#define ISPA_PRINT_H
#include "ispa_std.h"

typedef python_ispa_std_ast_printer_ASTPrinter ispa_printer;

/* text of a scalar the way the C++ printer streams it */
ispa_str ispa_text_int(int64_t v);
ispa_str ispa_text_bool(bool v);      /* streamed as a number: 1 / 0 */
ispa_str ispa_text_char(char v);
ispa_str ispa_text_float(double v);

/* `name: value` for scalars (a node_with_value of a non-structured value) */
void ispa_print_scalar(ispa_printer *p, ispa_str name, ispa_str text, bool last);
/* Node::print: `<empty>` or the tree of the value it holds */
void ispa_print_node(ispa_printer *p, python_ispa_std_node_Node *n);
/* the tree of a boxed value (print_data): objects print through their registered writer */
void ispa_print_any(ispa_printer *p, ispa_any a);
/* the text of a boxed value inside a concatenation */
ispa_str ispa_any_text(ispa_any a);
void ispa_print_node_name(ispa_printer *p, ispa_str name, bool last);
void ispa_print_up(ispa_printer *p, bool has_more_siblings);
void ispa_print_down(ispa_printer *p);
bool ispa_any_structured(ispa_any a);
static inline void ispa_print_any_value(ispa_printer *p, ispa_any a) { ispa_print_any(p, a); }
/* write a finished printer's text to a stream (the generated `print(os)`) */
void ispa_write_text(FILE *os, ispa_printer *p);
#endif
