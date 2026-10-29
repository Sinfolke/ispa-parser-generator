"""Explicit semantic operations for Python source compiled by ISPA.

These compiler bindings lower directly to LangAPI nodes. This module is input
to ISPA's Python frontend, rather than an executable CPython runtime module.
"""

size = __ispa_internal__("__ispa_ir__.size", 1)
push = __ispa_internal__("__ispa_ir__.push", 2)
extend = __ispa_internal__("__ispa_ir__.extend", 2)
clear = __ispa_internal__("__ispa_ir__.clear", 1)
token_cursor = __ispa_internal__("__ispa_ir__.token_cursor", 1)
lazy_token_cursor = __ispa_internal__("__ispa_ir__.lazy_token_cursor", 2)
reset_token_stream = __ispa_internal__("__ispa_ir__.reset_token_stream", 1)
