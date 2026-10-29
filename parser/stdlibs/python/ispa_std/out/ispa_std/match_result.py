"""MatchResult<Data, IT>: did the rule match, the node, and the cursor after it."""
from ispa_std.lexer import TokenCursor
from ispa_std.node import Node


class MatchResult:
    """MatchResult<Data, IT>: did the rule match, the node, and the cursor after it."""
    status: bool
    node: Node
    it: TokenCursor

    def __init__(self, status: bool, node: Node, it: TokenCursor):
        self.status = status
        self.node = node
        self.it = it

    @staticmethod
    def failed(it: TokenCursor) -> "MatchResult":
        return MatchResult(False, Node.blank(), it)

    def debug_type(self) -> str:
        return "ISPA.MatchResult"

    def debug_fields(self) -> list:
        from ispa_std.utility import Pair
        return [Pair("status", self.status), Pair("node", self.node)]
