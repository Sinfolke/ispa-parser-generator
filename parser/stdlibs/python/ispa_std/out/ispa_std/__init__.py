"""ispa_std: language-agnostic standard library, written once as ordinary code.

Only ispa_std.prim touches the compiler boundary. Import submodules directly
(ispa_std.vector, ispa_std.optional, ...); nothing is imported eagerly here so
that tests can install the CPython runtime before any library module loads.
"""
