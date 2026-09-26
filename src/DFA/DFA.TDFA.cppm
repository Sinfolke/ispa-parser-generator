// ============================================================================
// tdfa_core.hpp
//
// Toolchain-independent core of the TDFA construction.
//
// Nothing in here knows about TNFA / Closure / LangAPI. The NFA side is
// abstracted behind a "Provider" (see determinize()), so this header can be
// unit-tested with plain g++ (tdfa_core_test.cpp) and is then reused verbatim
// by DFA_build.cpp (`module; #include "tdfa_core.hpp"` in the global module
// fragment).
//
// Model (TDFA(0), Laurikari/Trofimov style):
//
//   * tag        one position-valued slot per capture boundary
//   * register   run-time storage that holds a tag value
//   * config     (NFA state, register of every tag)     -- "-1" = tag unset
//   * DFA state  set of configs; two states are the same iff they are equal
//                up to a consistent renaming of registers. The renaming is
//                paid for on the transition as parallel COPY operations.
//
// Register kinds
//   scalar   holds one position                 (SET)
//   history  holds the head of an append-only   (APPEND, COPY is O(1))
//            linked list of positions -- used for captures inside a loop so
//            that the result contains EVERY iteration, not only the last one.
//
// Output registers
//   Registers 0 .. layout.tags-1 are reserved: register t is where the value
//   of tag t is left when a token is accepted. The accept sequence copies the
//   accepting config's registers into them and nothing else happens: the DFA
//   builds no values and runs no user code. Whatever reads the captures
//   afterwards (the reduction) addresses them by tag number, which is static.
//   A tag the accepting config never set keeps the "unset" the runtime
//   initialised the register file with.
// ============================================================================

// This file is generated from tdfa_core.hpp (the copy the unit tests compile).
// Edit the header, then regenerate this module unit.
export module DFA.TDFA;

import std;

export namespace tdfa {

inline constexpr std::size_t NIL = std::numeric_limits<std::size_t>::max();
inline constexpr long long NONE = -1;

// ---------------------------------------------------------------------------
// Operations
//
//   Set        a <- position
//   Append     a <- append(b, position)        (b == NIL: empty list)
//   Copy       a <- b
//
// `next` selects the position: false = the boundary BEFORE the character that
// is being consumed by the transition, true = the boundary AFTER it. Ops on
// accept sequences always use next == false (nothing is consumed).
// ---------------------------------------------------------------------------
enum class OpKind { Set, Append, Copy };

struct Op {
    OpKind kind = OpKind::Set;
    bool next = false;
    std::size_t a = NIL;
    std::size_t b = NIL;
    auto operator==(const Op &) const -> bool = default;
};

inline auto defines(const Op &op) -> bool {
    (void)op;
    return true; // every op defines register `a`
}

template <class F>
void forEachRead(const Op &op, F &&f) {
    switch (op.kind) {
    case OpKind::Set: break;
    case OpKind::Append: if (op.b != NIL) f(op.b); break;
    case OpKind::Copy: f(op.b); break;
    }
}

// ---------------------------------------------------------------------------
// Input description
// ---------------------------------------------------------------------------

// One tag operation met on the epsilon path to an NFA state.
struct Event {
    std::size_t tag = 0;
    bool append = false; // history tag (append) or scalar tag (set)
    bool next = false;   // see Op::next
};

// One NFA state that belongs to a closure, together with the events on the
// (single, priority-resolved) path that reaches it.
struct Item {
    std::size_t nfa = NIL;
    std::size_t src = NIL; // index of the source config in the current DFA state; NIL for the start closure
    std::vector<Event> events;
};

template <class Extra>
struct Closed {
    std::vector<Item> items;
    Extra extra{};
};

template <class Extra>
struct Accept {
    std::size_t config = NIL;      // index into the DFA state's config list
    std::vector<Event> events;     // tag ops that happen at the accept boundary
    Extra extra{};
};

// Describes one capture for whoever reads the output registers. The core
// itself only needs Layout::tags.
struct CaptureInfo {
    bool list = false;             // history register (every iteration) instead of one span
    std::size_t begin_tag = 0;     // scalar: begin position. list: the history tag
    std::size_t end_tag = 0;       // scalar: end position.   list: unused
};

struct Layout {
    std::size_t tags = 0;          // output register t <-> tag t
    std::vector<CaptureInfo> captures;
};

// ---------------------------------------------------------------------------
// Output description
// ---------------------------------------------------------------------------
struct Config {
    std::size_t nfa = NIL;
    std::vector<long long> regs; // per tag, NONE if unset
};

template <class Extra>
struct Edge {
    char sym = 0;
    std::size_t target = NIL;
    std::vector<Op> ops;
    Extra extra{};
};

template <class Extra>
struct StateOut {
    std::vector<Config> configs;
    std::vector<Edge<Extra>> edges;
    bool accepting = false;
    std::vector<Op> accept_ops;
    Extra accept_extra{};
};

// ---------------------------------------------------------------------------
// Group: the ops produced while building ONE transition (or one accept).
// Value numbering: identical ops (same kind, position and input) share one
// register, so two configs that both "SET tag at the same boundary" alias.
// ---------------------------------------------------------------------------
struct GOp {
    Op op;
    bool b_new = false; // `b` refers to a register PRODUCED inside this group
};

class Group {
    std::size_t &next_reg_;
    std::map<std::tuple<int, bool, std::size_t>, std::size_t> vn_;

