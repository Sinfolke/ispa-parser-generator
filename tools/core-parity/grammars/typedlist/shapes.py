class nums_ast:
    items: list[Node]
    tail: Node
    def __init__(self, items: list[Node], tail: Node) -> None:
        self.items = items
        self.tail = tail
