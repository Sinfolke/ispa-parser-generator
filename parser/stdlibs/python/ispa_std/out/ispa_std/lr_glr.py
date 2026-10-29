"""lr_conflicts.hpp: LR parsing with automatic conflict resolution (GLR-style).

Used only for grammars with unresolved conflict cells. Stacks share immutable
prefixes and reductions share a packed forest. No semantic callbacks run until
the complete input has exactly one surviving derivation."""
from ispa_std.failures import runtime_error
from ispa_std.format import int_to_string, join
from ispa_std.lr import ABSENT, LR_CONFLICT, LR_INVALID, LRGrammar, LRParser
from ispa_std.lexer import TokenCursor
from ispa_std.match_result import MatchResult
from ispa_std.sortedmap import IntSet, StrIntMap

NODE_LIMIT = 250000
WORK_LIMIT = 2000000


class _Span:
    begin: int
    end: int
    leading: bool
    trailing: bool
    empty: bool

    def __init__(self, begin: int, end: int, leading: bool, trailing: bool, empty: bool):
        self.begin = begin
        self.end = end
        self.leading = leading
        self.trailing = trailing
        self.empty = empty


class _Alternative:
    rule: int
    children: list[int]

    def __init__(self, rule: int, children: list[int]):
        self.rule = rule
        self.children = children

    def equals(self, other: "_Alternative") -> bool:
        if self.rule != other.rule or len(self.children) != len(other.children):
            return False
        i = 0
        while i < len(self.children):
            if self.children[i] != other.children[i]:
                return False
            i = i + 1
        return True


class _Forest:
    span: _Span
    has_token: bool
    token: object
    alternatives: list[_Alternative]

    def __init__(self, span: _Span, has_token: bool, token: object):
        self.span = span
        self.has_token = has_token
        self.token = token
        self.alternatives = []


class _Stack:
    previous: int
    state: int
    forest: int

    def __init__(self, previous: int, state: int, forest: int):
        self.previous = previous
        self.state = state
        self.forest = forest


def _b(flag: bool) -> str:
    if flag:
        return "1"
    return "0"