    auto emit(OpKind kind, bool next, std::size_t base) -> std::size_t {
        const auto key = std::make_tuple(static_cast<int>(kind), next, base);
        if (const auto it = vn_.find(key); it != vn_.end())
            return it->second;
        const std::size_t reg = next_reg_++;
        ops.push_back(GOp{Op{kind, next, reg, base}, base != NIL && fresh.contains(base)});
        fresh.insert(reg);
        vn_.emplace(key, reg);
        return reg;
    }

public:
    std::vector<GOp> ops;
    std::set<std::size_t> fresh;

    explicit Group(std::size_t &next_reg) : next_reg_(next_reg) {}

    void apply(std::vector<long long> &regs, const Event &ev) {
        if (!ev.append) {
            regs[ev.tag] = static_cast<long long>(emit(OpKind::Set, ev.next, NIL));
        } else {
            const std::size_t base = regs[ev.tag] == NONE ? NIL : static_cast<std::size_t>(regs[ev.tag]);
            regs[ev.tag] = static_cast<long long>(emit(OpKind::Append, ev.next, base));
        }
    }
};

// ---------------------------------------------------------------------------
// Canonical key of a config list: registers renamed by first appearance.
// Equal keys <=> the two states are equal up to a bijective register renaming
// (aliasing between configs is part of the key).
// ---------------------------------------------------------------------------
using Key = std::vector<std::pair<std::size_t, std::vector<long long>>>;

inline auto canonical(const std::vector<Config> &configs) -> Key {
    std::map<long long, long long> ren;
    Key key;
    key.reserve(configs.size());
    for (const auto &c : configs) {
        std::vector<long long> regs;
        regs.reserve(c.regs.size());
        for (const long long r : c.regs) {
            if (r == NONE) {
                regs.push_back(NONE);
                continue;
            }
            const auto [it, inserted] = ren.emplace(r, static_cast<long long>(ren.size()));
            (void)inserted;
            regs.push_back(it->second);
        }
        key.emplace_back(c.nfa, std::move(regs));
    }
    return key;
}

// ---------------------------------------------------------------------------
// Sequentialise a set of PARALLEL assignments.
//
//   * an op that reads an OLD register r must run before any op that writes r
//   * an op that reads a register produced inside the group must run after
//     its producer
//   * a cycle (swap, rotate) is broken with a fresh temporary
// ---------------------------------------------------------------------------
inline auto readsOld(const GOp &g) -> bool {
    return (g.op.kind == OpKind::Append || g.op.kind == OpKind::Copy) && g.op.b != NIL && !g.b_new;
}

inline auto sequentialize(std::vector<GOp> pending, std::size_t &next_reg) -> std::vector<Op> {
    std::vector<Op> out;
    out.reserve(pending.size());

    const auto blocks = [](const GOp &i, const GOp &j) -> bool { // i must precede j
        if (j.b_new && j.op.b != NIL && j.op.b == i.op.a)
            return true;
        if (readsOld(i) && i.op.b == j.op.a)
            return true;
        return false;
    };

    while (!pending.empty()) {
        bool emitted = false;
        for (std::size_t j = 0; j < pending.size() && !emitted; ++j) {
            bool blocked = false;
            for (std::size_t i = 0; i < pending.size(); ++i) {
                if (i != j && blocks(pending[i], pending[j])) {
                    blocked = true;
                    break;
                }
            }
            if (!blocked) {
                out.push_back(pending[j].op);
                pending.erase(pending.begin() + static_cast<std::ptrdiff_t>(j));
                emitted = true;
            }
        }
        if (emitted)
            continue;

        bool broke = false;
        for (std::size_t i = 0; i < pending.size() && !broke; ++i) {
            if (!readsOld(pending[i]))
                continue;
            const std::size_t r = pending[i].op.b;
            const bool overwritten = std::any_of(pending.begin(), pending.end(), [&](const GOp &o) {
                return &o != &pending[i] && o.op.a == r;
            });
            if (!overwritten)
                continue;
            const std::size_t tmp = next_reg++;
            out.push_back(Op{OpKind::Copy, false, tmp, r});
            pending[i].op.b = tmp;
            broke = true;
        }
        if (!broke)
            throw std::logic_error("tdfa::sequentialize: unresolvable dependency cycle");
    }
    return out;
}

// ---------------------------------------------------------------------------
// Turn the ops of a transition into the final op list of its edge.
//   existing == nullptr : the target is a NEW state, registers stay as they are
//   existing != nullptr : the target already exists; rename to ITS registers
// ---------------------------------------------------------------------------
inline auto finishGroup(std::vector<GOp> gops,
                        const std::vector<Config> &fresh_cfgs,
                        const std::vector<Config> *existing,
                        const std::set<std::size_t> &fresh,
                        std::size_t &next_reg) -> std::vector<Op> {
    std::map<std::size_t, std::size_t> m; // register in `fresh_cfgs` -> register of the target
    std::set<std::size_t> needed;

    for (std::size_t j = 0; j < fresh_cfgs.size(); ++j) {
        for (std::size_t t = 0; t < fresh_cfgs[j].regs.size(); ++t) {
            const long long x = fresh_cfgs[j].regs[t];
            if (x == NONE)
                continue;
            const long long y = existing ? (*existing)[j].regs[t] : x;
            if (y == NONE)
                throw std::logic_error("tdfa::finishGroup: states are not isomorphic");
            const auto [it, inserted] = m.emplace(static_cast<std::size_t>(x), static_cast<std::size_t>(y));
            if (!inserted && it->second != static_cast<std::size_t>(y))
                throw std::logic_error("tdfa::finishGroup: inconsistent register renaming");
            needed.insert(static_cast<std::size_t>(y));
        }
    }

    // rename registers produced in this group
    for (auto &g : gops) {
        if (const auto it = m.find(g.op.a); it != m.end())
            g.op.a = it->second;
        if (g.b_new)
            if (const auto it = m.find(g.op.b); it != m.end())
                g.op.b = it->second;
    }

    // registers inherited from the source state that the target wants elsewhere
    for (const auto &[x, y] : m) {
        if (fresh.contains(x) || x == y)
            continue;
        gops.push_back(GOp{Op{OpKind::Copy, false, y, x}, false});
    }

    // drop ops whose result nobody wants
    std::vector<GOp> kept;
    for (auto it = gops.rbegin(); it != gops.rend(); ++it) {
        if (!needed.contains(it->op.a))
            continue;
        if (it->b_new && it->op.b != NIL)
            needed.insert(it->op.b);
        kept.push_back(*it);
    }
    std::reverse(kept.begin(), kept.end());

    return sequentialize(std::move(kept), next_reg);
}

// ---------------------------------------------------------------------------
// Liveness + interference colouring over the finished DFA.
// Removes dead ops, folds copies, returns the number of registers left.
// ---------------------------------------------------------------------------
template <class Extra>
auto allocateRegisters(std::vector<StateOut<Extra>> &states, std::size_t outputs) -> std::size_t {
    using RegSet = std::set<std::size_t>;
    const std::size_t n = states.size();

    // Whatever reads the captures after acceptance sees the output registers,
    // so they are live when an accept sequence ends.
    RegSet accept_live;
    for (std::size_t t = 0; t < outputs; ++t)
        accept_live.insert(t);

    const auto backward = [](const std::vector<Op> &ops, RegSet live, std::vector<char> *dead) -> RegSet {
        if (dead)
            dead->assign(ops.size(), 0);
        for (std::size_t k = ops.size(); k-- > 0;) {
            const Op &op = ops[k];
            if (defines(op)) {
                if (!live.contains(op.a)) {
                    if (dead)
                        (*dead)[k] = 1;
                    continue;
                }
                live.erase(op.a);
            }
            forEachRead(op, [&](std::size_t r) { live.insert(r); });
        }
        return live;
    };

    std::vector<RegSet> live_in(n);
    for (bool changed = true; changed;) {
        changed = false;
        for (std::size_t s = n; s-- > 0;) {
            RegSet l;
            for (const auto &e : states[s].edges) {
                const RegSet r = backward(e.ops, live_in[e.target], nullptr);
                l.insert(r.begin(), r.end());
            }
            if (states[s].accepting) {
                const RegSet r = backward(states[s].accept_ops, accept_live, nullptr);
                l.insert(r.begin(), r.end());
            }
            if (l != live_in[s]) {
                live_in[s] = std::move(l);
                changed = true;
            }
        }
    }

    // dead-op removal (does not change liveness: dead ops were ignored above)
    const auto strip = [&](std::vector<Op> &ops, const RegSet &after) {
        std::vector<char> dead;
        (void)backward(ops, after, &dead);
        std::vector<Op> kept;
        for (std::size_t k = 0; k < ops.size(); ++k)
            if (!dead[k])
                kept.push_back(ops[k]);
        ops = std::move(kept);
    };
    for (std::size_t s = 0; s < n; ++s) {
        for (auto &e : states[s].edges)
            strip(e.ops, live_in[e.target]);
        if (states[s].accepting)
            strip(states[s].accept_ops, accept_live);
    }

    // interference graph
    std::map<std::size_t, RegSet> adj;
    std::map<std::size_t, RegSet> prefer;
    const auto link = [&](std::size_t x, std::size_t y) {
        if (x == y)
            return;
        adj[x].insert(y);
        adj[y].insert(x);
    };
    const auto scan = [&](const std::vector<Op> &ops, RegSet live) {
        for (std::size_t k = ops.size(); k-- > 0;) {
            const Op &op = ops[k];
            adj[op.a]; // make sure every mentioned register is a node
            if (defines(op)) {
                for (const std::size_t r : live)
                    if (r != op.a && !(op.kind == OpKind::Copy && r == op.b))
                        link(op.a, r);
                if (op.kind == OpKind::Copy) {
                    prefer[op.a].insert(op.b);
                    prefer[op.b].insert(op.a);
                }
                live.erase(op.a);
            }
            forEachRead(op, [&](std::size_t r) {
                adj[r];
                live.insert(r);
            });
        }
    };
    for (std::size_t s = 0; s < n; ++s) {
        for (const std::size_t x : live_in[s])
            for (const std::size_t y : live_in[s])
                link(x, y);
        for (const auto &e : states[s].edges)
            scan(e.ops, live_in[e.target]);
        if (states[s].accepting)
            scan(states[s].accept_ops, accept_live);
    }
    // outputs interfere with each other (all live at once)
    for (std::size_t x = 0; x < outputs; ++x)
        for (std::size_t y = x + 1; y < outputs; ++y)
            link(x, y);

    std::map<std::size_t, std::size_t> color;
    std::size_t count = outputs;
    for (std::size_t t = 0; t < outputs; ++t)
        color[t] = t; // pre-coloured: output t lives in physical register t
    for (const auto &[reg, neighbours] : adj) {
        if (reg < outputs)
            continue;
        std::set<std::size_t> used;
        for (const std::size_t nb : neighbours)
            if (const auto it = color.find(nb); it != color.end())
                used.insert(it->second);
        std::optional<std::size_t> pick;
        if (const auto p = prefer.find(reg); p != prefer.end())
            for (const std::size_t other : p->second)
                if (const auto it = color.find(other); it != color.end() && !used.contains(it->second)) {
                    pick = it->second;
                    break;
                }
        if (!pick) {
            std::size_t c = 0;
            while (used.contains(c))
                ++c;
            pick = c;
        }
        color[reg] = *pick;
        count = std::max(count, *pick + 1);
    }

    const auto rename = [&](std::vector<Op> &ops) {
        std::vector<Op> out;
        for (Op op : ops) {
            if (op.a != NIL) op.a = color.at(op.a);
            if (op.b != NIL) op.b = color.at(op.b);
            if (op.kind == OpKind::Copy && op.a == op.b)
                continue; // coalesced away
            out.push_back(op);
        }
        ops = std::move(out);
    };
    for (auto &s : states) {
        for (auto &e : s.edges)
            rename(e.ops);
        rename(s.accept_ops);
    }
    return count;
}

// ---------------------------------------------------------------------------
// Determinisation.
//
// Provider requirements (P):
//   using Extra = ...;                                  // default constructible, copyable
//   Closed<Extra> start();
//   std::vector<char> symbols(const std::vector<size_t>& nfa);       // sorted, unique
//   Closed<Extra> move(const std::vector<size_t>& nfa, char sym);    // Item::src indexes `nfa`
//   std::optional<Accept<Extra>> accept(const std::vector<size_t>& nfa);
//
// `nfa` is always the NFA states of a DFA state in ascending order, which is
// also the order of its configs.
// ---------------------------------------------------------------------------
auto makeConfigs(const std::vector<Config> *src, std::vector<Item> items, Group &group, std::size_t tags)
    -> std::vector<Config> {
    // Closure order is irrelevant for the result (priority was already
    // resolved by "first path wins"); ascending NFA order gives every DFA
    // state one canonical layout.
    std::stable_sort(items.begin(), items.end(), [](const Item &x, const Item &y) { return x.nfa < y.nfa; });
    std::vector<Config> out;
    out.reserve(items.size());
    for (const auto &it : items) {
        Config c;
        c.nfa = it.nfa;
        c.regs = it.src == NIL ? std::vector<long long>(tags, NONE) : (*src)[it.src].regs;
        for (const auto &ev : it.events)
            group.apply(c.regs, ev);
        out.push_back(std::move(c));
    }
    return out;
}

template <class P>
auto determinize(P &p, const Layout &layout, std::size_t &register_count, bool allocate = true)
    -> std::vector<StateOut<typename P::Extra>> {
    using Extra = typename P::Extra;

    std::vector<StateOut<Extra>> out;
    std::map<Key, std::size_t> index;
    // registers 0 .. tags-1 are the output registers (see the header comment)
    std::size_t next_reg = layout.tags;

    const auto nfaOf = [](const std::vector<Config> &cs) {
        std::vector<std::size_t> v;
        v.reserve(cs.size());
        for (const auto &c : cs)
            v.push_back(c.nfa);
        return v;
    };

    // ---- start state: never re-entered, carries the entry ops -------------
    std::vector<Op> entry_ops;
    {
        auto start = p.start();
        Group g(next_reg);
        StateOut<Extra> s0;
        s0.configs = makeConfigs(nullptr, std::move(start.items), g, layout.tags);
        for (const auto &go : g.ops)
            entry_ops.push_back(go.op);
        out.push_back(std::move(s0));
    }

    for (std::size_t s = 0; s < out.size(); ++s) {
        const std::vector<Config> cfgs = out[s].configs; // out may reallocate below
        const std::vector<std::size_t> nfa = nfaOf(cfgs);

        // ---- accept --------------------------------------------------------
        if (auto acc = p.accept(nfa)) {
            Group g(next_reg);
            std::vector<long long> regs = cfgs.at(acc->config).regs;
            for (const auto &ev : acc->events)
                g.apply(regs, ev);

            std::vector<Op> ops;
            if (s == 0)
                ops = entry_ops;
            for (const auto &go : g.ops)
                ops.push_back(go.op);

            // Leave every tag the accepting config has set in its output register.
            for (std::size_t t = 0; t < layout.tags; ++t)
                if (regs[t] != NONE)
                    ops.push_back(Op{OpKind::Copy, false, t, static_cast<std::size_t>(regs[t])});
            out[s].accepting = true;
            out[s].accept_ops = std::move(ops);
            out[s].accept_extra = std::move(acc->extra);
        }

        // ---- transitions ---------------------------------------------------
        for (const char sym : p.symbols(nfa)) {
            auto mv = p.move(nfa, sym);
            if (mv.items.empty())
                continue;

            Group g(next_reg);
            std::vector<Config> ncfg = makeConfigs(&cfgs, std::move(mv.items), g, layout.tags);
            Key key = canonical(ncfg);

            std::size_t target;
            std::vector<Op> ops;
            if (const auto it = index.find(key); it == index.end()) {
                target = out.size();
                ops = finishGroup(g.ops, ncfg, nullptr, g.fresh, next_reg);
                index.emplace(std::move(key), target);
                StateOut<Extra> ns;
                ns.configs = std::move(ncfg);
                out.push_back(std::move(ns));
            } else {
                target = it->second;
                ops = finishGroup(g.ops, ncfg, &out[target].configs, g.fresh, next_reg);
            }

            if (s == 0)
                ops.insert(ops.begin(), entry_ops.begin(), entry_ops.end());

            out[s].edges.push_back(Edge<Extra>{sym, target, std::move(ops), std::move(mv.extra)});
        }
    }

    register_count = allocate ? allocateRegisters(out, layout.tags) : next_reg;
    return out;
}

} // namespace tdfa
