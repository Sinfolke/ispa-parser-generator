/**
 * C++ ispa std lib 1.3 (split)
 * Umbrella header: pulls in every chunk. Generated code should include only the chunks it uses
 * (<ispastdlib/node.hpp>, <ispastdlib/lexer.hpp>, ...). This file exists for old code and for hand-written users.
 */
#pragma once
#ifndef ISPA_STD_LIB_CPP
#define ISPA_STD_LIB_CPP
#include <ispastdlib/core.hpp>
#include <ispastdlib/python_collections.hpp>
#include <ispastdlib/failure.hpp>
#include <ispastdlib/object.hpp>
#include <ispastdlib/variant_io.hpp>
#include <ispastdlib/span.hpp>
#include <ispastdlib/traits.hpp>
#include <ispastdlib/ast_printer.hpp>
#include <ispastdlib/node.hpp>
#include <ispastdlib/token_flow.hpp>
#include <ispastdlib/to_string.hpp>
#include <ispastdlib/assign.hpp>
#include <ispastdlib/dfa_api.hpp>
#include <ispastdlib/dfa_run.hpp>
#include <ispastdlib/raw_token.hpp>
#include <ispastdlib/lexer.hpp>
#include <ispastdlib/parser_base.hpp>
#include <ispastdlib/parser_ll.hpp>
#include <ispastdlib/lr_parser.hpp>
#include <ispastdlib/lr_machine.hpp>
#endif // ISPA_STD_LIB_CPP
