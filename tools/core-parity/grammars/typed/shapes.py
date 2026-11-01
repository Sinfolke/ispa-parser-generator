class a_ast:
    type: Node
    value: Node
    def __init__(self, type: Node, value: Node) -> None:
        self.type = type
        self.value = value

class pair_ast:
    first: Node
    second: Node
    def __init__(self, first: Node, second: Node) -> None:
        self.first = first
        self.second = second