class LRGeneralizedParser(LRParser):
    work: int

    def __init__(self):
        LRParser.__init__(self)
        self.work = 0

    def _tick(self, forest: list[_Forest], stacks: list[_Stack]) -> None:
        self.work = self.work + 1
        if self.work > WORK_LIMIT or len(forest) + len(stacks) > NODE_LIMIT:
            runtime_error("LR automatic conflict resolution resource limit exceeded")

    def lr_parse_generalized(self, g: LRGrammar, pos: TokenCursor) -> MatchResult:
        self.work = 0
        forest = []
        stacks = [_Stack(ABSENT, 0, ABSENT)]
        forest_index = StrIntMap()
        stack_index = StrIntMap()
        cursor = 0
        active = IntSet()
        active.add(0)
        roots = IntSet()

        running = True
        while running:
            offset = [cursor]
            self._skip(g, pos, offset)
            cursor = offset[0]
            window = self._window(g, pos)
            pending = []
            pending.extend(active.items)
            closure = active.copy()
            shifted = IntSet()
            nxt = 0
            while nxt < len(pending):
                self._tick(forest, stacks)
                top_id = pending[nxt]
                nxt = nxt + 1
                top = stacks[top_id]
                selected = g.lr_action(top.state, window)
                actions = []
                if selected == LR_CONFLICT:
                    actions = g.lr_conflict(top.state, window)
                elif selected != LR_INVALID:
                    actions.append(selected)
                ai = 0
                while ai < len(actions):
                    action = actions[ai]
                    ai = ai + 1
                    self._tick(forest, stacks)
                    if action == 0:
                        if pos.isEnd() and top.forest != ABSENT and top.previous == 0:
                            span = forest[top.forest].span
                            if not ((span.begin != 0 and not span.leading) or (cursor > span.end and not span.trailing)):
                                roots.add(top.forest)
                    elif action > 0:
                        if not pos.isEnd():
                            span = _Span(cursor, cursor + 1, False, False, False)
                            leaf = self._intern(forest, forest_index, True, pos.current().kind(), span,
                                                True, pos.copy())
                            shifted.add(self._push(stacks, stack_index, top_id, action - 1, leaf))
                    else:
                        rule = -action - 1
                        count = g.lr_length(rule)
                        children = []
                        base = top_id
                        i = 0
                        while i < count and base != 0:
                            children.append(stacks[base].forest)
                            base = stacks[base].previous
                            i = i + 1
                        if len(children) == count:
                            children.reverse()
                            span = _Span(cursor, cursor, False, False, True)
                            allowed = self._gap(g, rule, 0)
                            valid = True
                            i = 0
                            while i < len(children):
                                child = forest[children[i]].span
                                if child.empty:
                                    allowed = allowed or child.leading or child.trailing
                                else:
                                    if span.empty:
                                        span.begin = child.begin
                                        span.leading = allowed or child.leading
                                    elif child.begin > span.end and not (allowed or child.leading):
                                        valid = False
                                    span.empty = False
                                    span.end = child.end
                                    allowed = child.trailing
                                allowed = allowed or self._gap(g, rule, i + 1)
                                i = i + 1
                            if valid:
                                span.trailing = allowed
                                if span.empty:
                                    span.leading = allowed
                                lhs = g.lr_lhs(rule)
                                value = self._intern(forest, forest_index, False, lhs, span, False, 0)
                                alternative = _Alternative(rule, children)
                                known = False
                                k = 0
                                while k < len(forest[value].alternatives):
                                    if forest[value].alternatives[k].equals(alternative):
                                        known = True
                                    k = k + 1
                                if not known:
                                    forest[value].alternatives.append(alternative)
                                dest = self._push(stacks, stack_index, base, g.lr_goto(stacks[base].state, lhs), value)
                                if closure.add(dest):
                                    pending.append(dest)
            if pos.isEnd():
                running = False
            else:
                if shifted.empty():
                    runtime_error("Unexpected input during LR automatic conflict resolution at token " + int_to_string(cursor))
                active = shifted
                pos.next()
                cursor = cursor + 1
        if roots.empty():
            runtime_error("Unexpected end of input during LR automatic conflict resolution")

        visiting = []
        ways = []
        i = 0
        while i < len(forest):
            visiting.append(0)
            ways.append(0)
            i = i + 1
        ambiguous = ABSENT
        order = []
        ri = 0
        while ri < len(roots.items) and ambiguous == ABSENT:
            root = roots.items[ri]
            ri = ri + 1
            if visiting[root] != 2:
                # frames: [node, alternative, child]
                pending_nodes = [[root, 0, 0]]
                visiting[root] = 1
                while len(pending_nodes) > 0 and ambiguous == ABSENT:
                    self._tick(forest, stacks)
                    frame = pending_nodes[len(pending_nodes) - 1]
                    node = forest[frame[0]]
                    if frame[1] == len(node.alternatives):
                        visiting[frame[0]] = 2
                        order.append(frame[0])
                        pending_nodes.pop()
                    else:
                        alt = node.alternatives[frame[1]]
                        if frame[2] == len(alt.children):
                            frame[1] = frame[1] + 1
                            frame[2] = 0
                        else:
                            child = alt.children[frame[2]]
                            frame[2] = frame[2] + 1
                            if visiting[child] == 1:
                                # productive nullable cycle
                                ambiguous = child
                            elif visiting[child] == 0:
                                visiting[child] = 1
                                pending_nodes.append([child, 0, 0])
        total = 0
        if ambiguous != ABSENT:
            total = 2
        else:
            oi = 0
            while oi < len(order):
                ident = order[oi]
                oi = oi + 1
                n = 0
                if forest[ident].has_token:
                    n = 1
                ai = 0
                while ai < len(forest[ident].alternatives):
                    product = 1
                    ci = 0
                    children = forest[ident].alternatives[ai].children
                    while ci < len(children):
                        product = min2(2, product * ways[children[ci]])
                        ci = ci + 1
                    n = min2(2, n + product)
                    ai = ai + 1
                ways[ident] = n
                if n > 1 and ambiguous == ABSENT:
                    ambiguous = ident
            ri = 0
            while ri < len(roots.items):
                total = min2(2, total + ways[roots.items[ri]])
                ri = ri + 1
        if total != 1:
            message = "Ambiguous LR input: multiple complete parses"
            if ambiguous != ABSENT:
                node = forest[ambiguous]
                message = message + " at tokens " + int_to_string(node.span.begin) + ".." + int_to_string(node.span.end)
                ai = 0
                while ai < len(node.alternatives):
                    rule = node.alternatives[ai].rule
                    if g.has_rule_label():
                        message = message + "\n  " + g.lr_rule_label(rule)
                    else:
                        message = message + "\n  production " + int_to_string(rule)
                    ai = ai + 1
            runtime_error(message)

        # Evaluate the chosen tree after recognition. Semantic results are not
        # memoized: two occurrences of a shared epsilon subtree still need two
        # action invocations. The traversal is iterative so deep trees work.
        # frames: [node, child, rhs]
        evaluating = [[roots.smallest(), 0, []]]
        result = 0
        while len(evaluating) > 0:
            self._tick(forest, stacks)
            frame = evaluating[len(evaluating) - 1]
            node = forest[frame[0]]
            if (not node.has_token) and frame[1] < len(node.alternatives[0].children):
                child = node.alternatives[0].children[frame[1]]
                frame[1] = frame[1] + 1
                evaluating.append([child, 0, []])
            else:
                if node.has_token:
                    produced = g.lr_shift(node.token.current())
                else:
                    produced = g.lr_reduce(node.alternatives[0].rule, frame[2])
                evaluating.pop()
                if len(evaluating) == 0:
                    result = produced
                else:
                    evaluating[len(evaluating) - 1][2].append(produced)
        return MatchResult(True, result, pos)

    def _key(self, parts: list[str]) -> str:
        out = []
        i = 0
        while i < len(parts):
            out.append(parts[i])
            i = i + 1
        return join(out, ",")

    def _intern(self, forest: list[_Forest], index: StrIntMap, terminal: bool, symbol: int, span: _Span,
                has_token: bool, token: object) -> int:
        self._tick(forest, [])
        key = self._key([_b(terminal), int_to_string(symbol), int_to_string(span.begin), int_to_string(span.end),
                         _b(span.leading), _b(span.trailing), _b(span.empty)])
        if index.try_emplace(key, len(forest)):
            forest.append(_Forest(span, has_token, token))
        return index.get(key)

    def _push(self, stacks: list[_Stack], index: StrIntMap, previous: int, state: int, value: int) -> int:
        self._tick([], stacks)
        key = self._key([int_to_string(previous), int_to_string(state), int_to_string(value)])
        if index.try_emplace(key, len(stacks)):
            stacks.append(_Stack(previous, state, value))
        return index.get(key)


def min2(a: int, b: int) -> int:
    if b < a:
        return b
    return a
