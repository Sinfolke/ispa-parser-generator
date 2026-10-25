a = 4
b = 12
c = a + b * 3
d = (a + b) * 3
e = 2 ** 3 ** 2
f = -a + +b
g = ~a & b

x = a < b
y = a <= b and b != 0
z = not x or y
check = a < b < 100
member = a in [1, 2, 3]
excluded = a not in [5, 6, 7]
identity = a is not None

shifted = (a << 2) | (b >> 1)
mixed = a + b * 2 ** 3 // 4 % 3
ternary = a if x else b
nested_ternary = a if x else b if y else c

values = [1, 2, a + b,]
empty_list = []
mapping = {'first': a, 'second': b,}
empty_dict = {}
pair = (a, b)
single = (a,)
empty_tuple = ()
grouped = (a + b)

result = calculate(a, b, scale=2)
result = obj.method(1, x=3)
item = values[0]
field = obj.field
nested = obj.items[0].method(a + b)
complex_value = factory(1, enabled=True).items[2]
even_more_complex = factory(1, enabled=True).items[compute(a + b * 2)].method(x if flag else y, mode='fast')[0]
a += 4
b -= 2
c *= 3
d /= 2
e //= 2
f %= 3
g **= 2
a &= 7
b |= 8
c ^= 3
d <<= 1
e >>= 2
