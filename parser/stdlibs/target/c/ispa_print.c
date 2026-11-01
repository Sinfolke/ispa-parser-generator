#include "ispa_print.h"

ispa_str ispa_text_int(int64_t v) { return python_ispa_std_format_int_to_string(v); }
ispa_str ispa_text_bool(bool v) { return ISPA_STR(v ? "1" : "0"); }
ispa_str ispa_text_char(char v) { return ispa_str_from_char(v); }
ispa_str ispa_text_float(double v) { char b[64]; int n = snprintf(b, sizeof b, "%g", v); return ispa_str_from(ispa_cur, b, (size_t)n); }

void ispa_print_scalar(ispa_printer *p, ispa_str name, ispa_str text, bool last) {
    python_ispa_std_ast_printer_ASTPrinter_scalar(p, name, text, last);
}

void ispa_print_any(ispa_printer *p, ispa_any a) {
    if (!a.tag) return;                                   /* an unset handle prints nothing */
    switch (a.tag->kind) {
        case ISPA_KIND_OBJ: if (a.tag->print && a.value) a.tag->print(a.value, p); break;
        case ISPA_KIND_NODE: ispa_print_node(p, (python_ispa_std_node_Node *)a.value); break;
        case ISPA_KIND_STR: ispa_print_scalar(p, ISPA_STR("value"), *(ispa_str *)a.value, true); break;
        case ISPA_KIND_INT: ispa_print_scalar(p, ISPA_STR("value"), ispa_text_int((int64_t)(intptr_t)a.value), true); break;
        default: break;
    }
}

void ispa_print_node(ispa_printer *p, python_ispa_std_node_Node *n) {
    if (!n || n->_empty) { python_ispa_std_ast_printer_ASTPrinter_node(p, ISPA_STR("<empty>"), false); return; }
    ispa_print_any(p, n->_data);
}

ispa_str ispa_any_text(ispa_any a) {
    if (!a.tag) return ISPA_STR("");
    switch (a.tag->kind) {
        case ISPA_KIND_STR: return *(ispa_str *)a.value;
        case ISPA_KIND_INT: return ispa_text_int((int64_t)(intptr_t)a.value);
        default: return ISPA_STR("");
    }
}

void ispa_write_text(FILE *os, ispa_printer *p) {
    ispa_str t = python_ispa_std_ast_printer_ASTPrinter_text(p);
    fwrite(t.data, 1, t.len, os);
}

void ispa_print_node_name(ispa_printer *p, ispa_str name, bool last) { python_ispa_std_ast_printer_ASTPrinter_node(p, name, last); }
void ispa_print_up(ispa_printer *p, bool more) { python_ispa_std_ast_printer_ASTPrinter_up(p, more); }
void ispa_print_down(ispa_printer *p) { python_ispa_std_ast_printer_ASTPrinter_down(p); }
bool ispa_any_structured(ispa_any a) { return a.tag && (a.tag->kind == ISPA_KIND_OBJ || a.tag->kind == ISPA_KIND_NODE); }
