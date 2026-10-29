"""ISPA compiler intrinsic boundary.

The Python frontend should intercept construct/enum instead of lowering their
function bodies. The fallback below is for normal CPython smoke tests only.
"""

class IRNode:
    def __init__(self, kind, fields):
        self.kind = kind
        self.fields = fields

    def __getattr__(self, name):
        if name in self.fields:
            return self.fields[name]
        raise AttributeError(name)

    def __repr__(self):
        return f"IRNode({self.kind!r}, {self.fields!r})"

def construct(kind, fields):
    return IRNode(kind, fields)

def enum(kind, member):
    return kind + "." + member
